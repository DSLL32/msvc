/****************************************************************************
*
*                            Open Watcom Project
*
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


/* Adapted from bld/wasm/c/tbyte.c at6b9cb443. The1994 application
 * performs binary64-to-ten-byte field conversion through its own helpers. */
#include "asmparse.h"
#include <recon.h>
#define TBYTE_MAX_EXP 16383
/* The original adds0x3c00 to the eleven-bit binary64 exponent. Avoid
 * inheriting the host's different long-double constants. */
#define BINARY64_EXP_BIAS 1023
#define LDBL_EXP_DIG 11
#define LDBL_MANTISSA_DIG 52
#define INBYTES 8
#define OUTBYTES 10
/* CF8bc0..8bcb, byte objects can alias. */
void swap(char *first, char *second)
{
    char tmp = *first;
    *first = *second;
    *second = tmp;
}
/* CF8bcc..8c11, reverse the byte order in the caller-owned array. */
void little_to_big_endian(char *data, unsigned char count)
{
    unsigned char i;
    for (i = 0; i < count / 2; i++) swap(&data[i], &data[count - 1 - i]);
}
/* CF8c14..8c90, copy exactly the requested mantissa bits. */
void copy_mantissa( char *input, char *output, unsigned char no_of_bits )
/***********************************************************************/
{
    #define     FIRST_BIT 0x80

                /* input_bit & output_bit are bit-masks -
                 * 1 bit is on at a time, moving left to right in the byte
                 */
    unsigned char      input_bit = 0x08;
    unsigned char      output_bit = 0x40;


    char        *input_byte = input + 1;
    char        *output_byte = output + 2;
    unsigned char      bitcount;

    for( bitcount = 0; bitcount < no_of_bits; bitcount++ ) {
        if( (*input_byte) & input_bit ) {
            /* if the input bit is set, turn on the corresponding output bit */
            (*output_byte) |= output_bit;
        }
        input_bit >>=1;
        if( !input_bit ) {
            input_bit = FIRST_BIT;
            input_byte++;
        }
        output_bit >>=1;
        if( !output_bit ) {
            output_bit = FIRST_BIT;
            output_byte++;
        }
    }
    return;
}



/* CF8c90..8d1a. */
void double2tbyte( double data, char *output )
/********************************************/
{
    double              save;
    char                *input;
    char                *tmp;
    unsigned short int  *tmp2;
    unsigned long int   sign, exponent;

    save = data;
    input = (char *)&data;

    exponent = *((unsigned short int *)(input+INBYTES-2));
    sign = exponent & 0x8000; /* get the 1st bit */
    exponent &= 0x7fff; /* get rid of sign bit */
    exponent >>= ( 15 - LDBL_EXP_DIG );
    /* now use this to kill off those digits in input */

    if( OUTBYTES == 10 && save != 0.0 ) {
        /* we need to add a 1 for the integer part */
        tmp = output + OUTBYTES - 3;
        (*tmp) |= 0x80; /* put a 1 in the integer bit */
    }

    /* now put the sign bit & exponent into the output */
    exponent -= BINARY64_EXP_BIAS;   /* unbiased */
    exponent += TBYTE_MAX_EXP;  /* biased for output type */

    tmp2 = (unsigned short int*)(output + OUTBYTES - 2);
    (*tmp2) |= exponent;

    if( sign ) {
        tmp = output + OUTBYTES - 1;
        (*tmp) |= 0x80; /* sign bit - leftmost bit of last byte */
    }

    /*=============================================================*/

    /* step 1 is done.
     * now flip both input & output into big-endian format & copy
     * the mantissa over bit by bit
     */

    little_to_big_endian( input, 8 );
    little_to_big_endian( output, 10 );

    copy_mantissa( input, output, LDBL_MANTISSA_DIG );

    little_to_big_endian( input, 8 ); /* same both ways */
    little_to_big_endian( output, 10 );

    return;
}
