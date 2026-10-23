#ifndef RECON_NATIVE_FLOAT_H
#define RECON_NATIVE_FLOAT_H
#include_next <float.h>
/* Microsoft's abstract control word is not the x87 control-word encoding. */
#define _MCW_EM 0x0008001f
#define _EM_INEXACT 0x00000001
#define _EM_UNDERFLOW 0x00000002
#define _EM_OVERFLOW 0x00000004
#define _EM_ZERODIVIDE 0x00000008
#define _EM_INVALID 0x00000010
#define _EM_DENORMAL 0x00080000
#define _MCW_RC 0x00000300
#define _RC_NEAR 0x00000000
#define _RC_DOWN 0x00000100
#define _RC_UP 0x00000200
#define _RC_CHOP 0x00000300
#define _MCW_PC 0x00030000
#define _PC_64 0x00000000
#define _PC_53 0x00010000
#define _PC_24 0x00020000
#define _MCW_IC 0x00040000
#define _IC_AFFINE 0x00040000
#define _IC_PROJECTIVE 0x00000000
#define _CW_DEFAULT (_RC_NEAR | _PC_64 | _EM_INVALID | _EM_ZERODIVIDE | _EM_OVERFLOW | _EM_UNDERFLOW | _EM_INEXACT)
#ifdef __cplusplus
extern "C" {
#endif
unsigned int _control87(unsigned int, unsigned int);
unsigned int _controlfp(unsigned int, unsigned int);
#ifdef __cplusplus
}
#endif
#endif
