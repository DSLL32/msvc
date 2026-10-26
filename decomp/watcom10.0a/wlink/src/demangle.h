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

#ifndef WLINK_DEMANGLE_H
#define WLINK_DEMANGLE_H
#include <recon.h>

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
typedef char *(*realloc_fn_t)( char *, unsigned );
/* Target1994 descriptor: output precedes scope_ptr in this version. */
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
    unsigned reserved : 28;
} output_desc;
typedef char old_output_descriptor_is_56[(sizeof(output_desc) == 56) ? 1 : -1];
#pragma pack(1)
typedef struct state_desc {
    unsigned prefix, suffix;
    unsigned char right : 1;
} state_desc;
#pragma pack()
typedef char old_state_descriptor_is_9[(sizeof(state_desc) == 9) ? 1 : -1];
extern void init_globals( realloc_fn_t );
extern int __is_mangled( const char *, unsigned );
extern void init_descriptor( output_desc *, outfunPtr, void *, const char *,
                             unsigned, char *, unsigned );
extern void demangleEmit( void **, dm_pts, int, const char * );
extern int do_demangle( output_desc * );
extern void do_copy( output_desc * );
extern char nextChar( output_desc * );
extern void advanceChar( output_desc * );
extern unsigned terminateOutput( output_desc * );
extern void setEmitIndex( output_desc *, unsigned );
extern void resetEmitIndex( output_desc *, unsigned );
extern void zapSpace( output_desc * );
extern void emitChar( output_desc *, char );
extern void emitStr( output_desc *, const char * );
extern void forceSuppression( output_desc * );
extern void unforceSuppression( output_desc * );
#endif
