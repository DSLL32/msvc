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


/* StructDef CF18d40..18e3b. Definition's stack and nesting state use
 * their original packed owner. The same first-pass symbol rules are retained. */
#include <string.h>
#include <ctype.h>
#include "breakout.h"
#include "direct.h"
#include <recon.h>
#ifdef __WATCOMC__
#pragma intrinsic(strlen,strcpy,strcat)
#endif

int StructDef(int i)
{
    char *name;
    asm_sym *symbol;
    dir_node *dir;
    if (i < 0) {
        AsmError(51);
        return -1;
    }
    name = AsmBuffer[i]->string_ptr;
    switch (AsmBuffer[i + 1]->value) {
    case T_STRUC: case T_STRUCT:
        symbol = AsmGetSymbol(name);
        if (Parse_Pass == PASS_1) {
            if (symbol != NULL) { AsmError(71); return -1; }
            dir = dir_insert(name, TAB_STRUCT);
        } else dir = (dir_node *)symbol;
        push((void **)&Definition.struct_stack, Definition.curr_struct);
        Definition.curr_struct = dir;
        ++Definition.struct_depth;
        break;
    case T_ENDS:
        if (Definition.curr_struct != NULL &&
            strcmp(name, Definition.curr_struct->sym.name) == 0) {
            Definition.curr_struct = pop((void **)&Definition.struct_stack);
            --Definition.struct_depth;
        } else { AsmError(51); return -1; }
        break;
    }
    return 1;
}

/* CF1902c..19228; no later loc==-1 scan in the shipped body. */
int AddFieldToStruct( int loc )
/*****************************/
{
    int offset;
    int count = 0;
    int i;
    struct_info *the_struct;
    field_list  *f;

    the_struct = Definition.curr_struct->e.structinfo;

    offset = the_struct->size; // offset of this element

    f = AsmAlloc( sizeof( field_list ) );

    /* now add the initializer to the structure's list */
    f->initializer = AsmAlloc( strlen( AsmBuffer[loc]->string_ptr ) + 1 );
    strcpy( f->initializer, AsmBuffer[ loc ]->string_ptr );

    /* now add the value to initialize the struct to */
    for( i = loc + 1; AsmBuffer[i]->token != T_FINAL; i++ ) {
        if( AsmBuffer[i]->string_ptr != NULL ) {
            count += strlen( AsmBuffer[i]->string_ptr ) + 1;
        }
        if( AsmBuffer[i]->token == T_STRING ) count += 2;
    }

    f->value = AsmAlloc( count + 1 );
    f->value[0] = '\0';

    for( i = loc + 1; AsmBuffer[i]->token != T_FINAL; i++ ) {
        if( AsmBuffer[i]->token == T_STRING ) {
            strcat( f->value, "<" );
        }
        if( AsmBuffer[i]->string_ptr != NULL ) {
            strcat( f->value, AsmBuffer[i]->string_ptr );
        }
        if( AsmBuffer[i]->token == T_STRING ) {
            strcat( f->value, ">" );
        }
        strcat( f->value, " " );
    }

    f->next = NULL;
    if( the_struct->head == NULL ) {
        the_struct->head = the_struct->tail = f;
    } else {
        the_struct->tail->next = f;
        the_struct->tail = f;
    }

    return( offset );
}


/* CF19228..19243, actual structure size remains a16-bit field. */
int GetStructSize(int index)
{
    dir_node *node;
    node = (dir_node *)AsmGetSymbol(AsmBuffer[index]->string_ptr);
    return node->e.structinfo->size;
}

/* CF18e3c..19029, adapted from released asmstruc.c. */
int InitializeStructure(asm_sym *sym, int index)
{
    char buffer[256];
    char *ptr;
    unsigned count;
    asm_sym *symbol;
    dir_node *node;
    field_list *field;
    symbol = AsmGetSymbol(AsmBuffer[index]->string_ptr);
    node = (dir_node *)symbol;
    PushLineQueue();
    if (AsmBuffer[index + 1]->token != T_STRING) {
        AsmError(51);
        return -1;
    }
    if (sym != NULL) {
        sym->total_size = symbol->total_size;
        sym->total_length = symbol->total_length;
        sym->first_size = symbol->first_size;
        sym->first_length = symbol->first_length;
    }
    ptr = AsmBuffer[index + 1]->string_ptr;
    for (field = node->e.structinfo->head; field != NULL; field = field->next) {
        strcpy(buffer, field->initializer);
        strcat(buffer, " ");
        if (ptr == NULL) strcat(buffer, field->value);
        else if (strpbrk(ptr, ",") != NULL) {
            count = strpbrk(ptr, ",") - ptr;
            strncat(buffer, ptr, count);
            ptr += count + 1 < strlen(ptr) ? count + 1 : strlen(ptr);
        } else {
            while (*ptr != '\0' && isspace((unsigned char)*ptr)) ptr++;
            if (*ptr == '\0') strcat(buffer, field->value);
            else strcat(buffer, ptr);
            ptr = NULL;
        }
        InputQueueLine(buffer);
    }
    return 1;
}
