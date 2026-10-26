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
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include "demangle.h"
#include <recon.h>

#if defined(__WATCOMC__)
/* Original public helper bodies inline both string-length scans. */
#pragma intrinsic(strlen)
#endif

/* Callback notation retained from the released demangler source. */
#define _output1(p)       (*data->outfun)( &data->cookie, p, 0, NULL )
#define _output2(p,v)     (*data->outfun)( &data->cookie, p, v, NULL )
#define _output3(p,v,t)   (*data->outfun)( &data->cookie, p, v, t )


/* Original0x4169d5..0x416a29. */
unsigned __demangle_l( const char *input, unsigned length, char *output, unsigned size )
{
    output_desc data;
    int mangled;

    init_globals( NULL );
    mangled = __is_mangled( input, length );
    init_descriptor( &data, demangleEmit, &data, input, length, output, size );
    if( mangled ) full_mangled_name( &data );
    else do_copy( &data );
    return( terminateOutput( &data ) );
}

static realloc_fn_t user_realloc;    /* Original0x430420. */
typedef struct replicate_desc {
    const char *ptr;
    unsigned len;
} replicate_desc;
static replicate_desc replicate[10]; /* Original0x4303d0. */
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

/* Original0x4167fc..0x416864 with its return at0x4167f6. The later
   source adds import-prefix recognition; that extra path is absent in1994. */
int full_mangled_name( output_desc *data )
{
    unsigned advances;

    advances = 1;
    switch( nextChar( data ) ) {
    case 'T':
        advances = 5;
        (*data->outfun)( &data->cookie, DM_TRUNCATED_NAME, 0, NULL );
        /* fall through */
    case 'W':
        advanceChar( data );
        if( nextChar( data ) == '?' ) {
            while( advances != 0 ) {
                advanceChar( data );
                if( nextChar( data ) == 0 ) return( 0 );
                --advances;
            }
            return( mangled_name( data ) );
        }
        break;
    }
    return( 0 );
}

/* Original0x416796..0x4167fc. Name and type decoding share insertion
   positions, with the actual output callback retaining each punctuation step. */
int mangled_name( output_desc *data )
{
    state_desc state;

    (*data->outfun)( &data->cookie, DM_MANGLED_NAME, 0, NULL );
    state.prefix = 0;
    state.suffix = 0;
    state.right = 0;
    if( scoped_name( data, &state ) ) {
        (*data->outfun)( &data->cookie, DM_RESET_INDEX, 0, NULL );
        (*data->outfun)( &data->cookie, DM_ZAP_SPACE, 0, NULL );
        return( type_encoding( data, &state ) );
    }
    return( 0 );
}

/* Original0x41670f..0x416796 with the shared boolean/return sequence
   at0x415695. Each enclosing scope is inserted before the current name. */
int scoped_name( output_desc *data, state_desc *state )
{
    (*data->outfun)( &data->cookie, DM_SCOPED_NAME, 0, NULL );
    if( sym_name( data, state ) ) {
        while( nextChar( data ) == ':' ) {
            (*data->outfun)( &data->cookie, DM_SET_INDEX, state->prefix, NULL );
            (*data->outfun)( &data->cookie, DM_SCOPE_SEPARATOR, 0, NULL );
            advanceChar( data );
            if( !scope( data, state ) ) return( 0 );
            (*data->outfun)( &data->cookie, DM_ZAP_SPACE, 0, NULL );
        }
        return( 1 );
    }
    return( 0 );
}

/* Original0x41662d..0x416671 selects an operator or ordinary identifier. */
int sym_name( output_desc *data, state_desc *state )
{
    (*data->outfun)( &data->cookie, DM_NAME, 0, NULL );
    if( nextChar( data ) == '$' ) {
        advanceChar( data );
        return( op_name( data, state ) );
    }
    return( name( data, state ) );
}

/* Original0x41637b..0x41645f. The operator family is selected by an
   ASCII-folded prefix; individual decoders retain their real function calls. */
int op_name( output_desc *data, state_desc *state )
{
    switch( tolower( (unsigned char)nextChar( data ) ) ) {
    case 'c': advanceChar( data ); return( ctor_or_convert( data, state ) );
    case 'd': advanceChar( data ); return( dtor_or_delete( data, state ) );
    case 'n': advanceChar( data ); return( op_new( data, state ) );
    case 'a': advanceChar( data ); return( assignment_function( data, state ) );
    case 'r': advanceChar( data ); return( relational_function( data, state ) );
    case 'o': advanceChar( data ); return( operator_function( data, state ) );
    case 't': advanceChar( data ); return( special_type_names( data, state ) );
    case 'w': advanceChar( data ); return( watcom_object( data, state ) );
    }
    return( 0 );
}

/* Original0x4162ff..0x41637b. Constructors defer their class spelling;
   conversion operators defer the converted type until type decoding. */
int ctor_or_convert( output_desc *data, state_desc *state )
{
    char ch;

    ch = nextChar( data );
    if( ch == 'T' ) {
        advanceChar( data );
        (*data->outfun)( &data->cookie, DM_CONSTRUCTOR, 0, NULL );
        data->pending_loc = data->count - data->index;
        data->ctdt_pending = 1;
        return( 1 );
    }
    if( ch == 'V' ) {
        advanceChar( data );
        (*data->outfun)( &data->cookie, DM_SET_INDEX, state->prefix, NULL );
        (*data->outfun)( &data->cookie, DM_OPERATOR_PREFIX, 0, NULL );
        (*data->outfun)( &data->cookie, DM_OPERATOR_CONVERT, 0, NULL );
        data->cv_pending = 1;
        return( 1 );
    }
    return( 0 );
}

/* Original0x41504d..0x415051 remains a function in this compiler build. */
void setEmitIndex( output_desc *data, unsigned index )
{
    data->index = index;
}

/* Original0x415031..0x41504d. A NULL spelling contributes no output. */
void emitStr( output_desc *data, const char *text )
{
    if( text != NULL ) {
        while( *text != 0 ) {
            emitChar( data, *text );
            ++text;
        }
    }
}

/* Original0x416671..0x41670f with shared return at0x41569a.
   The copied state is exactly nine bytes in the1994 image. */
int scope( output_desc *data, state_desc *state )
{
    char ch;
    state_desc nested;

    (*data->outfun)( &data->cookie, DM_SCOPE, 0, NULL );
    if( data->scope_name && data->scope_index > 0 ) --data->scope_index;
    ch = nextChar( data );
    if( ch == '?' ) {
        advanceChar( data );
        if( check_recurse() ) return( recursive_mangled_name( data, state ) );
    } else if( ch == ':' ) {
        advanceChar( data );
        nested = *state;
        nested.suffix = data->count - state->prefix;
        return( template_name( data, &nested ) );
    } else {
        return( sym_name( data, state ) );
    }
    return( 0 );
}

/* Original0x41645f..0x41662d with shared false/return fragments.
   The1994 digit branch directly uses the saved slot; the later source's
   next_replicate fallback check is absent from this image. */
int name( output_desc *data, state_desc *state )
{
    char ch;
    const char *identifier;
    unsigned length, slot;

    ch = nextChar( data );
    if( isdigit( (unsigned char)ch ) ) {
        slot = ch - '0';
        identifier = replicate[slot].ptr;
        length = replicate[slot].len;
        if( data->ctdt_pending ) {
            (*data->outfun)( &data->cookie, DM_RESET_INDEX, data->pending_loc, NULL );
            (*data->outfun)( &data->cookie, DM_CTOR_DTOR_NAME, 0, NULL );
            (*data->outfun)( &data->cookie, DM_IDENTIFIER, length, identifier );
            data->ctdt_pending = 0;
        }
        if( data->scope_name && data->scope_index == 0 ) {
            data->scope_name = 0;
            data->scope_ptr = identifier;
            data->scope_len = length;
        }
        (*data->outfun)( &data->cookie, DM_SET_INDEX, state->prefix, NULL );
        (*data->outfun)( &data->cookie, DM_IDENTIFIER, length, identifier );
        (*data->outfun)( &data->cookie, DM_EMIT_SPACE, 0, NULL );
        advanceChar( data );
        return( 1 );
    }
    identifier = data->input;
    length = 0;
    while( is_identifier( ch ) ) {
        ++length;
        advanceChar( data );
        ch = nextChar( data );
    }
    if( data->scope_name && data->scope_index == 0 ) {
        data->scope_name = 0;
        data->scope_ptr = identifier;
        data->scope_len = length;
    }
    if( ch == '$' ) {
        advanceChar( data );
        if( data->ctdt_pending ) {
            (*data->outfun)( &data->cookie, DM_RESET_INDEX, data->pending_loc, NULL );
            (*data->outfun)( &data->cookie, DM_CTOR_DTOR_NAME, 0, NULL );
            (*data->outfun)( &data->cookie, DM_IDENTIFIER, length, identifier );
            data->ctdt_pending = 0;
        }
        (*data->outfun)( &data->cookie, DM_SET_INDEX, state->prefix, NULL );
        (*data->outfun)( &data->cookie, DM_IDENTIFIER, length, identifier );
        (*data->outfun)( &data->cookie, DM_EMIT_SPACE, 0, NULL );
        if( next_replicate < 10 ) {
            replicate[next_replicate].ptr = identifier;
            replicate[next_replicate].len = length;
            ++next_replicate;
        }
        return( 1 );
    }
    return( 0 );
}

/* Original0x41544e..0x415474. The old spelling accepts periods as
   identifier characters in addition to ASCII alphanumerics/underscore. */
int is_identifier( char ch )
{
    return( isalnum( (unsigned char)ch ) || ch == '_' || ch == '.' );
}

/* Original0x415051..0x41506c. Backward offsets are measured from
   logical output count, clamped to the final available buffer position. */
void resetEmitIndex( output_desc *data, unsigned offset )
{
    unsigned index;

    index = data->count - offset;
    if( index < data->size ) data->index = index;
    else data->index = data->size - 1;
}

/* Original0x414f2f..0x414f8d removes one preceding space and closes
   the insertion gap without changing text beyond the bounded buffer. */
void zapSpace( output_desc *data )
{
    unsigned last;

    if( data->output == NULL || data->index > data->size ) return;
    if( data->output[data->index - 1] != ' ' ) return;
    --data->count;
    if( data->index != data->count + 1 ) {
        last = data->size < data->count ? data->size : data->count;
        if( last >= data->index ) {
            memmove( data->output + data->index - 1,
                     data->output + data->index, last - data->index + 1 );
        }
    }
    --data->index;
}

/* Original0x415e7e..0x415f8c plus callback/return at0x415693.
   A nested insertion state retains the surrounding suffix position. */
int template_name( output_desc *data, state_desc *state )
{
    state_desc nested;
    int first_argument;
    char ch;

    first_argument = 0;
    (*data->outfun)( &data->cookie, DM_TEMPLATE_NAME, 0, NULL );
    nested = *state;
    (*data->outfun)( &data->cookie, DM_RESET_INDEX, state->suffix, NULL );
    (*data->outfun)( &data->cookie, DM_TEMPLATE_PREFIX, 0, NULL );
    ch = nextChar( data );
    while( ch == '0' || ch == '1' || ch == '?' ) {
        if( first_argument ) {
            (*data->outfun)( &data->cookie, DM_RESET_INDEX, state->suffix, NULL );
            (*data->outfun)( &data->cookie, DM_ZAP_SPACE, 0, NULL );
            (*data->outfun)( &data->cookie, DM_TEMPLATE_ARG_SEPARATOR, 0, NULL );
        }
        nested.prefix = data->index;
        if( !template_arg( data, &nested ) ) return( 0 );
        first_argument = 1;
        ch = nextChar( data );
    }
    (*data->outfun)( &data->cookie, DM_RESET_INDEX, state->suffix, NULL );
    if( !first_argument ) (*data->outfun)( &data->cookie, DM_ZAP_SPACE, 0, NULL );
    (*data->outfun)( &data->cookie, DM_TEMPLATE_SUFFIX, 0, NULL );
    return( 1 );
}

/* Original0x415d91..0x415e7e plus the shared false/return fragments.
   Template argument prefixes distinguish integer, type and nested names. */
int template_arg( output_desc *data, state_desc *state )
{
    char ch;
    int value;

    (*data->outfun)( &data->cookie, DM_TEMPLATE_ARG, 0, NULL );
    ch = nextChar( data );
    if( ch == '0' ) {
        advanceChar( data );
        if( base_32_num( data, &value ) ) {
            ch = nextChar( data );
            if( ch == 'Z' ) {
                advanceChar( data );
                (*data->outfun)( &data->cookie, DM_SET_INDEX, state->prefix, NULL );
                (*data->outfun)( &data->cookie, DM_INTEGER, value, NULL );
                return( 1 );
            }
            if( ch == 'Y' ) {
                advanceChar( data );
                (*data->outfun)( &data->cookie, DM_SET_INDEX, state->prefix, NULL );
                (*data->outfun)( &data->cookie, DM_INTEGER, -value, NULL );
                return( 1 );
            }
        }
    } else if( ch == '1' ) {
        advanceChar( data );
        return( type_encoding( data, state ) );
    } else if( ch == '?' ) {
        advanceChar( data );
        if( check_recurse() ) return( recursive_mangled_name( data, state ) );
    }
    return( 0 );
}

/* Original0x415d48..0x415d91. Modifiers precede the unmodified type. */
int type_encoding( output_desc *data, state_desc *state )
{
    int result;
    char ch;

    result = 0;
    (*data->outfun)( &data->cookie, DM_TYPE_ENCODING, 0, NULL );
    ch = nextChar( data );
    if( typeChar( ch, CHAR_MODIFIER ) ) {
        if( modifier_list( data, state ) ) result = unmodified_type( data, state );
    } else {
        result = unmodified_type( data, state );
    }
    return( result );
}

/* Original0x4153cd..0x4153ec indexes the encountered31-entry type table. */
int typeChar( char ch, int grouping )
{
    unsigned char index;

    index = ch - 'A';
    if( index < sizeof(translate_type_encoding) / sizeof(translate_type_encoding[0]) )
        return( translate_type_encoding[index].grouping == grouping );
    return( 0 );
}

/* Original0x415ca9..0x415d48 and shared boolean/return at0x415695. */
int modifier_list( output_desc *data, state_desc *state )
{
    char ch;

    (*data->outfun)( &data->cookie, DM_MODIFIER_LIST, 0, NULL );
    ch = nextChar( data );
    while( typeChar( ch, CHAR_MODIFIER ) ) {
        advanceChar( data );
        if( ch != 'U' || nextChar( data ) != 'A' ) {
            (*data->outfun)( &data->cookie, DM_SET_INDEX, state->prefix, NULL );
            (*data->outfun)( &data->cookie, DM_CHAR_ENCODING, ch - 'A', NULL );
        }
        if( ch == 'J' && !based_encoding( data, state ) ) return( 0 );
        ch = nextChar( data );
    }
    return( 1 );
}

/* Original0x415a58..0x415b3a. Decode the unqualified type's actual
   punctuation before dispatching to its separate type-family decoder. */
int unmodified_type( output_desc *data, state_desc *state )
{
    char ch;

    (*data->outfun)( &data->cookie, DM_UNMODIFIED_TYPE, 0, NULL );
    ch = nextChar( data );
    if( ch == '$' ) {
        advanceChar( data );
        if( scoped_name( data, state ) && nextChar( data ) == '$' ) {
            advanceChar( data );
            return( 1 );
        }
    } else if( ch == '(' ) {
        advanceChar( data );
        return( function( data, state ) );
    } else if( ch == '.' ) {
        advanceChar( data );
        return( tq_function( data, state ) );
    } else if( ch == '[' ) {
        return( array( data, state ) );
    } else if( typeChar( ch, CHAR_POINTER ) ) {
        if( pointer( data, state ) ) {
            state->right = 1;
            return( type_encoding( data, state ) );
        }
    } else {
        return( basic_type( data, state ) );
    }
    return( 0 );
}

/* Original0x415554..0x4155e5 with callback/return at0x415691.
   Signed-char spelling depends on the immediately preceding unsigned modifier. */
int basic_type( output_desc *data, state_desc *state )
{
    char ch;

    (*data->outfun)( &data->cookie, DM_BASIC_TYPE, 0, NULL );
    ch = nextChar( data );
    if( typeChar( ch, CHAR_BASIC_TYPE ) ) {
        if( ch == 'C' && prevChar( data ) != 'U' ) {
            (*data->outfun)( &data->cookie, DM_SET_INDEX, state->prefix, NULL );
            (*data->outfun)( &data->cookie, DM_CHAR_ENCODING_SIGNED, 0, NULL );
        }
        advanceChar( data );
        (*data->outfun)( &data->cookie, DM_SET_INDEX, state->prefix, NULL );
        (*data->outfun)( &data->cookie, DM_CHAR_ENCODING, ch - 'A', NULL );
        return( 1 );
    }
    return( 0 );
}

/* Original0x4157af..0x415990 with shared return at0x415695.
   Initial-source function body matches the1994 callback/control sequence. */
int function( output_desc *data, state_desc *state )
{
    auto state_desc new_state;
    size_t          conv_offset;
    int             first_arg = 0;
    char            c;

    _output1( DM_FUNCTION );
    _output2( DM_RESET_INDEX, state->suffix );
    if( data->cv_pending ) {
        _output1( DM_EMIT_SPACE );
        conv_offset = data->index;
    } else {
        _output1( DM_ZAP_SPACE );
    }
    if( state->right ) {
        _output2( DM_SET_INDEX, state->prefix );
        _output1( DM_OPEN_PAREN );
        _output2( DM_RESET_INDEX, state->suffix );
        _output1( DM_CLOSE_PAREN );
        state->right = 0;
    }
    new_state = *state;
    _output1( DM_FUNCTION_PREFIX );
    setSuppression( data );
    c = nextChar( data );
    while( c != ')' ) {
        if( first_arg ) {
            _output2( DM_RESET_INDEX, state->suffix );
            _output1( DM_ZAP_SPACE );
            _output1( DM_FUNCTION_ARG_SEPARATOR );
        }
        new_state.prefix = data->index;
        if( !type_encoding( data, &new_state ) ) {
            return( 0 );
        }
        first_arg = 1;
        c = nextChar( data );
    }
    advanceChar( data );
    _output2( DM_RESET_INDEX, state->suffix );
    if( !first_arg ) {
        _output1( DM_ZAP_SPACE );
    }
    resetSuppression( data );
    _output1( DM_FUNCTION_SUFFIX );
    if( data->cv_pending ) {
        _output2( DM_SET_INDEX, conv_offset );
    } else {
        _output2( DM_SET_INDEX, state->prefix );
    }
    new_state.prefix = data->index;
    if( type_encoding( data, &new_state ) ) {
        data->cv_pending = 0;
        return( 1 );
    } else {
        return( 0 );
    }
}

/* Original0x41506c..0x415076. Nested argument suppression is conditional
   on the caller already being in suppressed-output mode. */
void setSuppression( output_desc *data )
{
    if( data->suppress_output ) ++data->suppress_output;
}

/* Original0x415076..0x415080 balances the conditional nesting count. */
void resetSuppression( output_desc *data )
{
    if( data->suppress_output ) --data->suppress_output;
}

/* Original0x415990..0x415a58 plus false/terminal callback fragments.
   Member-function qualifiers follow the function spelling in this format. */
int tq_function( output_desc *data, state_desc *state )
{
    char c;
    char const *tq_ptr;
    size_t tq_len;

    _output1( DM_THIS_FUNCTION );
    tq_ptr = data->input;
    tq_len = 0;
    c = nextChar( data );
    while( typeChar( c, CHAR_MODIFIER ) ) {
        // note: doesn't handle based modifiers
        tq_len++;
        advanceChar( data );
        c = nextChar( data );
    }
    if( c == '(' ) {
        advanceChar( data );
        if( function( data, state ) ) {
            _output2( DM_RESET_INDEX, state->suffix );
            if( tq_len > 0 ) {
                _output1( DM_EMIT_SPACE );
                while( tq_len-- ) {
                    _output2( DM_CHAR_ENCODING, (int)toupper(*tq_ptr++) - 'A' );
                }
                _output1( DM_ZAP_SPACE );
            }
            return( 1 );
        }
    }
    return( 0 );
}

/* Original0x41508b..0x415096 reverses the forced suppression bias. */
void unforceSuppression( output_desc *data )
{
    if( data->suppress_output > 2 ) data->suppress_output -= 3;
}

/* Original0x41623f..0x4162ff: destructor or scalar/array delete spelling. */
int dtor_or_delete( output_desc *data, state_desc *state )
{
    char c;

    c = nextChar( data );
    switch( c ) {
    case 'T':
        advanceChar( data );
        _output2( DM_SET_INDEX, state->prefix );
        _output1( DM_DESTRUCTOR_CHAR );
        _output1( DM_DESTRUCTOR );
        data->pending_loc = data->count - data->index;
        data->ctdt_pending = 1;
        return( 1 );
    case 'L':
        advanceChar( data );
        _output2( DM_SET_INDEX, state->prefix );
        _output1( DM_OPERATOR_PREFIX );
        _output1( DM_OPERATOR_DELETE );
        return( 1 );
    case 'A':
        advanceChar( data );
        _output2( DM_SET_INDEX, state->prefix );
        _output1( DM_OPERATOR_PREFIX );
        _output1( DM_OPERATOR_DELETE_ARRAY );
        return( 1 );
    }
    return( 0 );
}

/* Original0x4160c9..0x416119, shared false and callback-return chunks. */
int operator_function( output_desc *data, state_desc *state )
{
    unsigned char index;

    index = nextChar( data ) - 'A';
    if( index < sizeof(operatorFunction) / sizeof(operatorFunction[0]) ) {
        advanceChar( data );
        _output2( DM_SET_INDEX, state->prefix );
        _output1( DM_OPERATOR_PREFIX );
        _output2( DM_OPERATOR_FUNCTION, index );
        return( 1 );
    }
    return( 0 );
}

/* Original0x416169..0x4161b9, with the same compiler-shared terminal
   chunks as operator_function. The table index is an unsigned byte. */
int assignment_function( output_desc *data, state_desc *state )
{
    unsigned char index;

    index = nextChar( data ) - 'A';
    if( index < sizeof(assignmentFunction) / sizeof(assignmentFunction[0]) ) {
        advanceChar( data );
        _output2( DM_SET_INDEX, state->prefix );
        _output1( DM_OPERATOR_PREFIX );
        _output2( DM_ASSIGNMENT_FUNCTION, index );
        return( 1 );
    }
    return( 0 );
}

/* Original0x4155e5..0x4156a0, including the compiler-shared terminal
   region. Member pointers include their scoped class name before the type. */
int pointer( output_desc *data, state_desc *state )
{
    char  c;

    _output1( DM_POINTER );
    c = nextChar( data );
    if( typeChar( c, CHAR_POINTER ) ) {
        advanceChar( data );
        _output2( DM_SET_INDEX, state->prefix );
        _output2( DM_CHAR_ENCODING, (int)c - 'A' );
        if( c == 'M' ) {
            c = nextChar( data );
            if( c == '$' ) {
                advanceChar( data );
                if( !scoped_name( data, state ) ) {
                    return( 0 );
                }
                c = nextChar( data );
                if( c != '$' ) {
                    return( 0 );
                }
                advanceChar( data );
                _output1( DM_ZAP_SPACE );
            }
        }
        return( 1 );
    }
    return( 0 );
}

/* Original0x415f8c..0x416073 with shared false/boolean-return chunks.
   The1994 internal-object dictionary has fifteen entries, not the later19. */
int watcom_object( output_desc *data, state_desc *state )
{
    char first, second;
    unsigned length;
    int index;

    first = nextChar( data );
    if( first == 'A' ) {
        advanceChar( data );
        advanceChar( data );
        _output2( DM_SET_INDEX, state->prefix );
        _output2( DM_WATCOM_OBJECT, 0 );
    } else {
        advanceChar( data );
        second = nextChar( data );
        advanceChar( data );
        for( index = 1; index < 15; ++index ) {
            if( first == watcomObject[index].key[0] &&
                second == watcomObject[index].key[1] ) {
                _output2( DM_SET_INDEX, state->prefix );
                _output2( DM_WATCOM_OBJECT, index );
                break;
            }
        }
        if( index == 15 ) {
            _output2( DM_SET_INDEX, state->prefix );
            _output2( DM_WATCOM_OBJECT, 0 );
        }
    }
    if( base_36_2digit( data, &length ) ) {
        while( length-- ) advanceChar( data );
        if( nextChar( data ) == '$' ) {
            advanceChar( data );
            return( 1 );
        }
    }
    return( 0 );
}

/* Original0x415501..0x415554. The internal-name payload length has
   exactly two base36 digits; failure leaves the output length unchanged. */
int base_36_2digit( output_desc *data, unsigned *value )
{
    unsigned first, second;

    first = char_to_digit( nextChar( data ) );
    if( first < 36 ) {
        advanceChar( data );
        second = char_to_digit( nextChar( data ) );
        if( second < 36 ) {
            advanceChar( data );
            *value = first * 36 + second;
            return( 1 );
        }
    }
    return( 0 );
}

/* Original0x415474..0x4154a7. Invalid digits return the unsigned sentinel. */
unsigned char_to_digit( char ch )
{
    if( ch >= '0' && ch <= '9' ) return( ch - '0' );
    if( ch >= 'A' && ch <= 'Z' ) return( ch - 'A' + 10 );
    if( ch >= 'a' && ch <= 'z' ) return( ch - 'a' + 10 );
    return( (unsigned)-1 );
}

/* Original0x4156ff..0x4157af and shared return at0x41569a. */
int array( output_desc *data, state_desc *state )
{
    char c;

    _output1( DM_ARRAY );
    _output2( DM_RESET_INDEX, state->suffix );
    _output1( DM_ZAP_SPACE );
    if( state->right ) {
        _output2( DM_SET_INDEX, state->prefix );
        _output1( DM_OPEN_PAREN );
        _output2( DM_RESET_INDEX, state->suffix );
        _output1( DM_CLOSE_PAREN );
        state->right = 0;
    }
    c = nextChar( data );
    while( c == '[' ) {
        advanceChar( data );
        if( !dimension( data, state ) ) {
            return( 0 );
        }
        c = nextChar( data );
    }
    return( type_encoding( data, state ) );
}

/* Original0x4156a0..0x4156ff with shared false/callback-return paths. */
int dimension( output_desc *data, state_desc *state )
{
    int value;

    _output2( DM_RESET_INDEX, state->suffix );
    _output1( DM_ARRAY_PREFIX );
    if( base_10_num( data, &value ) ) {
        _output2( DM_ARRAY_SIZE, value );
        if( nextChar( data ) == ']' ) {
            advanceChar( data );
            _output1( DM_ARRAY_SUFFIX );
            return( 1 );
        }
    }
    return( 0 );
}

/* Original0x4154d9..0x415501 and shared return0x4154cf. An empty sequence denotes zero; the
   delimiter remains unread for the enclosing dimension parser. */
int base_10_num( output_desc *data, int *value )
{
    unsigned digit;
    int number;

    number = 0;
    for( ;; ) {
        digit = char_to_digit( nextChar( data ) );
        if( digit >= 10 ) break;
        number = number * 10 + digit;
        advanceChar( data );
    }
    *value = number;
    return( 1 );
}

/* Original0x41540a..0x415418 tail-calls the ordinary uppercase CRT leaf. */
char prevChar( output_desc *data )
{
    return( toupper( (unsigned char)data->input[-1] ) );
}

/* Original0x4154a7..0x4154d9; the final store/return is also used by
   base_10_num. Accumulation wraps at the original 32-bit word width. */
int base_32_num( output_desc *data, int *value )
{
    unsigned digit;
    unsigned number;

    number = 0;
    for( ;; ) {
        digit = char_to_digit( nextChar( data ) );
        if( digit >= 32 ) break;
        number = number * 32 + digit;
        advanceChar( data );
    }
    *value = number;
    return( 1 );
}

/* Original0x4153ec..0x41540a. The period alloca macro checks available
   stack before reserving400 bytes. As with PadLibraryPage, native builds
   use an actual small caller-frame allocation; callers test only nonzero. */
int check_recurse( void )
{
    return( (int)alloca( 400 ) );
}

/* Original0x416977..0x4169d5 and callback-return suffix0x415693. */
int recursive_mangled_name( output_desc *data, state_desc *state )
{
    char buffer[80];

    _output1( DM_RECURSE_BEGIN );
    data->input += demangle_recursive( data->input, buffer, sizeof(buffer) );
    _output1( DM_RECURSE_END );
    _output2( DM_SET_INDEX, state->prefix );
    _output3( DM_COPY_STRING, 0, buffer );
    return( 1 );
}

/* Original0x416928..0x416977. Recursive output uses a fixed caller buffer
   and temporarily disables the optional user reallocator. */
unsigned demangle_recursive( const char *input, char *output, unsigned size )
{
    realloc_fn_t saved_reallocator;
    output_desc data;

    saved_reallocator = user_realloc;
    user_realloc = NULL;
    init_descriptor( &data, demangleEmit, &data, input, 0, output, size );
    mangled_name( &data );
    terminateOutput( &data );
    user_realloc = saved_reallocator;
    return( data.input - input );
}

/* Original0x4161b9..0x41623f: scalar and array allocation operators. */
int op_new( output_desc *data, state_desc *state )
{
    if( nextChar( data ) == 'W' ) {
        advanceChar( data );
        _output2( DM_SET_INDEX, state->prefix );
        _output1( DM_OPERATOR_PREFIX );
        _output1( DM_OPERATOR_NEW );
        return( 1 );
    } else if( nextChar( data ) == 'A' ) {
        advanceChar( data );
        _output2( DM_SET_INDEX, state->prefix );
        _output1( DM_OPERATOR_PREFIX );
        _output1( DM_OPERATOR_NEW_ARRAY );
        return( 1 );
    }
    return( 0 );
}

/* Original0x416119..0x416169, unsigned six-entry relational table. */
int relational_function( output_desc *data, state_desc *state )
{
    unsigned char index;

    index = nextChar( data ) - 'A';
    if( index < sizeof(relationalFunction) / sizeof(relationalFunction[0]) ) {
        advanceChar( data );
        _output2( DM_SET_INDEX, state->prefix );
        _output1( DM_OPERATOR_PREFIX );
        _output2( DM_RELATIONAL_FUNCTION, index );
        return( 1 );
    }
    return( 0 );
}

/* Original0x415b3a..0x415ca9 with shared false/return regions. */
int based_encoding( output_desc *data, state_desc *state )
{
    unsigned length;
    state_desc new_state;

    _output1( DM_BASED_ENCODING );
    switch( nextChar( data ) ) {
    case 'S':
        advanceChar( data );
        _output1( DM_BASED_SELF );
        _output1( DM_BASED_SUFFIX );
        return( 1 );
    case 'V':
        advanceChar( data );
        _output1( DM_BASED_VOID );
        _output1( DM_BASED_SUFFIX );
        return( 1 );
    case 'L':
        advanceChar( data );
        if( base_36_2digit( data, &length ) ) {
            _output1( DM_BASED_STRING_PREFIX );
            _output3( DM_IDENTIFIER, length, data->input );
            while( length-- ) advanceChar( data );
            _output1( DM_BASED_STRING_SUFFIX );
            _output1( DM_BASED_SUFFIX );
            return( 1 );
        }
        break;
    case 'A':
    case 'F':
        advanceChar( data );
        if( nextChar( data ) == '$' ) {
            advanceChar( data );
            new_state = *state;
            new_state.prefix = data->index;
            new_state.suffix = data->count - data->index;
            if( scoped_name( data, &new_state ) ) {
                _output2( DM_RESET_INDEX, new_state.suffix );
                _output1( DM_ZAP_SPACE );
                _output1( DM_BASED_SUFFIX );
                return( 1 );
            }
        }
        break;
    }
    return( 0 );
}

/* Original0x416073..0x4160c9. This1994 anonymous-enum encoding expects
   the closing '$' immediately; the later source skips an encoded name. */
int special_type_names( output_desc *data, state_desc *state )
{
    if( nextChar( data ) == 'E' ) {
        advanceChar( data );
        _output2( DM_SET_INDEX, state->prefix );
        _output1( DM_ANONYMOUS_ENUM );
        if( nextChar( data ) == '$' ) {
            advanceChar( data );
            return( 1 );
        }
    }
    return( 0 );
}

/* Original0x415080..0x41508b. Nested suppression is represented by
   adding three only when no forced-suppression level is already present. */
void forceSuppression( output_desc *data )
{
    if( data->suppress_output <= 2 ) data->suppress_output += 3;
}

/* Public demangler APIs from initial demangle.c, matched to actual1994
   entries. This version has no do_demangle wrapper/import-prefix support. */
unsigned __demangle_r( const char *input, unsigned length, char **output,
                       unsigned size, realloc_fn_t reallocator )
{
    output_desc data;
    char *buffer;
    unsigned result;
    int mangled;
    init_globals( reallocator );
    mangled = __is_mangled( input, length );
    buffer = output == NULL ? NULL : *output;
    init_descriptor( &data, demangleEmit, &data, input, length, buffer, size );
    data.suppress_output = 0;
    if( mangled ) full_mangled_name( &data ); else do_copy( &data );
    result = terminateOutput( &data );
    if( output != NULL ) *output = data.output;
    return( result );
}
int __scope_name( const char *input, unsigned length, unsigned index,
                   const char **scope_name, unsigned *size )
{
    output_desc data;
    init_globals( NULL );
    *scope_name = NULL; *size = 0;
    if( !__is_mangled( input, length ) ) return( 0 );
    init_descriptor( &data, demangleEmit, &data, input, length, NULL, 0 );
    data.scope_name = 1; data.scope_index = index;
    full_mangled_name( &data );
    if( data.scope_len != 0 ) {
        *scope_name = data.scope_ptr; *size = data.scope_len;
        return( 1 );
    }
    return( 0 );
}
int checkInternal( const char *name )
{
    if( name[0] == '.' || ( name[0] == '$' && tolower( name[1] ) == 'w' ) ) return( 2 );
    if( name[0] == '$' && tolower( name[1] ) == 'c' && tolower( name[2] ) == 't' ) return( 3 );
    if( name[0] == '$' && tolower( name[1] ) == 'd' && tolower( name[2] ) == 't' ) return( 4 );
    return( 1 );
}
int __is_mangled_internal( const char *name, unsigned length )
{
    unsigned offset;
    length = length;
    offset = 2;
    switch( name[0] ) {
    case 'T': offset += 4;
    case 'W': if( name[1] == '?' ) return( checkInternal( name + offset ) );
    }
    return( 0 );
}
int __unmangled_name( const char *name, unsigned length, const char **base,
                       unsigned *size )
{
    const char *end;
    int mangled;
    *size = 0; *base = name;
    if( length == 0 ) length = strlen( name );
    end = name + length;
    mangled = __is_mangled( name, length );
    if( mangled ) {
        name += mangled; *base = name;
        if( *name == '$' ) {
            *size += 3; ++name;
            if( tolower( *name ) == 'w' ) ++*size;
        } else {
            for( ; name != end; ++name ) {
                if( *name == '$' ) break;
                ++*size;
            }
        }
        return( 1 );
    }
    *size = length;
    return( 0 );
}
unsigned __demangled_basename( const char *input, unsigned length,
                                char *output, unsigned size )
{
    output_desc data;
    int mangled;
    init_globals( NULL );
    mangled = __is_mangled( input, length );
    init_descriptor( &data, demangleEmit, &data, input, length, output, size );
    data.suppress_output = 0; data.base_name = 1;
    if( mangled ) full_mangled_name( &data ); else do_copy( &data );
    return( terminateOutput( &data ) );
}
void __parse_mangled_name( const char *input, unsigned length, void *cookie,
                            outfunPtr output )
{
    output_desc data;
    int mangled;
    init_globals( NULL );
    mangled = __is_mangled( input, length );
    init_descriptor( &data, output, cookie, input, length, NULL, 0 );
    data.suppress_output = 0;
    if( mangled ) full_mangled_name( &data ); else do_copy( &data );
}
unsigned __mangle_operator( const char *name, unsigned length, char *result )
{
    unsigned i;
    if( length == 0 ) length = strlen( name );
    for( i = 0; i < sizeof(operatorFunction)/sizeof(operatorFunction[0]); ++i ) {
        if( strncmp( name, operatorFunction[i], length ) == 0 && operatorFunction[i][length] == '\0' ) {
            result[0] = '$'; result[1] = 'o'; result[2] = i + 'a'; return( 3 );
        }
    }
    for( i = 0; i < sizeof(relationalFunction)/sizeof(relationalFunction[0]); ++i ) {
        if( strncmp( name, relationalFunction[i], length ) == 0 && relationalFunction[i][length] == '\0' ) {
            result[0] = '$'; result[1] = 'r'; result[2] = i + 'a'; return( 3 );
        }
    }
    for( i = 0; i < sizeof(assignmentFunction)/sizeof(assignmentFunction[0]); ++i ) {
        if( strncmp( name, assignmentFunction[i], length ) == 0 && assignmentFunction[i][length] == '\0' ) {
            result[0] = '$'; result[1] = 'a'; result[2] = i + 'a'; return( 3 );
        }
    }
    return( 0 );
}
