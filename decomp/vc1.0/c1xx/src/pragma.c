/* Pragma dispatcher. Common C-front-end branches follow the existing C1
   reconstruction; every C1XX route/call was checked against0x21c60..0x2312d,
   including its C++ init-segment/member-pointer and paged-output differences. */
#include "pragma.h"
#include "operators.h"
#include <locale.h>
#include <recon.h>

PragmaArgument pragma_argument; /* original0x95764 */
short listing_number; /* original0x94afc */
char *listing_title, *listing_subtitle; /* original0x94af0/0x94af4 */
char *optimization_pragma_text; /* original0x94ae8 */
int optimization_pragma_on; /* original0x94ae4 */
SegmentSymbol *default_bss_segment; /* original0x9f3e0 */
CppSymbol *initializer_segment_symbol; /* original0x9f3c4 */
char *initializer_segment_name; /* original0x9f350 */
extern SegmentSymbol *default_data_segment, *default_constant_segment;
extern int active_plmn_pragma, active_plmf_pragma;

/* Encountered option-letter rules at0x9e558..0x9e5e7 (18 eight-byte rows).
   Each target is an existing logical option object, not duplicated storage. */
PragmaOptimizationRule pragma_optimization_rules[] = {
    { 'a', 1, &normalized_Oa }, { 'w', 1, &normalized_Ow },
    { 't', 1, &options.option_Os }, { 's', 0, &options.option_Os },
    { 'c', 1, &options.option_Oc }, { 'g', 1, &options.option_Og },
    { 'e', 1, &options.option_Oe }, { 'l', 1, &normalized_Ol },
    { 'p', 1, &normalized_Op }, { 'z', 1, &normalized_Oz },
    { 'n', 0, &normalized_Oz }, { 'r', 1, &options.option_Or },
    { 'o', 1, &options.option_Oo }, { 'y', 1, &normalized_Oy },
    { 'q', 1, &normalized_Oq }, { 'f', 1, &options.option_Of },
    { 'v', 1, &options.option_Ov }, { 0, 0, 0 }
};
unsigned pragma_optimization_restore_mask = 0x03ffb800; /* original0xa34b4 */
unsigned pragma_optimization_disable_mask = 0x037eb800; /* original0xa34b8 */

/* Original spelling comparisons are inline, preserving the direct call graph. */
#define PRAGMA_TEXT_IS(text_, spelling_, result_) do { \
    char *left_ = (text_), *right_ = (spelling_); \
    while (*left_ && *left_ == *right_) { ++left_; ++right_; } \
    (result_) = *left_ == *right_; \
} while (0)
#define PRAGMA_BAD_TOKEN(code_) do { \
    compiler_warning(1, (code_), token_spellings[current_token_code]); \
    skip_source_line(); return; \
} while (0)
#define PRAGMA_BAD_FORM() do { \
    compiler_warning(1, 612); skip_source_line(); return; \
} while (0)

/* Whole5326B body, plus35-entry text dispatch0x23130..0x231bb. */
void process_pragma(void)
{
    unsigned char token;
    unsigned length, flags;
    int kind, match, allowed, on, level, number, seen_number;
    int done, valid, bad_level, bad_number;
    int segment_kind;
    char *text, *name, *class_name;
    SegmentSymbol **segment_target;
    ParserValue comment_text, optimization_text, segment_text, segment_class;
    PragmaOptimizationRule *rule;

    if (options.option_E) {
        if (token_allocation_flags & 0x200) write_work_bytes("#pragma", 7, 1, compiler_stack);
        else fwrite("#pragma", 7, 1, preprocessor_stream);
        token_allocation_flags |= 0x10000;
        for (;;) {
            token = read_parser_token();
            if (!token) { fatal_error(4); continue; }
            if (token == 1) break;
            if (token == 0x93) continue;
            text = token_spellings[current_token_code];
            for (length = 0; text[length]; ++length) ;
            if (token_allocation_flags & 0x200) write_work_bytes(text, length, 1, compiler_stack);
            else fwrite(text, length, 1, stdout);
        }
        token_allocation_flags &= ~0x10000UL;
        if (token_allocation_flags & 0x200) {
            if (preprocessor_stream) fputc('\n', preprocessor_stream);
            else write_work_character('\n', compiler_stack);
        }
        return;
    }
    if (options.prototypes_only) { skip_source_line(); return; }
    read_pragma_token();
    if (current_token_code == 1) return;
    if (current_token_code != 2) PRAGMA_BAD_TOKEN(84);
    kind = classify_pragma(current_token_value.name);
    switch (kind) {
    case 0: /* check_stack */
        read_pragma_token();
        switch (current_token_code) {
        case 1: normalized_Gs = options.configuration.option_Gs; break;
        case OP_SUBTRACT: normalized_Gs = 0; break;
        case OP_ADD: normalized_Gs = 1; break;
        case 0x80:
            normalized_Gs = read_pragma_argument(200) ? pragma_argument.integer : options.configuration.option_Gs;
            break;
        default: compiler_warning(1, 77); break;
        }
        break;
    case 1: /* loop_opt */
        if (declaration_mode) {
            read_parenthesized_pragma_argument(200);
            report_syntax_error(156);
        } else {
            normalized_Ol = read_parenthesized_pragma_argument(200) ?
                pragma_argument.integer : options.configuration.option_Ol;
            if (options.configuration.option_qc) normalized_Ol &= options.configuration.option_Ol;
        }
        break;
    case 2: process_pack_pragma(); break;
    case 3: /* intrinsic */
        if (options.configuration.option_qc) {
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
        if (read_pragma_token() != 0x80 || read_pragma_token() != 2) {
            warn_expected_identifier(); skip_source_line(); return;
        }
        record_context_symbol = (CppSymbol *)lookup_segment_symbol(current_token_value.name, name_unnamed, 0, 2);
        read_pragma_token();
        if (current_token_code != 0x40) PRAGMA_BAD_TOKEN(81);
        token_allocation_flags |= 0x20000;
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
        if (declaration_mode) record_line_information(10);
        break;
    case 9: /* plmn */
        normalized_PLMN = read_parenthesized_pragma_argument(200) ?
            pragma_argument.integer : !normalized_PLMN;
        active_plmn_pragma = normalized_PLMN;
        options.configuration.far_call_mode = 1;
        if (declaration_mode) record_line_information(20);
        break;
    case 10: /* plmf */
        normalized_PLMF = read_parenthesized_pragma_argument(200) ?
            pragma_argument.integer : !normalized_PLMF;
        active_plmf_pragma = normalized_PLMF;
        options.configuration.prologue_mode = 1;
        if (declaration_mode) record_line_information(21);
        break;
    case 11: case 12: /* page / skip */
        if (read_parenthesized_pragma_argument(7)) {
            listing_number = (short)pragma_argument.integer;
            if (listing_number < 1 || listing_number > 127) {
                compiler_warning(1, 106); listing_number = 1;
            }
        } else listing_number = 1;
        emit_listing_directive((unsigned char)(kind == 11 ? 3 : 12));
        break;
    case 13: /* subtitle */
        if (read_parenthesized_pragma_argument(3)) {
            listing_subtitle = pragma_argument.text;
            emit_listing_directive(8);
        }
        break;
    case 14: /* title */
        if (read_parenthesized_pragma_argument(3)) {
            listing_title = pragma_argument.text;
            emit_listing_directive(9);
        }
        break;
    case 15: /* pagesize */
        if (read_parenthesized_pragma_argument(7)) {
            listing_number = (short)pragma_argument.integer;
            if (listing_number < 15 || listing_number > 255) {
                compiler_warning(1, 107); listing_number = 63;
            }
        } else listing_number = 63;
        emit_listing_directive(7);
        break;
    case 16: /* linesize */
        if (read_parenthesized_pragma_argument(7)) {
            listing_number = (short)pragma_argument.integer;
            if (listing_number < 79 || listing_number > 132) {
                compiler_warning(1, 108); listing_number = 79;
            }
        } else listing_number = 79;
        emit_listing_directive(1);
        break;
    case 18: /* message */
        if (read_parenthesized_pragma_argument(3)) pragma_message(pragma_argument.text);
        pragma_message("\n");
        break;
    case 19: /* comment */
        if (!pragma_open_parenthesis()) return;
        if (read_pragma_token() != 2) {
            warn_ident_requires_string(); skip_source_line(); return;
        }
        text = current_token_value.name;
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
        if (!pending_pragma_text && current_token_code == 0x40 && read_pragma_token() == 3) {
            comment_text = current_token_value;
            while (read_pragma_token() == 3)
                concatenate_strings(&comment_text, &current_token_value);
            text_work_cursor = (unsigned char *)comment_text.name;
            pending_pragma_text = text_work_cursor;
        }
        if (current_token_code != 0x88) {
            warn_ident_requires_string(); skip_source_line(); return;
        }
        if (pending_pragma_text) record_line_information(11);
        break;
    case 20: /* segment */
        if (read_pragma_token() != 0x80 || read_pragma_token() != 2) {
            warn_expected_identifier(); skip_source_line(); return;
        }
        text = current_token_value.name;
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
                    segment_target = &default_constant_segment; segment_kind = 4;
                }
            }
        }
        token = read_pragma_token();
        if (token != 0x40) {
            if (current_token_code == 0x88) { *segment_target = 0; break; }
            warn_ident_requires_string(); skip_source_line(); return;
        }
        if (read_pragma_token() != 2) {
            warn_expected_identifier(); skip_source_line(); return;
        }
        *segment_target = lookup_segment_symbol(current_token_value.name, name_unnamed, segment_kind, 2);
        read_pragma_token();
        if (current_token_code != 0x88) PRAGMA_BAD_TOKEN(95);
        break;
    case 21: /* optimize */
        allowed = 1;
        if (declaration_mode) { report_syntax_error(156); allowed = 0; }
        if (read_pragma_token() != 0x80) break;
        if (read_pragma_token() != 3) {
            /* The original diagnostic path restores an unspecified local. */
            warn_ident_requires_string(); skip_source_line();
#ifdef RECON_NATIVE_TEST
            /* Original227bd reads entryESP-0x28; only225f9 writes it,
               after a successful string token. Preserve the Win32 bug. */
            __asm { int 3 }
#endif
            text_work_cursor = (unsigned char *)optimization_text.name;
            return;
        }
        optimization_text = current_token_value;
        while (read_pragma_token() == 3)
            concatenate_strings(&optimization_text, &current_token_value);
        pragma_argument.text = optimization_text.name;
        optimization_pragma_text = optimization_text.name;
        if (current_token_code != 0x40) {
            compiler_warning(1, 81, token_spellings[current_token_code]); skip_source_line();
            text_work_cursor = (unsigned char *)optimization_text.name;
            return;
        }
        if (!read_pragma_argument(200)) {
            text_work_cursor = (unsigned char *)optimization_pragma_text;
            text_work_cursor = (unsigned char *)optimization_text.name;
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
            if (options.configuration.option_qc) normalized_Ol &= options.configuration.option_Ol;
        }
        text_work_cursor = (unsigned char *)optimization_text.name;
        break;
    case 22: case 24: /* data_seg / code_seg */
        if (kind == 22) { segment_target = &default_data_segment; segment_kind = 1; }
        else { segment_target = &default_code_segment; segment_kind = 0; }
        if (read_pragma_token() != 0x80) PRAGMA_BAD_FORM();
        read_pragma_token();
        if (current_token_code != 3) {
            if (current_token_code == 0x88) { *segment_target = 0; break; }
            PRAGMA_BAD_FORM();
        }
        segment_text = current_token_value;
        while (read_pragma_token() == 3)
            concatenate_strings(&segment_text, &current_token_value);
        name = intern_identifier(segment_text.name);
        text_work_cursor = (unsigned char *)segment_text.name;
        if (current_token_code == 0x40) {
            read_pragma_token();
            if (current_token_code != 3) PRAGMA_BAD_FORM();
            segment_class = current_token_value;
            while (read_pragma_token() == 3)
                concatenate_strings(&segment_class, &current_token_value);
            class_name = intern_identifier(segment_class.name);
            text_work_cursor = (unsigned char *)segment_class.name;
        } else {
            if (current_token_code != 0x88) PRAGMA_BAD_FORM();
            class_name = name_unnamed;
        }
        *segment_target = lookup_segment_symbol(name, class_name, segment_kind, 2);
        if (!segment_kind && (token_allocation_flags & 0x40)) {
            flush_current_function(1);
            begin_initializer_function(0);
        }
        if (current_token_code != 0x88) PRAGMA_BAD_TOKEN(95);
        break;
    case 23: /* native_caller */
        read_pragma_token();
        switch (current_token_code) {
        case 1: case OP_ADD: normalized_Gn = 1; break;
        case OP_SUBTRACT: normalized_Gn = 0; break;
        case 0x80:
            normalized_Gn = read_pragma_argument(200) ? pragma_argument.integer : options.configuration.option_Gn;
            options.configuration.field7c = 1;
            break;
        default: compiler_warning(1, 77); break;
        }
        if (declaration_mode) report_syntax_error(156);
        break;
    case 25: /* inline_depth: the original assigns only the low word. */
        number = read_parenthesized_pragma_argument(7) ? pragma_argument.integer : 8;
        control_alignment = (unsigned short)number;
        if (options.configuration.option_Ob > 0) record_line_information(32);
        break;
    case 26: /* inline_recursion */
        number = read_parenthesized_pragma_argument(200) ? pragma_argument.integer : 0;
        control_alignment_mode = (unsigned short)number;
        if (options.configuration.option_Ob > 0) record_line_information(32);
        break;
    case 27: /* auto_inline */
        if (declaration_mode) {
            read_parenthesized_pragma_argument(200); report_syntax_error(156);
        } else {
            default_inline_candidate = read_parenthesized_pragma_argument(200) ? pragma_argument.integer : 1;
            inline_candidate = default_inline_candidate;
        }
        break;
    case 28: /* warning */
        if (read_pragma_token() != 0x80) { warn_bad_pragma(); break; }
        done = valid = bad_level = bad_number = 0;
        token = read_pragma_token();
        while (!done) {
            switch (token) {
            case 2:
                text = current_token_value.name;
                PRAGMA_TEXT_IS(text, "disable", match);
                if (match) level = 13;
                else {
                    PRAGMA_TEXT_IS(text, "error", match);
                    if (match) level = 14;
                    else { PRAGMA_TEXT_IS(text, "once", match); level = match ? 5 : 15; }
                }
                break;
            case 0x6f:
                number = current_token_value.expression->value.integer;
                level = current_token_code == 7 && number >= 1 && number <= 4 ? number : 15;
                break;
            case TOK_DEFAULT: level = 0; break;
            case 0x88: done = 1; goto warning_group_done;
            default: token = 1; done = 1; goto warning_group_done;
            }
            if (read_pragma_token() != 0x3f) {
                token = 1; done = 1; goto warning_group_done;
            }
            seen_number = 0;
            while ((token = read_pragma_token()) == 0x6f) {
                number = current_token_code == 7 ? (int)current_token_value.expression->value.integer : -1;
                if (level == 15) {
                    if (!bad_level) { bad_level = 1; compiler_warning(1, 615); }
                } else if (number == -1) {
                    if (!bad_number) { bad_number = 1; compiler_warning(1, 617); }
                } else set_warning_level(number, level);
                seen_number = 1;
            }
            if (!seen_number) { token = 1; done = 1; goto warning_group_done; }
            if (token == 0x88) done = 1;
            else if (token == 0x8a) token = read_pragma_token();
            else token = 1;
            if (token == 1) { done = 1; goto warning_group_done; }
            valid = 1;
warning_group_done: ;
        }
        if (token == 1 || !valid) warn_bad_pragma();
        break;
    case 29: /* hdrstop */
        if (declaration_mode) report_syntax_error(156);
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
        if (initializer_segment_symbol || options.configuration.field0c) report_syntax_error(356);
        if (!pragma_open_parenthesis()) return;
        read_pragma_token();
        if (current_token_code == 3) {
            compiler_warning(1, 75);
            segment_text = current_token_value;
            while (read_pragma_token() == 3)
                concatenate_strings(&segment_text, &current_token_value);
            text_work_cursor = (unsigned char *)segment_text.name;
            options.configuration.field0c = 3;
            initializer_segment_name = intern_identifier(segment_text.name);
        } else if (current_token_code == 2) {
            text = current_token_value.name;
            PRAGMA_TEXT_IS(text, "compiler", match);
            if (match) { compiler_warning(1, 74); options.configuration.field0c = 2; }
            else {
                PRAGMA_TEXT_IS(text, "lib", match);
                if (match) { compiler_warning(3, 73); options.configuration.field0c = 1; }
                else {
                    PRAGMA_TEXT_IS(text, "user", match);
                    if (!match) { compiler_warning(1, 109, text); skip_source_line(); return; }
                    options.configuration.field0c = 0;
                }
            }
            read_pragma_token();
        } else { warn_ident_requires_string(); skip_source_line(); return; }
        if (current_token_code != 0x88) {
            warn_ident_requires_string(); skip_source_line(); return;
        }
        break;
    case 31: /* vtordisp */
        pragma_vtordisp = read_parenthesized_pragma_argument(200) ? pragma_argument.integer : 1;
        break;
    case 33: /* pointers_to_members */
        if (!pragma_open_parenthesis()) return;
        for (;;) {
            if (read_pragma_token() != 2) PRAGMA_BAD_TOKEN(83);
            text = current_token_value.name;
            PRAGMA_TEXT_IS(text, "best_case", match);
            if (match) set_default_inheritance_policy(0);
            else {
                PRAGMA_TEXT_IS(text, "full_generality", match);
                if (match) set_default_inheritance_policy(1);
                else {
                    PRAGMA_TEXT_IS(text, "single_inheritance", match);
                    if (match) set_default_inheritance_model(1);
                    else {
                        PRAGMA_TEXT_IS(text, "multiple_inheritance", match);
                        if (match) set_default_inheritance_model(2);
                        else {
                            PRAGMA_TEXT_IS(text, "virtual_inheritance", match);
                            if (!match) { compiler_warning(1, 109, text); skip_source_line(); return; }
                            set_default_inheritance_model(3);
                        }
                    }
                }
            }
            if (read_pragma_token() == 0x40) continue;
            if (current_token_code != 0x88) PRAGMA_BAD_TOKEN(95);
            break;
        }
        break;
    case 32: /* setlocale */
        if (read_parenthesized_pragma_argument(3) && !setlocale(0, pragma_argument.text))
            report_syntax_error(175, pragma_argument.text);
        break;
    case 34: /* string_pooling */
        if (declaration_mode) {
            read_parenthesized_pragma_argument(200); report_syntax_error(156);
        } else options.option_Gf = read_parenthesized_pragma_argument(200) ?
            pragma_argument.integer : options.configuration.fieldb8;
        break;
    default:
        compiler_warning(1, 68); skip_source_line(); return;
    }
    finish_directive("pragma");
}

/* Original0x215c0..0x215d9: EOF is fatal inside a pragma. */
unsigned char read_pragma_token(void)
{
    unsigned char token = read_parser_token();
    if (!token) fatal_error(4);
    return token;
}

/* Encountered vocabulary: original35 suffix pointers at0x9e3e8,
   their35 result kinds at0x9e498 and the29 initial-letter bounds at0x9e478.
   The caller supplies an interned identifier; the supported alphabet is the
   same underscore/lowercase directory used by intrinsic-name lookup. */
typedef struct PragmaName { char *suffix; int kind; } PragmaName;
static PragmaName pragma_names[] = {
    { "uto_inline", 27 }, { "lloc_text", 5 },
    { "omment", 19 }, { "ode_seg", 24 },
    { "heck_stack", 0 }, { "heck_pointer", 7 },
    { "ata_seg", 22 }, { "unction", 4 }, { "drstop", 29 },
    { "ntrinsic", 3 }, { "nline_recursion", 26 },
    { "nline_depth", 25 }, { "nit_seg", 30 },
    { "oop_opt", 1 }, { "inesize", 16 }, { "essage", 18 },
    { "ative_caller", 23 }, { "ptimize", 21 },
    { "ointers_to_members", 33 }, { "lmn", 9 }, { "lmf", 10 },
    { "agesize", 15 }, { "age", 11 }, { "ack", 2 },
    { "witch_check", 8 }, { "ubtitle", 13 }, { "tring_pooling", 34 },
    { "kip", 12 }, { "etlocale", 32 }, { "egment", 20 },
    { "earch_lib", 17 }, { "ame_seg", 6 }, { "itle", 14 },
    { "tordisp", 31 }, { "arning", 28 }
};
static signed char pragma_name_groups[29] = {
    0,0,0,2,2,6,7,7,8,8,9,13,13,13,15,16,17,18,24,24,24,32,33,33,34,35,35,35,35
};

/* Original0x1f500..0x1f594, stdcall4. Compare only the candidates
   selected by the first identifier byte; the result is the dispatcher kind. */
int __stdcall classify_pragma(char *name)
{
    signed char first = (signed char)*name++;
    int index, end;
    char *left, *right;
    if (first < '_') return -1;
    index = pragma_name_groups[first - '_'];
    end = pragma_name_groups[first - '_' + 1];
    for (; index != end; ++index) {
        left = pragma_names[index].suffix;
        right = name;
        while (*left == *right && *left) { ++left; ++right; }
        if (*left == *right) return pragma_names[index].kind;
    }
    return -1;
}

/* Original0x21a50..0x21c59. pack(n), reset, and labeled push/pop
   retain their distinct token-consumption and repeated validation calls. */
void process_pack_pragma(void)
{
    int action = 0, alignment = 0, match;
    char *label = 0;
    PackingEntry *entry;
    if (!pragma_open_parenthesis()) return;
    read_pragma_token();
    switch (current_token_code) {
    case 2:
        PRAGMA_TEXT_IS(current_token_value.name, "push", match);
        if (match) action = 1;
        else {
            PRAGMA_TEXT_IS(current_token_value.name, "pop", match);
            if (!match) goto malformed;
            action = 2;
        }
        read_pragma_token();
        if (current_token_code == 0x88) goto apply;
        if (current_token_code != 0x40) goto malformed;
        read_pragma_token();
        if (current_token_code == 2) {
            label = current_token_value.name;
            read_pragma_token();
            if (current_token_code == 0x88) goto apply;
            if (current_token_code != 0x40) goto malformed;
            read_pragma_token();
            if (current_token_code != 7) goto malformed;
            goto number;
        }
        if (current_token_code == 7) goto number;
        goto apply;
    case 0x88:
        alignment = options.option_Zp == -1 ? 8 : options.option_Zp;
        goto apply;
    case 7:
number:
        alignment = validate_packing(current_token_value.expression->value.integer);
        if (read_pragma_token() != 0x88) goto malformed;
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
    compiler_warning(1, 110, token_spellings[current_token_code]);
}

/* Original0x215e0..0x21615. A bad opening token consumes the rest
   of the directive before returning failure to its individual handler. */
int pragma_open_parenthesis(void)
{
    if (read_pragma_token() == 0x80) return 1;
    compiler_warning(1, 83, token_spellings[current_token_code]);
    skip_source_line();
    return 0;
}

/* Original0x218e0..0x2191c, stdcall4. Invalid packings retain the
   current stack entry's alignment after issuing the level-one warning. */
int __stdcall validate_packing(int alignment)
{
    if (alignment == 1 || alignment == 2 || alignment == 4 ||
        alignment == 8 || alignment == 16) return alignment;
    compiler_warning(1, 86);
    return top_packing(packing_stack)->alignment;
}
