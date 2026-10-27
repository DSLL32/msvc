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
/* Native stack bounds differ from the Win32 CRT owner used by alloca. */
#if defined(__linux__)
#define RECON_WATCOM_ALLOCA_400 1
#endif
#include <malloc.h>
#include <string.h>
#include "diagnostic.h"
#include "demangle.h"
#include <recon.h>

#ifdef __WATCOMC__
#pragma intrinsic(strlen)
#endif

#define _output1(k) data->outfun( &data->cookie, (k), 0, 0 )
#define _output2(k,v) data->outfun( &data->cookie, (k), (v), 0 )
#define _output3(k,v,p) data->outfun( &data->cookie, (k), (v), (p) )

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

int checkInternal( const char *name )
{
    if( name[0] == '.' || (name[0] == '$' && tolower((unsigned char)name[1]) == 'w') )
        return 2;
    if( name[0] == '$' && tolower((unsigned char)name[1]) == 'c'
        && tolower((unsigned char)name[2]) == 't' ) return 3;
    if( name[0] == '$' && tolower((unsigned char)name[1]) == 'd'
        && tolower((unsigned char)name[2]) == 't' ) return 4;
    return 1;
}

extern int mangled_name( output_desc * );
int do_demangle( output_desc *data )
{
    unsigned advances;

    advances = 1;
    switch( nextChar( data ) ) {
    case 'T':
        advances += 4;
        data->outfun( &data->cookie, DM_TRUNCATED_NAME, 0, 0 );
        /* fall through */
    case 'W':
        advanceChar( data );
        if( nextChar( data ) == '?' ) {
            while( advances != 0 ) {
                advanceChar( data );
                if( nextChar( data ) == 0 ) return 0;
                --advances;
            }
            return mangled_name( data );
        }
        break;
    }
    return 0;
}

extern int scoped_name( output_desc *, state_desc * );
extern int type_encoding( output_desc *, state_desc * );
int mangled_name( output_desc *data )
{
    state_desc state;

    data->outfun( &data->cookie, DM_MANGLED_NAME, 0, 0 );
    state.prefix = 0;
    state.suffix = 0;
    state.right = 0;
    if( scoped_name( data, &state ) ) {
        data->outfun( &data->cookie, DM_RESET_INDEX, 0, 0 );
        data->outfun( &data->cookie, DM_ZAP_SPACE, 0, 0 );
        return type_encoding( data, &state );
    }
    return 0;
}

extern int sym_name( output_desc *, state_desc * );
extern int scope( output_desc *, state_desc * );
int scoped_name( output_desc *data, state_desc *state )
{
    _output1( DM_SCOPED_NAME );
    if( sym_name( data, state ) ) {
        while( nextChar( data ) == ':' ) {
            _output2( DM_SET_INDEX, state->prefix );
            _output1( DM_SCOPE_SEPARATOR );
            advanceChar( data );
            if( !scope( data, state ) ) return 0;
            _output1( DM_ZAP_SPACE );
        }
        return 1;
    }
    return 0;
}

extern int op_name( output_desc *, state_desc * );
extern int name( output_desc *, state_desc * );
int sym_name( output_desc *data, state_desc *state )
{
    _output1( DM_NAME );
    if( nextChar( data ) == '$' ) {
        advanceChar( data );
        return op_name( data, state );
    }
    return name( data, state );
}

typedef struct replicate_desc {
    const char *ptr;
    unsigned len;
} replicate_desc;
static replicate_desc replicate[10];      /* original450300 */
extern int is_identifier( unsigned char );

int name( output_desc *data, state_desc *state )
{
    unsigned char c;

    c = nextChar( data );
    if( isdigit( c ) ) {
        size_t len;
        char const *id;

        c = c - '0';
        /* This older binary indexes the ten-entry cache directly. */
        id = replicate[c].ptr;
        len = replicate[c].len;
        if( data->ctdt_pending ) {
            _output2( DM_RESET_INDEX, data->pending_loc );
            _output1( DM_CTOR_DTOR_NAME );
            _output3( DM_IDENTIFIER, len, id );
            data->ctdt_pending = 0;
        }
        if( data->scope_name && data->scope_index == 0 ) {
            data->scope_name = 0;
            data->scope_ptr = id;
            data->scope_len = len;
        }
        _output2( DM_SET_INDEX, state->prefix );
        _output3( DM_IDENTIFIER, len, id );
        _output1( DM_EMIT_SPACE );
        advanceChar( data );
        return( 1 );
    } else {
        char const *ptr = data->input;
        size_t  len = 0;
        while( is_identifier( c ) ) {
            len++;
            advanceChar( data );
            c = nextChar( data );
        }
        if( data->scope_name && data->scope_index == 0 ) {
            data->scope_name = 0;
            data->scope_ptr = ptr;
            data->scope_len = len;
        }
        if( c == '$' ) {
            advanceChar( data );
            if( data->ctdt_pending ) {
                _output2( DM_RESET_INDEX, data->pending_loc );
                _output1( DM_CTOR_DTOR_NAME );
                _output3( DM_IDENTIFIER, len, ptr );
                data->ctdt_pending = 0;
            }
            _output2( DM_SET_INDEX, state->prefix );
            _output3( DM_IDENTIFIER, len, ptr );
            _output1( DM_EMIT_SPACE );
            if( next_replicate < 10 ) {
                replicate[next_replicate].ptr = ptr;
                replicate[next_replicate].len = len;
                next_replicate++;
            }
            return( 1 );
        }
    }
    return( 0 );
}


int is_identifier( unsigned char c )
{
    return isalnum( c ) || c == '_' || c == '.';
}

void setEmitIndex( output_desc *data, unsigned index )
{
    data->index = index;
}

void resetEmitIndex( output_desc *data, unsigned offset )
{
    unsigned index;

    index = data->count - offset;
    if( index >= data->size ) index = data->size - 1;
    data->index = index;
}

void zapSpace( output_desc *data )
{
    unsigned last;

    if( data->output == 0 || data->index > data->size ) return;
    if( data->output[data->index - 1] != ' ' ) return;
    --data->count;
    if( data->index != data->count + 1 ) {
        last = data->size < data->count ? data->size : data->count;
        if( last >= data->index ) {
            memmove( &data->output[data->index - 1], &data->output[data->index],
                     last - data->index + 1 );
        }
    }
    --data->index;
}

extern int typeChar( unsigned char, int );
extern int modifier_list( output_desc *, state_desc * );
extern int unmodified_type( output_desc *, state_desc * );
int type_encoding( output_desc *data, state_desc *state )
{
    _output1( DM_TYPE_ENCODING );
    if( typeChar( (unsigned char)nextChar( data ), CHAR_MODIFIER ) ) {
        if( !modifier_list( data, state ) ) return 0;
    }
    return unmodified_type( data, state );
}

int typeChar( unsigned char c, int grouping )
{
    c -= LOWER_TABLE_LIMIT;
    if( c < sizeof(translate_type_encoding) / sizeof(translate_type_encoding[0]) )
        return translate_type_encoding[c].grouping == grouping;
    return 0;
}

extern int based_encoding( output_desc *, state_desc * );
int modifier_list( output_desc *data, state_desc *state )
{
    unsigned char c;

    _output1( DM_MODIFIER_LIST );
    c = nextChar( data );
    while( typeChar( c, CHAR_MODIFIER ) ) {
        advanceChar( data );
        if( c != 'U' || nextChar( data ) != 'A' ) {
            _output2( DM_SET_INDEX, state->prefix );
            _output2( DM_CHAR_ENCODING, (int)c - LOWER_TABLE_LIMIT );
        }
        if( c == 'J' && !based_encoding( data, state ) ) return 0;
        c = nextChar( data );
    }
    return 1;
}

void emitStr( output_desc *data, const char *text )
{
    if( text != 0 ) {
        while( *text != 0 ) emitChar( data, *text++ );
    }
}

extern int function( output_desc *, state_desc * );
extern int tq_function( output_desc *, state_desc * );
extern int array( output_desc *, state_desc * );
extern int pointer( output_desc *, state_desc * );
extern int basic_type( output_desc *, state_desc * );
int unmodified_type( output_desc *data, state_desc *state )
{
    unsigned char c;

    _output1( DM_UNMODIFIED_TYPE );
    c = nextChar( data );
    if( c == '$' ) {
        advanceChar( data );
        if( scoped_name( data, state ) && nextChar( data ) == '$' ) {
            advanceChar( data );
            return 1;
        }
    } else if( c == '(' ) {
        advanceChar( data );
        return function( data, state );
    } else if( c == '.' ) {
        advanceChar( data );
        return tq_function( data, state );
    } else if( c == '[' ) {
        return array( data, state );
    } else if( typeChar( c, CHAR_POINTER ) ) {
        if( pointer( data, state ) ) {
            state->right = 1;
            return type_encoding( data, state );
        }
    } else {
        return basic_type( data, state );
    }
    return 0;
}

extern void setSuppression( output_desc * );
extern void resetSuppression( output_desc * );
int function( output_desc *data, state_desc *state )
{
    state_desc new_state;
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


void setSuppression( output_desc *data )
{
    if( data->suppress_output != 0 ) ++data->suppress_output;
}

extern char prevChar( output_desc * );
int basic_type( output_desc *data, state_desc *state )
{
    unsigned char c;

    _output1( DM_BASIC_TYPE );
    c = nextChar( data );
    if( typeChar( c, CHAR_BASIC_TYPE ) ) {
        if( c == 'C' && prevChar( data ) != 'U' ) {
            _output2( DM_SET_INDEX, state->prefix );
            _output1( DM_CHAR_ENCODING_SIGNED );
        }
        advanceChar( data );
        _output2( DM_SET_INDEX, state->prefix );
        _output2( DM_CHAR_ENCODING, (int)c - LOWER_TABLE_LIMIT );
        return 1;
    }
    return 0;
}

void resetSuppression( output_desc *data )
{
    if( data->suppress_output != 0 ) --data->suppress_output;
}

extern int ctor_or_convert( output_desc *, state_desc * );
extern int dtor_or_delete( output_desc *, state_desc * );
extern int op_new( output_desc *, state_desc * );
extern int assignment_function( output_desc *, state_desc * );
extern int relational_function( output_desc *, state_desc * );
extern int operator_function( output_desc *, state_desc * );
extern int special_type_names( output_desc *, state_desc * );
extern int watcom_object( output_desc *, state_desc * );
int op_name( output_desc *data, state_desc *state )
{
    switch( tolower((unsigned char)nextChar( data )) ) {
    case 'c':
        advanceChar( data );
        return ctor_or_convert( data, state );
    case 'd':
        advanceChar( data );
        return dtor_or_delete( data, state );
    case 'n':
        advanceChar( data );
        return op_new( data, state );
    case 'a':
        advanceChar( data );
        return assignment_function( data, state );
    case 'r':
        advanceChar( data );
        return relational_function( data, state );
    case 'o':
        advanceChar( data );
        return operator_function( data, state );
    case 't':
        advanceChar( data );
        return special_type_names( data, state );
    case 'w':
        advanceChar( data );
        return watcom_object( data, state );
    }
    return 0;
}

int ctor_or_convert( output_desc *data, state_desc *state )
{
    char c;

    c = nextChar( data );
    if( c == 'T' ) {
        advanceChar( data );
        _output1( DM_CONSTRUCTOR );
        data->pending_loc = data->count - data->index;
        data->ctdt_pending = 1;
        return 1;
    } else if( c == 'V' ) {
        advanceChar( data );
        _output2( DM_SET_INDEX, state->prefix );
        _output1( DM_OPERATOR_PREFIX );
        _output1( DM_OPERATOR_CONVERT );
        data->cv_pending = 1;
        return 1;
    }
    return 0;
}

extern int check_recurse( void );
extern int recursive_mangled_name( output_desc *, state_desc * );
extern int template_name( output_desc *, state_desc * );
int scope( output_desc *data, state_desc *state )
{
    char c;
    state_desc new_state;

    _output1( DM_SCOPE );
    if( data->scope_name && data->scope_index > 0 ) --data->scope_index;
    c = nextChar( data );
    if( c == '?' ) {
        advanceChar( data );
        if( check_recurse() ) return recursive_mangled_name( data, state );
    } else if( c == ':' ) {
        advanceChar( data );
        new_state = *state;
        new_state.suffix = data->count - state->prefix;
        return template_name( data, &new_state );
    } else {
        return sym_name( data, state );
    }
    return 0;
}

extern int template_arg( output_desc *, state_desc * );
int template_name( output_desc *data, state_desc *state )
{
    state_desc new_state;
    int             first_arg = 0;
    char            c;

    _output1( DM_TEMPLATE_NAME );
    new_state = *state;
    _output2( DM_RESET_INDEX, state->suffix );
    _output1( DM_TEMPLATE_PREFIX );
    c = nextChar( data );
    while( c == '0' || c == '1' || c == '?' ) {
        if( first_arg ) {
            _output2( DM_RESET_INDEX, state->suffix );
            _output1( DM_ZAP_SPACE );
            _output1( DM_TEMPLATE_ARG_SEPARATOR );
        }
        new_state.prefix = data->index;
        if( !template_arg( data, &new_state ) ) {
            return( 0 );
        }
        first_arg = 1;
        c = nextChar( data );
    }
    _output2( DM_RESET_INDEX, state->suffix );
    if( !first_arg ) {
        _output1( DM_ZAP_SPACE );
    }
    _output1( DM_TEMPLATE_SUFFIX );
    return( 1 );
}


extern int base_32_num( output_desc *, int * );
int template_arg( output_desc *data, state_desc *state )
{
    char c;

    _output1( DM_TEMPLATE_ARG );
    c = nextChar( data );
    if( c == '0' ) {
        int value;
        advanceChar( data );
        if( base_32_num( data, &value ) ) {
            c = nextChar( data );
            if( c == 'Z' ) {
                advanceChar( data );
                _output2( DM_SET_INDEX, state->prefix );
                _output2( DM_INTEGER, value );
                return( 1 );
            } else if( c == 'Y' ) {
                advanceChar( data );
                _output2( DM_SET_INDEX, state->prefix );
                _output2( DM_INTEGER, -value );
                return( 1 );
            }
        }
    } else if( c == '1' ) {
        advanceChar( data );
        return( type_encoding( data, state ) );
    } else if( c == '?' ) {
        advanceChar( data );
        if( check_recurse() ) {
            return( recursive_mangled_name( data, state ) );
        }
    }
    return( 0 );
}


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
                    _output2( DM_CHAR_ENCODING, (int)toupper((unsigned char)*tq_ptr++) - LOWER_TABLE_LIMIT );
                }
                _output1( DM_ZAP_SPACE );
            }
            return( 1 );
        }
    }
    return( 0 );
}


int pointer( output_desc *data, state_desc *state )
{
    unsigned char c;

    _output1( DM_POINTER );
    c = nextChar( data );
    if( typeChar( c, CHAR_POINTER ) ) {
        advanceChar( data );
        _output2( DM_SET_INDEX, state->prefix );
        _output2( DM_CHAR_ENCODING, (int)c - LOWER_TABLE_LIMIT );
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
        return 1;
    case 'L':
        advanceChar( data );
        _output2( DM_SET_INDEX, state->prefix );
        _output1( DM_OPERATOR_PREFIX );
        _output1( DM_OPERATOR_DELETE );
        return 1;
    case 'A':
        advanceChar( data );
        _output2( DM_SET_INDEX, state->prefix );
        _output1( DM_OPERATOR_PREFIX );
        _output1( DM_OPERATOR_DELETE_ARRAY );
        return 1;
    }
    return 0;
}

int assignment_function( output_desc *data, state_desc *state )
{
    unsigned char c;

    c = nextChar( data ) - LOWER_TABLE_LIMIT;
    if( c < sizeof(assignmentFunction) / sizeof(assignmentFunction[0]) ) {
        advanceChar( data );
        _output2( DM_SET_INDEX, state->prefix );
        _output1( DM_OPERATOR_PREFIX );
        _output2( DM_ASSIGNMENT_FUNCTION, (int)c );
        return 1;
    }
    return 0;
}

int relational_function( output_desc *data, state_desc *state )
{
    unsigned char c;

    c = nextChar( data ) - LOWER_TABLE_LIMIT;
    if( c < sizeof(relationalFunction) / sizeof(relationalFunction[0]) ) {
        advanceChar( data );
        _output2( DM_SET_INDEX, state->prefix );
        _output1( DM_OPERATOR_PREFIX );
        _output2( DM_RELATIONAL_FUNCTION, (int)c );
        return 1;
    }
    return 0;
}

int operator_function( output_desc *data, state_desc *state )
{
    unsigned char c;

    c = nextChar( data ) - LOWER_TABLE_LIMIT;
    if( c < sizeof(operatorFunction) / sizeof(operatorFunction[0]) ) {
        advanceChar( data );
        _output2( DM_SET_INDEX, state->prefix );
        _output1( DM_OPERATOR_PREFIX );
        _output2( DM_OPERATOR_FUNCTION, (int)c );
        return 1;
    }
    return 0;
}

void unforceSuppression( output_desc *data )
{
    if( data->suppress_output > 2 ) data->suppress_output -= 3;
}

int op_new( output_desc *data, state_desc *state )
{
    if( nextChar( data ) == 'W' ) {
        advanceChar( data );
        _output2( DM_SET_INDEX, state->prefix );
        _output1( DM_OPERATOR_PREFIX );
        _output1( DM_OPERATOR_NEW );
        return 1;
    }
    if( nextChar( data ) == 'A' ) {
        advanceChar( data );
        _output2( DM_SET_INDEX, state->prefix );
        _output1( DM_OPERATOR_PREFIX );
        _output1( DM_OPERATOR_NEW_ARRAY );
        return 1;
    }
    return 0;
}

int __unmangled_name( const char *name, unsigned len,
                       const char **base, unsigned *size )
{
    const char *end;
    int mangled;

    *size = 0;
    *base = name;
    if( len == 0 ) len = strlen( name );
    end = name + len;
    mangled = __is_mangled( name, len );
    if( mangled ) {
        name += mangled;
        *base = name;
        if( *name == '$' ) {
            *size += 3;
            ++name;
            if( tolower((unsigned char)*name) == 'w' ) ++*size;
        } else {
            for( ; name != end; ++name ) {
                if( *name == '$' ) break;
                ++*size;
            }
        }
        return 1;
    }
    *size = len;
    return 0;
}

int watcom_object( output_desc *data, state_desc *state )
{
    key_desc srch;
    int i;
    unsigned len;
    char c;

    c = nextChar( data );
    if( c == 'A' ) {
        advanceChar( data );
        advanceChar( data );
        _output2( DM_SET_INDEX, state->prefix );
        _output2( DM_WATCOM_OBJECT, 0 );
    } else {
        srch.str[0] = c;
        advanceChar( data );
        c = nextChar( data );
        srch.str[1] = c;
        advanceChar( data );
        for( i = 1; i < num_elements(watcomObject); ++i ) {
            if( srch.val == watcomObject[i].u.val ) {
                _output2( DM_SET_INDEX, state->prefix );
                _output2( DM_WATCOM_OBJECT, i );
                break;
            }
        }
        if( i == num_elements(watcomObject) ) {
            _output2( DM_SET_INDEX, state->prefix );
            _output2( DM_WATCOM_OBJECT, 0 );
        }
    }
    if( base_36_2digit( data, &len ) ) {
        while( len-- ) advanceChar( data );
        c = nextChar( data );
        if( c == '$' ) {
            advanceChar( data );
            return 1;
        }
    }
    return 0;
}

unsigned char_to_digit( char c )
{
    if( c >= '0' && c <= '9' ) return c - '0';
    if( c >= 'A' && c <= 'Z' ) return c - 'A' + 10;
    if( c >= 'a' && c <= 'z' ) return c - 'a' + 10;
    return (unsigned)-1;
}

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
            return 1;
        }
    }
    return 0;
}

int check_recurse( void )
{
    return (int)alloca( 400 );
}

int recursive_mangled_name( output_desc *data, state_desc *state )
{
    char buff[80];

    _output1( DM_RECURSE_BEGIN );
    data->input += demangle_recursive( data->input, buff, sizeof(buff) );
    _output1( DM_RECURSE_END );
    _output2( DM_SET_INDEX, state->prefix );
    _output3( DM_COPY_STRING, 0, buff );
    return 1;
}

unsigned demangle_recursive( const char *input, char *output, unsigned size )
{
    realloc_fn_t save_reallocator;
    output_desc data;

    save_reallocator = user_realloc;
    user_realloc = NULL;
    init_descriptor( &data, demangleEmit, &data, input, 0, output, size );
    mangled_name( &data );
    terminateOutput( &data );
    user_realloc = save_reallocator;
    return data.input - input;
}

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
        if( !dimension( data, state ) ) return 0;
        c = nextChar( data );
    }
    return type_encoding( data, state );
}

int base_32_num( output_desc *data, int *value )
{
    unsigned dig;
    int v;

    v = 0;
    for( ;; ) {
        dig = char_to_digit( nextChar( data ) );
        if( dig >= 32 ) break;
        v = v * 32 + dig;
        advanceChar( data );
    }
    *value = v;
    return 1;
}

int base_10_num( output_desc *data, int *value )
{
    unsigned dig;
    int v;

    v = 0;
    for( ;; ) {
        dig = char_to_digit( nextChar( data ) );
        if( dig >= 10 ) break;
        v = v * 10 + dig;
        advanceChar( data );
    }
    *value = v;
    return 1;
}

int dimension( output_desc *data, state_desc *state )
{
    int value;
    char c;

    _output2( DM_RESET_INDEX, state->suffix );
    _output1( DM_ARRAY_PREFIX );
    if( base_10_num( data, &value ) ) {
        _output2( DM_ARRAY_SIZE, value );
        c = nextChar( data );
        if( c == ']' ) {
            advanceChar( data );
            _output1( DM_ARRAY_SUFFIX );
            return 1;
        }
    }
    return 0;
}

char prevChar( output_desc *data )
{
    char c;

    c = *(data->input - 1);
    return toupper( (unsigned char)c );
}

int based_encoding( output_desc *data, state_desc *state )
{
    char   c;
    unsigned len;

    _output1( DM_BASED_ENCODING );
    c = nextChar( data );
    switch( c ) {
    case 'S':
        advanceChar( data );
        _output1( DM_BASED_SELF );
        _output1( DM_BASED_SUFFIX );
        return( 1 );
        break;
    case 'V':
        advanceChar( data );
        _output1( DM_BASED_VOID );
        _output1( DM_BASED_SUFFIX );
        return( 1 );
        break;
    case 'L':
        advanceChar( data );
        if( base_36_2digit( data, &len ) ) {
            _output1( DM_BASED_STRING_PREFIX );
            _output3( DM_IDENTIFIER, len, data->input );
            while( len-- ) {
                advanceChar( data );
            }
            _output1( DM_BASED_STRING_SUFFIX );
            _output1( DM_BASED_SUFFIX );
            return( 1 );
        }
        break;
    case 'A':
    case 'F':
        advanceChar( data );
        c = nextChar( data );
        if( c == '$' ) {
            auto state_desc new_state;
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


int special_type_names( output_desc *data, state_desc *state )
{
    char c;

    c = nextChar( data );
    if( c == 'E' ) {
        advanceChar( data );
        _output2( DM_SET_INDEX, state->prefix );
        _output1( DM_ANONYMOUS_ENUM );
        /* The 1994 encoding requires the suffix immediately here. */
        if( nextChar( data ) == '$' ) {
            advanceChar( data );
            return 1;
        }
    }
    return 0;
}

void forceSuppression( output_desc *data )
{
    if( data->suppress_output <= 2 ) data->suppress_output += 3;
}

unsigned __demangled_basename( const char *input, unsigned len,
                               char *output, unsigned size )
{
    int mangled;
    output_desc data;

    init_globals( 0 );
    mangled = __is_mangled( input, len );
    init_descriptor( &data, demangleEmit, &data, input, len, output, size );
    data.suppress_output = 0;
    data.base_name = 1;
    if( mangled ) do_demangle( &data );
    else do_copy( &data );
    return terminateOutput( &data );
}

void __parse_mangled_name( const char *input, unsigned len,
                           void *cookie, outfunPtr ofn )
{
    int mangled;
    output_desc data;

    init_globals( 0 );
    mangled = __is_mangled( input, len );
    init_descriptor( &data, ofn, cookie, input, len, 0, 0 );
    data.suppress_output = 0;
    if( mangled ) do_demangle( &data );
    else do_copy( &data );
}

unsigned __mangle_operator( const char *op, unsigned len, char *result )
{
    int i;

    if( len == 0 ) len = strlen( op );
    for( i = 0; i < num_elements(operatorFunction); ++i ) {
        if( strncmp(op,operatorFunction[i],len) == 0
            && operatorFunction[i][len] == '\0' ) {
            *result++ = '$'; *result++ = 'o'; *result = i + 'a';
            return 3;
        }
    }
    for( i = 0; i < num_elements(relationalFunction); ++i ) {
        if( strncmp(op,relationalFunction[i],len) == 0
            && relationalFunction[i][len] == '\0' ) {
            *result++ = '$'; *result++ = 'r'; *result = i + 'a';
            return 3;
        }
    }
    for( i = 0; i < num_elements(assignmentFunction); ++i ) {
        if( strncmp(op,assignmentFunction[i],len) == 0
            && assignmentFunction[i][len] == '\0' ) {
            *result++ = '$'; *result++ = 'a'; *result = i + 'a';
            return 3;
        }
    }
    return 0;
}
