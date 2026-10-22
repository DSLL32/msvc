#ifndef RC_CRT_RUNTIME_H
#define RC_CRT_RUNTIME_H
#include "rc.h"
typedef void (__cdecl *CRTCallback)(void);
extern CRTCallback _FPinit;
extern CRTCallback *__onexitbegin, *__onexitend;
extern CRTCallback __xi_a[], __xi_z[], __xc_a[], __xc_z[];
extern CRTCallback __xp_a[], __xp_z[], __xt_a[], __xt_z[];
extern char _exitflag;
void __cdecl _initterm(CRTCallback *, CRTCallback *);
void __cdecl doexit(int, int, int);
#endif
