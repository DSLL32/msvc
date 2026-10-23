#include <float.h>

/* This is the default value loaded into the 80x87 control word.
   If you want higher precision, then change PC_53 to PC_64. The
   reason we have the default set to PC_53 is generate reproducible
   floating-point results regardless of the level of optimization
   chosen for the compiler.
   This value is loaded when the 80x87 is initialized by the startup
   code, and also when _fpreset is called.
*/
#pragma aux __8087cw "*";

/*                          0x1000  | 0x0000  | 0x0200 | 0x007F */
unsigned short __8087cw = IC_AFFINE | RC_NEAR | PC_53  | 0x007F;
