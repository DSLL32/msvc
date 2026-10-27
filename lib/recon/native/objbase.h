#ifndef RECON_NATIVE_OBJBASE_H
#define RECON_NATIVE_OBJBASE_H
/* The encountered native subset of the VC5 OLE32 SDK declaration surface. */
#include <win32.h>
#ifndef WINOLEAPI_
#ifdef __cplusplus
#define WINOLEAPI_(type) extern "C" type WINAPI
#else
#define WINOLEAPI_(type) extern type WINAPI
#endif
#endif
WINOLEAPI_(void) CoUninitialize(void);
#endif
