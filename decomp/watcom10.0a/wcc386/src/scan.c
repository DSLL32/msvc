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


#include <string.h>
#include "compiler.h"
#include "macros.h"
#include <recon.h>

/* The older scanner uses one generic delimiter class for =, :, and *,
 * and treats CR as whitespace. Keep its actual class numbering. */
enum scan_class {
    SCAN_NAME,SCAN_WIDE,SCAN_NUM,SCAN_QUESTION,SCAN_SLASH,SCAN_MINUS,
    SCAN_DELIM1,SCAN_DELIM2,SCAN_DOT,SCAN_STRING,SCAN_STRING2,
    SCAN_CHARCONST,SCAN_NEWLINE,SCAN_WHITESPACE,SCAN_INVALID,
    SCAN_MACRO,SCAN_EOF
};
#define EOF_CHAR 256
#define CONTINUE_CHAR_STRING 257
#define MACRO_CHAR 258
static unsigned char ClassTable[259];
int CurrChar,SizeOfCount;
int (*NextChar)();

unsigned char InitClassTable[]={
    '\n',SCAN_NEWLINE,' ',SCAN_WHITESPACE,'\f',SCAN_WHITESPACE,
    '\r',SCAN_WHITESPACE,'\t',SCAN_WHITESPACE,'\v',SCAN_WHITESPACE,
    '\'',SCAN_CHARCONST,'"',SCAN_STRING,
    '(',SCAN_DELIM1,')',SCAN_DELIM1,',',SCAN_DELIM1,';',SCAN_DELIM1,
    '?',SCAN_QUESTION,'/',SCAN_SLASH,'-',SCAN_MINUS,
    '[',SCAN_DELIM1,']',SCAN_DELIM1,'{',SCAN_DELIM1,'}',SCAN_DELIM1,
    '~',SCAN_DELIM1,'.',SCAN_DOT,'!',SCAN_DELIM2,'#',SCAN_DELIM2,
    '%',SCAN_DELIM2,'&',SCAN_DELIM2,'*',SCAN_DELIM2,'+',SCAN_DELIM2,
    ':',SCAN_DELIM2,'<',SCAN_DELIM2,'=',SCAN_DELIM2,'>',SCAN_DELIM2,
    '^',SCAN_DELIM2,'|',SCAN_DELIM2,'_',SCAN_NAME,'L',SCAN_WIDE,0,0
};

unsigned char TokValue[]={
    /* space !  quote #  $   %    &   quote */
    0,144,0,110,0,172,215,0,
    /* ( ) * + , - . / */
    6,7,168,226,2,229,12,170,
    /* 0 through 9, : ; < = > ? */
    26,25,24,23,22,21,20,19,18,17,4,5,222,142,218,3,
    /* @ A through O */
    126,35,2,24,15,5,20,42,36,30,0,4,39,44,32,11,
    /* P through Z, [ backslash ] ^ _ */
    3,6,18,9,7,8,41,45,10,12,13,8,0,9,149,1,
    /* backtick, a through o */
    0,35,2,24,15,5,20,42,36,30,0,4,39,44,32,11,
    /* p through z, { | } ~ DEL */
    3,6,18,9,7,8,41,45,10,12,13,10,210,11,13,0
};

void ScanInit(void)
{

    int i,c;

    memset(ClassTable,SCAN_INVALID,256);
    memset(&ClassTable['A'],SCAN_NAME,26);
    memset(&ClassTable['a'],SCAN_NAME,26);
    memset(&ClassTable['0'],SCAN_NUM,10);
    ClassTable[EOF_CHAR]=SCAN_EOF;
    ClassTable[CONTINUE_CHAR_STRING]=SCAN_STRING2;
    ClassTable[MACRO_CHAR]=SCAN_MACRO;
    for (i=0;;i+=2) {
        c=InitClassTable[i];
        if (c==0) break;
        ClassTable[c]=InitClassTable[i+1];
    }
    CurrChar='\n';
    CompFlags.pre_processing=0;
    CompFlags.scanning_comment=0;
    SizeOfCount=0;
    NextChar=GetNextChar;
}

int KwHashValue,HashValue;

int CalcHash(char *name,int length)
{
    /* ASCII token/keyword weights, as in the original ascii.c table.
     * Entries cover space through DEL; identifier letters share weights. */

    int hash;

    hash=length+TokValue[(unsigned char)name[0]-' ']+
                TokValue[(unsigned char)name[length-1]-' '];
    while (hash>=47) hash-=47;
    KwHashValue=hash;
    hashpjw(name);
    return HashValue;
}

void hashpjw(char *name)
{
    unsigned hash;
    unsigned char c;

    hash=(unsigned char)*name++;
    if (hash!=0) {
        c=(unsigned char)*name++;
        if (c!=0) {
            hash=(hash<<4)+c;
            for (;;) {
                hash&=0x0fff;
                c=(unsigned char)*name++;
                if (c==0) break;
                hash=(hash<<4)+c;
                hash^=hash>>12;
                hash&=0x0fff;
                c=(unsigned char)*name++;
                if (c==0) break;
                hash=(hash<<4)+c;
                hash^=hash>>12;
            }
        }
    }
    HashValue=hash%241;
    hash&=0x0fff;
    if (hash>=MACRO_HASH_SIZE) hash-=MACRO_HASH_SIZE;
    MacHashValue=hash;
}
