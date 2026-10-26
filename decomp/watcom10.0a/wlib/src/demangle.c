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
/* Adapted from bld/lib_misc/c/demangle.c at OpenWatcom initial commit
   6b9cb4438959d845a2b4dd4bf19f7908ecd608e9. Signatures/descriptor layout
   and individual bodies follow this1994 image, not later parser additions. */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "demangle.h"
#include <recon.h>

/* Original0x4169d5..0x416a29. */
unsigned __demangle_l( const char *input, unsigned length, char *output, unsigned size )
{
    output_desc data;
    int mangled;

    init_globals( NULL );
    mangled = __is_mangled( input, length );
    init_descriptor( &data, demangleEmit, &data, input, length, output, size );
    if( mangled ) do_demangle( &data );
    else do_copy( &data );
    return( terminateOutput( &data ) );
}

static realloc_fn_t user_realloc;    /* Original0x430420. */
static int next_replicate;           /* Original0x430424. */

/* Original0x416895..0x4168a5. */
void init_globals( realloc_fn_t reallocator )
{
    next_replicate = 0;
    user_realloc = reallocator;
}

/* Original0x4168a5..0x41690a. The older56-byte descriptor orders the
   output and scope pointers differently from the released source. */
void init_descriptor( output_desc *data, outfunPtr output, void *cookie,
                      const char *input, unsigned length, char *buffer, unsigned size )
{
    data->outfun = output;
    data->cookie = cookie;
    data->end = NULL;
    if( length != 0 ) data->end = input + length;
    data->input = input;
    data->output = buffer;
    data->scope_ptr = NULL;
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
}

/* Original0x416864..0x416895. Preserve the complete plain spelling
   through the same parser cursor and output callback as mangled names. */
void do_copy( output_desc *data )
{
    const char *text;
    int length;
    char ch;

    text = data->input;
    length = 0;
    ch = nextChar( data );
    advanceChar( data );
    while( ch ) {
        ++length;
        ch = nextChar( data );
        advanceChar( data );
    }
    (*data->outfun)( &data->cookie, DM_COPY_STRING, length, text );
}

/* Original0x415418..0x41543a. Input syntax is folded without changing
   the original bytes later passed to the output callback. */
char nextChar( output_desc *data )
{
    unsigned char ch;

    ch = *data->input;
    if( data->input == data->end ) ch = 0;
    else ch = toupper( ch );
    return( ch );
}

/* Original0x41543a..0x41544e. */
void advanceChar( output_desc *data )
{
    if( data->input != data->end && *data->input != '\0' ) ++data->input;
}

/* Tables belong to this encountered output dispatcher. Actual1994 Z is
   unused, and the internal-object dictionary has fifteen entries. */
#pragma pack(1)
typedef struct table_t { const char *string; char grouping; } table_t;
typedef struct assoc_desc { char key[2]; const char *name; } assoc_desc;
#pragma pack()
/* External linkage keeps this genuine const object visible in the period
   MAP, separating data from the preceding module's function extent. */
table_t const translate_type_encoding[] = {
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
    /* 'Z' */   { NULL,         CHAR_UNUSED },
    /* '[' */   { NULL,         CHAR_UNUSED },
    /* '\' */   { NULL,         CHAR_UNUSED },
    /* ']' */   { NULL,         CHAR_UNUSED },
    /* '^' */   { NULL,         CHAR_UNUSED },
    /* '_' */   { "",           CHAR_BASIC_TYPE } // for ctor/dtor return type
};

static char * const operatorFunction[] = {
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
static char const * const relationalFunction[] = {
    /* A */ "==",
    /* B */ "!=",
    /* C */ "<",
    /* D */ "<=",
    /* E */ ">",
    /* F */ ">="
};
static char const * const assignmentFunction[] = {
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

static const assoc_desc watcomObject[] = {
    { { 'A', '*' }, "__internal" },
    { { 'B', 'I' }, "__onceonly" },
    { { 'D', 'A' }, "__arrdtorblk" },
    { { 'D', 'F' }, "__defarg" },
    { { 'D', 'O' }, "__dtorobjblk" },
    { { 'M', 'P' }, "__mbrptrthunk" },
    { { 'S', 'I' }, "__staticinit" },
    { { 'T', 'H' }, "__throwblk" },
    { { 'T', 'S' }, "__typesig" },
    { { 'V', 'B' }, "__vbtbl" },
    { { 'V', 'F' }, "__vftbl" },
    { { 'V', 'M' }, "__vmtbl" },
    { { 'V', 'T' }, "__vfthunk" },
    { { 'S', 'T' }, "__typstattab" },
    { { 'C', 'M' }, "__stattabcmd" }
};

/* Original0x415182..0x4153cd, table415096..415182. */
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
        break;
    }
}


/* Original0x414f8d..0x415031. Insert at the active output position,
   counting suppressed/truncated output independently from buffer capacity. */
void emitChar( output_desc *data, char ch )
{
    unsigned adjustment, capacity, last;
    char *buffer;

    if( data->suppress_output > 2 ) return;
    ++data->count;
    if( data->output == NULL ) return;
    if( data->count == data->size - 1 && user_realloc != NULL ) {
        adjustment = 256;
        for( ;; ) {
            if( adjustment == 0 ) return;
            capacity = data->size + adjustment;
            buffer = (*user_realloc)( data->output, capacity );
            if( buffer != NULL ) break;
            adjustment >>= 1;
        }
        data->output = buffer;
        data->size = capacity;
    }
    if( data->index < data->size ) {
        if( data->index < data->count - 1 ) {
            last = data->size < data->count ? data->size : data->count;
            memmove( data->output + data->index + 1,
                     data->output + data->index, last - data->index - 1 );
        }
        data->output[data->index++] = ch;
    }
}

/* Original0x41690a..0x416928. The1994 body does not trim a trailing
   space, unlike the later released implementation. */
unsigned terminateOutput( output_desc *data )
{
    unsigned length;

    length = data->count;
    if( data->output != NULL ) {
        if( length >= data->size ) length = data->size - 1;
        data->output[length] = '\0';
    }
    return( length );
}
