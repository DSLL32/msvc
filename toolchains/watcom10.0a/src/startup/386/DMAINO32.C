/*
 *%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 *%	  Copyright (C) 1992, by WATCOM Systems Inc. All rights     %
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
  92/12/14	G. Coschi	alternate stack for F77 co-routines
  93/02/16	G. Coschi	__OS2Init() takes argument indicating if in
  				a DLL
  18-jun-93	John Dahms  	all library allocations use lib_xxx routines
*/
#define INCL_DOSMISC
#define INCL_DOSSEMAPHORES
#define INCL_DOSPROCESS
#define INCL_DOSEXCEPTIONS
#include <wos2.h>

#include <stdio.h>
#include <io.h>
#include <i86.h>
#include <stdlib.h>
#include "initfini.h"

extern	unsigned	__hmodule;

#pragma aux __LibMain "*" parm caller []

extern	char 		*_Envptr;

extern	char		*_LpCmdLine;	/* pointer to command line */
extern	char		*_LpPgmName;	/* pointer to program name */
extern	unsigned	__MaxThreads;
extern	unsigned	__ASTACKSIZ;	/* alternate stack size */
extern	char		*__ASTACKPTR;	/* alternate stack pointer */
extern  char		*_STACKLOW;

extern	unsigned	__dll_initialize(void);
extern	unsigned	__dll_terminate(void);
extern	void		__OS2Init(int, void *);
extern	int		__OS2AddThread(int, void *);
extern	void		__CommonInit(void);
extern	void		*__InitThreadProcessing(void);
extern	void		__InitMultipleThread(void);

#pragma aux	__ASTACKPTR "*";
#pragma aux	__ASTACKSIZ "*";


unsigned __LibMain( unsigned hmod, unsigned termination )
/*******************************************************/
{
    unsigned	rc;
    unsigned	i;
    PTIB	pptib;
    PPIB	pppib;

    if( termination != 0 ) {
	rc = __dll_terminate();
	__FiniRtns( 0, 255 );
	return( rc );
    }
    DosGetInfoBlocks( &pptib, &pppib );
    _Envptr = pppib->pib_pchenv;
    _LpCmdLine = "";
    _LpPgmName = "";

    __hmodule = hmod;

    __InitRtns( 1 );
    if( __InitThreadProcessing() == NULL ) return( 0 );
    __OS2Init( TRUE, NULL );
    for( i = 2; i <= __MaxThreads; i++ ) {
	if( !__OS2AddThread( i, NULL ) ) return( 0 );
    }
    __InitRtns( 15 );
    __InitMultipleThread();
    __InitRtns( 255 );
    __CommonInit();
    /* allocate alternate stack for F77 */
    __ASTACKPTR = _STACKLOW + __ASTACKSIZ;
    return( __dll_initialize() );
}
