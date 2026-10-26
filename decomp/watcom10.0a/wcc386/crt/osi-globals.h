#ifndef WCC_OSI_GLOBALS_H
#define WCC_OSI_GLOBALS_H
#include <recon.h>
extern char *_Copyright;
extern char *_BreakFlagPtr;
extern char __OS;
#if defined(__WATCOMC__)
#pragma aux __OS "*";
#endif
enum osi_os_type { OS_DOS,OS_OS2,OS_NT,OS_WIN };
#endif
