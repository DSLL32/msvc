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

/* CF14664..1468c, original token-class predicate. */
signed char is_optr(int i)
{
    switch (AsmBuffer[i]->token) {
    case T_REG: case T_NUM: case T_ID: case T_STRING:
    case T_NOOP: case T_PATH: return 0;
    }
    return 1;
}
/* CF1450c..1455e; the shipped1994 allocator has no expression cache. */
expr_list *create_expr(void)
{
    expr_list *node = AsmAlloc(sizeof(*node));
    node->label = node->override = node->instr = -2;
    node->base_reg = node->idx_reg = -2;
    node->expr_type = -2;
    node->value = 0;
    node->scale = 1;
    node->string = NULL;
    node->indirect = 0;
    node->explicit = 0;
    return node;
}

/* CF14560..1461e. The1994 body rejects unsupported token classes before
 * allocating; it does not accept the later RES_ID operand case. */
expr_list *get_operand(int i)
{
    expr_list *node;
    unsigned char *text;
    unsigned char token = AsmBuffer[i]->token;
    if (token != T_NUM && token != T_ID && token != T_STRING && token != T_REG)
        return NULL;
    node = create_expr();
    switch (token) {
    case T_NUM:
        node->type = EXPR_CONST;
        node->value = AsmBuffer[i]->value;
        break;
    case T_STRING:
        node->type = EXPR_CONST;
        node->string = AsmBuffer[i]->string_ptr;
        node->value = 0;
        for (text = (unsigned char *)node->string; *text != '\0'; ++text)
            node->value = ((unsigned long)node->value << 8) | *text;
        break;
    case T_REG:
        node->type = EXPR_REG;
        node->base_reg = i;
        break;
    default:
        node->type = EXPR_ADDR;
        node->label = i;
        break;
    }
    return node;
}

/* CF1468c..146fe; convert a permitted leading sign in place. */
signed char is_unary(int i, char sign)
{
    unsigned char token = AsmBuffer[i]->token;
    if (token == T_UNARY_OPERATOR) return 1;
    switch (token) {
    case T_INS:
        if (AsmBuffer[i]->value == T_NOT) return 1;
        break;
    case T_POSITIVE: case T_NEGATIVE:
        return 1;
    case '+': case '-':
        if (sign) {
            AsmBuffer[i]->token = token == '+' ? T_POSITIVE : T_NEGATIVE;
            return 1;
        }
        break;
    }
    return 0;
}

/* CF14700..1472b; instruction values and token classes have separate
 * namespaces. The shipped non-instruction comparison zero-extends the byte. */
signed char cmp_token(int i, long token)
{
    if (AsmBuffer[i]->token == T_INS) return AsmBuffer[i]->value == token;
    return (long)(unsigned char)AsmBuffer[i]->token == token;
}

/* CF1529c..15699. The shipped evaluator lacks the later opening-
 * parenthesis bounds check and frees temporary results through AsmFree. */
expr_list *evaluate( int *i, int end )
{
    expr_list           *token_1 = NULL;
    expr_list           *token_2 = NULL;
    signed char               start_token_2;
    char                token_needed;
    signed char               queue[2];       // operator queue; store the index
                                        // of the operator token

    token_needed = 0;

    queue[0] = queue[1] = (-2);

    /*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
    /* Look at first token, which may be an unary operator or an operand */
    /*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

    if( cmp_token( *i, '(' ) ) {
        (*i)++;

        token_1 = evaluate( i, end );
        if( token_1 == NULL ) {
            return( NULL );
        }
    } else if( is_unary( *i, 1 ) ) {
        token_needed = 1;
        optr_enqueue( *i, queue );
    } else if( cmp_token( *i, '[' ) ) {
        if( *i == 0 ) {
            return( NULL );
        }
        if (cmp_token((*i)-1, ')')) InternalError("..\\c\\asmeval.c", 737);
        (*i)++;
        token_1 = evaluate( i, end );
        if( token_1 == NULL ) {
            return( NULL );
        }
        token_1->indirect = 1;
    } else {
        token_1 = get_operand( *i );
        if( token_1 == NULL ) {
            return( NULL );
        }
    }

    /*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
    /* If an unary operator is not found, now read the operator */
    /*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

    if( !token_needed ) {

        (*i)++;
        if( *i > end ) {
            /* no operator is found; return token_1 as result */
            return( token_1 );
        }

        /* Read the operator */
        if( cmp_token( *i, ')' ) ||
            cmp_token( *i, ']' ) ) {
            return( token_1 );
        } else if( cmp_token( *i, '[' ) ) {
            AsmBuffer[*i]->token = '+';
        } else if( !is_optr(*i) ) {
            AsmError( 102 );
            AsmFree( token_1 );
            return( NULL );
        }
        token_needed = 1;
        optr_enqueue( *i, queue );
    }

    while( token_needed ) {

        /*:::::::::::::::::::::::::::*/
        /* Now read the next operand */
        /*:::::::::::::::::::::::::::*/

        (*i)++;
        start_token_2 = *i;
        if( *i > end ) {
            AsmError( 103 );
            AsmFree( token_1 );
            return( NULL );
        }

        if( cmp_token( *i, '(' ) ||
            cmp_token( *i, '[' ) ) {
            (*i)++;
            token_2 = evaluate( i, end );
            if( token_2 == NULL ) {
                AsmFree( token_1 );
                return( NULL );
            }
        } else if( is_unary( *i, 1 ) ) {
            token_2 = evaluate( i, end );
            if( token_2 == NULL ) {
                AsmFree( token_1 );
                return( NULL );
            }
        } else if( is_optr( *i ) ) {
            AsmError( 103 );
            AsmFree( token_1 );
            return( NULL );
        } else {
            token_2 = get_operand( *i );
            if( token_2 == NULL ) {
                AsmFree( token_1 );
                return( NULL );
            }
        }

        /*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
        /* Look at the next operator and compare its priority with 1st one */
        /*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

        (*i)++;
        if( *i <= end ) {
            if( cmp_token( *i, '[' ) ) {
                AsmBuffer[*i]->token = '+';
            } else while( cmp_token( *i, ']' ) ) {
                if( *i == end ) break;
                (*i)++;
            }
        }

        if( *i <= end ) {
            if( !is_optr( *i ) || is_unary( *i, 0 ) ||
                cmp_token( *i, '(' ) ) {
                AsmError( 102 );
                AsmFree( token_1 );
                AsmFree( token_2 );
                return( NULL );
            } else if( !cmp_token( *i, ')' ) &&
                       !cmp_token( *i, ']' ) ) {

                if( cmp_token( *i, '[' ) ) {
                    AsmBuffer[*i]->token = '+';
                }
                if( get_precedence( *i ) < get_precedence( queue[0] ) ) {
                    (*i) = start_token_2;
                    AsmFree( token_2 );
                    token_2 = evaluate( i, end );
                    if( token_2 == NULL ) {
                        AsmFree( token_1 );
                        return( NULL );
                    }
                } else {
                    optr_enqueue( *i, queue );
                }
            }
        }

        /*::::::::::::::*/
        /* Now evaluate */
        /*::::::::::::::*/

        if( token_1 == NULL ) {
            /* the operator is unary */
            token_1 = create_expr();
        }

        if( calculate( token_1, token_2, queue[0] )== -1 ) {
            AsmFree( token_1 );
            AsmFree( token_2 );
            return( NULL );
        }

        AsmFree( token_2 );
        optr_dequeue( queue );
        if( queue[0] == (-2) ) {
            token_needed = 0;
        }
    }
    return( token_1 );
}

/* CF1569c..15881. The1994 switch has neither the later FLAT setup
 * nor LOCK-prefix exclusion. Keep its instruction-position distinctions. */
int is_expr(int i)
{
    switch (AsmBuffer[i]->token) {
    case T_INS:
    case T_RES_ID:
        switch (AsmBuffer[i]->value) {
        case T_EQ: case T_NE: case T_LT: case T_LE:
        case T_GT: case T_GE: case T_MOD: case T_NUM:
            return 1;
        case T_SHL: case T_SHR: case T_NOT:
        case T_AND: case T_OR: case T_XOR:
            if (i == 0 || AsmBuffer[i - 1]->token == T_COLON) return 0;
            return 1;
        default:
            return 0;
        }
    case T_UNARY_OPERATOR:
        return i + 1 < TokCnt;
    case T_REG:
        return AsmBuffer[i]->value != T_ST;
    case '+': case '-':
        if (AsmBuffer[i + 1]->token == T_FLOAT) return 0;
        /* fall through */
    case '*': case '/': case T_NUM:
    case '(': case ')': case '[': case ']':
        return 1;
    case T_COLON:
        if (i == 1 || (AsmBuffer[i + 1]->token == T_DIRECTIVE &&
                       AsmBuffer[i + 1]->value == T_EQU2)) return 0;
        return 1;
    case T_ID:
        return i != 0;
    case T_STRING:
        return 1;
    default:
        return 0;
    }
}

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

/* CF15950..15e00; unlike the2002 Store cache, the1994 body owns one
 * separately allocated token for each original index in this local table. */
int fix(expr_list *res, int start, int end)
{
    struct asm_tok *store[100];
    int size = 0, diff, i, old_start, old_end, need_number;
    if (res->type == EXPR_CONST) {
        if (res->override != -2) {
            *AsmBuffer[start++] = *AsmBuffer[res->override];
            AsmBuffer[start++]->token = T_COLON;
        }
        if (res->string == NULL) {
            AsmBuffer[start]->token = T_NUM;
            AsmBuffer[start++]->value = res->value;
        } else {
            AsmBuffer[start]->token = T_STRING;
            AsmBuffer[start++]->string_ptr = res->string;
        }
    } else if (res->type == EXPR_REG && !res->indirect) {
        if (res->override != -2) {
            *AsmBuffer[start++] = *AsmBuffer[res->override];
            AsmBuffer[start++]->token = T_COLON;
            AsmBuffer[start++]->token = '[';
        }
        AsmBuffer[start]->token = T_REG;
        AsmBuffer[start++]->value = AsmBuffer[res->base_reg]->value;
        if (res->override != -2) AsmBuffer[start++]->token = ']';
    } else {
        if (res->type != EXPR_REG) ++size;
        if (res->instr != -2) ++size;
        if (res->override != -2) size += 2;
        need_number = 1;
        if (res->scale != 1) size += 2;
        if (res->base_reg != -2) { size += 3; need_number = 0; }
        if (res->idx_reg != -2) { size += 3; need_number = 0; }
        if (res->label != -2) need_number = 0;
        if (res->value != 0) need_number = 1;
        if (need_number) size += 3;
        old_start = start;
        old_end = end;
        for (i = start; i <= end; ++i) {
            store[i] = AsmAlloc(sizeof(*store[i]));
            *store[i] = *AsmBuffer[i];
        }
        diff = size - (end - start + 1);
        if (diff > 0) {
            for (i = TokCnt - 1; i > end; --i)
                *AsmBuffer[i + diff] = *AsmBuffer[i];
            for (i = 0; TakeOut[i] != -1; ++i)
                if (TakeOut[i] > end) TakeOut[i] += diff;
            end += diff;
            TokCnt += diff;
        }
        if (res->instr != -2) {
            AsmBuffer[start]->token = T_UNARY_OPERATOR;
            AsmBuffer[start++]->value = store[res->instr]->value;
        }
        if (res->override != -2) {
            *AsmBuffer[start++] = *store[res->override];
            AsmBuffer[start++]->token = T_COLON;
        }
        if (res->label != -2 && res->type != EXPR_REG)
            *AsmBuffer[start++] = *store[res->label];
        if (res->base_reg != -2) {
            AsmBuffer[start++]->token = '[';
            AsmBuffer[start]->token = T_REG;
            AsmBuffer[start++]->value = store[res->base_reg]->value;
            AsmBuffer[start++]->token = ']';
        }
        if (res->idx_reg != -2) {
            AsmBuffer[start++]->token = '[';
            AsmBuffer[start]->token = T_REG;
            AsmBuffer[start++]->value = store[res->idx_reg]->value;
            if (res->scale != 1) {
                AsmBuffer[start++]->token = '*';
                AsmBuffer[start]->token = T_NUM;
                AsmBuffer[start++]->value = res->scale;
                res->scale = 1;
            }
            AsmBuffer[start++]->token = ']';
        }
        if (need_number) {
            AsmBuffer[start++]->token = '[';
            AsmBuffer[start]->token = T_NUM;
            AsmBuffer[start++]->value = res->value;
            AsmBuffer[start++]->token = ']';
        }
        for (i = old_start; i <= old_end; ++i) AsmFree(store[i]);
        if (start - 1 > end) InternalError("..\\c\\asmeval.c", 1150);
    }
    for (i = start; i <= end; ++i) AsmBuffer[i]->token = T_NOOP;
    return end;
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

/* CF14620..14655, original assertion asmeval.c line161. */
void optr_enqueue(int index, signed char queue[])
{
    signed char i;
    if (queue[0] == -2) {
        i = 0;
    } else {
        if (queue[1] != -2) InternalError("..\\c\\asmeval.c",161);
        i = 1;
    }
    queue[i] = index;
}

/* CF14828..1529b, initial source adapted to1994: no later
 * duplicate-label addition diagnostic or FLAT token-rewrite workaround. */
signed char calculate( expr_list *token_1,expr_list *token_2, unsigned char index )
/* Perform the operation between token_1 and token_2 */
{
    char                token;
    struct asm_sym      *sym;

    token = AsmBuffer[index]->token;

    token_1->string = NULL;

    switch( token ) {
        case T_POSITIVE:
            /*
             * The only format allowed is:
             *        + constant
             */

            MakeConst( token_2 );
            if( token_2->type != EXPR_CONST ) {
                AsmError( 122 );
            }
            token_1->type = EXPR_CONST;
            token_1->value = token_2->value;
            break;
        case T_NEGATIVE:
            /*
             * The only format allowed is:
             *        - constant
             */

            MakeConst( token_2 );
            if( token_2->type != EXPR_CONST ) {
                AsmError( 129 );
            }
            token_1->type = EXPR_CONST;
            token_1->value = -token_2->value;
            break;
        case '+':
            /*
             * The only formats allowed are:
             *        constant + constant
             *        constant + address
             *         address + register       ( only inside [] )
             *        register + register       ( only inside [] )
             *        register + constant       ( only inside [] )
             *        address  + address        ( only inside [] )
             */

            if( check_same( token_1, token_2, EXPR_CONST ) ) {

                token_1->value += token_2->value;

            } else if( check_same( token_1, token_2, EXPR_ADDR ) ) {

                index_connect( token_1, token_2 );
                if( token_2->label != (-2) ) {
                    MakeConst( token_1 );
                }
                if( token_1->label == (-2) ) {
                    token_1->label = token_2->label;

                }
                token_1->indirect |= token_2->indirect;

            } else if( check_both( token_1, token_2, EXPR_CONST, EXPR_ADDR ) ) {

                if( token_1->type == EXPR_CONST ) {
                    token_2->value += token_1->value;
                    token_2->indirect |= token_1->indirect;
                    TokenAssign( token_1, token_2 );
                } else {
                    token_1->value += token_2->value;
                }

            } else if( check_both( token_1, token_2, EXPR_ADDR, EXPR_REG ) ) {

                    if( token_1->type == EXPR_REG ) {
                        if( token_2->instr != (-2) ) {
                            AsmError( 132 );
                            return( (-1) );
                        }
                        index_connect( token_2, token_1 );
                        token_2->indirect |= token_1->indirect;
                        TokenAssign( token_1, token_2 );
                    } else {
                        index_connect( token_1, token_2 );
                        token_1->indirect |= token_2->indirect;
                    }

            } else if( check_same( token_1, token_2, EXPR_REG ) ) {

                    index_connect( token_1, token_2 );
                    token_1->indirect |= token_2->indirect;
                    token_1->type = EXPR_ADDR;

            } else if( check_both( token_1, token_2, EXPR_CONST, EXPR_REG ) ) {

                    if( token_2->type == EXPR_REG ) {
                        token_1->base_reg = token_2->base_reg;
                        token_1->idx_reg = token_2->idx_reg;
                        token_2->base_reg = (-2);
                        token_2->idx_reg = (-2);
                    }
                    token_1->value += token_2->value;
                    token_1->indirect |= token_2->indirect;
                    token_1->type = EXPR_ADDR;
            } else {
                    /* Error */
                    AsmError( 118 );
                    return( (-1) );
            }
            break;
        case '-':
            /*
             * The only formats allowed are:
             *        constant - constant
             *         address - constant       ( only in this order )
             *         address - address
             *        register - constant       ( only inside [] and in this
             *                                    order )
             */

            MakeConst( token_2 );
            if( check_same( token_1, token_2, EXPR_CONST ) ) {

                token_1->value -= token_2->value;

            } else if( token_1->type == EXPR_ADDR &&
                       token_2->type == EXPR_CONST ) {

                token_1->value -= token_2->value;

            } else if( check_same( token_1, token_2, EXPR_ADDR ) ){

                if( token_2->label == (-2) ) {
                    token_1->value -= token_2->value;
                    token_1->base_reg = token_2->base_reg;
                    token_1->idx_reg = token_2->idx_reg;
                    token_1->scale = token_2->scale;
                    token_1->indirect |= token_2->indirect;
                } else {
                    if( token_1->label == (-2) ) {
                        AsmError( 51 );
                        return( (-1) );
                    }
                    sym = AsmLookup( AsmBuffer[token_1->label]->string_ptr );
                    if( sym == NULL ) return( (-1) );

                    if( Parse_Pass > PASS_1 && sym->state == SYM_UNDEFINED ) {
                        AsmError( 130 );
                        return( (-1) );
                    }
                    token_1->value += sym->offset;
                    sym = AsmLookup( AsmBuffer[token_2->label]->string_ptr );
                    if( sym == NULL ) return( (-1) );

                    if( Parse_Pass > PASS_1 && sym->state == SYM_UNDEFINED ) {
                        AsmError( 130 );
                        return( (-1) );
                    }
                    token_1->value -= sym->offset;
                    token_1->value -= token_2->value;
                    token_1->label = (-2);
                    token_1->base_reg = token_2->base_reg;
                    token_1->idx_reg = token_2->idx_reg;
                    token_1->scale = token_2->scale;
                    if( token_1->base_reg == (-2) && token_1->idx_reg == (-2) ) {
                        token_1->type = EXPR_CONST;
                        token_1->indirect = 0;
                    } else {
                        token_1->type = EXPR_ADDR;
                        token_1->indirect |= token_2->indirect;
                    }
                    token_1->explicit = 0;
                    token_1->expr_type = (-2);
                }

            } else if( token_1->type == EXPR_REG &&
                        token_2->type == EXPR_CONST ) {

                token_1->value = -1 * token_2->value;
                token_1->indirect |= token_2->indirect;
                token_1->type = EXPR_ADDR;

            } else {
                /* Error */
                AsmError( 119 );
                return( (-1) );
            }
            break;
        case '*':
            /*
             * The only formats allowed are:
             *        constant * constant
             *        register * scaling factor ( 1, 2, 4 or 8 )
             *                   386 only
             */
            MakeConst( token_1 );
            MakeConst( token_2 );
            if( check_same( token_1, token_2, EXPR_CONST ) ) {
                token_1->value *= token_2->value;
            } else if( check_both( token_1, token_2, EXPR_REG, EXPR_CONST ) ) {
                /* scaling factor */
                if( token_2->type == EXPR_REG ) {
                    /* scale * reg */
                    token_1->idx_reg = token_2->base_reg;
                    token_1->base_reg = (-2);
                    token_1->scale = token_1->value;
                    token_1->value = 0;
                    token_2->base_reg = (-2);
                } else {
                    /* reg * scale */
                    token_1->idx_reg = token_1->base_reg;
                    token_1->base_reg = (-2);
                    token_1->scale = token_2->value;
                }
                token_1->indirect |= token_2->indirect;
                token_1->type = EXPR_ADDR;
            } else {
                AsmError( 120 );
                return( (-1) );
            }
            break;
        case '/':
            /*
             * The only formats allowed are:
             *        constant / constant
             */
            MakeConst( token_1 );
            MakeConst( token_2 );
            if( check_same( token_1, token_2, EXPR_CONST ) ) {
                token_1->value /= token_2->value;
            } else {
                AsmError( 121 );
                return( (-1) );
            }
            break;
        case T_COLON:
            /*
             * The only formats allowed are:
             *        register : anything ----- segment override
             *           label : address ( label = address with no offset
             *                             and no instruction attached;
             *                             also only segment or group is
             *                             allowed. )
             */
            if( token_2->override != (-2) ) {
                 /* Error */
                 AsmError( 131 );
                 return( (-1) );
            }

            if( token_1->type == EXPR_REG ) {

                 if( token_1->base_reg != (-2) && token_1->idx_reg != (-2) ) {
                     AsmError( 106 );
                     return( (-1) );
                 }
                 token_2->override = token_1->base_reg;
                 token_2->indirect |= token_1->indirect;
                 token_2->type = EXPR_ADDR;
                 TokenAssign( token_1, token_2 );

            } else if( token_2->type == EXPR_ADDR &&
                        token_1->type == EXPR_ADDR &&
                        token_1->override == (-2) &&
                        token_1->instr == (-2)    &&
                        token_1->value == 0        &&
                        token_1->base_reg == (-2) &&
                        token_1->idx_reg == (-2) ) {

                sym = AsmLookup( AsmBuffer[token_1->label]->string_ptr );
                if( sym == NULL ) return( (-1) );

                if( sym->state == SYM_GRP || sym->state == SYM_SEG ) {
                    token_2->override = token_1->label;
                    token_2->indirect |= token_1->indirect;
                    TokenAssign( token_1, token_2 );
                } else {
                    AsmError( 133 );
                    return( (-1) );
                }
            } else {
                 AsmError( 134 );
                 return( (-1) );
            }
            break;
        case T_INS:
            MakeConst( token_1 );
            MakeConst( token_2 );
            if( AsmBuffer[index]->value == T_NOT ) {
                if( token_2->type != EXPR_CONST ) {
                    AsmError( 117 );
                    return( (-1) );
                }
                token_1->type = EXPR_CONST;
            } else {
                if( !check_same( token_1, token_2, EXPR_CONST ) ) {
                    AsmError( 117 );
                    return( (-1) );
                }
            }
            switch( AsmBuffer[index]->value ) {
                case T_EQ:
                    token_1->value = ( token_1->value == token_2->value ? -1:0 );
                    break;
                case T_NE:
                    token_1->value = ( token_1->value != token_2->value ? -1:0 );
                    break;
                case T_LT:
                    token_1->value = ( token_1->value < token_2->value ? -1:0 );
                    break;
                case T_LE:
                    token_1->value = ( token_1->value <= token_2->value ? -1:0 );
                    break;
                case T_GT:
                    token_1->value = ( token_1->value > token_2->value ? -1:0 );
                    break;
                case T_GE:
                    token_1->value = ( token_1->value >= token_2->value ? -1:0 );
                    break;
                case T_MOD:
                    token_1->value %= token_2->value;
                    break;
                case T_SHL:
                    token_1->value = token_1->value << token_2->value;
                    break;
                case T_SHR:
                    token_1->value = token_1->value >> token_2->value;
                    break;
                case T_NOT:
                    token_1->value = ~(token_2->value);
                    break;
                case T_AND:
                    token_1->value &= token_2->value;
                    break;
                case T_OR:
                    token_1->value |= token_2->value;
                    break;
                case T_XOR:
                    token_1->value ^= token_2->value;
                    break;
            }
            break;
        case T_UNARY_OPERATOR:
            if( token_2->type != EXPR_ADDR ) {
                AsmError( 132 );
                return( (-1) );
            } else if( token_2->instr != (-2) ) {
                AsmError( 132 );
                return( (-1) );
            }
            switch( AsmBuffer[index]->value ) {
            case T_LENGTH:
            case T_SIZE:
            case T_LENGTHOF:
            case T_SIZEOF:
                sym = AsmLookup( AsmBuffer[token_2->label]->string_ptr );
                if( sym == NULL ) return( (-1) );
                switch( AsmBuffer[index]->value ) {
                case T_LENGTH:
                    token_1->value = sym->first_length;
                    break;
                case T_LENGTHOF:
                    token_1->value = sym->total_length;
                    break;
                case T_SIZE:
#if 0 /* this gives a screwy number for structure sizes */
                    token_1->value = sym->first_size;
                    break;
#endif
                case T_SIZEOF:
                    token_1->value = sym->total_size;
                    break;
                }
                token_1->label = (-2);
                token_1->base_reg = (-2);
                token_1->idx_reg = (-2);
                token_1->override = (-2);
                token_1->instr = (-2);
                token_1->type = EXPR_CONST;
                token_1->indirect = 0;
                token_1->explicit = 0;
                token_1->expr_type = (-2);
                break;
            default:
                TokenAssign( token_1, token_2 );
                token_1->instr = index;
                break;
            }
            break;
    }
    return( 1 );
}

/* CF14804..14826, transfers the complete38-byte expression owner. */
void TokenAssign(expr_list *first, expr_list *second)
{
    *first = *second;
    second->base_reg = -2;
    second->idx_reg = -2;
}

/* CF14658..14664, remove the oldest queued operator. */
void optr_dequeue(signed char queue[])
{
    queue[0] = queue[1];
    queue[1] = -2;
}

/* CF1472c..1473c, matching expression owner classes. */
signed char check_same(expr_list *first, expr_list *second, signed char type)
{
    return first->type == type && second->type == type;
}

/* CF1473c..14755, either ordering of the two expression classes. */
signed char check_both(expr_list *first, expr_list *second, signed char type1, signed char type2)
{
    return (first->type == type1 && second->type == type2) ||
           (first->type == type2 && second->type == type1);
}

/* CF14340..1450c, original MASM-style precedence and cold assertion. */
int get_precedence( int i )
{
    /* Base on MASM 6.0 pg.18 Table 1.3 */

    char        token;

    token = AsmBuffer[i]->token;

    switch( token ) {
        case T_UNARY_OPERATOR:
            switch( AsmBuffer[i]->value ) {
                case T_LENGTH:
                case T_SIZE:
                case T_LENGTHOF:
                case T_SIZEOF:
                    return( 2 );
                case T_SEG2:
                case T_OFFSET:
                    return( 5 );
            }
            break;
        case T_INS:
            switch( AsmBuffer[i]->value ) {
                case T_MOD:
                case T_SHL:
                case T_SHR:
                    return( 8 );
                case T_EQ:
                case T_NE:
                case T_LT:
                case T_LE:
                case T_GT:
                case T_GE:
                    return( 10 );
                case T_NOT:
                    return( 11 );
                case T_AND:
                    return( 12 );
                case T_OR:
                case T_XOR:
                    return( 13 );
            }
        case T_COLON:
            return( 4 );
        case T_POSITIVE:
        case T_NEGATIVE:
            return( 7 );
        case '*':
        case '/':
            return( 8 );
        case '+':
        case '-':
            return( 9 );
        default:
            InternalError("..\\c\\asmeval.c",95);
    }
    return( -1 );
}

/* CF14798..14803, structure-field displacement to constant conversion. */
void MakeConst( expr_list *token )
{
    struct asm_sym      *sym;

    if( token->type != EXPR_ADDR ) return;
    if( token->label == (-2) ) return;
    sym = AsmLookup( AsmBuffer[token->label]->string_ptr );
    if( sym == NULL ) return;
    if( sym->state != SYM_STRUCT_FIELD ) return;
    token->value += sym->offset;
    token->label = (-2);
    if( token->base_reg != (-2) ) return;
    if( token->idx_reg != (-2) ) return;
    if( token->override != (-2) ) return;
    token->instr = (-2);
    token->type = EXPR_CONST;
    token->indirect = 0;
    token->explicit = 0;
    token->expr_type = 0;
}

/* CF14758..14795, connect the two register/scale owners. */
void index_connect( expr_list *tok_1, expr_list *tok_2 )
/* Connects the register lists */
{
    int         reg;

    if( tok_2->base_reg != (-2) ) {
        reg = tok_2->base_reg;
    } else if( tok_2->idx_reg != (-2) ) {
        reg = tok_2->idx_reg;
    } else {
        return;
    }
    if( tok_1->base_reg == (-2) && tok_2->scale == 1 ) {
        tok_1->base_reg = reg;
    } else {
        tok_1->idx_reg = reg;
    }
    tok_1->scale *= tok_2->scale;
}
