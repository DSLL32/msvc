/* Process startup, original 0x01011950..0x01011aa6. */
#include "rc.h"
#include <stdlib.h>
#include <excpt.h>
#include <recon.h>

char *_acmdln;
char *_aenvptr;
extern unsigned int _osver, _winver, _winmajor, _winminor;
extern int __argc;
extern char **__argv, **_environ;
unsigned int os2_version, os2_major, os2_minor;
unsigned int compatibility_version, compatibility_major, compatibility_minor;

void __cdecl _heap_init(void);
void __cdecl _ioinit(void);
void __cdecl _setargv(void);
void __cdecl _setenvp(void);
void __cdecl _cinit(void);
int __cdecl _XcptFilter(unsigned long, struct _EXCEPTION_POINTERS *);
int __cdecl resource_main(int, char **, char **);

void __cdecl mainCRTStartup(void)
{
    unsigned long version;
    int startup_exception;

    _acmdln = GetCommandLineA();
    _aenvptr = GetEnvironmentStrings();
    version = GetVersion();
    _winminor = (version >> 8) & 255;
    _winmajor = version & 255;
    _winver = (_winmajor << 8) + _winminor;
    _osver = version >> 16;
    os2_version = _osver;
    os2_major = os2_version & 255;
    os2_minor = (os2_version >> 8) & 255;
    compatibility_version = version & 65535;
    compatibility_major = compatibility_version & 255;
    compatibility_minor = (compatibility_version >> 8) & 255;
    _heap_init();
    _ioinit();
    _setargv();
    _setenvp();
    _cinit();
    __try {
        exit(resource_main(__argc, __argv, _environ));
    }
    __except (_XcptFilter(startup_exception = GetExceptionCode(),
                         GetExceptionInformation())) {
        _exit(startup_exception);
    }
}

void (__cdecl *_aexit_rtn)(int) = _exit;
void __cdecl _FF_MSGBANNER(void);
void __cdecl _NMSG_WRITE(int);
/* Original 0x01011ab0..0x01011ad1. */
void __cdecl _amsg_exit(int message)
{
    __asm int 3
}
