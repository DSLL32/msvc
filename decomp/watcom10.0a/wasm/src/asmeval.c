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

/* Adapted from bld/wasm/c/asmeval.c at commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9; CF15f58..160b6.
 * The original frees expression results through AsmFree, not the later cache. */
#include "asminit.h"
#include <recon.h>

int TokCnt;
int EvalExpr(int count, int start_tok, int end_tok)
{
    int i = start_tok;
    int start, num;
    unsigned char final = 0;
    struct expr_list *result;
    if (AsmBuffer[end_tok]->token == T_FINAL) final = 1;
    TokCnt = count;
    if (fix_parant() == -1) return -1;
    while (i < TokCnt && i <= end_tok) {
        if (is_expr(i)) {
            start = i++;
            num = 0;
            for (;;) {
                if (i >= TokCnt || i > end_tok || !is_expr(i)) break;
                ++i;
                ++num;
            }
            if (!(num == 1 && AsmBuffer[start]->token == T_REG &&
                  AsmBuffer[start + 1]->token == ':')) {
                i = start;
                result = evaluate(&i, i + num);
                if (result == NULL) return -1;
                i = fix(result, start, start + num);
                if (count != TokCnt) {
                    end_tok += TokCnt - count;
                    count = TokCnt;
                }
                AsmFree(result);
            }
        }
        ++i;
    }
    fix_final();
    if (AsmBuffer[TokCnt]->token == T_NOOP || final) {
        AsmBuffer[TokCnt]->token = T_FINAL;
        AsmBuffer[TokCnt]->string_ptr = NULL;
    }
    return TokCnt;
}

/* CF15884..1594f. Actual 100-int TakeOut owner at279e0..27b70. */
int TakeOut[100];
int fix_parant(void)
{
    int i, store = 0, dup_count = 0, non_dup_bracket_count = 0;
    for (i = 0; i < TokCnt; ++i) {
        if (AsmBuffer[i]->token == T_RES_ID && AsmBuffer[i]->value == T_DUP) {
            ++dup_count;
            if (AsmBuffer[++i]->token != '(') {
                AsmError(105);
                return -1;
            }
            TakeOut[store++] = i;
            AsmBuffer[i]->token = T_NOOP;
        } else if (AsmBuffer[i]->token == '(') {
            ++non_dup_bracket_count;
        } else if (AsmBuffer[i]->token == ')') {
            if (non_dup_bracket_count) --non_dup_bracket_count;
            else {
                --dup_count;
                TakeOut[store++] = -i;
                AsmBuffer[i]->token = T_NOOP;
            }
        }
    }
    if (dup_count != 0) {
        AsmError(101);
        return -1;
    }
    TakeOut[store] = -1;
    return 1;
}

/* CF15e08..15f55; restore DUP parentheses and remove NOOP tokens. */
void fix_final(void)
{
    int start, end, i, diff, dup_count = 0;
    for (i = 0;; ++i) {
        if (dup_count == 0 && TakeOut[i] == -1) break;
        if (TakeOut[i] > 0) {
            AsmBuffer[TakeOut[i]]->token = '(';
            ++dup_count;
        } else {
            AsmBuffer[-TakeOut[i]]->token = ')';
            --dup_count;
        }
        if (i >= 100) InternalError("..\\c\\asmeval.c", 1183);
    }
    for (start = 0; start < TokCnt; ++start) {
        if (AsmBuffer[start]->token == T_NOOP) {
            for (end = start + 1;; ++end) {
                if (AsmBuffer[end]->token != T_NOOP || end == TokCnt) break;
            }
            if (end == TokCnt) {
                TokCnt = start;
                return;
            }
            diff = end - start;
            if (diff < 0) InternalError("..\\c\\asmeval.c", 1197);
            for (i = end; i < TokCnt; ++i) *AsmBuffer[i - diff] = *AsmBuffer[i];
            TokCnt -= diff;
        }
    }
}
