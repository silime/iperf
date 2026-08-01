#include "pthread.h"
#include <errno.h>
#include <process.h>
#include <stdlib.h>
struct thread_start { void *(*routine)(void *); void *argument; };
static unsigned __stdcall thread_entry(void *argument) {
    struct thread_start *start = argument; void *(*routine)(void *) = start->routine; void *arg = start->argument;
    free(start); routine(arg); return 0;
}
int pthread_attr_init(pthread_attr_t *attr) { *attr = 0; return 0; }
int pthread_attr_destroy(pthread_attr_t *attr) { (void)attr; return 0; }
int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*routine)(void *), void *arg) {
    uintptr_t handle; struct thread_start *start = malloc(sizeof(*start)); (void)attr;
    if (!start) return ENOMEM; start->routine = routine; start->argument = arg;
    handle = _beginthreadex(NULL, 0, thread_entry, start, 0, NULL);
    if (!handle) { free(start); return errno ? errno : EAGAIN; } thread->handle = (HANDLE)handle; return 0;
}
int pthread_cancel(pthread_t thread) { return CancelSynchronousIo(thread.handle) || GetLastError() == ERROR_NOT_FOUND ? 0 : ESRCH; }
int pthread_join(pthread_t thread, void **retval) { DWORD r = WaitForSingleObject(thread.handle, INFINITE); if (retval) *retval = NULL; CloseHandle(thread.handle); return r == WAIT_OBJECT_0 ? 0 : ESRCH; }
int pthread_setcanceltype(int type, int *oldtype) { (void)type; if (oldtype) *oldtype=0; return 0; }
int pthread_setcancelstate(int state, int *oldstate) { (void)state; if (oldstate) *oldstate=0; return 0; }
int pthread_mutexattr_init(pthread_mutexattr_t *attr) { *attr=0; return 0; }
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type) { *attr=type; return 0; }
int pthread_mutexattr_destroy(pthread_mutexattr_t *attr) { (void)attr; return 0; }
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) { (void)attr; InitializeSRWLock(mutex); return 0; }
int pthread_mutex_destroy(pthread_mutex_t *mutex) { (void)mutex; return 0; }
int pthread_mutex_lock(pthread_mutex_t *mutex) { AcquireSRWLockExclusive(mutex); return 0; }
int pthread_mutex_unlock(pthread_mutex_t *mutex) { ReleaseSRWLockExclusive(mutex); return 0; }
int pthread_sigmask(int how, const void *set, void *oldset) { (void)how; (void)set; (void)oldset; return 0; }

