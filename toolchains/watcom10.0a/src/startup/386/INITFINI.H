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
// 11-nov-93	Greg Bentz	defined to rationalize init and fini routines
//
//
#ifndef _INITFINI_H_INCLUDED
#define _INITFINI_H_INCLUDED

#if defined(__386__)
  #define __PARMREG1	eax
  #define __PARMREG2	edx
#else
  #define __PARMREG1	ax
  #define __PARMREG2	dx
#endif

extern void __InitRtns(int);
// - takes priority limit parm in __PARMREG1
//	code will run init routines whose
//	priority is <= __PARMREG1 (really [0-255])
//	__PARMREG1==255 -> run all init routines
//	__PARMREG1==15  -> run init routines whose priority is <= 15
#pragma aux __InitRtns "*" parm [__PARMREG1]

extern void __FiniRtns(int,int);
// - takes priority limit range in __PARMREG1, __PARMREG2
//	code will run fini routines whose
//	priority is >= __PARMREG1 (really [0-255]) and
//		    <= __PARMREG2 (really [0-255])
//	__PARMREG1==0 ,__PARMREG2==255 -> run all fini routines
//	__PARMREG1==16,__PARMREG2==255 -> run fini routines in range 16...255
//	__PARMREG1==16,__PARMREG2==40  -> run fini routines in range 16...40
#pragma aux __FiniRtns "*" parm [__PARMREG1] [__PARMREG2]

#undef __PARMREG1
#undef __PARMREG2

#endif
