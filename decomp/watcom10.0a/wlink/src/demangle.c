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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "diagnostic.h"
#include "demangle.h"
#include <recon.h>

extern int checkInternal( const char * );

int __is_mangled_internal( const char *name, unsigned len )
{
    unsigned offset;

    len = len;
    offset = 2;
    switch( name[0] ) {
    case 'T':
        offset += 4;
        /* fall through */
    case 'W':
        if( name[1] == '?' ) return checkInternal( name + offset );
        break;
    }
    return 0;
}

unsigned __demangle_l( const char *input, unsigned len, char *output, unsigned size )
{
    int mangled;
    unsigned outlen;
    output_desc data;

    init_globals( 0 );
    mangled = __is_mangled( input, len );
    init_descriptor( &data, demangleEmit, &data, input, len, output, size );
    if( mangled ) do_demangle( &data );
    else do_copy( &data );
    outlen = terminateOutput( &data );
    return outlen;
}

static realloc_fn_t user_realloc;    /* original450350 */
static int next_replicate;           /* original450354 */

void init_globals( realloc_fn_t reallocator )
{
    next_replicate = 0;
    user_realloc = reallocator;
}

int __is_mangled( const char *name, unsigned len )
{
    unsigned offset;

    len = len;
    offset = 2;
    switch( name[0] ) {
    case 'T':
        offset += 4;
        /* fall through */
    case 'W':
        if( name[1] == '?' ) return offset;
        break;
    }
    return 0;
}

void init_descriptor( output_desc *data, outfunPtr ofn, void *cookie,
                      const char *input, unsigned len, char *output,
                      unsigned size )
{
    data->outfun = ofn;
    data->cookie = cookie;
    data->end = 0;
    if( len != 0 ) data->end = input + len;
    data->input = input;
    data->output = output;
    data->scope_ptr = 0;
    data->size = size;
    data->count = 0;
    data->index = 0;
    data->pending_loc = 0;
    data->scope_len = 0;
    data->suppress_output = 1;
    data->scope_index = 0;
    data->ctdt_pending = 0;
    data->cv_pending = 0;
    data->scope_name = 0;
    data->base_name = 0;
    /* Original reserved stack bits remain undefined and are never flags. */
    data->reserved = 0;
}

void do_copy( output_desc *data )
{
    const char *ptr;
    int len;
    char c;

    ptr = data->input;
    len = 0;
    c = nextChar( data );
    advanceChar( data );
    while( c ) {
        ++len;
        c = nextChar( data );
        advanceChar( data );
    }
    data->outfun( &data->cookie, DM_COPY_STRING, len, ptr );
}

char nextChar( output_desc *data )
{
    char c;

    c = *data->input;
    if( data->input == data->end ) c = '\0';
    else c = toupper( (unsigned char)c );
    return c;
}

void advanceChar( output_desc *data )
{
    if( data->input != data->end && *data->input != '\0' ) ++data->input;
}

// mangled character translations
#pragma pack(1)
typedef struct table_t {
    char const         *string;    // translated string
    char                        grouping;   // flavour of character
} table_t;
#pragma pack()
typedef char old_type_table_is_5[(sizeof(table_t) == 5) ? 1 : -1];

#define LOWER_TABLE_LIMIT           'A'

table_t const  translate_type_encoding[] = {
#define CHAR_UNUSED         0
#define CHAR_BASIC_TYPE     1
#define CHAR_POINTER        2
#define CHAR_MODIFIER       3
    /* 'A' */   { "char ",      CHAR_BASIC_TYPE },
    /* 'B' */   { "float ",     CHAR_BASIC_TYPE },
    /* 'C' */   { "char ",      CHAR_BASIC_TYPE },
    /* 'D' */   { "double ",    CHAR_BASIC_TYPE },
    /* 'E' */   { "... ",       CHAR_BASIC_TYPE },
    /* 'F' */   { "far ",       CHAR_MODIFIER },
    /* 'G' */   { "far16 ",     CHAR_MODIFIER },
    /* 'H' */   { "huge ",      CHAR_MODIFIER },
    /* 'I' */   { "int ",       CHAR_BASIC_TYPE },
    /* 'J' */   { "__based(",   CHAR_MODIFIER },
    /* 'K' */   { NULL,         CHAR_UNUSED },
    /* 'L' */   { "long ",      CHAR_BASIC_TYPE },
    /* 'M' */   { "::* ",       CHAR_POINTER },
    /* 'N' */   { "near ",      CHAR_MODIFIER },
    /* 'O' */   { NULL,         CHAR_UNUSED },
    /* 'P' */   { "* ",         CHAR_POINTER },
    /* 'Q' */   { "bool ",      CHAR_BASIC_TYPE },
    /* 'R' */   { "& ",         CHAR_POINTER },
    /* 'S' */   { "short ",     CHAR_BASIC_TYPE },
    /* 'T' */   { "long double ",CHAR_BASIC_TYPE },
    /* 'U' */   { "unsigned ",  CHAR_MODIFIER },
    /* 'V' */   { "void ",      CHAR_BASIC_TYPE },
    /* 'W' */   { "wchar_t ",   CHAR_BASIC_TYPE },
    /* 'X' */   { "const ",     CHAR_MODIFIER },
    /* 'Y' */   { "volatile ",  CHAR_MODIFIER },
    /* 'Z' */   { NULL, CHAR_UNUSED },
    /* '[' */   { NULL,         CHAR_UNUSED },
    /* '\' */   { NULL,         CHAR_UNUSED },
    /* ']' */   { NULL,         CHAR_UNUSED },
    /* '^' */   { NULL,         CHAR_UNUSED },
    /* '_' */   { "",           CHAR_BASIC_TYPE } // for ctor/dtor return type
};

static char  * const  operatorFunction[] = {
    /* A */ ">>",
    /* B */ "<<",
    /* C */ "!",
    /* D */ "[]",
    /* E */ "->",
    /* F */ "*",
    /* G */ "++",
    /* H */ "--",
    /* I */ "-",
    /* J */ "+",
    /* K */ "&",
    /* L */ "->*",
    /* M */ "/",
    /* N */ "%",
    /* O */ ",",
    /* P */ "()",
    /* Q */ "~",
    /* R */ "^",
    /* S */ "|",
    /* T */ "&&",
    /* U */ "||"
};
static char const  * const  relationalFunction[] = {
    /* A */ "==",
    /* B */ "!=",
    /* C */ "<",
    /* D */ "<=",
    /* E */ ">",
    /* F */ ">="
};
static char const  * const  assignmentFunction[] = {
    /* A */ "=",
    /* B */ "*=",
    /* C */ "+=",
    /* D */ "-=",
    /* E */ "/=",
    /* F */ "%=",
    /* G */ ">>=",
    /* H */ "<<=",
    /* I */ "&=",
    /* J */ "|=",
    /* K */ "^="
};
#define num_elements( __a ) (sizeof(__a) / sizeof(__a[0]))

typedef union key_desc {
    char        str[2];
    short       val;
} key_desc;
#pragma pack(1)
typedef struct assoc_desc {
    key_desc            u;
    char       *name;
} assoc_desc;
#pragma pack()
typedef char old_assoc_is_6[(sizeof(assoc_desc) == 6) ? 1 : -1];
static assoc_desc const  watcomObject[] = {
    { "A*",  "__internal" },
    { "BI",  "__onceonly" },
    { "DA",  "__arrdtorblk" },
    { "DF",  "__defarg" },
    { "DO",  "__dtorobjblk" },
    { "MP",  "__mbrptrthunk" },
    { "SI",  "__staticinit" },
    { "TH",  "__throwblk" },
    { "TS",  "__typesig" },
    { "VB",  "__vbtbl" },
    { "VF",  "__vftbl" },
    { "VM",  "__vmtbl" },
    { "VT",  "__vfthunk" },
    { "ST",  "__typstattab" },
    { "CM",  "__stattabcmd" },
};

void demangleEmit( void **cookie, dm_pts dp, int value, char const *ptr )
{
    output_desc *data = *((output_desc **)cookie);

    switch( dp ) {
    case DM_BASIC_TYPE:
    case DM_POINTER:
    case DM_ARRAY:
    case DM_FUNCTION:
    case DM_THIS_FUNCTION:
    case DM_UNMODIFIED_TYPE:
    case DM_BASED_ENCODING:
    case DM_MODIFIER_LIST:
    case DM_TEMPLATE_ARG:
    case DM_RECURSE_BEGIN:
    case DM_RECURSE_END:
    case DM_TEMPLATE_NAME:
    case DM_DESTRUCTOR:
    case DM_CONSTRUCTOR:
    case DM_OPERATOR_CONVERT:
    case DM_CTOR_DTOR_NAME:
    case DM_SCOPED_NAME:
    case DM_NAME:
    case DM_SCOPE:
    case DM_MANGLED_NAME:
    case DM_TRUNCATED_NAME:
        // ignore these for demangling
        break;
    case DM_TYPE_ENCODING:
        if( data->base_name ) {
            forceSuppression( data );
        }
        break;
    case DM_SET_INDEX:
        setEmitIndex( data, value );
        break;
    case DM_RESET_INDEX:
        resetEmitIndex( data, value );
        break;
    case DM_ZAP_SPACE:
        zapSpace( data );
        break;
    case DM_EMIT_SPACE:
        emitChar( data, ' ' );
        break;
    case DM_IDENTIFIER:
    case DM_COPY_STRING:
        if( data->base_name && data->ctdt_pending ) {
            unforceSuppression( data );
        }
        if( value == 0 ) {
            emitStr( data, ptr );
        } else {
            while( value-- ) {
                emitChar( data, *ptr++ );
            }
        }
        if( data->base_name && data->ctdt_pending ) {
            forceSuppression( data );
        }
        break;
    case DM_INTEGER:
        {
            char buff[12];
            itoa( value, buff, 10 );
            emitStr( data, buff );
        }
        break;
    case DM_ARRAY_SIZE:
        if( value != 0 ) {
            char buff[12];
            itoa( value, buff, 10 );
            emitStr( data, buff );
        }
        break;
    case DM_OPEN_PAREN:
        emitChar( data, '(' );
        break;
    case DM_CLOSE_PAREN:
        emitChar( data, ')' );
        break;
    case DM_CHAR_ENCODING_SIGNED:
        emitStr( data, "signed " );
        break;
    case DM_ARRAY_PREFIX:
        emitStr( data, "[" );
        break;
    case DM_ARRAY_SUFFIX:
        emitStr( data, "]" );
        break;
    case DM_FUNCTION_PREFIX:
        emitStr( data, "( " );
        break;
    case DM_FUNCTION_SUFFIX:
        emitStr( data, ")" );
        if( data->cv_pending ) {
            unforceSuppression( data );
        }
        break;
    case DM_FUNCTION_ARG_SEPARATOR:
        emitStr( data, ", " );
        break;
    case DM_BASED_SUFFIX:
        emitStr( data, ") " );
        break;
    case DM_BASED_STRING_PREFIX:
        emitStr( data, "\"" );
        break;
    case DM_BASED_SELF:
        emitStr( data, "__self" );
        break;
    case DM_BASED_VOID:
        emitStr( data, "void" );
        break;
    case DM_BASED_STRING_SUFFIX:
        emitStr( data, "\"" );
        break;
    case DM_TEMPLATE_PREFIX:
        emitStr( data, "<" );
        break;
    case DM_TEMPLATE_SUFFIX:
        emitStr( data, ">" );
        break;
    case DM_TEMPLATE_ARG_SEPARATOR:
        emitStr( data, "," );
        break;
    case DM_ANONYMOUS_ENUM:
        emitStr( data, "__anonymous_enum " );
        break;
    case DM_OPERATOR_PREFIX:
        emitStr( data, "operator " );
        break;
    case DM_OPERATOR_NEW:
        emitStr( data, "new" );
        break;
    case DM_OPERATOR_NEW_ARRAY:
        emitStr( data, "new []" );
        break;
    case DM_OPERATOR_DELETE:
        emitStr( data, "delete" );
        break;
    case DM_OPERATOR_DELETE_ARRAY:
        emitStr( data, "delete []" );
        break;
    case DM_DESTRUCTOR_CHAR:
        emitChar( data, '~'  );
        break;
    case DM_SCOPE_SEPARATOR:
        if( data->base_name ) {
            forceSuppression( data );
        }
        emitStr( data, "::" );
        break;
    case DM_CHAR_ENCODING:
        emitStr( data, translate_type_encoding[ value ].string );
        break;
    case DM_WATCOM_OBJECT:
        emitStr( data, watcomObject[ value ].name );
        break;
    case DM_OPERATOR_FUNCTION:
        emitStr( data, operatorFunction[ value ] );
        break;
    case DM_RELATIONAL_FUNCTION:
        emitStr( data, relationalFunction[ value ] );
        break;
    case DM_ASSIGNMENT_FUNCTION:
        emitStr( data, assignmentFunction[ value ] );
        break;
    default:
        #if 0 || defined(TEST)
            printf( "ERROR: demangleEmit unknown dm_pts(%d)\n", dp );
            ++errors;
        #endif
        break;
    }
}


void emitChar( output_desc *data, char c )
{
    size_t adjust_size;
    size_t test_size;
    char *test_realloc;

    if( data->suppress_output > 2 ) {
        return;
    }
    data->count++;
    if( data->output == NULL ) {
        /* count the characters mode */
        return;
    }
    if( data->count == (data->size-1) ) {
        if( user_realloc != NULL ) {
            adjust_size = 256;
            for(;;) {
                if( adjust_size == 0 ) {
                    return;
                }
                test_size = data->size + adjust_size;
                test_realloc = user_realloc( data->output, test_size );
                if( test_realloc != NULL ) break;
                adjust_size >>= 1;
            }
            data->output = test_realloc;
            data->size = test_size;
        }
    }
    if( data->index < data->size ) {
        if( data->index < (data->count-1) ) {
            size_t last = (data->size < data->count ? data->size : data->count);
            memmove( &data->output[data->index+1],
                     &data->output[data->index],
                     (last - data->index) - 1);
        }
        data->output[data->index] = c;
        data->index++;
    }
}


unsigned terminateOutput( output_desc *data )
{
    unsigned outlen;

    outlen = data->count;
    if( data->output != 0 ) {
        if( outlen >= data->size ) outlen = data->size - 1;
        data->output[outlen] = '\0';
    }
    return outlen;
}
