#ifndef RECON_NATIVE_SIGNAL_H
#define RECON_NATIVE_SIGNAL_H
#include_next <signal.h>
#if defined(RECON_CRT_PROFILE_WATCOM_100A) && !defined(RECON_SHIM_IMPL)
/* Watcom 10.0a SIGNAL.H tokens; these are not Linux signal numbers. */
typedef void (*__sig_func)(int);
#undef SIG_IGN
#undef SIG_DFL
#undef SIG_ERR
#define SIG_IGN ((__sig_func)1)
#define SIG_DFL ((__sig_func)2)
#define SIG_ERR ((__sig_func)3)
#undef SIGABRT
#undef SIGFPE
#undef SIGILL
#undef SIGINT
#undef SIGSEGV
#undef SIGTERM
#undef SIGBREAK
#undef SIGUSR1
#undef SIGUSR2
#define SIGABRT 1
#define SIGFPE 2
#define SIGILL 3
#define SIGINT 4
#define SIGSEGV 5
#define SIGTERM 6
#define SIGBREAK 7
#define SIGUSR1 8
#define SIGUSR2 9
#define SIGUSR3 10
#define SIGIDIVZ 11
#define SIGIOVFL 12
#define _SIGMIN 1
#define _SIGMAX 12
#endif
#endif
