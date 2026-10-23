/*
 *%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 *%       Copyright (C) 1986,1991 by WATCOM Systems Inc. All rights %
 *%       reserved. No part of this software may be reproduced      %
 *%       in any form or by any means - graphic, electronic or      %
 *%       mechanical, including photocopying, recording, taping     %
 *%       or information storage and retrieval systems - except     %
 *%       with the written permission of WATCOM Systems Inc.        %
 *%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  Modified:     By:             Reason:
  ---------     ---             -------
  18-mar-92     Craig Eisler    defined for NT
  17-nov-92     F.W.Crigger     call  __NTMainInit
  23-nov-92	Craig Eisler	exception handling
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <windows.h>
#include "initfini.h"

typedef struct _REGISTRATION_RECORD {
    struct _REGISTRATION_RECORD	*RegistrationRecordPrev;
    void			*RegistrationRecordFilter;
} REGISTRATION_RECORD;
#define __EXCEPTION_RECORD struct _REGISTRATION_RECORD

extern	void		*__InitThreadData( void * );
extern	void		__NTMainInit( void *, void * );
extern	void		__CMain( void );

extern	unsigned	__ThreadDataSize;

#pragma aux __CMain  "*"
#pragma aux __NTMain "*"

void __NTMain( void )
/*******************/
{
    REGISTRATION_RECORD	rr;
    void		*tdata;

    __InitRtns( 1 );

    tdata = __alloca( __ThreadDataSize );
    memset( tdata, 0, __ThreadDataSize );
    // first_thread_data.__allocated = 0;

    __NTMainInit( &rr, tdata );
    __CMain();
}
