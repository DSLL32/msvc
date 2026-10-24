#ifndef RECON_NATIVE_MBCTYPE_H
#define RECON_NATIVE_MBCTYPE_H
#include <mbstring.h>
/* Encountered Microsoft SDK classification and codepage-selection surface.
 * Selection itself remains unsupported; the array aliases shared CRT state. */
#define _M1 0x04
#define _MB_CP_LOCALE (-4)
#ifdef __cplusplus
extern "C" {
#endif
extern unsigned char _mbctype[257];
int _setmbcp(int);
#ifdef __cplusplus
}
#endif
#endif
