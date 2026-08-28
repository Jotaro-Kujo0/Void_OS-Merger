/*
 * common/compat.h — Cross-platform compatibility layer.
 *
 * Provides Windows (MSVC) shims for POSIX headers and functions so the
 * codebase compiles cleanly on both Linux and Windows 11.
 *
 * Usage: Replace platform-specific POSIX includes in each source file
 *        with: #include <common/compat.h>
 */

#ifndef VOM_COMMON_COMPAT_H
#define VOM_COMMON_COMPAT_H

/* ======================================================================
 * Windows (MSVC) Platform
 * ====================================================================== */
#ifdef _WIN32

/* ---- Windows headers ---- */
/* winsock2.h MUST come before windows.h for struct timeval, fd_set, SOCKET */
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  include <windows.h>
#  include <io.h>
#  include <process.h>
#  include <direct.h>
#  include <signal.h>

/* ---- Standard C headers always available ---- */
#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>
#  include <stdbool.h>
#  include <stdint.h>
#  include <stdarg.h>
#  include <time.h>
#  include <fcntl.h>
#  include <errno.h>

/* ---- ssize_t ---- */
#  include <BaseTsd.h>
typedef SSIZE_T ssize_t;

/* ---- getcwd ---- */
#  define getcwd _getcwd

/* ---- File mode constants ---- */
#  ifndef F_OK
#    define F_OK 0
#  endif
#  ifndef X_OK
#    define X_OK 0
#  endif
#  ifndef W_OK
#    define W_OK 2
#  endif
#  ifndef R_OK
#    define R_OK 4
#  endif
#  define access _access

/* ---- O_NONBLOCK (stub — Windows uses ioctlsocket for non-blocking sockets) ---- */
#  ifndef O_NONBLOCK
#    define O_NONBLOCK 0
#  endif

/* ---- POSIX file descriptors ---- */
#  ifndef STDIN_FILENO
#    define STDIN_FILENO  0
#  endif
#  ifndef STDOUT_FILENO
#    define STDOUT_FILENO 1
#  endif
#  ifndef STDERR_FILENO
#    define STDERR_FILENO 2
#  endif

/* ---- strcasecmp ---- */
#  define strcasecmp  _stricmp
#  define strncasecmp _strnicmp

/* ---- sleep / usleep ---- */
#  define sleep(seconds)   Sleep((DWORD)((seconds) * 1000))
#  define usleep(usec)     Sleep((DWORD)((usec) / 1000))

/* ---- gettimeofday ---- */
struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};
static inline int gettimeofday(struct timeval *tv, struct timezone *tz) {
    (void)tz;
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER li;
    li.LowPart  = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    /* FILETIME is 100-ns intervals since 1601-01-01.
       Unix epoch offset = 11644473600 seconds = 116444736000000000 in 100-ns. */
    li.QuadPart -= 116444736000000000ULL;
    tv->tv_sec  = (long)(li.QuadPart / 10000000ULL);
    tv->tv_usec = (long)((li.QuadPart % 10000000ULL) / 10ULL);
    return 0;
}

/* ---- sysconf ---- */
#  define _SC_NPROCESSORS_ONLN  1
#  define _SC_NPROCESSORS_CONF  2
static inline long sysconf(int name) {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    if (name == _SC_NPROCESSORS_ONLN || name == _SC_NPROCESSORS_CONF) {
        return (long)si.dwNumberOfProcessors;
    }
    return -1;
}

/* ---- getpid ---- */
#  define getpid() GetCurrentProcessId()

/* ---- pid_t ---- */
typedef DWORD pid_t;

/* ---- ssize_t read (POSIX read) ---- */
#define read(fd, buf, count) _read(fd, buf, (unsigned int)(count))

/* ---- poll.h ---- */
/* <winsock2.h> already provides struct pollfd, POLLIN, POLLOUT, POLLERR,
   POLLHUP, POLLNVAL. WSAPoll() is the Windows equivalent of poll().
   We provide a poll() wrapper that calls WSAPoll(). */
#ifndef POLLIN
#  define POLLIN   0x0100
#endif

/* poll() shim via WSAPoll() */
static inline int poll(struct pollfd *fds, int nfds, int timeout_ms) {
    return WSAPoll(fds, (ULONG)nfds, timeout_ms);
}

/* ---- Signal handling ---- */
#  define SIGKILL 9
#  ifndef SIGTERM
#    define SIGTERM 15
#  endif

/* Minimal sigaction/sigemptyset shim */
typedef int sigset_t;
typedef void (*posix_sa_handler_t)(int);
struct sigaction {
    posix_sa_handler_t sa_handler;
    sigset_t sa_mask;
};
static inline void sigemptyset(sigset_t *s) { if (s) *s = 0; }
static inline void sigaction(int sig, const struct sigaction *sa, struct sigaction *old) {
    (void)old;
    signal(sig, sa->sa_handler);
}

/* ---- Process management shims ---- */
/* pipe() — wraps _pipe with binary mode */
#define pipe(fds) _pipe((fds), 4096, _O_BINARY)

/* pid_t, fork, exec, wait, kill — Windows process management */
typedef HANDLE vom_pid_t;

/* daemon() — no-op on Windows */
#define daemon(nochdir, noclose) ((void)(nochdir), (void)(noclose), 0)

/* WIFEXITED / WEXITSTATUS for process exit codes */
#ifndef WEXITSTATUS
#  define WEXITSTATUS(status) ((status) & 0xFF)
#endif
#ifndef WIFEXITED
#  define WIFEXITED(status)   1  /* simplified: assume normal exit */
#endif
#ifndef WNOHANG
#  define WNOHANG 1
#endif

/* ---- struct option for getopt_long ---- */
#ifndef _STRUCT_OPTION
#  define _STRUCT_OPTION
struct option {
    const char *name;
    int         has_arg;
    int        *flag;
    int         val;
};
#  define no_argument       0
#  define required_argument 1
#  define optional_argument 2
#endif

/* ---- Minimal getopt / getopt_long for Windows ---- */
/* Getopt global state — static per-TU, avoids linker extern issues */
static int   _vom_optind = 1;
static int   _vom_opterr = 1;
static int   _vom_optopt = 0;
static char *_vom_optarg = NULL;
static int   _vom_getopt_initialized = 0;
static char *_vom_getopt_next = NULL;

#undef optind
#undef opterr
#undef optopt
#undef optarg
#define optind  _vom_optind
#define opterr  _vom_opterr
#define optopt  _vom_optopt
#define optarg  _vom_optarg

static inline int getopt(int argc, char *const argv[], const char *optstring) {
    char c;
    const char *place;

    if (!_vom_getopt_initialized || _vom_getopt_next == NULL) {
        if (optind == 0) { optind = 1; opterr = 1; }
        _vom_getopt_initialized = 1;
    }

    if (_vom_getopt_next != NULL && *_vom_getopt_next != '\0') {
        _vom_getopt_next++;
    } else {
        if (optind >= argc) return -1;
        if (argv[optind][0] != '-' || argv[optind][1] == '\0') return -1;
        if (argv[optind][1] == '-' && argv[optind][2] == '\0') {
            optind++;
            return -1;
        }
        _vom_getopt_next = &argv[optind][1];
        optind++;
    }

    c = *_vom_getopt_next;
    place = strchr(optstring, c);
    if (place == NULL || c == ':') {
        if (opterr) fprintf(stderr, "%s: unknown option '-%c'\n", argv[0], c);
        optopt = c;
        return '?';
    }
    if (*(place + 1) == ':') {
        if (*_vom_getopt_next != '\0') {
            optarg = _vom_getopt_next;
            _vom_getopt_next = NULL;
        } else if (optind < argc) {
            optarg = argv[optind++];
        } else {
            if (opterr) fprintf(stderr, "%s: option '-%c' requires an argument\n", argv[0], c);
            optopt = c;
            return (*optstring == ':') ? ':' : '?';
        }
    }
    return c;
}

static inline int getopt_long(int argc, char *const argv[], const char *optstring,
                              const struct option *longopts, int *longindex) {
    /* Try long options first */
    if (optind < argc && argv[optind][0] == '-' && argv[optind][1] == '-') {
        if (argv[optind][2] == '\0') { optind++; return -1; }
        const char *arg = &argv[optind][2];
        for (int i = 0; longopts[i].name != NULL; i++) {
            size_t len = strlen(longopts[i].name);
            if (strncmp(arg, longopts[i].name, len) == 0 && (arg[len] == '=' || arg[len] == '\0')) {
                if (longindex) *longindex = i;
                optind++;
                if (longopts[i].has_arg == required_argument) {
                    if (arg[len] == '=') {
                        optarg = (char *)&arg[len + 1];
                    } else if (optind < argc) {
                        optarg = argv[optind++];
                    } else {
                        if (opterr) fprintf(stderr, "%s: option '--%s' requires an argument\n", argv[0], longopts[i].name);
                        return '?';
                    }
                } else if (longopts[i].has_arg == optional_argument) {
                    if (arg[len] == '=') optarg = (char *)&arg[len + 1];
                    else optarg = NULL;
                } else {
                    optarg = NULL;
                }
                if (longopts[i].flag) {
                    *longopts[i].flag = longopts[i].val;
                    return 0;
                }
                return longopts[i].val;
            }
        }
        fprintf(stderr, "%s: unrecognized option '--%s'\n", argv[0], arg);
        optind++;
        return '?';
    }
    return getopt(argc, argv, optstring);
}

/* ---- fork (not supported on Windows, returns -1) ---- */
static inline pid_t fork(void) {
    return (pid_t)(-1);
}
/* Note: execv is already declared by <process.h> on Windows. */

/* waitpid wrapper */
typedef struct {
    int  status;
    pid_t pid;
} _vom_wait_result_t;

static inline _vom_wait_result_t _vom_waitpid_wrap(pid_t pid, int *status, int options) {
    _vom_wait_result_t result = {0, -1};
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | SYNCHRONIZE, FALSE, (DWORD)pid);
    if (hProcess == NULL) return result;

    if (options & WNOHANG) {
        DWORD exitCode;
        if (GetExitCodeProcess(hProcess, &exitCode)) {
            if (exitCode == STILL_ACTIVE) {
                if (status) *status = 0;
                CloseHandle(hProcess);
                result.pid = 0;  /* no child yet */
                return result;
            }
            if (status) *status = (int)exitCode;
            result.pid = pid;
        }
    } else {
        DWORD wait_result = WaitForSingleObject(hProcess, INFINITE);
        if (wait_result == WAIT_OBJECT_0) {
            DWORD exitCode;
            GetExitCodeProcess(hProcess, &exitCode);
            if (status) *status = (int)exitCode;
            result.pid = pid;
        }
    }
    CloseHandle(hProcess);
    return result;
}

#define waitpid(pid, status, options) (_vom_waitpid_wrap((pid_t)(pid), (status), (options)).pid)

/* kill() — uses TerminateProcess on Windows */
static inline int kill(pid_t pid, int sig) {
    if (sig == SIGKILL || sig == SIGTERM) {
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, (DWORD)pid);
        if (hProcess) {
            TerminateProcess(hProcess, (UINT)sig);
            CloseHandle(hProcess);
            return 0;
        }
    }
    return -1;
}

/* dup2() — uses _dup2 */
#define dup2(oldfd, newfd) _dup2(oldfd, newfd)

/* pipe() — uses _pipe with binary mode */
#undef pipe
#define pipe(fds) _pipe((fds), 4096, _O_BINARY)

/* ---- initgroup — no-op on Windows ---- */
#define initgroup(name, gid) ((void)(name), (void)(gid), 0)

/* ---- pthreads via Windows SRWLOCK and native threads ---- */
/* SRWLOCK can be statically initialized, unlike CRITICAL_SECTION */

typedef SRWLOCK pthread_mutex_t;

#  ifndef PTHREAD_MUTEX_INITIALIZER
#    define PTHREAD_MUTEX_INITIALIZER SRWLOCK_INIT
#  endif

typedef int (*pthread_thread_func_t)(void *);

static inline int pthread_mutex_init(pthread_mutex_t *mutex, void *attr) {
    (void)attr;
    InitializeSRWLock(mutex);
    return 0;
}

static inline int pthread_mutex_destroy(pthread_mutex_t *mutex) {
    (void)mutex;
    /* SRWLOCKs do not need explicit destruction */
    return 0;
}

static inline int pthread_mutex_lock(pthread_mutex_t *mutex) {
    AcquireSRWLockExclusive(mutex);
    return 0;
}

static inline int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    ReleaseSRWLockExclusive(mutex);
    return 0;
}

/* Thread creation — lightweight wrapper */
typedef HANDLE pthread_t;

struct _vom_thread_arg {
    void *(*func)(void *);
    void *arg;
};

static unsigned __stdcall _vom_thread_entry(void *param) {
    struct _vom_thread_arg *targ = (struct _vom_thread_arg *)param;
    targ->func(targ->arg);
    free(targ);
    return 0;
}

static inline int pthread_create(pthread_t *thread, void *attr,
                                 void *(*start_routine)(void *), void *arg) {
    (void)attr;
    struct _vom_thread_arg *targ = (struct _vom_thread_arg *)malloc(sizeof(*targ));
    if (!targ) return -1;
    targ->func = start_routine;
    targ->arg  = arg;

    HANDLE h = (HANDLE)_beginthreadex(NULL, 0, _vom_thread_entry, targ, 0, NULL);
    if (h == 0) { free(targ); return -1; }
    *thread = h;
    return 0;
}

static inline int pthread_join(pthread_t thread, void **retval) {
    (void)retval;
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    return 0;
}

/* ---- popen ---- */
#define popen _popen
#define pclose _pclose

/* ---- setTimeout / timeGetTime compat ---- */
#define usleep_for_file_ops(usec) Sleep((DWORD)((usec) / 1000))

/* ---- Provide /proc/stat and /proc/meminfo stubs ---- */
/* On Windows, sys_info.c already has #if guards for Linux-only paths.
   Provide these _SC constants for sysconf(). */

/* ======================================================================
 * POSIX / Linux Platform
 * ====================================================================== */
#else  /* !_WIN32 */

#include <unistd.h>
#include <strings.h>
#include <sys/time.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <getopt.h>
#include <fcntl.h>

#if defined(__linux__) || defined(__ANDROID__)
#  include <sys/sysinfo.h>
#  include <sys/stat.h>
#endif

/* pthreads — available natively on Linux/macOS */
#include <pthread.h>

#endif /* _WIN32 */

#endif /* VOM_COMMON_COMPAT_H */
