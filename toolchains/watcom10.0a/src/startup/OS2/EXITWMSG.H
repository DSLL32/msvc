//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%     Copyright (C) 1994, by WATCOM International Inc.  All rights    %
//%     reserved.  No part of this software may be reproduced or        %
//%     used in any form or by any means - graphic, electronic or       %
//%     mechanical, including photocopying, recording, taping or        %
//%     information storage and retrieval systems - except with the     %
//%     written permission of WATCOM International Inc.                 %
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
// Date		By		Reason
// ----		--		------
// 11-nov-93	Greg Bentz	defined to rationalize exit_with_msg
//
//
#ifndef _EXITWMSG_H_INCLUDED
#define _EXITWMSG_H_INCLUDED

#ifdef __cplusplus
    extern "C" {
#endif

#undef _EWM_FAR
#undef _EWM_PARM1
#undef _EWM_PARM2
#if defined(__386__)
    #define _EWM_FAR
    #define _EWM_PARM1  eax
    #define _EWM_PARM2  edx
#else
    #define _EWM_FAR    __far
    #define _EWM_PARM1  ax dx
    #define _EWM_PARM2  bx
#endif

// C interface
// - tracks normal calling convention
// - this is the funtion that is called from ASM and from C, C++
// - note there is no #pragma aborts so that debugger can trace out
extern void __exit_with_msg( char _EWM_FAR *, unsigned );
extern void __fatal_runtime_error( char _EWM_FAR *, unsigned );

// ASM interface
// - always uses register calling convention
// - this function is only called from the C implementation
//   of __exit_with_msg
extern void __do_exit_with_msg( char _EWM_FAR *, unsigned );
#pragma aux __do_exit_with_msg "*__" \
	    parm caller [_EWM_PARM1] [_EWM_PARM2];

#undef _EWM_FAR
#undef _EWM_PARM1
#undef _EWM_PARM2

// WVIDEO interface
// this function should be called before __exit_with_msg()
// to allow WVIDEO to trap runtime errors.
extern int __EnterWVIDEO( char far *string );

#ifdef __cplusplus
    };
#endif
#endif
