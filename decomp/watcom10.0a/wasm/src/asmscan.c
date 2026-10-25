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
