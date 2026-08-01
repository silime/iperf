[CmdletBinding()]
param(
    [string]$Server = '192.168.1.43',
    [int]$Port = 5201,
    [string]$BindAddress = '',
    [int]$DurationSeconds = 10,
    [int]$Repetitions = 3,
    [int]$ParallelStreams = 4,
    [string]$UdpBandwidth = '100M',
    [int]$MaxAttempts = 3,
    [int]$RetryDelaySeconds = 2,
    [double]$MinimumThroughputRatio = 0.85,
    [double]$MaximumCpuRatio = 1.50,
    [double]$CpuTolerancePoints = 5.0,
    [double]$UdpLossTolerancePoints = 1.0,
    [string]$ProjectIperf = '',
    [string]$ReferenceArchive = '',
    [string]$ResultsDirectory = ''
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

if ($DurationSeconds -lt 1) { throw 'DurationSeconds must be at least 1.' }
if ($Repetitions -lt 1) { throw 'Repetitions must be at least 1.' }
if ($ParallelStreams -lt 1) { throw 'ParallelStreams must be at least 1.' }
if ($MaxAttempts -lt 1) { throw 'MaxAttempts must be at least 1.' }
if ($RetryDelaySeconds -lt 0) { throw 'RetryDelaySeconds cannot be negative.' }

if ([string]::IsNullOrWhiteSpace($ProjectIperf)) {
    $ProjectIperf = Join-Path $PSScriptRoot 'build\windows-msvc-x64\iperf3.exe'
}
if ([string]::IsNullOrWhiteSpace($ReferenceArchive)) {
    $ReferenceArchive = Join-Path $PSScriptRoot 'iperf-3.21-win64.zip'
}
if ([string]::IsNullOrWhiteSpace($ResultsDirectory)) {
    $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
    $ResultsDirectory = Join-Path $PSScriptRoot "diagnostics\comparison-$stamp"
}

if (-not (Test-Path -LiteralPath $ProjectIperf -PathType Leaf)) {
    throw "Project binary is missing: $ProjectIperf`nBuild it with: cmake --preset windows-msvc-x64; cmake --build --preset windows-msvc-x64"
}
if (-not (Test-Path -LiteralPath $ReferenceArchive -PathType Leaf)) {
    throw "Reference archive is missing: $ReferenceArchive"
}

New-Item -ItemType Directory -Force -Path $ResultsDirectory | Out-Null
$referenceDirectory = Join-Path $ResultsDirectory 'reference'
Expand-Archive -LiteralPath $ReferenceArchive -DestinationPath $referenceDirectory -Force
$referenceIperf = Join-Path $referenceDirectory 'iperf3.exe'
if (-not (Test-Path -LiteralPath $referenceIperf -PathType Leaf)) {
    throw "Reference archive does not contain iperf3.exe: $ReferenceArchive"
}

function ConvertTo-CommandLineArgument {
    param([string]$Value)
    return '"' + $Value.Replace('\', '\').Replace('"', '\"') + '"'
}

function Invoke-CapturedProcess {
    param(
        [string]$FilePath,
        [string[]]$ArgumentList
    )

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $FilePath
    $startInfo.Arguments = ($ArgumentList | ForEach-Object {
        ConvertTo-CommandLineArgument ([string]$_)
    }) -join ' '
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true

    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    try {
        [void]$process.Start()
        $stdoutTask = $process.StandardOutput.ReadToEndAsync()
        $stderrTask = $process.StandardError.ReadToEndAsync()
        $process.WaitForExit()
        $stopwatch.Stop()
        return [pscustomobject]@{
            ExitCode = $process.ExitCode
            StandardOutput = $stdoutTask.Result
            StandardError = $stderrTask.Result
            WallSeconds = $stopwatch.Elapsed.TotalSeconds
            ProcessCpuSeconds = $process.TotalProcessorTime.TotalSeconds
        }
    } finally {
        $stopwatch.Stop()
        $process.Dispose()
    }
}

function Get-Median {
    param([double[]]$Values)
    if ($null -eq $Values -or $Values.Count -eq 0) { return [double]::NaN }
    $sorted = @($Values | Sort-Object)
    $middle = [math]::Floor($sorted.Count / 2)
    if (($sorted.Count % 2) -eq 1) { return [double]$sorted[$middle] }
    return ([double]$sorted[$middle - 1] + [double]$sorted[$middle]) / 2.0
}

function Get-JsonPropertyValue {
    param(
        [object]$Object,
        [string[]]$Names
    )
    foreach ($name in $Names) {
        if ($null -ne $Object -and $Object.PSObject.Properties.Name -contains $name) {
            return $Object.$name
        }
    }
    return $null
}

function Invoke-IperfRun {
    param(
        [string]$Implementation,
        [string]$Executable,
        [object]$Scenario,
        [int]$Iteration,
        [int]$Sequence
    )

    $arguments = @('-c', $Server, '-p', [string]$Port, '-t', [string]$DurationSeconds,
                   '-O', '1', '-J')
    if (-not [string]::IsNullOrWhiteSpace($BindAddress)) {
        $arguments += @('-B', $BindAddress)
    }
    $arguments += @($Scenario.Arguments)

    Write-Host ("[{0}] {1,-9} {2} iteration {3}/{4}" -f
        $Sequence, $Implementation, $Scenario.Name, $Iteration, $Repetitions)
    $safeName = '{0:D2}-{1}-{2}-r{3}' -f $Sequence, $Scenario.Name, $Implementation, $Iteration
    $captured = $null
    for ($attempt = 1; $attempt -le $MaxAttempts; $attempt++) {
        $captured = Invoke-CapturedProcess -FilePath $Executable -ArgumentList $arguments
        $attemptName = "$safeName-attempt$attempt"
        [System.IO.File]::WriteAllText((Join-Path $ResultsDirectory "$attemptName.stdout.json"), $captured.StandardOutput)
        [System.IO.File]::WriteAllText((Join-Path $ResultsDirectory "$attemptName.stderr.txt"), $captured.StandardError)
        if ($captured.ExitCode -eq 0) { break }
        if ($attempt -lt $MaxAttempts) {
            Write-Warning ("{0} failed with exit code {1}; retrying in {2}s" -f
                $Implementation, $captured.ExitCode, $RetryDelaySeconds)
            Start-Sleep -Seconds $RetryDelaySeconds
        }
    }

    $parsed = $null
    $parseError = ''
    if ($captured.ExitCode -eq 0) {
        try { $parsed = $captured.StandardOutput | ConvertFrom-Json }
        catch { $parseError = $_.Exception.Message }
    }

    $throughput = [double]::NaN
    $hostCpu = [double]::NaN
    $lostPercent = 0.0
    if ($null -ne $parsed) {
        $end = $parsed.end
        $sum = Get-JsonPropertyValue $end @('sum_received', 'sum', 'sum_sent')
        if ($null -ne $sum -and $null -ne $sum.bits_per_second) {
            $throughput = [double]$sum.bits_per_second / 1000000.0
        }
        $udpSum = Get-JsonPropertyValue $end @('sum', 'sum_received')
        if ($Scenario.Protocol -eq 'UDP' -and $null -ne $udpSum -and
            $udpSum.PSObject.Properties.Name -contains 'lost_percent') {
            $lostPercent = [double]$udpSum.lost_percent
        }
        if ($null -ne $end.cpu_utilization_percent -and
            $end.cpu_utilization_percent.PSObject.Properties.Name -contains 'host_total') {
            $hostCpu = [double]$end.cpu_utilization_percent.host_total
        }
    }

    [pscustomobject]@{
        Scenario = $Scenario.Name
        Protocol = $Scenario.Protocol
        Implementation = $Implementation
        Iteration = $Iteration
        Sequence = $Sequence
        ExitCode = $captured.ExitCode
        ThroughputMbps = $throughput
        HostCpuPercent = $hostCpu
        UdpLostPercent = $lostPercent
        WallSeconds = $captured.WallSeconds
        ProcessCpuSeconds = $captured.ProcessCpuSeconds
        ParseError = $parseError
        StandardError = $captured.StandardError.Trim()
    }
}

$implementations = @{
    Reference = (Resolve-Path -LiteralPath $referenceIperf).Path
    Project = (Resolve-Path -LiteralPath $ProjectIperf).Path
}
$scenarios = @(
    [pscustomobject]@{ Name='tcp-forward-p1'; Protocol='TCP'; Arguments=@('-P', '1') },
    [pscustomobject]@{ Name='tcp-forward-parallel'; Protocol='TCP'; Arguments=@('-P', [string]$ParallelStreams) },
    [pscustomobject]@{ Name='tcp-reverse-p1'; Protocol='TCP'; Arguments=@('-P', '1', '-R') },
    [pscustomobject]@{ Name='tcp-reverse-parallel'; Protocol='TCP'; Arguments=@('-P', [string]$ParallelStreams, '-R') },
    [pscustomobject]@{ Name='udp-forward'; Protocol='UDP'; Arguments=@('-u', '-b', $UdpBandwidth) }
)

$metadata = [ordered]@{
    StartedAt = (Get-Date).ToString('o')
    Server = $Server
    Port = $Port
    BindAddress = $BindAddress
    DurationSeconds = $DurationSeconds
    Repetitions = $Repetitions
    ParallelStreams = $ParallelStreams
    UdpBandwidth = $UdpBandwidth
    MaxAttempts = $MaxAttempts
    RetryDelaySeconds = $RetryDelaySeconds
    MinimumThroughputRatio = $MinimumThroughputRatio
    MaximumCpuRatio = $MaximumCpuRatio
    CpuTolerancePoints = $CpuTolerancePoints
    ReferenceArchiveSha256 = (Get-FileHash -LiteralPath $ReferenceArchive -Algorithm SHA256).Hash
    ReferenceVersion = ((Invoke-CapturedProcess $referenceIperf @('--version')).StandardOutput.Trim())
    ProjectVersion = ((Invoke-CapturedProcess $ProjectIperf @('--version')).StandardOutput.Trim())
}
$metadata | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $ResultsDirectory 'metadata.json')

$rawResults = [System.Collections.Generic.List[object]]::new()
$sequence = 0
for ($scenarioIndex = 0; $scenarioIndex -lt $scenarios.Count; $scenarioIndex++) {
    $scenario = $scenarios[$scenarioIndex]
    for ($iteration = 1; $iteration -le $Repetitions; $iteration++) {
        # Alternate execution order to reduce bias from changing Wi-Fi/server conditions.
        $order = if ((($scenarioIndex + $iteration) % 2) -eq 0) {
            @('Project', 'Reference')
        } else {
            @('Reference', 'Project')
        }
        foreach ($implementation in $order) {
            $sequence++
            $rawResults.Add((Invoke-IperfRun -Implementation $implementation `
                -Executable $implementations[$implementation] -Scenario $scenario `
                -Iteration $iteration -Sequence $sequence))
            Start-Sleep -Seconds 1
        }
    }
}

$rawResults | Export-Csv -LiteralPath (Join-Path $ResultsDirectory 'raw-results.csv') -NoTypeInformation
$rawResults | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $ResultsDirectory 'raw-results.json')

$summary = foreach ($scenario in $scenarios) {
    $reference = @($rawResults | Where-Object {
        $_.Scenario -eq $scenario.Name -and $_.Implementation -eq 'Reference' -and $_.ExitCode -eq 0
    })
    $project = @($rawResults | Where-Object {
        $_.Scenario -eq $scenario.Name -and $_.Implementation -eq 'Project' -and $_.ExitCode -eq 0
    })
    $referenceThroughput = Get-Median @($reference | ForEach-Object { [double]$_.ThroughputMbps })
    $projectThroughput = Get-Median @($project | ForEach-Object { [double]$_.ThroughputMbps })
    $referenceCpu = Get-Median @($reference | ForEach-Object { [double]$_.HostCpuPercent })
    $projectCpu = Get-Median @($project | ForEach-Object { [double]$_.HostCpuPercent })
    $referenceLoss = Get-Median @($reference | ForEach-Object { [double]$_.UdpLostPercent })
    $projectLoss = Get-Median @($project | ForEach-Object { [double]$_.UdpLostPercent })
    $ratio = if ($referenceThroughput -gt 0) { $projectThroughput / $referenceThroughput } else { 0 }
    $throughputPass = $reference.Count -eq $Repetitions -and $project.Count -eq $Repetitions -and
        $ratio -ge $MinimumThroughputRatio
    $cpuPass = [double]::IsNaN($referenceCpu) -or [double]::IsNaN($projectCpu) -or
        $projectCpu -le (($referenceCpu * $MaximumCpuRatio) + $CpuTolerancePoints)
    $lossPass = $scenario.Protocol -ne 'UDP' -or $projectLoss -le ($referenceLoss + $UdpLossTolerancePoints)

    [pscustomobject]@{
        Scenario = $scenario.Name
        ReferenceMbps = [math]::Round($referenceThroughput, 2)
        ProjectMbps = [math]::Round($projectThroughput, 2)
        ThroughputRatio = [math]::Round($ratio, 3)
        ReferenceCpuPercent = [math]::Round($referenceCpu, 2)
        ProjectCpuPercent = [math]::Round($projectCpu, 2)
        ReferenceUdpLossPercent = [math]::Round($referenceLoss, 3)
        ProjectUdpLossPercent = [math]::Round($projectLoss, 3)
        SuccessfulReferenceRuns = $reference.Count
        SuccessfulProjectRuns = $project.Count
        ThroughputPass = $throughputPass
        CpuPass = $cpuPass
        LossPass = $lossPass
        Pass = $throughputPass -and $cpuPass -and $lossPass
    }
}

$summary | Format-Table Scenario,ReferenceMbps,ProjectMbps,ThroughputRatio,
    ReferenceCpuPercent,ProjectCpuPercent,ThroughputPass,CpuPass,LossPass,Pass -AutoSize
$summary | Export-Csv -LiteralPath (Join-Path $ResultsDirectory 'summary.csv') -NoTypeInformation
$summary | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $ResultsDirectory 'summary.json')

$markdown = [System.Collections.Generic.List[string]]::new()
$markdown.Add('# iperf3 Windows performance comparison')
$markdown.Add('')
$markdown.Add("Minimum accepted project/reference throughput ratio: $MinimumThroughputRatio")
$markdown.Add('')
$markdown.Add('| Scenario | Reference Mbps | Project Mbps | Ratio | Ref CPU % | Project CPU % | Pass |')
$markdown.Add('|---|---:|---:|---:|---:|---:|:---:|')
foreach ($row in $summary) {
    $markdown.Add("| $($row.Scenario) | $($row.ReferenceMbps) | $($row.ProjectMbps) | $($row.ThroughputRatio) | $($row.ReferenceCpuPercent) | $($row.ProjectCpuPercent) | $($row.Pass) |")
}
[System.IO.File]::WriteAllLines((Join-Path $ResultsDirectory 'report.md'), $markdown)

Write-Host "Results: $ResultsDirectory"
$failed = @($summary | Where-Object { -not $_.Pass })
if ($failed.Count -gt 0) {
    [Console]::Error.WriteLine(
        "Performance comparison failed: " + (($failed | ForEach-Object Scenario) -join ', '))
    exit 2
}
Write-Host 'Performance comparison passed.'
