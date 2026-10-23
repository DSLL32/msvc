/*
 *%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 *%	  Copyright (C) 1991, by WATCOM Systems Inc. All rights     %
 *%	  reserved. No part of this software may be reproduced	    %
 *%	  in any form or by any means - graphic, electronic or	    %
 *%	  mechanical, including photocopying, recording, taping     %
 *%	  or information storage and retrieval systems - except     %
 *%	  with the written permission of WATCOM Systems Inc.	    %
 *%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  Modified:	By:		Reason:
  ---------	---		-------
  91/07/03	G. Coschi	initial implementation
  ----------------------------------------------- Version 9.0 -------
  92/08/04	D.J.Gaudet	make argv[0] the path to the executable
  92/10/26	Greg Bentz	make InitRtns() come after InitThreadProc
  92/11/07	F.W.Crigger	cleaned up thread support
  93/02/16	G. Coschi	__OS2Init() takes argument indicating if in
  				a DLL
*/

#define INCL_DOSEXCEPTIONS
#include <wos2.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include "initfini.h"

extern	void		__OS2MainInit( EXCEPTIONREGISTRATIONRECORD *,
				       void *, unsigned, char *,
				       char * );
extern	void		__CMain( void );

extern	unsigned	__ThreadDataSize;

#pragma aux __CMain   "*"
#pragma aux __OS2Main "*" parm caller []


void __OS2Main( unsigned hmod, unsigned reserved, char *env, char *cmd )
/**********************************************************************/
{
    EXCEPTIONREGISTRATIONRECORD	xcpt;
    void			*tdata;

    __InitRtns( 1 );

    tdata = __alloca( __ThreadDataSize );
    memset( tdata, 0, __ThreadDataSize );
    // first_thread_data.__allocated = 0;

    reserved = reserved;
    __OS2MainInit( &xcpt, tdata, hmod, env, cmd );
    __CMain();
}
