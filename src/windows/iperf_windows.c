#include "iperf_windows.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int socket_started;
static void invalid_parameter_handler(const wchar_t *expression, const wchar_t *function,
                                      const wchar_t *file, unsigned int line, uintptr_t reserved)
{ (void)expression; (void)function; (void)file; (void)line; (void)reserved; }

void iperf_windows_set_errno(int error)
{
    switch (error) {
    case WSAEINTR: errno=EINTR; break; case WSAEWOULDBLOCK: errno=EAGAIN; break;
    case WSAEACCES: errno=EACCES; break; case WSAEFAULT: errno=EFAULT; break;
    case WSAEINVAL: errno=EINVAL; break; case WSAEMFILE: errno=EMFILE; break;
    case WSAEINPROGRESS: errno=EINPROGRESS; break; case WSAEALREADY: errno=EALREADY; break;
    case WSAENOTSOCK: errno=ENOTSOCK; break; case WSAEDESTADDRREQ: errno=EDESTADDRREQ; break;
    case WSAEMSGSIZE: errno=EMSGSIZE; break; case WSAEPROTOTYPE: errno=EPROTOTYPE; break;
    case WSAENOPROTOOPT: errno=ENOPROTOOPT; break; case WSAEPROTONOSUPPORT: errno=EPROTONOSUPPORT; break;
    case WSAEOPNOTSUPP: errno=EOPNOTSUPP; break; case WSAEAFNOSUPPORT: errno=EAFNOSUPPORT; break;
    case WSAEADDRINUSE: errno=EADDRINUSE; break; case WSAEADDRNOTAVAIL: errno=EADDRNOTAVAIL; break;
    case WSAENETDOWN: errno=ENETDOWN; break; case WSAENETUNREACH: errno=ENETUNREACH; break;
    case WSAECONNABORTED: errno=ECONNABORTED; break; case WSAECONNRESET: errno=ECONNRESET; break;
    case WSAENOBUFS: errno=ENOBUFS; break; case WSAEISCONN: errno=EISCONN; break;
    case WSAENOTCONN: errno=ENOTCONN; break; case WSAETIMEDOUT: errno=ETIMEDOUT; break;
    case WSAECONNREFUSED: errno=ECONNREFUSED; break; case WSAEHOSTUNREACH: errno=EHOSTUNREACH; break;
    default: errno=EIO; break;
    }
}
int iperf_windows_init(void) { WSADATA data; if (socket_started) return 0; _set_invalid_parameter_handler(invalid_parameter_handler); if (WSAStartup(MAKEWORD(2,2), &data)) return -1; socket_started=1; return 0; }
void iperf_windows_cleanup(void) { if (socket_started) { WSACleanup(); socket_started=0; } }
static SOCKET socket_from_fd(int fd) { return (SOCKET)(uintptr_t)(unsigned int)fd; }
static int is_socket(int fd) { int type=0, len=sizeof(type); return getsockopt(socket_from_fd(fd), SOL_SOCKET, SO_TYPE, (char *)&type, &len)==0; }
static int socket_to_fd(SOCKET socket_value) { if(socket_value==INVALID_SOCKET) { iperf_windows_set_errno(WSAGetLastError()); return -1; } if(socket_value>INT_MAX) { closesocket(socket_value); errno=EMFILE; return -1; } return (int)socket_value; }
int iperf_win_socket(int domain,int type,int protocol) { return socket_to_fd(socket(domain,type,protocol)); }
int iperf_win_close(int fd) { int r; if(fd<0) { errno=EBADF; return -1; } if(_get_osfhandle(fd)!=(intptr_t)-1) return _close(fd); r=closesocket(socket_from_fd(fd)); if(r==SOCKET_ERROR) iperf_windows_set_errno(WSAGetLastError()); return r; }
int iperf_win_read(int fd, void *buf, size_t count) { int r; if (!is_socket(fd)) return _read(fd,buf,(unsigned int)count); r=recv(socket_from_fd(fd),buf,(int)count,0); if(r==SOCKET_ERROR) iperf_windows_set_errno(WSAGetLastError()); return r; }
int iperf_win_write(int fd, const void *buf, size_t count) { int r; if (!is_socket(fd)) return _write(fd,buf,(unsigned int)count); r=send(socket_from_fd(fd),buf,(int)count,0); if(r==SOCKET_ERROR) iperf_windows_set_errno(WSAGetLastError()); return r; }
int iperf_win_connect(int s,const struct sockaddr *name,socklen_t len) { int r=connect(socket_from_fd(s),name,len); if(r==SOCKET_ERROR) { int error=WSAGetLastError(); if(error==WSAEWOULDBLOCK) errno=EINPROGRESS; else iperf_windows_set_errno(error); } return r; }
int iperf_win_accept(int s,struct sockaddr *addr,socklen_t *len) { return socket_to_fd(accept(socket_from_fd(s),addr,len)); }
int iperf_win_bind(int s,const struct sockaddr *name,socklen_t len) { int r=bind(socket_from_fd(s),name,len); if(r==SOCKET_ERROR) iperf_windows_set_errno(WSAGetLastError()); return r; }
int iperf_win_listen(int s,int backlog) { int r=listen(socket_from_fd(s),backlog); if(r==SOCKET_ERROR) iperf_windows_set_errno(WSAGetLastError()); return r; }
int iperf_win_shutdown(int s,int how) { int r=shutdown(socket_from_fd(s),how); if(r==SOCKET_ERROR) iperf_windows_set_errno(WSAGetLastError()); return r; }
int iperf_win_send(int s,const void *buf,size_t len,int flags) { int r=send(socket_from_fd(s),buf,(int)len,flags); if(r==SOCKET_ERROR) iperf_windows_set_errno(WSAGetLastError()); return r; }
int iperf_win_recv(int s,void *buf,size_t len,int flags) { int r=recv(socket_from_fd(s),buf,(int)len,flags); if(r==SOCKET_ERROR) iperf_windows_set_errno(WSAGetLastError()); return r; }
int iperf_win_recvfrom(int s,void *buf,size_t len,int flags,struct sockaddr *from,socklen_t *fromlen) { int r=recvfrom(socket_from_fd(s),buf,(int)len,flags,from,fromlen); if(r==SOCKET_ERROR) iperf_windows_set_errno(WSAGetLastError()); return r; }
int iperf_win_getsockname(int s,struct sockaddr *name,socklen_t *len) { int r=getsockname(socket_from_fd(s),name,len); if(r==SOCKET_ERROR) iperf_windows_set_errno(WSAGetLastError()); return r; }
int iperf_win_getpeername(int s,struct sockaddr *name,socklen_t *len) { int r=getpeername(socket_from_fd(s),name,len); if(r==SOCKET_ERROR) iperf_windows_set_errno(WSAGetLastError()); return r; }
int iperf_win_select(int nfds,fd_set *r,fd_set *w,fd_set *e,const struct timeval *tv) { int n=select(nfds,r,w,e,tv); if(n==SOCKET_ERROR) iperf_windows_set_errno(WSAGetLastError()); return n; }
int iperf_win_setsockopt(int s,int level,int name,const void *value,socklen_t len) { int r; DWORD timeout; if(level==SOL_SOCKET && (name==SO_RCVTIMEO || name==SO_SNDTIMEO) && len==sizeof(struct timeval)) { const struct timeval *tv=value; timeout=(DWORD)(tv->tv_sec*1000UL+tv->tv_usec/1000UL); value=&timeout; len=sizeof(timeout); } r=setsockopt(socket_from_fd(s),level,name,value,len); if(r==SOCKET_ERROR) iperf_windows_set_errno(WSAGetLastError()); return r; }
int iperf_win_getsockopt(int s,int level,int name,void *value,socklen_t *len) { int r=getsockopt(socket_from_fd(s),level,name,value,len); if(r==SOCKET_ERROR) iperf_windows_set_errno(WSAGetLastError()); return r; }
int iperf_win_fcntl(int fd,int cmd,...) { u_long mode; va_list args; if(cmd==F_GETFL) return 0; va_start(args,cmd); mode=(va_arg(args,int)&O_NONBLOCK)?1UL:0UL; va_end(args); if(ioctlsocket(socket_from_fd(fd),FIONBIO,&mode)==SOCKET_ERROR) { iperf_windows_set_errno(WSAGetLastError()); return -1; } return 0; }

int gettimeofday(struct timeval *tv,void *tz) { FILETIME ft; ULARGE_INTEGER v; (void)tz; GetSystemTimePreciseAsFileTime(&ft); v.LowPart=ft.dwLowDateTime; v.HighPart=ft.dwHighDateTime; v.QuadPart-=116444736000000000ULL; tv->tv_sec=(long)(v.QuadPart/10000000ULL); tv->tv_usec=(long)((v.QuadPart%10000000ULL)/10ULL); return 0; }
int clock_gettime(int id,struct timespec *ts) { if(id==CLOCK_REALTIME) { struct timeval tv; gettimeofday(&tv,NULL); ts->tv_sec=tv.tv_sec; ts->tv_nsec=tv.tv_usec*1000L; } else { LARGE_INTEGER c,f; QueryPerformanceCounter(&c); QueryPerformanceFrequency(&f); ts->tv_sec=c.QuadPart/f.QuadPart; ts->tv_nsec=(long)(((c.QuadPart%f.QuadPart)*1000000000ULL)/f.QuadPart); } return 0; }
int nanosleep(const struct timespec *req,struct timespec *rem) { (void)rem; Sleep((DWORD)(req->tv_sec*1000ULL+(req->tv_nsec+999999)/1000000)); return 0; }
unsigned int sleep(unsigned int seconds) { Sleep(seconds*1000U); return 0; }
int usleep(unsigned int usecs) { Sleep((usecs+999U)/1000U); return 0; }
int getrusage(int who,struct rusage *u) { FILETIME c,e,k,t; ULARGE_INTEGER ki,ui; (void)who; if(!GetProcessTimes(GetCurrentProcess(),&c,&e,&k,&t)) return -1; ki.LowPart=k.dwLowDateTime;ki.HighPart=k.dwHighDateTime;ui.LowPart=t.dwLowDateTime;ui.HighPart=t.dwHighDateTime;u->ru_stime.tv_sec=(long)(ki.QuadPart/10000000ULL);u->ru_stime.tv_usec=(long)((ki.QuadPart%10000000ULL)/10ULL);u->ru_utime.tv_sec=(long)(ui.QuadPart/10000000ULL);u->ru_utime.tv_usec=(long)((ui.QuadPart%10000000ULL)/10ULL);return 0; }
int uname(struct utsname *n) { DWORD size=sizeof(n->nodename); SYSTEM_INFO i; strcpy_s(n->sysname,sizeof(n->sysname),"Windows");GetComputerNameA(n->nodename,&size);GetNativeSystemInfo(&i);strcpy_s(n->machine,sizeof(n->machine),i.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64?"x86_64":"unknown");strcpy_s(n->release,sizeof(n->release),"NT");strcpy_s(n->version,sizeof(n->version),"native");return 0; }
int tcgetattr(int fd,struct termios *t) { (void)fd;t->c_lflag=ECHO;return 0; }
int tcsetattr(int fd,int action,const struct termios *t) { (void)fd;(void)action;(void)t;return 0; }
int mkstemp(char *path) { if(_mktemp_s(path,strlen(path)+1)) return -1; return _open(path,_O_CREAT|_O_EXCL|_O_RDWR|_O_BINARY|_O_TEMPORARY,_S_IREAD|_S_IWRITE); }
void *mmap(void *addr,size_t length,int prot,int flags,int fd,long offset) { (void)addr;(void)prot;(void)flags;(void)fd;(void)offset;return malloc(length); }
int munmap(void *addr,size_t length) { (void)length;free(addr);return 0; }
int daemon(int nochdir,int noclose) { (void)nochdir;(void)noclose;errno=ENOTSUP;return -1; }
int strcasecmp(const char *left,const char *right) { return _stricmp(left,right); }
int strncasecmp(const char *left,const char *right,size_t count) { return _strnicmp(left,right,count); }
char *strsignal(int sig) { static char text[32]; sprintf_s(text,sizeof(text),"signal %d",sig); return text; }
int kill(pid_t pid,int sig) { HANDLE process; DWORD code; (void)sig; process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,(DWORD)pid); if(!process) { errno=ESRCH; return -1; } if(!GetExitCodeProcess(process,&code) || code!=STILL_ACTIVE) { CloseHandle(process); errno=ESRCH; return -1; } CloseHandle(process); return 0; }
int poll(struct pollfd *fds,unsigned long nfds,int timeout) { int r=WSAPoll(fds,nfds,timeout); if(r==SOCKET_ERROR) iperf_windows_set_errno(WSAGetLastError()); return r; }
