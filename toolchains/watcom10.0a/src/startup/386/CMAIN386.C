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
  22-nov-86     F.W.Crigger     Initial implementation
  19-nov-92	F.W.Crigger	__CommonInit() and _init_files moved to cinit
*/
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

extern	int		___Argc;	/* argument count */
extern	char		**___Argv;	/* argument vector */
#ifndef __OSI__
extern	void	__CommonInit( void );
extern	unsigned	__ASTACKSIZ;	/* alternate stack size */
extern	char		*__ASTACKPTR;	/* alternate stack pointer */
#pragma	aux	__ASTACKPTR "*"
#pragma	aux	__ASTACKSIZ "*"
#endif

extern	int	main( int, char ** );

#pragma aux     __CMain  "*";

void __CMain()
/************/
{
    #ifndef __OSI__
	/* allocate alternate stack for F77 */
	__ASTACKPTR = (char *)alloca( __ASTACKSIZ ) + __ASTACKSIZ;
	__CommonInit();
    #endif
    exit( main( ___Argc, ___Argv ) );
}
