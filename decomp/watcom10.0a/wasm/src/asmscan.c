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

/* Adapted from bld/wasm/c/asmscan.c at initial commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9. CF79a8..7b3e. */
#include <ctype.h>
#include <string.h>
#include "asminit.h"
#include <recon.h>
#ifdef __WATCOMC__
#pragma intrinsic(strlen,strcpy)
#endif

char *CurrString;

int AsmScan(char *string, char *stringbuf)
{
    char *ptr, *output_ptr;
    unsigned int buf_index = 0;
    output_ptr = stringbuf;
    for (CurrString = ptr = string; ; ) {
        AsmBuffer[buf_index]->string_ptr = output_ptr;
        while (isspace(*ptr)) ++ptr;
        if (*ptr == '\0') break;
        if (isalpha(*ptr) || *ptr == '_' || *ptr == '$' || *ptr == '@'
            || *ptr == '?' || *ptr == '\\' || (*ptr == '.' && buf_index == 0)) {
            if (get_id(&buf_index, &ptr, &output_ptr) == -1) return -1;
            if (AsmBuffer[buf_index]->token == T_DIRECTIVE
                && (AsmBuffer[buf_index]->value == T_INCLUDE
                    || AsmBuffer[buf_index]->value == T_INCLUDELIB)) {
                ++buf_index;
                get_inc_path(&buf_index, &ptr, &output_ptr);
            }
        } else if (isdigit(*ptr)) {
            if (get_number(AsmBuffer[buf_index], &ptr, &output_ptr) == -1) return -1;
        } else if (*ptr == '`') {
            if (get_id_in_backquotes(AsmBuffer[buf_index], &ptr, &output_ptr) == -1) return -1;
        } else {
            if (get_special_symbol(AsmBuffer[buf_index], &ptr, &output_ptr) == -1) return -1;
        }
        ++buf_index;
        if (buf_index >= 100) {
            AsmError(104); /* TOO_MANY_TOKENS */
            return -1;
        }
    }
    AsmBuffer[buf_index]->token = T_FINAL;
    *output_ptr = '\0';
    return buf_index;
}

/* Actual CF71c0..74da:32-bit results only; invalid digits report error after
 * the original pointer/string updates, rather than newer T_BAD_NUM recovery. */
int get_number(struct asm_tok *buf, char **input, char **output)
{
    char *ptr = *input;
    char *dig_start;
    unsigned char first_char_0 = 0;
    unsigned extra = 0, len, base = 0, digits_seen = 0;
    unsigned long val;
#define VALID_BINARY 0x0003U
#define VALID_OCTAL 0x00ffU
#define VALID_DECIMAL 0x03ffU
#define OK_NUM(t) ((digits_seen & ~VALID_##t) == 0)
    if (*ptr == '0') {
        if (tolower((unsigned char)ptr[1]) == 'x') {
            ptr += 2;
            base = 16;
        } else {
            ++ptr;
            first_char_0 = 1;
        }
    }
    dig_start = ptr;
    for (;;) {
        switch (tolower((unsigned char)*ptr)) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            digits_seen |= 1U << (*ptr - '0');
            break;
        case 'a': digits_seen |= 1U << 10; break;
        case 'b':
            if (base == 0 && OK_NUM(BINARY) && !isxdigit(ptr[1])
                && tolower((unsigned char)ptr[1]) != 'h') {
                base = 2; extra = 1; goto done_scan;
            }
            digits_seen |= 1U << 11;
            break;
        case 'c': digits_seen |= 1U << 12; break;
        case 'd':
            if (base == 0 && OK_NUM(DECIMAL) && !isxdigit(ptr[1])
                && tolower((unsigned char)ptr[1]) != 'h') {
                base = 10; extra = 1; goto done_scan;
            }
            digits_seen |= 1U << 13;
            break;
        case 'e': digits_seen |= 1U << 14; break;
        case 'f': digits_seen |= 1U << 15; break;
        case 'y': base = 2; extra = 1; goto done_scan;
        case 'h': base = 16; extra = 1; goto done_scan;
        case 'o': base = 8; extra = 1; goto done_scan;
        case '.': case 'r': return get_float(buf, input, output);
        default: goto done_scan;
        }
        ++ptr;
    }
done_scan:
    if (digits_seen == 0) {
        if (!first_char_0) return get_string(buf, input, output);
        digits_seen |= 1;
        first_char_0 = 0;
        dig_start = *input;
    }
    if (!Options.allow_c_octals) first_char_0 = 0;
    if (base == 0) base = first_char_0 ? 8 : 10;
    len = ptr - *input + extra;
    strncpy(*output, *input, len);
    buf->string_ptr = *output;
    *output += len;
    **output = '\0';
    ++*output;
    *input = ptr + extra;
    switch (base) {
    case 10:
        if (OK_NUM(DECIMAL)) break;
        /* fall through */
    case 8:
        if (OK_NUM(OCTAL)) break;
        /* fall through */
    case 2:
        if (OK_NUM(BINARY)) break;
        AsmError(99); /* INVALID_NUMBER_DIGIT */
        return -1;
    }
    val = 0;
    while (dig_start < ptr) {
        val *= base;
        if (isdigit(*dig_start)) {
            val += *dig_start - '0';
        } else {
            val += tolower((unsigned char)*dig_start) - 'a' + 10;
        }
        ++dig_start;
    }
    buf->token = T_NUM;
    buf->value = val;
    return 1;
#undef OK_NUM
#undef VALID_BINARY
#undef VALID_OCTAL
#undef VALID_DECIMAL
}

/* CF753c..7737, actual old lexical classes and comment tail copy. */
int get_id( unsigned int *buf_index, char **input, char **output )
/***********************************************************************/
/* get_id could change buf_index, if a COMMENT directive is found */
{
    struct asm_tok  *buf;
    char            cur_char;
    int             count;

    buf = AsmBuffer[ *buf_index ];

    buf->string_ptr = *output;
    if( **input != '\\' ) {
        buf->token = T_ID;
    } else {
        buf->token = T_PATH;
    }
    buf->value = 0;

    *(*output)++ = *(*input)++;
    for( ; ; ) {
        cur_char = **input;
        /* if character is part of a valid name, add it */
        if( isalpha( cur_char )
            || isdigit( cur_char )
            || cur_char == '_'
            || cur_char == '$'
            || cur_char == '@'
            || cur_char == '?'  ) {
            *(*output)++ = *(*input)++;
        } else if( cur_char == '\\' ) {
            *(*output)++ = *(*input)++;
            buf->token = T_PATH;
        } else  {
            break;
        }
    }
    *(*output)++ = '\0';

    /* now decide what to do with it */

    if( buf->token == T_PATH ) return( 1 );
    count = get_instruction_position( buf->string_ptr );
    if( count == -2 ) {
        buf->token = T_ID;
        if( buf->string_ptr[1] == '\0' && buf->string_ptr[0] == '?' ) {
            buf->token = T_QUESTION_MARK;
        }
    } else {
        buf->value = AsmOpTable[count].token;
        // count = AsmOpcode[count].position;

        if( AsmOpTable[count].opnd_type[0] == OP_SPECIAL ) {
            if( AsmOpTable[count].rm_byte & OP_REGISTER ) {
                buf->token = T_REG;
            } else if( AsmOpTable[count].rm_byte & OP_RES_ID ) {
                buf->token = T_RES_ID;
            } else if( AsmOpTable[count].rm_byte & OP_UNARY_OPERATOR ) {
                buf->token = T_UNARY_OPERATOR;
            } else if( AsmOpTable[count].rm_byte & OP_DIRECTIVE ) {
                buf->token = T_DIRECTIVE;


                switch( AsmOpTable[count].token ) {
                case T_COMMENT:
                    /* save the whole line .. we need to check
                     * if the delim. char shows up 2 times */
                    (*output)++;
                    *(*output) = '\0';
                    (*output)++;
                    (*buf_index)++;
                    buf = AsmBuffer[ *buf_index ];
                    buf->string_ptr = *output;
                    strcpy( buf->string_ptr, *input );
                    (*output) += strlen( *input );
                    *(*output) = '\0';
                    (*output)++;
                    (*input) += strlen( *input );
                    buf->token = T_STRING;
                    buf->value = 0;
                    break;
                } /* default do nothing */

            } else if( AsmOpTable[count].rm_byte & OP_DIRECT_EXPR ) {
                buf->token = T_DIRECT_EXPR;
            } else {
                buf->token = T_INS;
            }
        } else {
            buf->token = T_INS;
        }
    }
    return( 1 );
}


/* Initial bld/wasm/c/asmscan.c; CF70a0..70fb.  The spelling is
 * copied from the shared mnemonic pool; a leading directive dot is omitted. */
void GetInsString(unsigned short token, char *string, int length)
{
    int index;
    if (length > AsmOpcode[token].len) {
        length = AsmOpcode[token].len;
        index = AsmOpcode[token].index;
        if (AsmChars[index] == '.') {
            ++index;
            --length;
        }
        strncpy(string, &AsmChars[index], length);
        string[length] = '\0';
    } else {
        *string = '\0';
    }
}

/* Initial asmscan.c get_special_symbol; CF784c..78f4. */
int get_special_symbol(struct asm_tok *buf, char **input, char **output)
{
    char symbol;
    buf->string_ptr = *output;
    symbol = **input;
    switch (symbol) {
    case '.': case ',': case '+': case '-': case '*': case '/':
    case '[': case ']': case '(': case ')': case ':': case '%':
        buf->token = symbol;
        *(*output)++ = *(*input)++;
        *(*output)++ = '\0';
        break;
    case '=':
        buf->token = T_DIRECTIVE;
        buf->value = T_EQU2;
        *(*output)++ = *(*input)++;
        *(*output)++ = '\0';
        break;
    default:
        return get_string(buf,input,output);
    }
    return 1;
}

/* Initial asmscan.c get_string; CF7738..7849. MAX_TOK_LEN is80
 * in this version; reaching that limit retains its original no-NUL tail. */
int get_string(struct asm_tok *buf, char **input, char **output)
{
    char symbol;
    int count;
    buf->string_ptr = *output;
    symbol = **input;
    buf->token = T_STRING;
    switch (symbol) {
    case '"': case '\'': break;
    case '<': symbol = '>'; break;
    case '{': symbol = '}'; break;
    default:
        for (count = 0; **input != '\0' && !isspace((unsigned char)**input)
                         && **input != ','; ++count) {
            *(*output)++ = *(*input)++;
        }
        *(*output)++ = '\0';
        buf->value = count;
        return 1;
    }
    ++*input;
    count = 0;
    while (count < 80) {
        if (**input == symbol) {
            if ((*input)[1] == symbol) {
                ++*input;
                *(*output)++ = *(*input)++;
                ++count;
            } else {
                *(*output)++ = '\0';
                ++*input;
                buf->value = count;
                break;
            }
        } else if (**input == '\0' || **input == '\n') {
            AsmError(51);
            return -1;
        } else {
            *(*output)++ = *(*input)++;
            ++count;
        }
    }
    return 1;
}

/* Initial asmscan.c get_inc_path; CF78f4..79a6. The surrounding
 * AsmScan writes the final terminator for an undelimited path. */
int get_inc_path(unsigned int *buf_index, char **input, char **output)
{
    char symbol;
    AsmBuffer[*buf_index]->token = T_PATH;
    AsmBuffer[*buf_index]->value = 0;
    AsmBuffer[*buf_index]->string_ptr = *output;
    while (isspace((unsigned char)**input)) ++*input;
    symbol = **input;
    switch (symbol) {
    case '\'': case '"': case '<': case '{':
        if (get_special_symbol(AsmBuffer[*buf_index],input,output) == -1) return -1;
        return 1;
    default:
        while (**input && !isspace((unsigned char)**input))
            *(*output)++ = *(*input)++;
        return 1;
    }
}

/* CF70fc..71c0: decimal token recognition, not numeric conversion. */
int get_float(struct asm_tok *buf, char **input, char **output)
{
    char got_e = 0;
    char *ptr = *input;
    for (; *ptr != '\0'; ptr++) {
        if (isdigit((unsigned char)*ptr)) continue;
        if (isspace((unsigned char)*ptr)) break;
        switch (tolower((unsigned char)*ptr)) {
        case '.': continue;
        case 'r':
            *ptr = ' ';
            goto done_scanning_float;
        case 'e':
            if (!got_e) {
                got_e = 1;
                switch (ptr[1]) {
                case '+':
                case '-': ptr++;
                default: continue;
                }
            }
        default: return get_string(buf, input, output);
        }
    }
done_scanning_float:
    buf->token = T_FLOAT;
    strncpy(*output, *input, ptr - *input);
    buf->string_ptr = *output;
    *output += ptr - *input;
    **output = '\0';
    (*output)++;
    *input = ptr;
    buf->value = 0;
    return 1;
}

/* CF74dc..753c, quoted identifiers preserve literal punctuation. */
int get_id_in_backquotes( struct asm_tok *buf, char **input, char **output )
/*********************************************************************************/
{
    buf->string_ptr = *output;
    buf->token = T_ID;
    buf->value = 0;

    /* copy char from input to output & inc both */
    (*input)++;             // strip off the backquotes
    for( ; **input != '`'; ) {
        *(*output)++ = *(*input)++;
        if( **input == '\0' || **input == ';' ) {
            AsmError( 51 );
            return( -1 );
        }
    }
    (*input)++;         /* don't output the last '`' */
    *(*output)++ = '\0';
    return( 1 );
}
