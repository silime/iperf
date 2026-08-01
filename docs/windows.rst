Native Windows build
====================

The native Windows port uses the Microsoft C compiler, CMake, the Universal
CRT, and WinSock 2.  It does not require Cygwin, MSYS2, or a POSIX runtime.
MSVC was selected because it is the platform ABI compiler, ships with the
Windows SDK, integrates with Visual Studio's CMake support, and produces a
binary with no third-party runtime dependency.

Supported functionality
-----------------------

The Windows build supports the iperf3 command-line client and server, TCP and
UDP, IPv4 and IPv6, reverse mode, parallel streams, JSON output, CPU affinity,
timestamps, and the static ``iperf`` library.  The compatibility layer uses
WinSock for networking, Windows threads and SRW locks, QueryPerformanceCounter
for monotonic time, and BCryptGenRandom for payload entropy.

SCTP, OpenSSL authentication, sendfile/zero-copy, socket pacing, bind-to-device,
and daemon mode are not enabled.  Use an ordinary foreground process or a
Windows service wrapper for long-running servers.  Unsupported optional
features are omitted from ``iperf3 --version`` and rejected by the existing
iperf3 option validation.

Prerequisites
-------------

Install Visual Studio 2026 with the **Desktop development with C++** workload,
including CMake and a Windows SDK.  Both the x64 Native Tools command prompt
and Visual Studio's **Open a local folder** workflow are supported.

Build and test
--------------

From an x64 Native Tools command prompt::

  cmake --preset windows-msvc-x64
  cmake --build --preset windows-msvc-x64
  ctest --preset windows-msvc-x64

The executable is generated under
``build/windows-msvc-x64/iperf3.exe``.  The preset uses Ninja; make sure the
Visual Studio bundled CMake and Ninja tools are on ``PATH``.  The equivalent
direct build is::

  cmake -S . -B build/windows-ninja -G Ninja -DCMAKE_BUILD_TYPE=Release
  cmake --build build/windows-ninja
  ctest --test-dir build/windows-ninja --output-on-failure

Development plan and verification matrix
----------------------------------------

The port is organized so that platform code remains under ``src/windows`` and
the existing Unix Autotools build is unaffected.

1. Keep the Windows feature configuration explicit in
   ``src/iperf_config_windows.h.in``; do not claim features until their native
   implementation has protocol-level tests.
2. Keep POSIX-shaped adapters small and backed by documented Windows APIs.
   Every WinSock failure must be translated to ``errno`` because the portable
   iperf core uses it for control flow and diagnostics.
3. Build the library, CLI, and the portable unit tests with MSVC x64.  Run
   ``t_units``, ``t_timer``, ``t_uuid``, and ``t_api`` after compatibility
   changes.
4. Run a one-shot local Windows server and connect the Windows client to it to
   exercise both sides and cleanup paths.
5. Test against a current non-Windows iperf3 server in TCP upload, TCP reverse,
   and rate-limited UDP modes.  Add parallel-stream, IPv6, and JSON cases when
   changing socket or command-line code.
6. Re-run the original Autotools build on a Unix CI host before upstreaming,
   since source headers shared by both builds must remain portable.

The socket-facing API in upstream iperf3 stores descriptors in ``int``.  The
Windows adapter checks every newly created ``SOCKET`` before conversion and
fails with ``EMFILE`` if a handle cannot be represented, avoiding silent
truncation on 64-bit Windows.
