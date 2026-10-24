#ifndef RECON_NATIVE_MALLOC_H
#define RECON_NATIVE_MALLOC_H
#include_next <malloc.h>
#if defined(RECON_CRT_PROFILE_WATCOM_100A) && !defined(RECON_SHIM_IMPL)
#define _HEAPOK 0
#define _HEAPEMPTY 1
#define _HEAPBADBEGIN 2
#define _HEAPBADNODE 3
#define _HEAPEND 4
#define _HEAPBADPTR 5
#ifdef __cplusplus
extern "C" {
#endif
int _heapchk(void);
int _nheapchk(void);
#ifdef __cplusplus
}
#endif
#endif
#endif
