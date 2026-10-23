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
  10-apr-92	Craig Eisler	made it work
  25-sep-92	Craig Eisler	handle thread/process specific stuff
  28-sep-92	G. Coschi	fixed problems with DLLs
  21-oct-92	Craig Eisler	use thread local storage
  16-dec-92	G. Coschi	call __InitMultipleThread() when LibMain()
				called with DLL_PROCESS_ATTACH
  05-feb-93	Craig Eisler	check __NTInit for failure
*/
#include <windows.h>
#include "initfini.h"

extern int APIENTRY LibMain( HANDLE, DWORD, LPVOID );
extern int __NTInit( int, void *);
extern void __CommonInit( void );

extern void __InitMultipleThread( void );
extern BOOL __NTAddThread(void *);
extern void __NTRemoveThread( void );
extern BOOL __NTThreadInit( void );
extern void __NTThreadFini( void );


int APIENTRY _LibMain( HANDLE hdll, DWORD reason, LPVOID reserved )
{
    static DWORD version;
    int	rc;

    switch( reason ) {
    case DLL_THREAD_ATTACH:
	if( !__NTAddThread( NULL ) ) {
	    return( FALSE );
	}
	rc = LibMain( hdll, reason, reserved );
	break;
    case DLL_PROCESS_ATTACH:
	__InitRtns( 1 );
	if( !__NTInit( TRUE, NULL ) ) {
	    return( FALSE );
	}
    	if( !__NTThreadInit() ) {
	    return( FALSE );
	}
	__InitRtns( 15 );
	__InitMultipleThread();
	__InitRtns( 255 );
	__CommonInit();
	rc = LibMain( hdll, reason, reserved );
	if( rc ) {
	    version = GetVersion();
	    if( version & 0x80000000 ) {
		if( __NTAddThread( NULL ) ) {
		    rc = LibMain( hdll, DLL_THREAD_ATTACH, reserved );
		} else {
		    rc = FALSE;
		}
	    }
	}
	if( !rc ) {
	    __FiniRtns( 0, 255 );
	}
	break;
    case DLL_THREAD_DETACH:
	rc = LibMain( hdll, reason, reserved );
	__NTRemoveThread();
	break;
    case DLL_PROCESS_DETACH:
	if( version & 0x80000000 ) {
	    LibMain( hdll, DLL_THREAD_DETACH, reserved );
	    __NTRemoveThread();
	}
	rc = LibMain( hdll, reason, reserved );
    	__FiniRtns( 0, 255 );
    }
    return( rc );
}
