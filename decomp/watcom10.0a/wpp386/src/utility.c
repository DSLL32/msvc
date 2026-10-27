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

/* Initial plusplus/c/utility.c at6b9cb443, matched against the actual
 *1994 vector-copy and compiler-owned allocation routines. */
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "compiler.h"
#include "cmdln-options.h"
#include "utility.h"
#include "scanchar.h"
#include "pending.h"
#include <recon.h>

char *stvcpy_after(char *target, const char **source, unsigned size)
{
    const char *p;
    for (p=*source; size>0; --size) *target++=*p++;
    *target++='\0';
    *source=p;
    return target;
}

char *stvcpy(char *target, const char *source, unsigned size)
{
    return stvcpy_after(target,&source,size)-1;
}

char *vctsave(const char *vector, unsigned size)
{
    char *result;
    result=(char *)CMemAlloc(size+1);
    stvcpy(result,vector,size);
    return result;
}

char *stpcpy_after(char *target, const char **source)
{
    const char *p;
    for (p=*source; (*target++=*p++)!=0; ) ;
    *source=p;
    return target;
}

char *stpcpy(char *target, const char *source)
{
    return stpcpy_after(target,&source)-1;
}

char *strsave(const char *name)
{
    return vctsave(name,strlen(name));
}

/* Initial utility.c and actual1994 CF5ab9b..5abb6 agree. */
int strpref(const char *prefix,const char *str)
{
    while (*prefix) {
        if (*prefix!=*str) return 0;
        ++prefix;
        ++str;
    }
    return 1;
}

char *CStrSave(const char *name)
{
    unsigned length=strlen(name)+1;
    char *copy=(char *)CPermAlloc(length);
    memcpy(copy,name,length);
    return copy;
}

/* Initial utility.c SafeAtof, actual CF5aa33..5aa7d. The period
 * stdlib.h expands atof(p) to strtod(p,NULL); spell the same public
 * conversion boundary explicitly in both builds. Old ERANGE is14. */
double SafeAtof(char *text)
{
    double result;
    errno=0;
    result=strtod(text,NULL);
    if (errno) {
        if (result==0.0 && errno==14) CErr1(173);
        else CErr1(172);
    }
    return result;
}

/* Literal escape conversion, matched to actual CF5aa97..5ab5e. */
unsigned char hex_dig(                   // GET HEXADECIMAL DIGIT FOR CHAR (OR 16)
    unsigned char c )                    // - character
{
    char char_class;            // - character class

    char_class = CharSet[ c ];
    if(( char_class & (C_HX|C_DI) ) == 0 ) {
        return( 16 );
    }
    if( char_class & C_HX ) {
        /* a-f or A-F */
        c = (( c | 0x20 ) - 'a' ) + 10 + '0';
    }
    return( c - '0' );
}


unsigned char octal_dig(                 // GET OCTAL DIGIT FOR CHAR (OR 8)
    unsigned char chr )                  // - character
{
    char retn;                  // - digit returned

    if( ( chr >= '0' ) && ( chr <= '7' ) ) {
        retn = chr - '0';
    } else {
        retn = 8;
    }
    return( retn );
}


unsigned char escape_char(               // GET ESCAPE CHAR FOR A LETTER
    unsigned char chr )                  // - character after "\"
{
    switch( chr ) {
      case 'a' :
        chr = 7;
        break;
      case 'b' :
        chr = 8;
        break;
      case 'f' :
        chr = 12;
        break;
      case 'n' :
        chr = 10;
        break;
      case 'r' :
        chr = 13;
        break;
      case 't' :
        chr = 9;
        break;
      case 'v' :
        chr = 11;
        break;
      case '\\' :
      case '\'' :
      case '\"' :
      case '?'  :
        break;
      default :
        chr = 0;
        break;
    }
    return( chr );
}


unsigned char classify_escape_char(      // CLASSIFY TYPE OF ESCAPE
    unsigned char chr )                  // - character after "\"
{
    char retn;                  // - classification returned

    if( octal_dig( chr ) != 8 ) {
        retn = 1;
    } else if( chr == 'x' ) {
        retn = 2;
    } else {
        retn = escape_char( chr );
    }
    return( retn );
}


