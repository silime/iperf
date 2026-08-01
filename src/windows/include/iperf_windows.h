#pragma once
#ifndef _WIN32
#error "iperf_windows.h is only for native Windows builds"
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <bcrypt.h>
#include <io.h>
#include <fcntl.h>
#include <process.h>
#include <signal.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef SSIZE_T ssize_t;
typedef int socklen_t;
typedef int pid_t;
typedef unsigned int uid_t;
typedef unsigned long long u_int64_t;
typedef unsigned int u_int32_t;
typedef unsigned short u_int16_t;
typedef unsigned char u_int8_t;
typedef unsigned int uint;
typedef int clockid_t;

#ifndef __attribute__
#define __attribute__(x)
#endif
#ifndef IFNAMSIZ
#define IFNAMSIZ 256
#endif
#ifndef BYTE_ORDER
#define LITTLE_ENDIAN 1234
#define BIG_ENDIAN 4321
#define BYTE_ORDER LITTLE_ENDIAN
#endif
#ifndef SHUT_WR
#define SHUT_WR SD_SEND
#endif
#ifndef SIGPIPE
#define SIGPIPE 13
#endif
#ifndef SIGHUP
#define SIGHUP 1
#endif
#ifndef SIG_BLOCK
#define SIG_BLOCK 0
#endif
#ifndef ENOTSUP
#define ENOTSUP WSAEOPNOTSUPP
#endif
#ifndef EWOULDBLOCK
#define EWOULDBLOCK EAGAIN
#endif
#ifndef S_IRUSR
#define S_IRUSR _S_IREAD
#define S_IWUSR _S_IWRITE
#endif

typedef unsigned long sigset_t;
static inline int sigemptyset(sigset_t *set) { *set = 0; return 0; }
static inline int sigaddset(sigset_t *set, int sig) { *set |= (1UL << (sig & 31)); return 0; }
struct rusage { struct timeval ru_utime; struct timeval ru_stime; };
#define RUSAGE_SELF 0
struct utsname { char sysname[64], nodename[256], release[64], version[128], machine[64]; };
struct termios { unsigned int c_lflag; };
#define ECHO 0x00000008
#define TCSAFLUSH 0
#define PROT_READ 1
#define PROT_WRITE 2
#define MAP_SHARED 1
#define MAP_FAILED ((void *)-1)

#define strdup _strdup
#define fileno _fileno
#define unlink _unlink
#define ftruncate(fd, size) _chsize_s((fd), (size))
#define open _open
#define O_RDONLY _O_RDONLY
#define O_WRONLY _O_WRONLY
#define O_RDWR _O_RDWR
#define O_CREAT _O_CREAT
#define O_TRUNC _O_TRUNC

int iperf_windows_init(void);
void iperf_windows_cleanup(void);
void iperf_windows_set_errno(int error);
int iperf_win_close(int fd);
int iperf_win_socket(int domain, int type, int protocol);
int iperf_win_read(int fd, void *buf, size_t count);
int iperf_win_write(int fd, const void *buf, size_t count);
int iperf_win_connect(int s, const struct sockaddr *name, socklen_t namelen);
int iperf_win_accept(int s, struct sockaddr *addr, socklen_t *addrlen);
int iperf_win_bind(int s, const struct sockaddr *name, socklen_t namelen);
int iperf_win_listen(int s, int backlog);
int iperf_win_shutdown(int s, int how);
int iperf_win_send(int s, const void *buf, size_t len, int flags);
int iperf_win_recv(int s, void *buf, size_t len, int flags);
int iperf_win_recvfrom(int s, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen);
int iperf_win_getsockname(int s, struct sockaddr *name, socklen_t *namelen);
int iperf_win_getpeername(int s, struct sockaddr *name, socklen_t *namelen);
int iperf_win_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, const struct timeval *timeout);
int iperf_win_setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen);
int iperf_win_getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen);
int iperf_win_fcntl(int fd, int cmd, ...);
int gettimeofday(struct timeval *tv, void *tz);
int clock_gettime(int clock_id, struct timespec *ts);
int nanosleep(const struct timespec *req, struct timespec *rem);
unsigned int sleep(unsigned int seconds);
int usleep(unsigned int usecs);
int getrusage(int who, struct rusage *usage);
int uname(struct utsname *name);
int tcgetattr(int fd, struct termios *termios_p);
int tcsetattr(int fd, int optional_actions, const struct termios *termios_p);
int mkstemp(char *path_template);
void *mmap(void *addr, size_t length, int prot, int flags, int fd, long offset);
int munmap(void *addr, size_t length);
int daemon(int nochdir, int noclose);
ssize_t getline(char **lineptr, size_t *n, FILE *stream);
char *strsignal(int sig);
int kill(pid_t pid, int sig);
int poll(struct pollfd *fds, unsigned long nfds, int timeout);

#ifndef IPERF_WINDOWS_IMPLEMENTATION
#define socket iperf_win_socket
#define close iperf_win_close
#define read iperf_win_read
#define write iperf_win_write
#define connect iperf_win_connect
#define accept iperf_win_accept
#define bind iperf_win_bind
#define listen iperf_win_listen
#define shutdown iperf_win_shutdown
#define send iperf_win_send
#define recv iperf_win_recv
#define recvfrom iperf_win_recvfrom
#define getsockname iperf_win_getsockname
#define getpeername iperf_win_getpeername
#define select iperf_win_select
#define setsockopt iperf_win_setsockopt
#define getsockopt iperf_win_getsockopt
#define fcntl iperf_win_fcntl
#endif
#define F_GETFL 1
#define F_SETFL 2
#define O_NONBLOCK 0x4000
#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1

#ifdef __cplusplus
}
#endif
