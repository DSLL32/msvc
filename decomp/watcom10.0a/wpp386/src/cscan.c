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

/* Initial Open Watcom cscan.c (6b9cb443), adapted to the actual 1994
 * classifier. CR is whitespace here, and delimiters use the older combined
 * classes. Later tokenSource/ReScanPtr initialization is absent. */
#include <string.h>
#include "frontend.h"
#include "pending.h"
#include <recon.h>

enum scan_class {
    SCAN_NAME, SCAN_WIDE, SCAN_NUM, SCAN_QUESTION, SCAN_DELIM,
    SCAN_HASH, SCAN_COMPOUND_DELIM, SCAN_ASSIGN_DELIM, SCAN_SLASH,
    SCAN_COLON, SCAN_MINUS, SCAN_FLOAT, SCAN_STRING, SCAN_STRING2,
    SCAN_LSTRING2, SCAN_CHARCONST, SCAN_NEWLINE, SCAN_WHITESPACE,
    SCAN_INVALID, SCAN_EOF
};
enum { LCHR_EOF=256, LCHR_CONTINUE_STRING, LCHR_CONTINUE_LSTRING, LCHR_MAX };
static unsigned char ClassTable[LCHR_MAX];
unsigned char PPState;
static const unsigned char InitClassTable[] = {
    '\n',SCAN_NEWLINE, ' ',SCAN_WHITESPACE, '\f',SCAN_WHITESPACE,
    '\r',SCAN_WHITESPACE, '\t',SCAN_WHITESPACE, '\v',SCAN_WHITESPACE,
    '\'',SCAN_CHARCONST, '"',SCAN_STRING,
    '(',SCAN_DELIM, ')',SCAN_DELIM, ',',SCAN_DELIM, ';',SCAN_DELIM,
    '?',SCAN_QUESTION, '[',SCAN_DELIM, ']',SCAN_DELIM,
    '{',SCAN_DELIM, '}',SCAN_DELIM, '~',SCAN_DELIM, '.',SCAN_FLOAT,
    '!',SCAN_ASSIGN_DELIM, '#',SCAN_HASH, '%',SCAN_ASSIGN_DELIM,
    '&',SCAN_COMPOUND_DELIM, '*',SCAN_ASSIGN_DELIM,
    '+',SCAN_COMPOUND_DELIM, '-',SCAN_MINUS, '/',SCAN_SLASH, ':',SCAN_COLON,
    '<',SCAN_COMPOUND_DELIM, '=',SCAN_ASSIGN_DELIM, '>',SCAN_COMPOUND_DELIM,
    '^',SCAN_ASSIGN_DELIM, '|',SCAN_COMPOUND_DELIM,
    '_',SCAN_NAME, 'L',SCAN_WIDE, 0,0
};

void ScanInit(void)
{
    const unsigned char *p;

    PPState = 0;
    CompFlags.scanning_c_comment = 0;
    memset(ClassTable, SCAN_INVALID, sizeof(ClassTable));
    memset(&ClassTable['A'], SCAN_NAME, 26);
    memset(&ClassTable['a'], SCAN_NAME, 26);
    memset(&ClassTable['0'], SCAN_NUM, 10);
    ClassTable[LCHR_EOF] = SCAN_EOF;
    ClassTable[LCHR_CONTINUE_STRING] = SCAN_STRING2;
    ClassTable[LCHR_CONTINUE_LSTRING] = SCAN_LSTRING2;
    for (p = InitClassTable; *p; p += 2) ClassTable[p[0]] = p[1];
}

int NextToken(void) { STUB_TRAP(); return 0; }
