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
/*1994 descriptor layout differs from the later source: output precedes
   scope_ptr, and only four status bits are present. */
#ifndef WLIB_DEMANGLE_H
#define WLIB_DEMANGLE_H
#include "wlib.h"
#include <recon.h>

int __is_mangled( const char *name, unsigned length );

typedef char *(*realloc_fn_t)( char *, unsigned );
typedef enum {
    DM_BASIC_TYPE,
    DM_CHAR_ENCODING,
    DM_CHAR_ENCODING_SIGNED,
    DM_SET_INDEX,
    DM_POINTER,
    DM_ZAP_SPACE,
    DM_EMIT_SPACE,
    DM_RESET_INDEX,
    DM_ARRAY_PREFIX,
    DM_ARRAY_SIZE,
    DM_ARRAY_SUFFIX,
    DM_ARRAY,
    DM_OPEN_PAREN,
    DM_CLOSE_PAREN,
    DM_FUNCTION,
    DM_FUNCTION_PREFIX,
    DM_FUNCTION_SUFFIX,
    DM_FUNCTION_ARG_SEPARATOR,
    DM_THIS_FUNCTION,
    DM_UNMODIFIED_TYPE,
    DM_BASED_ENCODING,
    DM_BASED_SUFFIX,
    DM_BASED_SELF,
    DM_BASED_VOID,
    DM_BASED_STRING_PREFIX,
    DM_IDENTIFIER,
    DM_COPY_STRING,
    DM_BASED_STRING_SUFFIX,
    DM_MODIFIER_LIST,
    DM_TYPE_ENCODING,
    DM_TEMPLATE_ARG,
    DM_INTEGER,
    DM_RECURSE_BEGIN,
    DM_RECURSE_END,
    DM_TEMPLATE_NAME,
    DM_TEMPLATE_PREFIX,
    DM_TEMPLATE_SUFFIX,
    DM_TEMPLATE_ARG_SEPARATOR,
    DM_WATCOM_OBJECT,
    DM_ANONYMOUS_ENUM,
    DM_OPERATOR_PREFIX,
    DM_OPERATOR_FUNCTION,
    DM_RELATIONAL_FUNCTION,
    DM_ASSIGNMENT_FUNCTION,
    DM_OPERATOR_NEW,
    DM_OPERATOR_DELETE,
    DM_DESTRUCTOR_CHAR,
    DM_DESTRUCTOR,
    DM_CONSTRUCTOR,
    DM_OPERATOR_CONVERT,
    DM_CTOR_DTOR_NAME,
    DM_SCOPED_NAME,
    DM_NAME,
    DM_SCOPE,
    DM_SCOPE_SEPARATOR,
    DM_MANGLED_NAME,
    DM_TRUNCATED_NAME,
    DM_OPERATOR_DELETE_ARRAY,
    DM_OPERATOR_NEW_ARRAY
} dm_pts;

typedef void (*outfunPtr)( void **, dm_pts, int, const char * );
typedef struct output_desc {
    outfunPtr outfun;
    void *cookie;
    const char *input, *end;
    char *output;
    const char *scope_ptr;
    unsigned size, count, index, pending_loc, scope_len;
    int suppress_output, scope_index;
    unsigned ctdt_pending : 1;
    unsigned cv_pending : 1;
    unsigned scope_name : 1;
    unsigned base_name : 1;
} output_desc;
typedef char WlibDemangleDescriptorSize[(sizeof(output_desc) == 56) ? 1 : -1];
void init_globals( realloc_fn_t reallocator );
void init_descriptor( output_desc *, outfunPtr, void *, const char *, unsigned, char *, unsigned );
#pragma pack(1)
typedef struct state_desc {
    unsigned prefix, suffix;
    char right : 1;
} state_desc;
#pragma pack()
typedef char WlibDemangleStateSize[(sizeof(state_desc) == 9) ? 1 : -1];
int full_mangled_name( output_desc *data );
int scoped_name( output_desc *, state_desc * );
int sym_name( output_desc *, state_desc * );
int op_name( output_desc *, state_desc * );
int ctor_or_convert( output_desc *, state_desc * );
int dtor_or_delete( output_desc *, state_desc * );
int op_new( output_desc *, state_desc * );
int assignment_function( output_desc *, state_desc * );
int relational_function( output_desc *, state_desc * );
int operator_function( output_desc *, state_desc * );
int special_type_names( output_desc *, state_desc * );
int watcom_object( output_desc *, state_desc * );
int base_36_2digit( output_desc *, unsigned * );
unsigned char_to_digit( char );
int name( output_desc *, state_desc * );
int is_identifier( char );
int scope( output_desc *, state_desc * );
int check_recurse( void );
int recursive_mangled_name( output_desc *, state_desc * );
int template_name( output_desc *, state_desc * );
int template_arg( output_desc *, state_desc * );
int base_32_num( output_desc *, int * );
int type_encoding( output_desc *, state_desc * );
int modifier_list( output_desc *, state_desc * );
int based_encoding( output_desc *, state_desc * );
int unmodified_type( output_desc *, state_desc * );
int function( output_desc *, state_desc * );
void setSuppression( output_desc * );
void resetSuppression( output_desc * );
int tq_function( output_desc *, state_desc * );
int array( output_desc *, state_desc * );
int dimension( output_desc *, state_desc * );
int base_10_num( output_desc *, int * );
int pointer( output_desc *, state_desc * );
int basic_type( output_desc *, state_desc * );
char prevChar( output_desc * );
int typeChar( char, int );
int mangled_name( output_desc *data );
void do_copy( output_desc *data );
unsigned terminateOutput( output_desc *data );
void demangleEmit( void **cookie, dm_pts operation, int value, const char *text );
char nextChar( output_desc *data );
void advanceChar( output_desc *data );
void setEmitIndex( output_desc *data, unsigned index );
void resetEmitIndex( output_desc *data, unsigned offset );
void zapSpace( output_desc *data );
void emitChar( output_desc *data, char ch );
void emitStr( output_desc *data, const char *text );
void forceSuppression( output_desc *data );
void unforceSuppression( output_desc *data );
#endif
