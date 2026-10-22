/* Pragma dispatch and state shared with listing, segment and option readers. */
#include <locale.h>
#include "pragma.h"
#include "preprocessor.h"
#include "lexer.h"
#include "parser.h"
#include "keywords.h"
#include "macros.h"
#include "pch.h"
#include "source-input.h"
#include "global-ir.h"
#include "control-ir.h"

PragmaArgument pragma_argument; /* original 0x63920 */
short listing_number; /* original 0x63850 */
char *listing_subtitle, *listing_title; /* original 0x604b4, 0x604b0 */
char *optimization_pragma_text; /* original 0x604ac */
int optimization_pragma_on; /* original 0x60834 */
int native_caller_pragma_seen; /* original 0x6bbcc */
int default_string_pooling; /* original 0x6bc08 */
FrontendSymbol *default_data_segment; /* original 0x6b7a8 */
FrontendSymbol *default_bss_segment; /* original 0x6b7a0 */
FrontendSymbol *default_const_segment; /* original 0x6b7a4 */

/* The original dispatcher compares fixed spellings inline. Keep those
   comparisons local so no new string-comparison call is added to its graph. */
#define PRAGMA_TEXT_IS(text_, spelling_, result_) do { \
    char *left_ = (text_), *right_ = (spelling_); \
    while (*left_ && *left_ == *right_) { ++left_; ++right_; } \
    (result_) = *left_ == *right_; \
} while (0)
#define PRAGMA_BAD_TOKEN(code_) do { \
    compiler_warning(1, (code_), token_names[current_token_code]); \
    skip_source_line(); return; \
} while (0)
#define PRAGMA_BAD_FORM() do { \
    compiler_warning(1, 612); skip_source_line(); return; \
} while (0)

/* Original 0x1ebf0..0x1fd14; pragma dispatch table 0x1fd14..0x1fda0. */
void process_pragma(void)
{
    unsigned char token;
    unsigned length, flags;
    int kind, match, allowed, on, level, number, seen_number;
    int done, valid, bad_level, bad_number;
    int segment_kind;
    char *text, *name, *class_name;
    FrontendSymbol **segment_target;
    TokenValue comment_text, optimization_text, segment_text, segment_class;
    PragmaOptimizationRule *rule;

    if (options.option_E) {
        fwrite("#pragma", 7, 1, preprocessor_stream);
        scanner_flags |= 0x10000;
        for (;;) {
            token = read_parser_token();
            if (!token) { fatal_error(4); continue; }
            if (token == TOK_NEWLINE) break;
            if (token == TOK_PP_OUTPUT) continue;
            text = token_names[current_token_code];
            for (length = 0; text[length]; ++length) ;
            fwrite(text, length, 1, stdout);
        }
        scanner_flags &= ~0x10000UL;
        return;
    }
    if (options.prototypes_only) { skip_source_line(); return; }
    read_pragma_token();
    if (current_token_code == TOK_NEWLINE) return;
    if (current_token_code != TOK_IDENTIFIER) PRAGMA_BAD_TOKEN(84);
    kind = classify_pragma(current_token_value.identifier);
    switch (kind) {
    case 0: /* check_stack */
        read_pragma_token();
        switch (current_token_code) {
        case TOK_NEWLINE: normalized_Gs = options.option_Gs; break;
        case TOK_MINUS: normalized_Gs = 0; break;
        case TOK_PLUS: normalized_Gs = 1; break;
        case TOK_LEFT_PAREN:
            normalized_Gs = read_pragma_argument(200) ? pragma_argument.integer : options.option_Gs;
            break;
        default: compiler_warning(1, 77); break;
        }
        break;
    case 1: /* loop_opt */
        if (parameter_nesting) {
            read_parenthesized_pragma_argument(200);
            report_syntax_error(156);
        } else {
            normalized_Ol = read_parenthesized_pragma_argument(200) ?
                pragma_argument.integer : options.option_Ol;
            if (options.option_qc) normalized_Ol &= options.option_Ol;
        }
        break;
    case 2: process_pack_pragma(); break;
    case 3: /* intrinsic */
        if (options.option_qc) {
            compiler_warning(1, 118);
            skip_source_line();
            return;
        }
        if (!pragma_open_parenthesis()) return;
        visit_pragma_names(enable_intrinsic_function);
        break;
    case 4: /* function */
        if (!pragma_open_parenthesis()) return;
        visit_pragma_names(disable_intrinsic_function);
        break;
    case 5: /* alloc_text */
        if (read_pragma_token() != TOK_LEFT_PAREN || read_pragma_token() != TOK_IDENTIFIER) {
            warn_expected_identifier(); skip_source_line(); return;
        }
        record_context_symbol = declare_named_entity(current_token_value.identifier, name_unnamed, 0, 2);
        read_pragma_token();
        if (current_token_code != TOK_COMMA) PRAGMA_BAD_TOKEN(81);
        scanner_flags |= 0x20000;
        visit_pragma_names(assign_allocated_text);
        break;
    case 6: /* same_seg */
        compiler_warning(1, 143); skip_source_line(); return;
    case 7: /* check_pointer */
        normalized_Zr = read_parenthesized_pragma_argument(200) ? pragma_argument.integer : options.z_r;
        record_line_information(9);
        break;
    case 8: /* switch_check */
        optimization_enabled = read_parenthesized_pragma_argument(200) ?
            pragma_argument.integer : !optimization_enabled;
        if (parameter_nesting) record_line_information(10);
        break;
    case 9: /* plmn */
        normalized_PLMN = read_parenthesized_pragma_argument(200) ?
            pragma_argument.integer : !normalized_PLMN;
        active_plmn_pragma = normalized_PLMN;
        plmn_pragma_seen = 1;
        if (parameter_nesting) record_line_information(20);
        break;
    case 10: /* plmf */
        normalized_PLMF = read_parenthesized_pragma_argument(200) ?
            pragma_argument.integer : !normalized_PLMF;
        active_plmf_pragma = normalized_PLMF;
        plmf_pragma_seen = 1;
        if (parameter_nesting) record_line_information(21);
        break;
    case 11: case 12: /* page / skip */
        if (read_parenthesized_pragma_argument(TOK_INT_CONSTANT)) {
            listing_number = (short)pragma_argument.integer;
            if (listing_number < 1 || listing_number > 127) {
                compiler_warning(1, 106); listing_number = 1;
            }
        } else listing_number = 1;
        emit_listing_directive((unsigned char)(kind == 11 ? 3 : 12));
        break;
    case 13: /* subtitle */
        if (read_parenthesized_pragma_argument(TOK_STRING)) {
            listing_subtitle = pragma_argument.text;
            emit_listing_directive(8);
        }
        break;
    case 14: /* title */
        if (read_parenthesized_pragma_argument(TOK_STRING)) {
            listing_title = pragma_argument.text;
            emit_listing_directive(9);
        }
        break;
    case 15: /* pagesize */
        if (read_parenthesized_pragma_argument(TOK_INT_CONSTANT)) {
            listing_number = (short)pragma_argument.integer;
            if (listing_number < 15 || listing_number > 255) {
                compiler_warning(1, 107); listing_number = 63;
            }
        } else listing_number = 63;
        emit_listing_directive(7);
        break;
    case 16: /* linesize */
        if (read_parenthesized_pragma_argument(TOK_INT_CONSTANT)) {
            listing_number = (short)pragma_argument.integer;
            if (listing_number < 79 || listing_number > 132) {
                compiler_warning(1, 108); listing_number = 79;
            }
        } else listing_number = 79;
        emit_listing_directive(1);
        break;
    case 18: /* message */
        if (read_parenthesized_pragma_argument(TOK_STRING)) pragma_message(pragma_argument.text);
        pragma_message("\\n");
        break;
    case 19: /* comment */
        if (!pragma_open_parenthesis()) return;
        if (read_pragma_token() != TOK_IDENTIFIER) {
            warn_ignored_token(); skip_source_line(); return;
        }
        text = current_token_value.identifier;
        pending_pragma_text = 0;
        PRAGMA_TEXT_IS(text, "compiler", match);
        if (match) { pending_pragma_kind = 1; pending_pragma_text = (unsigned char *)""; }
        else {
            PRAGMA_TEXT_IS(text, "lib", match);
            if (match) pending_pragma_kind = 4;
            else {
                PRAGMA_TEXT_IS(text, "exestr", match);
                if (match) pending_pragma_kind = 3;
                else {
                    PRAGMA_TEXT_IS(text, "user", match);
                    if (!match) { compiler_warning(1, 109, text); skip_source_line(); return; }
                    pending_pragma_kind = 6;
                }
            }
        }
        read_pragma_token();
        if (!pending_pragma_text && current_token_code == TOK_COMMA && read_pragma_token() == TOK_STRING) {
            comment_text = current_token_value;
            while (read_pragma_token() == TOK_STRING)
                concatenate_string_literals(&comment_text, &current_token_value);
            text_work_cursor = (unsigned char *)comment_text.identifier;
            pending_pragma_text = text_work_cursor;
        }
        if (current_token_code != TOK_RIGHT_PAREN) {
            warn_ignored_token(); skip_source_line(); return;
        }
        if (pending_pragma_text) record_line_information(11);
        break;
    case 20: /* segment */
        if (read_pragma_token() != TOK_LEFT_PAREN || read_pragma_token() != TOK_IDENTIFIER) {
            warn_expected_identifier(); skip_source_line(); return;
        }
        text = current_token_value.identifier;
        PRAGMA_TEXT_IS(text, "_TEXT", match);
        if (match) { segment_target = &default_code_segment; segment_kind = 0; }
        else {
            PRAGMA_TEXT_IS(text, "_DATA", match);
            if (match) { segment_target = &default_data_segment; segment_kind = 1; }
            else {
                PRAGMA_TEXT_IS(text, "_BSS", match);
                if (match) { segment_target = &default_bss_segment; segment_kind = 3; }
                else {
                    PRAGMA_TEXT_IS(text, "CONST", match);
                    if (!match) { compiler_warning(1, 109, text); skip_source_line(); return; }
                    segment_target = &default_const_segment; segment_kind = 4;
                }
            }
        }
        token = read_pragma_token();
        if (token != TOK_COMMA) {
            if (current_token_code == TOK_RIGHT_PAREN) { *segment_target = 0; break; }
            warn_ignored_token(); skip_source_line(); return;
        }
        if (read_pragma_token() != TOK_IDENTIFIER) {
            warn_expected_identifier(); skip_source_line(); return;
        }
        *segment_target = declare_named_entity(current_token_value.identifier, name_unnamed, segment_kind, 2);
        read_pragma_token();
        if (current_token_code != TOK_RIGHT_PAREN) PRAGMA_BAD_TOKEN(95);
        break;
    case 21: /* optimize */
        allowed = 1;
        if (parameter_nesting) { report_syntax_error(156); allowed = 0; }
        if (read_pragma_token() != TOK_LEFT_PAREN) break;
        if (read_pragma_token() != TOK_STRING) {
            /* The original diagnostic path restores an unspecified local. */
            warn_ignored_token(); skip_source_line();
            text_work_cursor = (unsigned char *)optimization_text.identifier;
            return;
        }
        optimization_text = current_token_value;
        while (read_pragma_token() == TOK_STRING)
            concatenate_string_literals(&optimization_text, &current_token_value);
        pragma_argument.text = optimization_text.identifier;
        optimization_pragma_text = optimization_text.identifier;
        if (current_token_code != TOK_COMMA) {
            compiler_warning(1, 81, token_names[current_token_code]); skip_source_line();
            text_work_cursor = (unsigned char *)optimization_text.identifier;
            return;
        }
        if (!read_pragma_argument(200)) {
            text_work_cursor = (unsigned char *)optimization_pragma_text;
            text_work_cursor = (unsigned char *)optimization_text.identifier;
            return;
        }
        optimization_pragma_on = pragma_argument.integer;
        if (allowed) {
            on = optimization_pragma_on;
            if (!*optimization_pragma_text) {
                flags = pack_option_flags();
                if (on) flags = (flags & ~pragma_optimization_restore_mask) |
                                (initial_option_flags & pragma_optimization_restore_mask);
                else flags &= ~pragma_optimization_disable_mask;
                unpack_option_flags(flags);
            } else {
                while (*optimization_pragma_text) {
                    for (rule = pragma_optimization_rules; rule->letter; ++rule)
                        if (*optimization_pragma_text == rule->letter)
                            *rule->target = on ? rule->enabled : !rule->enabled;
                    ++optimization_pragma_text;
                }
            }
            if (options.option_qc) normalized_Ol &= options.option_Ol;
        }
        text_work_cursor = (unsigned char *)optimization_text.identifier;
        break;
    case 22: case 24: /* data_seg / code_seg */
        if (kind == 22) { segment_target = &default_data_segment; segment_kind = 1; }
        else { segment_target = &default_code_segment; segment_kind = 0; }
        if (read_pragma_token() != TOK_LEFT_PAREN) PRAGMA_BAD_FORM();
        read_pragma_token();
        if (current_token_code != TOK_STRING) {
            if (current_token_code == TOK_RIGHT_PAREN) { *segment_target = 0; break; }
            PRAGMA_BAD_FORM();
        }
        segment_text = current_token_value;
        while (read_pragma_token() == TOK_STRING)
            concatenate_string_literals(&segment_text, &current_token_value);
        name = intern_identifier(segment_text.identifier);
        text_work_cursor = (unsigned char *)segment_text.identifier;
        if (current_token_code == TOK_COMMA) {
            read_pragma_token();
            if (current_token_code != TOK_STRING) PRAGMA_BAD_FORM();
            segment_class = current_token_value;
            while (read_pragma_token() == TOK_STRING)
                concatenate_string_literals(&segment_class, &current_token_value);
            class_name = intern_identifier(segment_class.identifier);
            text_work_cursor = (unsigned char *)segment_class.identifier;
        } else {
            if (current_token_code != TOK_RIGHT_PAREN) PRAGMA_BAD_FORM();
            class_name = name_unnamed;
        }
        *segment_target = declare_named_entity(name, class_name, segment_kind, 2);
        if (current_token_code != TOK_RIGHT_PAREN) PRAGMA_BAD_TOKEN(95);
        break;
    case 23: /* native_caller */
        read_pragma_token();
        switch (current_token_code) {
        case TOK_NEWLINE: case TOK_PLUS: normalized_Gn = 1; break;
        case TOK_MINUS: normalized_Gn = 0; break;
        case TOK_LEFT_PAREN:
            normalized_Gn = read_pragma_argument(200) ? pragma_argument.integer : options.option_Gn;
            native_caller_pragma_seen = 1;
            break;
        default: compiler_warning(1, 77); break;
        }
        if (parameter_nesting) report_syntax_error(156);
        break;
    case 25: /* inline_depth: the original assigns only the low word. */
        number = read_parenthesized_pragma_argument(TOK_INT_CONSTANT) ? pragma_argument.integer : 8;
        control_alignment = (control_alignment & 0xffff0000UL) | (unsigned short)number;
        if (options.option_Ob > 0) record_line_information(32);
        break;
    case 26: /* inline_recursion */
        number = read_parenthesized_pragma_argument(200) ? pragma_argument.integer : 0;
        control_alignment_mode = (control_alignment_mode & 0xffff0000UL) | (unsigned short)number;
        if (options.option_Ob > 0) record_line_information(32);
        break;
    case 27: /* auto_inline */
        if (parameter_nesting) {
            read_parenthesized_pragma_argument(200); report_syntax_error(156);
        } else {
            default_inline_candidate = read_parenthesized_pragma_argument(200) ? pragma_argument.integer : 1;
            inline_candidate = default_inline_candidate;
        }
        break;
    case 28: /* warning */
        if (read_pragma_token() != TOK_LEFT_PAREN) { warn_bad_pragma(); break; }
        done = valid = bad_level = bad_number = 0;
        token = read_pragma_token();
        while (!done) {
            switch (token) {
            case TOK_IDENTIFIER:
                text = current_token_value.identifier;
                PRAGMA_TEXT_IS(text, "disable", match);
                if (match) level = 13;
                else {
                    PRAGMA_TEXT_IS(text, "error", match);
                    if (match) level = 14;
                    else { PRAGMA_TEXT_IS(text, "once", match); level = match ? 5 : 15; }
                }
                break;
            case TOK_CONSTANT:
                number = current_token_value.literal->value.integer;
                level = current_token_code == TOK_INT_CONSTANT && number >= 1 && number <= 4 ? number : 15;
                break;
            case TOK_DEFAULT: level = 0; break;
            case TOK_RIGHT_PAREN: done = 1; goto warning_group_done;
            default: token = TOK_NEWLINE; done = 1; goto warning_group_done;
            }
            if (read_pragma_token() != TOK_COLON) {
                token = TOK_NEWLINE; done = 1; goto warning_group_done;
            }
            seen_number = 0;
            while ((token = read_pragma_token()) == TOK_CONSTANT) {
                number = current_token_code == TOK_INT_CONSTANT ? (int)current_token_value.literal->value.integer : -1;
                if (level == 15) {
                    if (!bad_level) { bad_level = 1; compiler_warning(1, 615); }
                } else if (number == -1) {
                    if (!bad_number) { bad_number = 1; compiler_warning(1, 617); }
                } else set_warning_level(number, level);
                seen_number = 1;
            }
            if (!seen_number) { token = TOK_NEWLINE; done = 1; goto warning_group_done; }
            if (token == TOK_RIGHT_PAREN) done = 1;
            else if (token == TOK_SEMICOLON) token = read_pragma_token();
            else token = TOK_NEWLINE;
            if (token == TOK_NEWLINE) { done = 1; goto warning_group_done; }
            valid = 1;
warning_group_done: ;
        }
        if (token == TOK_NEWLINE || !valid) warn_bad_pragma();
        break;
    case 29: /* hdrstop */
        if (parameter_nesting) report_syntax_error(156);
        if (!include_depth && pch_mode) finish_pch_capture();
        if ((unsigned char)options.pch_create && !pch_output_written && include_depth <= 0) {
            browser_include_begin();
            synchronize_pch_pragma();
            prepare_pch_output(options.source_file);
            finish_pch_output();
            browser_include_begin();
        } else skip_source_line();
        break;
    case 30: /* init_seg */
        compiler_warning(1, 157); skip_source_line(); return;
    case 32: /* setlocale */
        if (read_parenthesized_pragma_argument(TOK_STRING) && !setlocale(0, pragma_argument.text))
            report_syntax_error(175, pragma_argument.text);
        break;
    case 34: /* string_pooling */
        if (parameter_nesting) {
            read_parenthesized_pragma_argument(200); report_syntax_error(156);
        } else options.option_Gf = read_parenthesized_pragma_argument(200) ?
            pragma_argument.integer : default_string_pooling;
        break;
    default:
        compiler_warning(1, 68); skip_source_line(); return;
    }
    finish_directive("pragma");
}

/* Original 0x1e550..0x1e56a. EOF is fatal inside a pragma directive. */
unsigned char read_pragma_token(void)
{
    unsigned char token = read_parser_token();
    if (!token) fatal_error(4);
    return token;
}

typedef struct PragmaName {
    char *name;
    int kind;
} PragmaName;

/* Spellings and dispatcher IDs, grouped by the initial character as in the
   original lookup. These are pragma names rather than a copied data image. */
static PragmaName pragma_names[] = {
    { "auto_inline", 27 }, { "alloc_text", 5 },
    { "comment", 19 }, { "code_seg", 24 },
    { "check_stack", 0 }, { "check_pointer", 7 },
    { "data_seg", 22 }, { "function", 4 }, { "hdrstop", 29 },
    { "intrinsic", 3 }, { "inline_recursion", 26 },
    { "inline_depth", 25 }, { "init_seg", 30 },
    { "loop_opt", 1 }, { "linesize", 16 }, { "message", 18 },
    { "native_caller", 23 }, { "optimize", 21 },
    { "pointers_to_members", 33 }, { "plmn", 9 }, { "plmf", 10 },
    { "pagesize", 15 }, { "page", 11 }, { "pack", 2 },
    { "switch_check", 8 }, { "subtitle", 13 }, { "string_pooling", 34 },
    { "skip", 12 }, { "setlocale", 32 }, { "segment", 20 },
    { "search_lib", 17 }, { "same_seg", 6 }, { "title", 14 },
    { "vtordisp", 31 }, { "warning", 28 }
};

/* Original 0x1c4b0..0x1c545. Compare candidates within the initial-letter
   group. The original uses a compact directory to locate the same group. */
int __stdcall classify_pragma(char *name)
{
    PragmaName *entry;
    char *left, *right;
    if ((signed char)*name < '_') return -1;
    for (entry = pragma_names; entry < pragma_names + 35; ++entry) {
        if (*entry->name != *name) continue;
        left = name + 1;
        right = entry->name + 1;
        while (*left && *left == *right) { ++left; ++right; }
        if (*left == *right) return entry->kind;
    }
    return -1;
}

/* Original 0x20090..0x2010d. Accept either internal or displayed C4xxx
   warning numbers, and retain the range of explicit 700-series controls. */
void __stdcall set_warning_level(int number, int level)
{
    unsigned warning = (unsigned)number;
    if (warning > 4000) warning -= 4000;
    if (warning < 1 || warning > 799) {
        compiler_warning(1, 616, number, 4001, 4799);
        return;
    }
    warning_levels[warning] = (unsigned char)level;
    if (warning >= 700 && warning < 800) {
        if (lowest_warning_700 >= warning) lowest_warning_700 = warning;
        if (highest_warning_700 <= warning) highest_warning_700 = warning;
    }
}

/* Original 0x1e570..0x1e5a6. Reject a malformed pragma argument list
   before its directive-specific reader consumes any arguments. */
int pragma_open_parenthesis(void)
{
    if (read_pragma_token() == TOK_LEFT_PAREN) return 1;
    compiler_warning(1, 83, token_names[current_token_code]);
    skip_source_line();
    return 0;
}

/* Original 0x1e5d0..0x1e647. Apply a pragma callback to a comma-separated
   identifier list. Local-scope use is diagnosed and consumes the list only. */
void __stdcall visit_pragma_names(void (__stdcall *callback)(char *))
{
    unsigned char token;
    if (parameter_nesting) {
        callback = 0;
        report_syntax_error(156);
    }
    for (;;) {
        if (read_pragma_token() != TOK_IDENTIFIER) {
            compiler_warning(1, 82, token_names[current_token_code]);
            return;
        }
        if (callback) callback(current_token_value.identifier);
        token = read_pragma_token();
        if (token == TOK_COMMA) continue;
        if (token != TOK_RIGHT_PAREN) warn_ignored_token();
        return;
    }
}

/* Original 0x1e9e0..0x1ebea. Read pack(n), pack(), and the push/pop
   forms with an optional interned label and replacement alignment. */
void process_pack_pragma(void)
{
    int action = 0, alignment = 0, match;
    char *label = 0;
    PackingEntry *entry;
    if (!pragma_open_parenthesis()) return;
    read_pragma_token();
    switch (current_token_code) {
    case TOK_IDENTIFIER:
        PRAGMA_TEXT_IS(current_token_value.identifier, "push", match);
        if (match) action = 1;
        else {
            PRAGMA_TEXT_IS(current_token_value.identifier, "pop", match);
            if (!match) goto malformed;
            action = 2;
        }
        read_pragma_token();
        if (current_token_code == TOK_RIGHT_PAREN) goto apply;
        if (current_token_code != TOK_COMMA) goto malformed;
        read_pragma_token();
        if (current_token_code == TOK_IDENTIFIER) {
            label = current_token_value.identifier;
            read_pragma_token();
            if (current_token_code == TOK_RIGHT_PAREN) goto apply;
            if (current_token_code != TOK_COMMA) goto malformed;
            if (read_pragma_token() != TOK_INT_CONSTANT) goto malformed;
            goto number;
        }
        if (current_token_code == TOK_INT_CONSTANT) goto number;
        /* The original applies the action after other consumed tokens here. */
        goto apply;
    case TOK_RIGHT_PAREN:
        alignment = options.option_Zp == -1 ? 8 : options.option_Zp;
        goto apply;
    case TOK_INT_CONSTANT:
number:
        alignment = validate_packing(current_token_value.literal->value.integer);
        if (read_pragma_token() != TOK_RIGHT_PAREN) goto malformed;
        goto apply;
    default:
        goto malformed;
    }
apply:
    if (!action) {
        entry = top_packing(packing_stack);
        entry->alignment = validate_packing(alignment);
    } else if (action == 1) push_pragma_packing(alignment, label);
    else pop_pragma_packing(alignment, label);
    return;
malformed:
    compiler_warning(1, 110, token_names[current_token_code]);
}
