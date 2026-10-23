/*
   Define constants so that OS/2-specific source files can be used for both
   OS/2 1.x and OS/2 2.x.
*/
#ifndef WOS2_H_INCLUDED
#define WOS2_H_INCLUDED

#define INCL_BASE
#include <os2.h>

#if defined(__386__)

#define	OS_UINT		ULONG
#define	OS_PUINT	PULONG
#define __FAR
#if defined(__OS2__)
    #if defined(__OS220__)
	#undef  __OS220__
    #endif
    #define __OS220__
#endif

#else

#define	APIRET		USHORT
#define	OS_UINT		USHORT
#define	OS_PUINT	PUSHORT
#define __FAR		__far

/* values returned by DosQHandType() */

#define HANDTYPE_FILE     0x0000
#define HANDTYPE_DEVICE   0x0001
#define HANDTYPE_PIPE     0x0002
#define HANDTYPE_NETWORK  0x8000

#endif

#endif /* WOS2_H_INCLUDED */
