/*
 *%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 *%	  Copyright (C) 1990, by WATCOM Systems Inc. All rights     %
 *%	  reserved. No part of this software may be reproduced	    %
 *%	  in any form or by any means - graphic, electronic or	    %
 *%	  mechanical, including photocopying, recording, taping     %
 *%	  or information storage and retrieval systems - except     %
 *%	  with the written permission of WATCOM Systems Inc.	    %
 *%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  Modified:	By:		Reason:
  ---------	---		-------
  09-jun-88	M.Glaum 	Created
  08-aug-89	John Dahms	tweaked
  12-mar-90	F.W.Crigger	Add #pragma library for DOSCALLS
  15-sep-91	F.W.Crigger	_LpCmdLine, _LpPgmName added. _main_entry_
				deleted.
  ------------------------------------------------- Version 9.0 ----------
  04-aug-92	D.J.Gaudet	set argv[0] to the path to the executable 
  ------------------------------------------------- Version 10.0 ---------
  11-nov-93	Greg Bentz	exit_with_msg to xmsgo16
  28-jan-94	Greg Bentz	change exit_with_msg to __fatal_runtime_error
*/

#pragma library (DOSCALLS);

#define INCL_DOSDEVICES
#define INCL_SUB
#define INCL_DOSSEMAPHORES
#define INCL_DOSINFOSEG
#define INCL_DOSMISC
#define INCL_DOSMEMMGR
#include <wos2.h>
#include <string.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <stddef.h>
#include "exitwmsg.h"
#include "initfini.h"

#if defined(__SW_BM)

/* semaphore control for file handles */

ULONG			__iosemaphore[_NFILES];
int			__iosemcount[_NFILES];
TID			__iothreadid[_NFILES];

extern	void		*__InitThreadProcessing(void);
extern	void		__SetupThreadProcessing(int);

int			__far *_threadid;
extern	unsigned	__MaxThreads;

extern	unsigned	*__MultipleThread();
#define _STACKLOW	(*(__MultipleThread()))

#else

unsigned		_STACKLOW;

#endif

/* global data */

char __far *		_LpCmdLine;	/* pointer to command line */
char __far *		_LpPgmName;	/* pointer to program name */
unsigned		_dynend;
unsigned		_curbrk;
unsigned		_STACKTOP;
unsigned short		_HShift;
int			_cbyte;     /* used by getch, getche */
int			_cbyte2;    /* used by getch */
unsigned char __near	_osmajor;
unsigned char __near	_osminor;
unsigned char __near	_osmode;
#ifdef __SW_BD
#include <setjmp.h>
jmp_buf			JmpBuff;
int			RetCode;
#endif

/* End of static data - used in OS/2 DLL to find beginning of near heap */
extern char		end;

void __far __null_FPE_handler()
    {
    }

void	(__far *__FPE_handler)() = __null_FPE_handler;

extern	int _CMain();
	#pragma aux _CMain "_*";

	#pragma aux _OS2Main "_*" parm caller [ dx ax ] [ cx bx ];


int _OS2Main( char far *stklow, char far * stktop,
	       unsigned envseg, unsigned cmdoff ) {
/*************************************************/
    

    cmdoff = cmdoff;	/* supress warnings */
    envseg = envseg;
    stktop = stktop;

    /* set up global variables */
#if defined(__SW_BD)
    _STACKTOP = 0;
    _curbrk = _dynend = (unsigned)&end;
    stklow = NULL;
#else
    _STACKTOP = FP_OFF( stktop );
    _curbrk = _dynend = _STACKTOP;
#endif
    DosGetHugeShift( (PUSHORT)&_HShift );
    DosGetMachineMode( (PBYTE)&_osmode );
    {
    unsigned short	version;

    DosGetVersion( (PUSHORT)&version );
    _osmajor = version >> 8;
    _osminor = version & 0xff;
    }

#if defined(__SW_BD)
    _LpPgmName = "";
    _LpCmdLine = "";
#else
    /* copy progname and arguments to bottom of stack */
    {
    char		far *src;
    char		far *pgmp;

    src = MK_FP( envseg, cmdoff );
    _LpPgmName = stklow;
    /* back up from the ao: pointer to the eo: pointer (see OS/2 2.0 docs)*/
    for( pgmp = src - 1; *--pgmp != '\0'; );
    ++pgmp;
    while( *stklow++ = *pgmp++ );
    while( *src ) ++src;
    ++src;
    _LpCmdLine = stklow;
    while( *stklow++ = *src++ );
    }
#endif

#if defined(__SW_BM)
    {
	SEL		globalseg;
	SEL		localseg;

	DosGetInfoSeg( &globalseg, &localseg );
	_threadid = MK_FP( localseg, offsetof( LINFOSEG, tidCurrent ) );
	if( __InitThreadProcessing() == NULL ) _Not_Enough_Memory();
	#if defined(__SW_BD)
	{
	    unsigned	i;
	    unsigned	j;
	    j = __MaxThreads;
	    for( i = 1; i <= j; i++ ) {
		__SetupThreadProcessing( i );
	    }
	}
	#else
	    __SetupThreadProcessing( 1 );
	#endif
	_STACKLOW = (unsigned)stklow;
    }
#else
    _STACKLOW = (unsigned)stklow; /* set bottom of stack */
#endif
    {
	// make sure the iomode array is of the proper length
	// this needs to be done before the InitRtns
	extern	void	__grow_iomode(int);

	if( _osmode == OS2_MODE ) {
	    __grow_iomode( 100 );
	}
    }
    __InitRtns( 255 );
#ifdef __SW_BD
    {
	int status;
        status = setjmp( JmpBuff );
	if( status == 0 ) return( _CMain() );
	return( RetCode );
    }
#else
    return( _CMain() );
#endif
}


void _Not_Enough_Memory() {
/*************************/

    __fatal_runtime_error( "Not enough memory\r\n", 1 );
}


void	__exit( unsigned ret_code ) {
/***********************************/

    __FiniRtns( 0, 255 );
#ifdef __SW_BD
    RetCode = ret_code;
    longjmp( JmpBuff, 1 );
#else
    DosExit( EXIT_PROCESS, ret_code );
#endif
}
