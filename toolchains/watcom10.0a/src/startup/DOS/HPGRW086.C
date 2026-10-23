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
  25-aug-94	Greg Bentz	split out of cmain086.c
*/
#include <malloc.h>

void __CMain_nheapgrow( void ) {
    #if !defined(__SW_BD)
	#if (defined(M_I86CM) || defined(M_I86LM) || defined(M_I86HM))
	    _nheapgrow();           /* grow near heap to 64K */
	#endif
    #endif
}
