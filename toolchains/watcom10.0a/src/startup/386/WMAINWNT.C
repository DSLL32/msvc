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
  18-mar-92     Craig Eisler    defined for NT
  23-nov-92	Craig Eisler	exception handling
*/
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <malloc.h>
#include <windows.h>
#include "initfini.h"

typedef struct _REGISTRATION_RECORD {
    struct _REGISTRATION_RECORD	*RegistrationRecordPrev;
    void			*RegistrationRecordFilter;
} REGISTRATION_RECORD;
#define __EXCEPTION_RECORD struct _REGISTRATION_RECORD

extern void __NTMainInit( void *, void * );
extern void __CommonInit( void );

extern	unsigned	__ASTACKSIZ;	/* alternate stack size */
extern	char		*__ASTACKPTR;	/* alternate stack pointer */
extern	unsigned	__ThreadDataSize;

#pragma	aux	__ASTACKPTR "*"
#pragma	aux	__ASTACKSIZ "*"

int APIENTRY WinMain( HANDLE hInstance, HANDLE hPrevInstance,
    LPSTR lpCmdLine, int nShowCmd );

#pragma aux __WinMain "*";
void __WinMain( void )
{
    char		*str;
    REGISTRATION_RECORD	rr;
    void		*tdata;

    __InitRtns( 1 );

    tdata = __alloca( __ThreadDataSize );
    memset( tdata, 0, __ThreadDataSize );
    // first_thread_data.__allocated = 0;

    __NTMainInit( &rr, tdata );
    /* allocate alternate stack for F77 */
    __ASTACKPTR = (char *)alloca( __ASTACKSIZ ) + __ASTACKSIZ;
    __CommonInit();
    str = strdup( GetCommandLine() );
    while( !isspace( *str ) && *str != 0 ) str++;
    while( isspace( *str ) ) str++;
    exit( WinMain( GetModuleHandle( NULL ), 0, str, SW_SHOWDEFAULT ) );
}
