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
* Description: Original-source attribution retained for the adapted entry.
*
****************************************************************************/

/* Adapted from bld/wasm/c/expand.c, initial commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9; CF19a1c..19a41.
 * createconstant's fifth bool argument is the original callee-cleaned stack
 * argument after EAX/EDX/EBX/ECX. Native uses the authorized GCC ABI. */
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "asminit.h"
#include <recon.h>

#ifdef __WATCOMC__
/* Shipped bodies expand these string operations in place. */
#pragma intrinsic(strlen,strcpy,strcat)
#endif

/* CF199e0..19a1c. A named constant definition must begin at token0. */
int DefineConstant(int index, unsigned char redefine, unsigned char expand_early)
{
    if (index != 0) {
        AsmError(51);
        return -1;
    }
    return createconstant(AsmBuffer[index]->string_ptr, 0, index + 2,
                          redefine, expand_early);
}

int StoreConstant(char *name, char *value, signed char redefine)
{
    AsmScan(value, StringBuf);
    return createconstant(name, 0, 0, redefine, 0);
}

/* CF19ae4..19e2f. No later '$' location-label expansion or extra_value field. */
int createconstant(char *name, int value, int start, signed char redefine,
                   unsigned char expand_early)
{
    struct asm_tok *new;
    dir_node *dir;
    asm_sym *sym;
    int i, count;
    sym = AsmGetSymbol(name);
    if (sym == NULL) {
        dir = dir_insert(name, TAB_CONST);
        if (dir == NULL) return -1;
        dir->e.constinfo->redefine = redefine;
        dir->e.constinfo->expand_early = expand_early;
        dir->e.constinfo->data = NULL;
        dir->e.constinfo->count = 0;
        dir->sym.state = SYM_UNDEFINED;
    } else {
        dir = (dir_node *)sym;
        if (sym->state == SYM_UNDEFINED) {
            dir->e.constinfo = AsmAlloc(sizeof(const_info));
            dir->e.constinfo->redefine = redefine;
            dir->e.constinfo->expand_early = expand_early;
            sym->grpidx = sym->segidx = sym->offset = 0;
            dir->e.constinfo->data = NULL;
        } else if (sym->state != SYM_CONST
                   || (!dir->e.constinfo->redefine && Parse_Pass == PASS_1)) {
            AsmError(1049); /* LABEL_ALREADY_DEFINED */
            return -1;
        }
    }
    if (value) {
        new = AsmAlloc(sizeof(*new));
        new[0].token = T_NUM;
        new[0].value = 1;
        new[0].string_ptr = NULL;
        if (dir->e.constinfo->data != NULL) {
            FreeInfo(dir);
            dir->e.constinfo = AsmAlloc(sizeof(const_info));
            dir->e.constinfo->redefine = redefine;
            dir->e.constinfo->expand_early = expand_early;
        }
        dir->e.constinfo->count = 1;
        dir->e.constinfo->data = new;
        dir->sym.state = SYM_CONST;
        return 1;
    }
    if (ExpandTheWorld(start, 0) == -1) return -1;
    for (i = start; AsmBuffer[i]->token != T_FINAL; ++i) ;
    count = i - start;
    new = AsmAlloc(count * sizeof(*new));
    for (i = 0; i < count; ++i) {
        if (AsmBuffer[start+i]->token == T_STRING
            && AsmBuffer[start+i]->value == 0) {
            --i;
            --count;
            continue;
        }
        new[i].token = AsmBuffer[start+i]->token;
        new[i].value = AsmBuffer[start+i]->value;
        if (AsmBuffer[start+i]->string_ptr == NULL) {
            new[i].string_ptr = NULL;
        } else {
            new[i].string_ptr = AsmAlloc(strlen(AsmBuffer[start+i]->string_ptr) + 1);
            strcpy(new[i].string_ptr, AsmBuffer[start+i]->string_ptr);
        }
    }
    if (dir->e.constinfo->data != NULL) {
        FreeInfo(dir);
        dir->e.constinfo = AsmAlloc(sizeof(const_info));
        dir->e.constinfo->redefine = redefine;
        dir->e.constinfo->expand_early = expand_early;
    }
    dir->e.constinfo->count = count;
    dir->e.constinfo->data = new;
    dir->sym.state = SYM_CONST;
    return 1;
}

/* CF19e30..19e6d. Token_Count is the actual shared expression-line count;
 * StoreConstant does not assign the separate AsmScan return value to it. */
int Token_Count;
int ExpandTheWorld(int start_pos, unsigned char early_only)
{
    int val;
    if (ExpandAllConsts(start_pos, early_only) == -1) return -1;
    if (!early_only) {
        val = EvalExpr(Token_Count, start_pos, Token_Count);
        if (val == -1) val = 0;
        Token_Count = val;
    }
    return 1;
}

/* CF19e70..19ee0. Unlike the later source, this body does not reset
 * Globals.expand_count on entry; input_get owns that per-line reset. */
int ExpandAllConsts(int start_pos, unsigned char early_only)
{
    int i;
    if (AsmBuffer[start_pos + 1]->token == T_DIRECTIVE) {
        switch (AsmBuffer[start_pos + 1]->value) {
        case T_EQU:
        case T_EQU2:
        case T_TEXTEQU:
            start_pos += 2;
        }
    }
    for (i = start_pos; AsmBuffer[i]->token != T_FINAL; ++i) {
        if (AsmBuffer[i]->token != T_ID) continue;
        switch (ExpandSymbol(i, early_only)) {
        case -1: return -1;
        case 2: --i; break;
        }
    }
    return 1;
}

/* Initial bld/wasm/c/expand.c, MakeConstant; actual CF19a44..19ae4.
 * The20-byte local and18-byte mnemonic capacity are original. */
void MakeConstant(long token)
{
    char buffer[20];
    buffer[0] = '\0';
    strcat(buffer, "__");
    GetInsString((unsigned short)token, buffer + 2, 18);
    strcat(buffer, "__");
    strupr(buffer);
    createconstant(buffer, 1, 0, 1, 0);
}


/* Initial expand.c ExpandSymbol, actual CF19450..19531. */
int ExpandSymbol(int index, int early_only)
{
    asm_sym *sym;
    dir_node *dir;
    int j;
    sym = AsmGetSymbol(AsmBuffer[index]->string_ptr);
    if (sym == NULL) return 1;
    if (sym->state == SYM_CONST) {
        dir = (dir_node *)sym;
        if (!dir->e.constinfo->expand_early && early_only) return 1;
        AddTokens(AsmBuffer,index,dir->e.constinfo->count - 1);
        for (j = 0; j < dir->e.constinfo->count; ++j) {
            AsmBuffer[index + j]->token = dir->e.constinfo->data[j].token;
            AsmBuffer[index + j]->value = dir->e.constinfo->data[j].value;
            AsmBuffer[index + j]->string_ptr = dir->e.constinfo->data[j].string_ptr;
        }
        ++Globals.expand_count;
        if (Globals.expand_count >= 20) {
            AsmError(1081);
            return -1;
        }
        return 2;
    }
    return 1;
}

/* ExpandProcString CF19534..199de; temporary token copies are stack
 * allocations in1994, before the later AsmTmpAlloc cache. */
int ExpandProcString( int index )
/*******************************/
{
    signed char               i;
    signed char               cnt;
    signed char               count = 0; /* number of words in the name string */
    char                *string;
    char                *word;
    char                *replace = NULL;
    char                buffer[256];
    label_list          *label;

    string = alloca( strlen( AsmBuffer[index]->string_ptr ) + 1 );
    strcpy( string, AsmBuffer[index]->string_ptr );
    wipe_space( string );
    word = strtok( string, " \t" );
    while( word != NULL ) {
        count++;

        if (CurrProc == NULL) InternalError("..\\c\\expand.c", 125);
        label = label_cmp( word, CurrProc->e.procinfo->paralist );
        if( label == NULL ) {
            label = label_cmp( word, CurrProc->e.procinfo->locallist );
        }
        if( label != NULL ) {
            if( label->replace != NULL ) {
                replace = label->replace;
            }
        }

        if( replace != NULL ) {
            if( *replace == '\0' ) {
                AsmErr( 1128, replace+1 );
                return( -1 );
            }
            if( index > 0 && AsmBuffer[index-1]->token == T_DIRECTIVE ) {
                switch( AsmBuffer[index-1]->value ) {
                case T_IFDEF:
                case T_IFNDEF:
                    /* do NOT expand strings in IFDEF and IFNDEF ins.
                     * we want to know if they are defined, NOT their value
                     */
                    return( 1 );
                }
            }
            if( AsmBuffer[index+1]->token == T_DIRECTIVE ) {
                /* this will never happen with multiple words in a string */
                switch( AsmBuffer[index+1]->value ) {
                case T_EQU:
                case T_EQU2:
                case T_TEXTEQU:
                    return( 1 );
                }
            }
            break;
        }
        word = strtok( NULL, " \t" );
    }
    if( replace == NULL ) return( 1 );



    /* now we need to build the new line string to pass through the scanner */
    buffer[0] = '\0';
    /* NOTE: if we have a T_DIRECTIVE, token_count is always set to 1 !??! */
    for( i=0; i < Token_Count; i++ ) {
        if( i != index ) {
            // if( expand_directive_string( buffer, i ) == -1 ) return( -1 );
            if( AsmBuffer[i]->token == T_STRING &&
                *AsmBuffer[i]->string_ptr == '\0' ) {
                strcat( buffer, "<>" );
            } else {
                strcat( buffer, AsmBuffer[i]->string_ptr );
            }
        } else {
            if( AsmBuffer[i]->token == T_PERCENT ) {
                /* don't save the % */
                i++;
            }
            // if( expand_directive_string( buffer, i ) == -1 ) return( -1 );

            /* copy the string in ... 1 word at a time */
            string = alloca( strlen( AsmBuffer[index]->string_ptr ) + 1 );
            strcpy( string, AsmBuffer[index]->string_ptr );
            wipe_space( string );
            word = strtok( string, " \t" );
            for( cnt = 1; cnt < count; cnt++ ) {
                strcat( buffer, word );
                strcat( buffer, " " );
                word = strtok( NULL, " \t" );
            }
            strcat( buffer, replace );
            word = strtok( NULL, " \t" );
            while( word != NULL ) {
                strcat( buffer, " " );
                strcat( buffer, word );
                word = strtok( NULL, " \t" );
            }
        }
        strcat( buffer, " " );
    }
    /* make sure this line goes at the front of the queue */
    PushLineQueue();
    InputQueueLine( buffer );
    AsmBuffer[0]->token = 0;
    AsmBuffer[0]->string_ptr = NULL;
    AsmBuffer[0]->value = 0;
    return( 2 );
}


/* CF193c0..193e8: case-insensitive procedure parameter/local lookup. */
label_list *label_cmp(char *word, label_list *head)
{
    label_list *label;
    for (label = head; label != NULL; label = label->next) {
        if (stricmp(label->label, word) == 0) return label;
    }
    return NULL;
}

/* CF193e8..1944e, shift token contents while retaining buffer-slot owners. */
void AddTokens(struct asm_tok **buffer, int start, int count)
{
    int i;
    if (count != 0) {
        for (i = Token_Count; i >= start; --i) {
            buffer[i + count]->token = buffer[i]->token;
            buffer[i + count]->value = buffer[i]->value;
            buffer[i + count]->string_ptr = buffer[i]->string_ptr;
        }
        Token_Count += count;
    }
}
