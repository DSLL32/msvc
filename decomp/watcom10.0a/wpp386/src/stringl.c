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


/* Adapted initial6b9cb443 stringl.c. Actual1994 CF4d952..4d965
 * only clears the unique-string flag, with no later list/count resets. */
#include <stddef.h>
#include "compiler.h"
#include "stringl.h"
#include "scan.h"
#include "frontend.h"
#include "scanchar.h"
#include "utility.h"
#define DEFINE_UNICODE
#include "unicode.h"
#include "pending.h"
#include <recon.h>
unsigned int JIS2Unicode(unsigned short dbc)
{ (void)dbc; STUB_TRAP(); return 0; }
static struct { unsigned unique:1; } stringData;
void stringInit(INITFINI *defn) { (void)defn; stringData.unique=0; }

static struct string_constant *uniqueStrings,*trashedStrings;
static unsigned stringCount;
struct string_constant *initLiteral(struct string_constant *literal)
{
    literal->next=NULL;
    literal->cg_handle=NULL;
    literal->segid=0;
    literal->concat=0;
    literal->multi_line=0;
    literal->wide_string=0;
    return literal;
}
struct string_constant *findLiteral(unsigned len)
{
    struct string_constant *literal=trashedStrings,*prev=NULL;
    while(literal) {
        if(len<=literal->len && len>=literal->len/2) {
            if(prev) prev->next=literal->next;
            else trashedStrings=literal->next;
            return initLiteral(literal);
        }
        prev=literal;
        literal=literal->next;
    }
    literal=CPermAlloc(sizeof(*literal)+len);
    return initLiteral(literal);
}
unsigned compressLiteral( char *tgt, char *str, unsigned len )
/********************************************************************/
{
    unsigned chr;               // - current character
    char     chr_1;             // - one char ahead of current char
    char     classification;    // - escape classification
    int      max_digs;          // - max digits remaining
    unsigned new_len;           // - length after escapes processed
    unsigned wide_string;

#define store_chr(tgt,chr)  if(tgt) *tgt++ = (chr)

    wide_string = 0;
    if( CurToken == 163 ) {
        wide_string = 1;
    }
    new_len = 0;
    for( ; len > 0; ) {
        chr = (unsigned char)*str++;
        --len;
        if( ( len > 0 ) && ( chr == '\\' ) ) {
            chr = (unsigned char)*str++;
            -- len;
            classification = classify_escape_char( chr );
            if( classification == 1 ) {
                chr_1 = octal_dig( chr );
                chr = 0;
                max_digs = 3;
                for( ; ; ) {
                    chr = ( chr << 3 ) | chr_1;
                    if( len == 0 ) break;
                    if( --max_digs == 0 ) break;
                    chr_1 = octal_dig( *str );
                    if( chr_1 == 8 ) break;
                    -- len;
                    ++ str;
                }
            } else if( classification == 2 ) {
                if( ( len > 1 ) && ( 16 != hex_dig( *str ) ) ) {
                    chr = 0;
                    max_digs = 8;
                    for( ; ; ) {
                        if( len == 0 ) break;
                        if( max_digs == 0 ) break;
                        chr_1 = hex_dig( *str );
                        if( chr_1 == 16 ) break;
                        chr = ( chr << 4 ) | chr_1 ;
                        -- max_digs;
                        ++ str;
                        -- len;
                    }
                }
            } else if( classification != 0 ) {
                chr = classification;
            }
            if( wide_string ) {
                store_chr( tgt, chr );
                ++ new_len;
                chr = chr >> 8;
            }
        } else {
            if( CharSet[chr] & C_DB ) {       /* if double-byte character */
                if( CompFlags.jis_to_unicode && wide_string ) {
                    chr = (chr << 8) + (unsigned char)*str;
                    chr = JIS2Unicode( chr );
                    store_chr( tgt, chr );
                    chr = chr >> 8;
                } else {
                    store_chr( tgt, chr );
                    chr = (unsigned char)*str;
                }
                ++ new_len;
                ++ str;
                -- len;
            } else if( wide_string ) {
                if( CompFlags.use_unicode ) {           /* 05-jun-91 */
                    chr = UniCode[ chr ];
                } else if( CompFlags.jis_to_unicode ) {
                    chr = JIS2Unicode( chr );
                }
                store_chr( tgt, chr );
                ++ new_len;
                chr = chr >> 8;
//          } else {
//              _ASCIIOUT( chr );
            }
        }
        store_chr( tgt, chr );
        ++new_len;
    }
    --new_len;  /* take one of the '\0' from the end */
    return( new_len );
}


#undef store_chr

struct string_constant *makeLiteral(char *text,unsigned length)
{
    unsigned new_len=length;
    struct string_constant *literal;
    if(CurToken==163) new_len=compressLiteral(NULL,text,length+1);
    literal=findLiteral(new_len);
    literal->len=compressLiteral(literal->string,text,length+1);
    if(CurToken==163) literal->wide_string=1;
    return literal;
}
struct string_constant *stringAdd(struct string_constant *literal,
                                   struct string_constant **list)
{
    literal->next=*list;
    *list=literal;
    return literal;
}
struct string_constant *StringCreate(char *text,unsigned length)
{
    ++stringCount;
    return stringAdd(makeLiteral(text,length),&uniqueStrings);
}

void StringTrash(struct string_constant *literal)
{
    struct string_constant **head=&uniqueStrings,*search;
    while((search=*head)!=NULL) {
        if(literal==search) {
            --stringCount;
            *head=search->next;
            literal->next=trashedStrings;
            trashedStrings=literal;
            return;
        }
        head=&search->next;
    }
}
