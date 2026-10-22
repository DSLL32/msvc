#ifndef RC_LOWIO_H
#define RC_LOWIO_H
#include "rc.h"
#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <errno.h>
extern unsigned char _osfile[];
extern long _osfhnd[];
extern int _nhandle, _umaskval, _fmode;
extern unsigned long _doserrno;
int __cdecl _alloc_osfhnd(void);
int __cdecl _set_osfhnd(int, long);
int __cdecl _free_osfhnd(int);
void __cdecl _dosmaperr(unsigned long);
#endif
