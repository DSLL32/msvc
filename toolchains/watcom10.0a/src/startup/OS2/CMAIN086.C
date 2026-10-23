/*
 *%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 *%       Copyright (C) 1986,1988 by WATCOM Systems Inc. All rights %
 *%       reserved. No part of this software may be reproduced      %
 *%       in any form or by any means - graphic, electronic or      %
 *%       mechanical, including photocopying, recording, taping     %
 *%       or information storage and retrieval systems - except     %
 *%       with the written permission of WATCOM Systems Inc.        %
 *%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  Modified:     By:             Reason:
  ---------     ---             -------
  22-nov-86     F.W.Crigger     Initial implementation
  16-feb-87     M.Glaum         Fix Command Line
  14-sep-87     F.W.Crigger     Don't remove quotes from parms
  21-oct-87     F.W.Crigger     Added fcloseall routine
  02-nov-87     A.F.Scian       made quoted parms like Microsoft and Turbo
  12-nov-87     F.W.Crigger     Added call to _setenvp()
  16-nov-87     F.W.Crigger     added fflush for stdxxx in exit.
  02-dec-87     F.W.Crigger     added call to _clock_init().
  08-dec-87     M.Glaum         added big data code
  13-dec-87     A.F.Scian       made atexit fail to add more functions
                                if exit handling has been initiated
  04-jan-88     F.W.Crigger     Allow '\\' in _SplitParms
  04-feb-88     F.W.Crigger     Added _init_files routine
---------------------------------------------------------Version 6.0--------
  03-apr-88     F.W.Crigger     set _amblksiz to 1 for memory allocations
                                done during initialization, then 8K for user.
  25-may-88     F.W.Crigger     _init_files exported for Express
  03-aug-88     A.F.Scian       moved DS expansion from CSTART to here
---------------------------------------------------------Version 6.5--------
  24-oct-88     F.W.Crigger     _init_files - add logic for _NFILES > 20
                                _iomode array moved to this file
  07-dec-88     F.W.Crigger     clock initialization done by cstart now
                                Do indirect call through _main_entry_ to
                                get to user's program instead of always
                                calling _init_argv and then main
  19-dec-88     F.W.Crigger     __Null_argv() defined so main works in overlay
  18-jan-89     F.W.Crigger     parms added to prototype for _main_entry
  25-may-89     John Dahms      moved out _NFiles and iomode variables
  08-aug-89     John Dahms      adapted for OS2
  09-jul-90     F.W.Crigger     Turn on _IONBF for stdout file if it isatty
---------------------------------------------------------Version 8.0--------
  16-jan-91     A.F.Scian       added code for DOS versions before 3.30
                                that can increase the number of file handles
                                (technique is due to Harvey from Opus Forte)
  22-feb-91     A.F.Scian       removed setting of _amblksiz to 1
  16-mar-91     B.J. Stecher    allowed _main_entry_ to be outside of DGROUP
                                in big data models (for -nd, -zt switches)
  15-sep-91	F.W.Crigger	__Argc, __Argv added. _main_entry_ deleted
---------------------------------------------------------Version 8.5--------
  03-dec-91	G. Coschi	allowed >20 files for OS/2
---------------------------------------------------------Version 9.0--------
  04-mar-92	F.W.Crigger	___Argc, ___Argv moved to separate module
  19-may-92	F.W.Crigger	Added code to handle > 20 files in Windows
  22-nov-92	F.W.Crigger	change reference to _iomode array
  30-nov-92	F.W.Crigger	_init_files moved to separate module
   3-jan-92	B.J. Stecher	don't need _init_files anymore
  23-aug-94	Greg Bentz	call to _nheapgrow driven by initializer
  				in fmalloc object
*/

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#pragma aux     _CMain  "_*";

/*
	___Argc, ___Argv will be filled in by an initializer routine
	if main is defined with parameters.
*/
extern	int	___Argc;	/* argument count */
extern	char **	___Argv;	/* argument vector */


#if defined(__SW_BD)
 extern	unsigned __dll_initialize();
 int _CMain()
    {
        _amblksiz = 8 * 1024;   /* set minimum memory block allocation */
        return( __dll_initialize() );
    }
#else
 extern	int	main( int, char ** );
 void _CMain()
    {
        _amblksiz = 8 * 1024;   /* set minimum memory block allocation */
        exit( main( ___Argc, ___Argv ) );
    }
#endif

