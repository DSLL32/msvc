/* VC5 C++ LR parser. State/production relations are target-specific logical
 * data. Semantic actions retain their distinct callees and original transfers.
 * Native currently supports the control path to the first-token boundary;
 * wider reductions, including action102's member-pointer by-value ABI, remain
 * unsupported before any evaluation/copy/call. */
#include <stddef.h>
#include <string.h>
#include <new.h>
#include "parser.h"
#include "parser-grammar.h"
#include "parser-actions.h"
#include "declaration.h"
#include <recon.h>
#pragma intrinsic(memcpy, memset)

#include "parser-state.h"

#ifndef RECON_NATIVE_TEST
int parser_saved_state;
int parser_recovery_action;
DWORD * parser_saved_state_top;
ParserSemanticView * parser_saved_value_top;
int label_stack_count;
int initializer_started = 0;
ParserDeclarationView parser_special_qualifiers;
ParserLabelStack * continue_labels;
ParserLabelStack * break_labels;
ParserLabelStack * loop_labels;
ParserLabelStack * switch_labels;
ParserLabelStack * default_labels;
#endif

#define parser_context_depth frontend_bss.parser_context_depth
#define P_CONTEXT (*(ParserFrontendContextView *)&frontend_bss.saved_context_stack)
#define current_symbol_arena (*(ParserSymbolArenaView *)&frontend_bss.symbol_arena_kind)
#define P_SYMBOL(p) ((ParserSymbolView *)(p))
#define PARSER_FUNCTION_KIND(k) ((k)==0x10 || (k)==7 || (k)==6 || (k)==0x11 || (k)==8 || (k)==0x19)
#define current_source_frame ((ParserSourceFrame *)frontend_bss.saved_context_stack.top->object)
#define R(n) (*(ParserSemanticView *)&right[n])
#define RESULT (*(ParserSemanticView *)&values[value_top + 1])

int RECON_THISCALL CompilerParser::Run(void)
{
    ParserTokenView *saved_lookahead = parser_lookahead;
    int saved_production = parser_production;
    ParserSemanticView *right;
    const ParserState *state_info;
    const ParserTransition *transition;
    const ParserProduction *production;
    const ParserGoto *goto_info;
    int state = 0, state_top = -1, value_top = -1;
    int terminal, rule, i, recovery;

    ParserDeclarationView local_declaration;
    CompilerDeclaration *declaration;
    CompilerDeclarationContext local_context;
    CompilerDeclarator local_declarator, *declarator, *last_declarator;
    CompilerDeclaratorQualifiers local_qualifiers;
    CompilerSymbol *symbol, *alias_symbol, *declaration_symbol, *declared_symbol, *owner_symbol;
    ParserBindingOwner *owner;
    ParserBindingScope *scope_symbol;
    CompilerSymbolLink *alias_link;
    CompilerType *type;
    CompilerRuntimeFunction *function;
    CompilerNameEntry *name;
    CompilerScopeMatch *scope_lookup;
    ParserTokenView *parser_token;
    ParserRecordedContext *saved_context, *recorded_declaration;
    ParserDeferredRecord *deferred;
    ParserDeferredList *deferred_list;
    ParserSavedParameters *saved_parameters;
    CompilerArena *arena;
    BYTE *memory;
    CompilerExpression *expression, *expression_list, *converted;
    unsigned arena_kind;
    int renamed_parameter;
    parser_lookahead = NULL;
    goto push_state;

read_action:
    state_info = &parser_states[state];
    if (state_info->needs_token) {
        if (!parser_lookahead)
            parser_lookahead = get_next_token();
        /* Original dereferences the returned token; unlike the C frontend,
         * it does not map a NULL return to a synthetic terminal. */
#ifdef RECON_NATIVE_TEST
        if (parser_lookahead->kind >= PARSER_TOKEN_KIND_COUNT)
            recon_unsupported("parser: token kind outside known domain");
#endif
        terminal = parser_token_classes[parser_lookahead->kind];
        transition = parser_shifts + state_info->first_shift;
        for (i = 0; i < state_info->shift_count; ++i, ++transition) {
            if (transition->symbol == terminal) {
                /* Original compares the current state pointer against
                 * this+0xb004. Native uses the corresponding logical index;
                 * this does not promise storage before the allocation. */
                if (state_top > 1019) {
                    report_parser_error("yacc stack overflow");
                    parser_result = 1;
                    goto finished;
                }
                copy_token_value(&values[value_top + 1], parser_lookahead);
                parser_lookahead = NULL;
                if (recovery_shifts > 0)
                    --recovery_shifts;
                state = transition->destination;
                goto push_state;
            }
        }
    }
    rule = state_info->default_production;
    if (state_info->exception_count) {
        if (!parser_lookahead)
            parser_lookahead = get_next_token();
#ifdef RECON_NATIVE_TEST
        if (parser_lookahead->kind >= PARSER_TOKEN_KIND_COUNT)
            recon_unsupported("parser: token kind outside known domain");
#endif
        terminal = parser_token_classes[parser_lookahead->kind];
        transition = parser_exceptions + state_info->first_exception;
        for (i = 0; i < state_info->exception_count; ++i, ++transition) {
            if (transition->symbol == terminal) {
                rule = transition->destination;
            break;
            }
        }
        if (rule < 0) {
            parser_result = 0;
            goto finished;
        }
    }
    if (rule == 0) {
        parser_saved_state_top = &states[state_top];
        parser_saved_value_top = &values[value_top];
        parser_saved_state = state;
        parser_recovery_action = 0;
        recovery = Recover();
        state_top = parser_saved_state_top - states;
        value_top = parser_saved_value_top - values;
        state = parser_saved_state;
        switch (recovery) {
        case 0:
            goto push_state;
        case 1:
            parser_result = 1;
            goto finished;
        case 2:
            goto read_action;
        default:
            goto finished;
        }
    }
    parser_production = rule;
    production = &parser_productions[rule];
    right = &values[value_top];
    state_top -= production->length;
    value_top -= production->length;

#ifdef RECON_NATIVE_TEST
    /* Rules 1/2 bypass the original action switch. The recovered no-op
     * family shares its original logical-goto target and reads, copies and
     * calls nothing. Wider value-reading actions, including action 102,
     * remain guarded before their semantic accesses or transfers. */
    switch (rule) {
    case 1: case 2: case 3: case 16: case 19: case 127: case 149: case 118: case 117:
    case 197: case 196: case 189: case 173: case 167: case 154: case 150: case 125: case 67:
    case 126: case 119: case 177: case 13: case 66: case 28: case 62: case 63:
    case 230: case 237: case 262: case 235: case 241: case 261: case 212: case 269: case 270: case 274: case 325: case 290: case 291:
    case 104: case 105: case 312: case 313: case 106: case 425: case 194: case 228: case 54:
    /* These declarator actions transfer the two known pointer fields or
     * pass the already-typed modifier to its genuine pending consumer. */
    case 254: case 260: case 443: case 258: case 259: case 256:
    case 191: case 193: case 248: case 210: case 250: case 229: case 226:
    case 218: case 211: case 217: case 215: case 208: case 209: case 190:
    case 341: case 342: case 192: case 264: case 202: case 403: case 371: case 343:
    /* Initializer actions use the existing symbol/expression arms; the
     * declarator arm consumes the two pointers produced by196. */
    case 179: case 182: case 219: case 30:
    /* The recorded function marker carries the established symbol pointer
     * directly to the genuine pending body initializer. */
    case 31: case 32:
    /* These recovered initializer reductions only set established global
     * bits or the bounded declaration-control row's existing flag. */
    case 34: case 39: case 41: case 43:
    /* Argument and constructor-initializer lists retain the established
     * expression pointer arm and pass it to the real list helpers. */
    case 36: case 37: case 51: case 52: case 421: case 422: case 447: case 423: case 424:
    /* The ordinary constructor initializer reads a name and its expression
     * list; no declaration callback representation is transferred. */
    case 35: case 40:
    /* Identifier and operator-name records pass their established name
     * pointer to the real pending expression lookup. */
    case 413: case 415:
    /* Binary addition reads the established expression pointers and the
     * canonical operator token DWORD; no declaration value is transferred. */
    case 362:
    /* Allocation preparation updates the known declaration-control flags
     * and passes the existing pointer/token arms to the genuine child. */
    case 388: case 389: case 390: case 391: case 409:
    /* Function-body completion reads the existing symbol arm and the
     * grounded function-class/type/operation bytes; it copies no value. */
    case 156:
    /* The enum declaration carries the same typed callback arm; the real
     * OwnerSymbol child precedes later completion/browser consumers. */
    case 134:
    /* Grounded enum type, flag and scalar-value reductions. */
    case 124: case 132: case 133: case 147: case 148: case 223: case 224:
    /* Actual31530 writes only BYTE1; no wider semantic bytes are read. */
    case 450:
    /* Recovered family assigns only the known pointer arm to NULL. */
    case 33: case 49: case 213: case 266: case 324: case 344: case 445:
        break;
    /* The same no-op family is written out in the semantic switch below. */
    case 6: case 20: case 21: case 22: case 29: case 50: case 53: case 55:
    case 56: case 57: case 58: case 59: case 60: case 61: case 64: case 80:
    case 81: case 82: case 83: case 84: case 91: case 94: case 95: case 103:
    case 107: case 108: case 120: case 121: case 123: case 128: case 131: case 135:
    case 136: case 144: case 145: case 151: case 153: case 155: case 159: case 162:
    case 163: case 174: case 175: case 184: case 185: case 195: case 199: case 216:
    case 225: case 231: case 232: case 238: case 239: case 249: case 257: case 271:
    case 272: case 273: case 276: case 277: case 297: case 299: case 300: case 306:
    case 307: case 321: case 368: case 369: case 382: case 383: case 384: case 385:
    case 386: case 404: case 408: case 419: case 446:
        break;
    default:
        recon_unsupported("parser: unmodeled reduction semantic arm");
    }
#endif
    switch (rule) {
/* Handwritten semantic actions; native support is bounded by the guard above.
 * R(n) is right[n], RESULT is values[value_top+1]. No machine registers
 * or original addresses are implementation operands. Pending names below
 * refer to genuine distinct callees; their ABI/type ledger is separate.
 * Declaration rows retain authentic40-byte transfers only in Win32;
 * native must guard before any unmodeled semantic/context consumer. */
case 3:
case 19:
    parser_result = syntax_errors;
    goto finished;
case 4:
    remember_parser_context(R(-1).pointer); /*3342d*/
    break;
case 5:
    remember_parser_context(NULL);
    break;
/* The common no-op groups do not assign an implicit global semantic value.
 * They retain the already existing first RHS slot, exactly as Run does. */
case 6: case 20: case 21: case 22: case 29:
case 50: case 53: case 55: case 56: case 57: case 58: case 59: case 60: case 61:
case 64: case 80: case 81: case 82: case 83: case 84: case 91: case 94: case 95:
case 103: case 107: case 108: case 120: case 121: case 123: case 128: case 131:
case 135: case 136: case 144: case 145: case 151: case 153: case 155: case 159:
case 162: case 163: case 174: case 175: case 184: case 185: case 195: case 199:
case 216: case 225: case 231: case 232: case 238: case 239: case 249: case 257:
case 271: case 272: case 273: case 276: case 277: case 297: case 299: case 300:
case 306: case 307: case 321: case 368: case 369: case 382: case 383: case 384:
case 385: case 386: case 404: case 408: case 419: case 446:
    break;
case 7:
    if (parser_lookahead && parser_lookahead->kind == 0x76) {
        memcpy(&(*declaration_spec_row(parser_context_depth - 1)), &R(-1).declaration, 40);
        (*declaration_control_row(parser_context_depth - 1)).flags |= 1;
    } else {
        memcpy(&(*declaration_spec_row(parser_context_depth)), &R(-1).declaration, 40);
        (*declaration_control_row(parser_context_depth)).flags |= 1;
    }
    break;
case 8:
    --declaration_nesting;
    R(-1).declaration_value.declarator->ApplyDeclaration(&R(-2).declaration);
    R(-1).declaration_value.declarator->Resolve();
    ((CompilerRuntimeFunction *)R(-1).declaration_value.symbol)->linkage_flags |= 0x40;
    if (frontend_data.declaration_context_index >= 0) {
        R(-1).declaration_value.symbol->kind = 3;
        declaration_symbol = NULL;
        declaration = ((*declaration_control_row(parser_context_depth)).flags & 1) ?
            &(*declaration_spec_row(parser_context_depth)) : default_declaration(0); /*21a97*/
        if (!declare_initialized_symbol(declaration, R(-1).declaration_value.symbol,
                R(-1).declaration_value.declarator, &declaration_symbol)) /*25ba1*/
            frontend_data.translation_unit->SetScopeMode(1);
        current_declaration_symbol = declaration_symbol ? declaration_symbol : R(-1).declaration_value.symbol;
    } else {
        if (!(frontend_data.compiler_mode_flags & 0x40)) {
            declaration = ((*declaration_control_row(parser_context_depth)).flags & 1) ?
                &(*declaration_spec_row(parser_context_depth)) : default_declaration(0);
            current_declaration_symbol = declare_parameter_symbol(declaration,
                R(-1).declaration_value.symbol, R(-1).declaration_value.declarator); /*1d78f*/
        } else current_declaration_symbol = R(-1).declaration_value.symbol;
        frontend_data.translation_unit->SetScopeMode(1);
        process_declaration_specifiers(&R(-2).declaration); /*d8c2*/
    }
    last_declarator = R(-1).declaration_value.declarator;
    break;
case 9:
    frontend_data.compiler_flags |= 0x01000000;
    frontend_data.compiler_mode_flags &= ~2U;
    --declaration_nesting;
    declaration = ((*declaration_control_row(parser_context_depth)).flags & 1) ?
        &(*declaration_spec_row(parser_context_depth)) : default_declaration(0);
    R(-1).declaration_value.declarator->ApplyDeclaration(declaration);
    R(-1).declaration_value.declarator->Resolve();
    ((CompilerRuntimeFunction *)R(-1).declaration_value.symbol)->linkage_flags |= 0x40;
    if (frontend_data.declaration_context_index >= 0) {
        R(-1).declaration_value.symbol->kind = 3;
        declaration_symbol = NULL;
        declaration = ((*declaration_control_row(parser_context_depth)).flags & 1) ?
            &(*declaration_spec_row(parser_context_depth)) : default_declaration(0);
        if (!declare_initialized_symbol(declaration, R(-1).declaration_value.symbol,
                R(-1).declaration_value.declarator, &declaration_symbol))
            frontend_data.translation_unit->SetScopeMode(1);
        current_declaration_symbol = declaration_symbol ? declaration_symbol : R(-1).declaration_value.symbol;
    } else {
        if (!(frontend_data.compiler_mode_flags & 0x40)) {
            declaration = ((*declaration_control_row(parser_context_depth)).flags & 1) ?
                &(*declaration_spec_row(parser_context_depth)) : default_declaration(0);
            current_declaration_symbol = declare_parameter_symbol(declaration,
                R(-1).declaration_value.symbol, R(-1).declaration_value.declarator);
        } else current_declaration_symbol = R(-1).declaration_value.symbol;
        frontend_data.translation_unit->SetScopeMode(1);
    }
    last_declarator = R(-1).declaration_value.declarator;
    break;
case 10:
    frontend_data.compiler_flags |= 0x01000000;
    frontend_data.compiler_mode_flags &= ~2U;
    frontend_data.translation_unit->SetScopeMode(1);
    declaration = ((*declaration_control_row(parser_context_depth)).flags & 1) ?
        &(*declaration_spec_row(parser_context_depth)) : default_declaration(0);
    R(-1).declaration_value.declarator->ApplyDeclaration(declaration);
    R(-1).declaration_value.declarator->Resolve();
    declaration = ((*declaration_control_row(parser_context_depth)).flags & 1) ?
        &(*declaration_spec_row(parser_context_depth)) : default_declaration(0);
    current_declaration_symbol = declare_symbol(declaration, R(-1).declaration_value.symbol,
        R(-1).declaration_value.declarator); /*19920*/
    --declaration_nesting;
    last_declarator = R(-1).declaration_value.declarator;
    (*declaration_control_row(parser_context_depth)).flags &= ~3;
    break;
case 11:
    frontend_data.compiler_flags &= ~0x01000000U;
    frontend_data.compiler_mode_flags &= ~2U;
    frontend_data.translation_unit->SetScopeMode(1);
    R(-1).declaration_value.declarator->ApplyDeclaration(&R(-2).declaration);
    R(-1).declaration_value.declarator->Resolve();
    current_declaration_symbol = declare_symbol(&R(-2).declaration, R(-1).declaration_value.symbol,
        R(-1).declaration_value.declarator);
    --declaration_nesting;
    last_declarator = R(-1).declaration_value.declarator;
    (*declaration_control_row(parser_context_depth)).flags &= ~3;
    break;
case 12:
case 122:
    process_parser_directive(R(-1).pointer); /*320b7*/
    break;
case 13:
    finish_external_declaration(); /*d9d1*/
    break;
case 14:
    process_end_of_input(); /*4eb57*/
    break;
case 15:
    process_deferred_parser_input(); /*868db*/
    break;
case 16:
    if (frontend_bss.error_count) {
        parser_result = frontend_bss.error_count;
        goto finished;
    }
    break;
case 17:
    process_pch_boundary(); /*3cf00*/
    break;
case 18:
    if (!(frontend_data.compiler_mode_flags & 0x40)) {
        compiler_warning(0x1bf);
    } else frontend_data.compiler_mode_flags &= ~0x40U;
    process_external_recovery(0); /*4ddd2*/
    ClearRecovery(); /* genuine member85e60 */
    parser_lookahead = NULL;
    break;
case 23:
    ClearRecovery(); /*85e60; no direct replacement in final graph*/
    break;
case 24:
    frontend_data.compiler_flags |= 0x01000000;
    frontend_data.compiler_mode_flags &= ~2U;
    declaration = ((*declaration_control_row(parser_context_depth)).flags & 1) ?
        &(*declaration_spec_row(parser_context_depth)) : default_declaration(0);
    R(0).declaration_value.declarator->ApplyDeclaration(declaration);
    R(0).declaration_value.declarator->Resolve();
    declaration = ((*declaration_control_row(parser_context_depth)).flags & 1) ?
        &(*declaration_spec_row(parser_context_depth)) : default_declaration(0);
    declare_symbol(declaration, R(0).declaration_value.symbol, R(0).declaration_value.declarator);
    --declaration_nesting;
    last_declarator = R(0).declaration_value.declarator;
    frontend_data.translation_unit->SetScopeMode(1);
    (*declaration_control_row(parser_context_depth)).flags &= ~3;
    break;
case 25:
    current_parser_expression = R(0).expression; /*c0a08, role still provisional*/
    process_external_initializer(); /*4141*/
    frontend_data.compiler_mode_flags &= ~0x40U;
    break;
case 26:
    finish_external_initializer(); /*5962*/
    break;
case 27:
    begin_function_definition(R(0).pointer); /*359b7, exact wider input type pending*/
    break;
case 28:
    frontend_data.compiler_flags &= ~0x01000000U;
    frontend_data.compiler_mode_flags &= ~2U;
    frontend_data.translation_unit->SetScopeMode(1);
    R(0).declaration_value.declarator->ApplyDeclaration(&R(-1).declaration);
    R(0).declaration_value.declarator->Resolve();
    if (!frontend_bss.declaration_scope_mode)
        RESULT.symbol = declare_symbol(&R(-1).declaration,
            R(0).declaration_value.symbol, R(0).declaration_value.declarator);
    else {
        process_merged_declaration(merge_symbol_declaration(&R(-1).declaration,
            R(0).declaration_value.symbol, R(0).declaration_value.declarator), 0); /*28e4d*/
        parser_lookahead = parser_end_marker; /*actual initializedcc9b0 owner unresolved*/
    }
    --declaration_nesting;
    last_declarator = R(0).declaration_value.declarator;
    (*declaration_control_row(parser_context_depth)).flags &= ~3;
    break;
case 30:
    frontend_data.source_mode_flags |= 2;
    set_function_parameters(R(0).recorded_parameters); /*35e45*/
    break;
case 31:
case 32:
    RESULT.pointer = begin_function_body(R(0).symbol); /*22308*/
    break;
case 33: case 49: case 213: case 266: case 324: case 344: case 445:
    RESULT.pointer = NULL;
    break;
case 34:
    frontend_data.compiler_flags |= 1;
    frontend_data.compiler_mode_flags |= 0x100;
    break;
case 35:
    if (!parser_lookahead)
        parser_lookahead = P_CONTEXT.ReadToken(); /*8848*/
    if (parser_lookahead->kind != 0x75) {
        compiler_warning(0x264, parser_lookahead);
        defer_parser_token(parser_lookahead); /*2b78a*/
        parser_lookahead = parser_semicolon; /*actualcc8d4 owner unresolved*/
    }
    frontend_data.compiler_flags &= ~1U;
    frontend_data.compiler_mode_flags &= ~0x100U;
    RESULT.expression = R(0).expression;
    break;
case 36:
    RESULT.expression = append_parser_expression(NULL, R(0).expression); /*3220e*/
    break;
case 37:
    RESULT.expression = append_parser_expression(R(-2).expression, R(0).expression);
    break;
case 38:
    compiler_warning(0x276, 0x4c, 0x7f);
    RESULT.expression = append_parser_expression(R(-2).expression, R(0).expression);
    break;
case 39: case 41: case 43:
    (*declaration_control_row(parser_context_depth)).flags |= 1;
    break;
case 40:
    RESULT.expression = make_binary_node(0x76,
        symbol_expression(lookup_builtin(R(-4).name), 0), R(-1).expression, 0); /*65c0/47e6*/
    frontend_data.compiler_flags |= 1;
    if (parser_context_depth > 0) --parser_context_depth;
    break;
case 42:
    RESULT.expression = make_binary_node(0x76,
        symbol_expression(lookup_builtin(R(-4).indirect_name->name), 0), R(-1).expression, 0);
    frontend_data.compiler_flags |= 1;
    if (parser_context_depth > 0) --parser_context_depth;
    break;
case 44:
    RESULT.expression = make_binary_node(0x76,
        symbol_expression(R(-4).indirect_name->symbol, 0), R(-1).expression, 0);
    frontend_data.compiler_flags |= 1;
    if (parser_context_depth > 0) --parser_context_depth;
    break;
case 45:
    RESULT.pointer = NULL;
    compiler_warning(0x15f);
    frontend_data.compiler_flags |= 1;
    break;
case 46:
    RESULT.pointer = NULL;
    compiler_warning(0x15f);
    frontend_data.compiler_flags |= 1;
    if (parser_context_depth > 0) --parser_context_depth;
    break;
case 47:
    RESULT.pointer = NULL;
    ClearRecovery();
    recover_parser_construct(parser_lookahead, 0x75, 0x7f, 1); /*86173*/
    break;
case 48:
    RESULT.pointer = R(-1).pointer;
    if (parser_context_depth > 0) --parser_context_depth;
    break;
case 51:
    RESULT.expression = append_expression_list(NULL, R(0).expression, 2); /*12057*/
    break;
case 52:
    RESULT.expression = append_expression_list(R(-2).expression, R(0).expression, 2);
    break;
case 54:
    pop_declaration_context(); /*42072, exact scalar linkage-depth pop*/
    break;
case 62:
    if (R(-1).pointer) {
        current_parser_expression = R(0).expression;
        process_external_initializer();
    } else frontend_data.compiler_flags |= 0x00800000;
    frontend_data.compiler_mode_flags &= ~0x40U;
    break;
case 63:
    if (R(-3).pointer) finish_external_initializer();
    else frontend_data.compiler_flags &= ~0x00800000U;
    frontend_data.source_mode_flags &= ~2U;
    break;
case 65:
    ClearRecovery();
    frontend_data.compiler_flags &= ~0x00800000U;
    frontend_data.compiler_mode_flags &= ~0x40U;
    frontend_data.source_mode_flags &= ~2U;
    break;
case 66:
#ifdef __linux__
    memcpy(&local_declaration, &R(-1).declaration, 36);
    local_declaration.callback = R(-1).declaration.callback;
#else
    memcpy(&local_declaration, &R(-1).declaration, 40);
#endif
    process_declaration_specifiers(&local_declaration);
    RESULT.declaration_value = R(0).declaration_value; /*exact twoDWORD stores*/
    break;
case 67:
#ifdef __linux__
    memcpy(&local_declaration, &R(0).declaration, 36);
    local_declaration.callback = R(0).declaration.callback;
#else
    memcpy(&local_declaration, &R(0).declaration, 40);
#endif
    process_declaration_specifiers(&local_declaration);
    finish_declaration_specifiers(&local_declaration); /*28d18*/
    RESULT.declaration_value.symbol = NULL; /*firstDWORD is retained*/
    break;
case 68:
    process_declared_type(R(-2).pointer, &R(-1).declaration); /*4d66e*/
    break;
case 69:
    process_function_type(R(-2).pointer, R(-1).pointer); /*4e35c*/
    break;
case 70:
    process_declared_type(R(-1).pointer, NULL);
    break;
case 71:
    process_member_declaration(1, &R(-1).declaration, NULL, NULL); /*b54b5*/
    break;
case 72:
    R(-1).declaration_value.declarator->ApplyDeclaration(&R(-2).declaration);
    R(-1).declaration_value.declarator->Resolve();
    process_member_declaration(1, &R(-2).declaration,
        R(-1).declaration_value.symbol, R(-1).declaration_value.declarator);
    frontend_data.translation_unit->SetScopeMode(1);
    last_declarator = R(-1).declaration_value.declarator;
    --declaration_nesting;
    break;
case 73:
    compiler_warning(0x38d, P_SYMBOL(R(-1).declaration_value.symbol)->link.name);
    discard_member_declaration(1, &R(-1).declaration_value); /*86557*/
    frontend_data.translation_unit->SetScopeMode(1);
    --declaration_nesting;
    break;
case 74:
    process_member_declaration(R(-3).token_kind, &R(-1).declaration, NULL, NULL);
    break;
case 75:
    R(-1).declaration_value.declarator->ApplyDeclaration(&R(-2).declaration);
    R(-1).declaration_value.declarator->Resolve();
    process_member_declaration(R(-4).token_kind, &R(-2).declaration,
        R(-1).declaration_value.symbol, R(-1).declaration_value.declarator);
    frontend_data.translation_unit->SetScopeMode(1);
    last_declarator = R(-1).declaration_value.declarator;
    --declaration_nesting;
    break;
case 76:
    /* Only the actual28-byte context clear, type write and+20 clear occur.
     * The final DWORD of the40-byte local declaration remains unwritten. */
    memset(&local_declaration.context, 0, 28);
    local_declaration.context.class_flags = 0x30;
    local_declaration.type = frontend_data.builtin_types.type_0000;
    local_declaration.object = NULL;
    R(-1).declaration_value.declarator->ApplyDeclaration(&local_declaration);
    R(-1).declaration_value.declarator->Resolve();
    process_member_declaration(R(-3).token_kind, &local_declaration,
        R(-1).declaration_value.symbol, R(-1).declaration_value.declarator);
    if (R(-1).declaration_value.declarator)
        R(-1).declaration_value.declarator->Reset(); /*physical1300 alias; no bodycredit*/
    frontend_data.translation_unit->SetScopeMode(1);
    --declaration_nesting;
    break;
case 77:
    ClearRecovery();
    process_declared_type(NULL, NULL);
    break;
case 78:
    begin_type_declaration(); /*4d103*/
    break;
case 79:
    R(-1).parameter_description->Normalize();
    RESULT.pointer = finish_type_declaration(R(-1).parameter_description->parameters); /*4d439*/
    break;
case 85:
    --declaration_nesting;
    R(0).declaration_value.declarator->ApplyDeclaration(&R(-1).declaration);
    R(0).declaration_value.declarator->Resolve();
    if (frontend_data.declaration_context_index >= 0) {
        R(0).declaration_value.symbol->kind = 3;
        declaration = ((*declaration_control_row(parser_context_depth)).flags & 1) ?
            &(*declaration_spec_row(parser_context_depth)) : default_declaration(0);
        if (!declare_initialized_symbol(declaration, R(0).declaration_value.symbol,
                R(0).declaration_value.declarator, NULL))
            frontend_data.translation_unit->SetScopeMode(1);
        declaration_symbol = R(0).declaration_value.symbol;
    } else {
        if (!(frontend_data.compiler_mode_flags & 0x40)) {
            declaration = ((*declaration_control_row(parser_context_depth)).flags & 1) ?
                &(*declaration_spec_row(parser_context_depth)) : default_declaration(0);
            declaration_symbol = declare_parameter_symbol(declaration,
                R(0).declaration_value.symbol, R(0).declaration_value.declarator);
        } else declaration_symbol = R(0).declaration_value.symbol;
        frontend_data.translation_unit->SetScopeMode(1);
        process_declaration_specifiers(&R(-1).declaration);
    }
    if (declaration_symbol->kind == 7 || declaration_symbol->kind == 8)
        ((CompilerRuntimeFunction *)declaration_symbol)->linkage_flags |= 0x20;
    RESULT.symbol = declaration_symbol;
    last_declarator = R(0).declaration_value.declarator;
    break;
case 86:
    frontend_data.compiler_flags |= 0x01000000;
    frontend_data.compiler_mode_flags &= ~2U;
    --declaration_nesting;
    declaration = ((*declaration_control_row(parser_context_depth)).flags & 1) ?
        &(*declaration_spec_row(parser_context_depth)) : default_declaration(0);
    R(0).declaration_value.declarator->ApplyDeclaration(declaration);
    R(0).declaration_value.declarator->Resolve();
    if (frontend_data.declaration_context_index >= 0) {
        R(0).declaration_value.symbol->kind = 3;
        declaration = ((*declaration_control_row(parser_context_depth)).flags & 1) ?
            &(*declaration_spec_row(parser_context_depth)) : default_declaration(0);
        if (!declare_initialized_symbol(declaration, R(0).declaration_value.symbol,
                R(0).declaration_value.declarator, NULL))
            frontend_data.translation_unit->SetScopeMode(1);
        declaration_symbol = R(0).declaration_value.symbol;
    } else {
        if (!(frontend_data.compiler_mode_flags & 0x40)) {
            declaration = ((*declaration_control_row(parser_context_depth)).flags & 1) ?
                &(*declaration_spec_row(parser_context_depth)) : default_declaration(0);
            declaration_symbol = declare_parameter_symbol(declaration,
                R(0).declaration_value.symbol, R(0).declaration_value.declarator);
        } else declaration_symbol = R(0).declaration_value.symbol;
        frontend_data.translation_unit->SetScopeMode(1);
    }
    if (declaration_symbol->kind == 7 || declaration_symbol->kind == 8)
        ((CompilerRuntimeFunction *)declaration_symbol)->linkage_flags |= 0x20;
    RESULT.symbol = declaration_symbol;
    last_declarator = R(0).declaration_value.declarator;
    break;
case 87: case 88: case 98:
    (*declaration_control_row(parser_context_depth)).flags &= ~2;
    break;
case 89:
case 90:
    /* Distinct productions share the same literal28-byte allocation and
     * three writes. The other fields are not zeroed/defaulted here. */
    arena = &frontend_bss.arenas[5]; /*actualc65a0 receiver*/
    memory = arena->cursor;
    arena->cursor += 28;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    saved_parameters = (ParserSavedParameters *)memory;
    if (saved_parameters) {
        saved_parameters->parameters = ((ParserParameterScopeView *)frontend_data.translation_unit->current)->parameters;
        saved_parameters->kind = 3;
        saved_parameters->has_parameters = saved_parameters->parameters != NULL;
    }
    RESULT.saved_parameters = saved_parameters;
    break;
/* Productions92/93 allocate distinct genuine virtual owners. Their complete
 * actual vtable identities and wider flags remain in recovery, not replaced
 * by guessed raw-vtable arrays or a shared fake helper. */
case 96:
    frontend_data.compiler_mode_flags |= 0x08000000;
    begin_declaration_scope(); /*36034*/
    prepare_declaration_lookup(); /*2200e*/
    break;
case 97:
    if (current_source_frame->source_flags & 1)
        frontend_bss.debug_module->Slot9c(P_CONTEXT.position, 0);
    break;
case 99:
    arena = &frontend_bss.arenas[0];
    memory = arena->cursor;
    arena->cursor += 12;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    deferred_list = (ParserDeferredList *)memory;
    if (deferred_list) {
        deferred_list->head = deferred_list->tail = R(0).deferred;
        deferred_list->flags = (deferred_list->flags & ~1) |
            ((R(0).deferred->flags >> 3) & 1);
    }
    RESULT.deferred_list = deferred_list;
    break;
case 100:
    R(-2).deferred_list->tail->next = R(0).deferred;
    R(0).deferred->previous = R(-2).deferred_list->tail;
    R(-2).deferred_list->tail = R(0).deferred;
    if (R(0).deferred->flags & 8) R(-2).deferred_list->flags |= 1;
    break;
case 92:
    if (current_source_frame->source_flags & 1)
        frontend_bss.debug_module->Slota0(P_CONTEXT.position, 0);
    if (!(frontend_data.compiler_mode_flags & 0x10000000))
        compiler_warning(0x3e, P_SYMBOL(R(0).symbol)->link.name);
    arena = &frontend_bss.arenas[0];
    memory = arena->cursor;
    arena->cursor += 20;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    deferred = memory ? new(memory) ParserDeferredPointer : NULL;
    if (deferred) {
        deferred->next = deferred->previous = NULL;
        deferred->flags &= ~7;
        ((ParserDeferredPointer *)deferred)->payload = P_SYMBOL(R(0).symbol)->payload;
    }
    goto prepend_deferred_record;
case 93:
    if (current_source_frame->source_flags & 1)
        frontend_bss.debug_module->Slota0(P_CONTEXT.position, 0);
    arena = &frontend_bss.arenas[frontend_data.constant_arena_index];
    memory = arena->cursor;
    arena->cursor += 24;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    expression = (CompilerExpression *)memory;
    expression->kind = 0xcc;
    expression->flags = 0;
    expression->value.pointer = R(0).pointer; /*actual+10, other payload bytes unwritten*/
    expression_list = append_expression_list(NULL, expression, 2);
    arena = &frontend_bss.arenas[0];
    memory = arena->cursor;
    arena->cursor += 20;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    deferred = memory ? new(memory) ParserDeferredListItem : NULL;
    if (deferred) {
        deferred->next = deferred->previous = NULL;
        deferred->flags = (deferred->flags & ~7) | 1;
        ((ParserDeferredListItem *)deferred)->payload = expression_list;
    }
prepend_deferred_record:
    deferred_list = deferred_contexts[parser_context_depth - 1];
    if (!deferred_list) {
        arena = &frontend_bss.arenas[0];
        memory = arena->cursor;
        arena->cursor += 12;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        deferred_list = (ParserDeferredList *)memory;
        if (deferred_list) {
            deferred_list->head = deferred_list->tail = deferred;
            deferred_list->flags = (deferred_list->flags & ~1) | ((deferred->flags >> 3) & 1);
        }
        deferred_contexts[parser_context_depth - 1] = deferred_list;
    } else {
        deferred->next = deferred_list->head;
        deferred_list->head->previous = deferred;
        deferred_list->head = deferred;
        if (deferred->flags & 8) deferred_list->flags |= 1;
    }
    finish_deferred_declaration(); /*4461f*/
    frontend_data.compiler_mode_flags &= ~0x01000000U;
    frontend_data.source_mode_flags &= ~0x10U;
    if ((frontend_data.compiler_mode_flags & 0x32000000) ||
        ((frontend_data.compiler_mode_flags & 0x40000000) &&
         !(frontend_data.source_mode_flags & 8) && !(frontend_data.compiler_flags & 0x200)) ||
        (frontend_data.source_mode_flags & 0x10) ||
        ((frontend_data.compiler_mode_flags & 0x04000000) && !(frontend_data.compiler_flags & 0x8000)))
        frontend_data.constant_arena_index = frontend_data.tagged_type_arena_index = 0;
    else frontend_data.constant_arena_index = frontend_data.tagged_type_arena_index =
        (frontend_data.source_mode_flags & 1) ? 4 : 5;
    (*declaration_control_row(parser_context_depth)).flags &= ~2;
    break;
case 101:
    RESULT.deferred = make_deferred_expression(R(0).expression); /*3386e*/
    break;
case 102:
    RESULT.deferred = make_deferred_declaration(R(-1).declaration,
        &R(0).declaration_value); /*33385, genuine40B by-value arg + pointer*/
    break;
case 104:
    frontend_data.compiler_mode_flags &= ~0x20U;
    break;
case 105: case 312: case 313:
    (*declaration_control_row(parser_context_depth)).flags &= ~3;
    break;
case 106:
#ifdef __linux__
    memcpy(&local_declaration, &R(-2).declaration, 36);
    local_declaration.callback = R(-2).declaration.callback;
#else
    memcpy(&local_declaration, &R(-2).declaration, 40);
#endif
    push_declaration_context(parse_linkage_specifier(&local_declaration,
        R(0).expression)); /*41ec5, returns scalar0/1/2 for unsupported/C++/C*/
    if (!parser_lookahead && !current_recovery_shifts()) /*db42*/
        frontend_bss.arenas[5].ResetStorage(); /*db4c, real helper*/
    parser_token_text_cursor = compiler_token_text;
    (*declaration_control_row(parser_context_depth)).flags &= ~3;
    if (parser_lookahead && parser_lookahead->kind != 0x75)
        frontend_data.compiler_mode_flags |= 0x20;
    break;
case 109:
    finish_namespace_declaration(R(-2).symbol); /*ae251*/
    break;
case 110:
    RESULT.symbol = begin_namespace_declaration(NULL); /*adf2e*/
    break;
case 111:
    RESULT.symbol = begin_namespace_declaration(R(-1).name);
    break;
case 112:
    frontend_data.translation_unit->SetScopeMode(0);
    symbol = R(-1).symbol;
    /* Original follows the symbol's link-relative alias, then the two
     * actual owner pointers. A missing link retains the input symbol. */
    alias_link = P_SYMBOL(symbol)->link.next;
    if (alias_link) {
        alias_symbol = (CompilerSymbol *)CONTAINING_RECORD(alias_link, CompilerNamedSymbol, link);
        if (alias_symbol && P_SYMBOL(alias_symbol)->binding->owner->scope && P_SYMBOL(alias_symbol)->binding)
            symbol = alias_symbol;
    }
    process_using_declaration(symbol, 0, 0); /*ae288*/
    break;
case 113:
    frontend_data.translation_unit->SetScopeMode(0);
    symbol = R(-1).symbol;
    alias_link = P_SYMBOL(symbol)->link.next;
    if (alias_link) {
        alias_symbol = (CompilerSymbol *)CONTAINING_RECORD(alias_link, CompilerNamedSymbol, link);
        if (alias_symbol && P_SYMBOL(alias_symbol)->binding->owner->scope && P_SYMBOL(alias_symbol)->binding) {
            process_using_directive(alias_symbol); /*ae4a1*/
            break;
        }
    }
    compiler_warning(0x369, symbol);
    break;
case 114:
    process_using_directive(R(-1).declaration.object); /*RHS[-1]+20*/
    break;
case 115:
    frontend_data.translation_unit->SetScopeMode(0);
    declare_namespace_alias(R(-3).name, R(-1).symbol); /*af040*/
    break;
case 116:
    compiler_warning(0x122, 0xaf);
    if (parser_context_depth > 0) --parser_context_depth;
    break;
case 117:
    {
        unsigned depth = (parser_lookahead && parser_lookahead->kind == 0x76) ?
            parser_context_depth - 1 : parser_context_depth;
        ParserDeclarationView *destination = declaration_spec_row(depth);
#ifdef __linux__
        memcpy(destination, &R(0).declaration, 36);
        destination->callback = R(0).declaration.callback;
#else
        memcpy(destination, &R(0).declaration, 40);
#endif
        (*declaration_control_row(depth)).flags |= 1;
    }
    break;
case 118:
    if (R(0).declaration.callback != 0 && R(0).declaration.object)
        (frontend_data.translation_unit->*R(0).declaration.callback)(R(0).declaration.object);
    break;
case 119:
#ifdef __linux__
    /* The canonical saved owner holds the native member pointer in full. */
    memcpy(&saved_parser_value.declaration, &R(0).declaration, 36);
    saved_parser_value.declaration.callback = R(0).declaration.callback;
#else
    memcpy(&saved_parser_value.declaration, &R(0).declaration, 40); /*BA8C8*/
#endif
    if (parser_lookahead && parser_lookahead->Kind() == 0x7f) break; /*fdb1*/
    parser_lookahead = defer_parser_token(parser_lookahead); /*2b78a returns token*/
    frontend_data.compiler_flags |= 0x80000000;
    saved_context = (ParserRecordedContext *)allocate_parser_storage(sizeof(ParserDeclarationToken), current_symbol_arena.Reference()); /*2b7ac: original48/native52*/
    /* The pending Read entry traps before its unresolved declaration transfer. */
    recorded_declaration = parse_recorded_declaration(saved_context ?
        saved_context->Read(0x22, &saved_parser_value.known) : NULL); /*2b7db/cd8f*/
    frontend_data.compiler_flags &= ~0x80000000U;
#ifdef __linux__
    {
        ParserDeclarationView *value = recorded_declaration->GetDeclaration();
        memcpy(&RESULT.declaration, value, 36);
        RESULT.declaration.callback = value->callback;
    }
#else
    memcpy(&RESULT.declaration, recorded_declaration->GetDeclaration(), 40); /*2b830*/
#endif
    break;
case 124:
    RESULT.declaration.type = current_enumerator_type(); /*3715b*/
    break;
case 125:
    RESULT.declaration.type = finish_class_specifier(); /*26828*/
    finish_class_scope(); /*28852*/
    if (!parser_lookahead)
        parser_lookahead = P_CONTEXT.ReadToken();
    break;
case 126:
    restore_class_scope(); /*28895*/
    break;
case 127:
    if (R(-1).declaration.callback)
        (frontend_data.translation_unit->*R(-1).declaration.callback)(R(-1).declaration.object);
    finish_class_declaration(R(-1).declaration.object, R(-1).declaration.type); /*256c1*/
    finish_class_definition(); /*259dd*/
    break;
case 129:
    if (R(-1).declaration.callback)
        (frontend_data.translation_unit->*R(-1).declaration.callback)(R(-1).declaration.object);
    finish_class_declaration(R(-1).declaration.object, R(-1).declaration.type);
    frontend_data.compiler_flags |= 3;
    break;
case 130:
    if (!parser_lookahead)
        parser_lookahead = P_CONTEXT.ReadToken();
    if (parser_lookahead->Kind() != 0x75) {
        compiler_warning(0x265, parser_lookahead->Kind());
        defer_parser_token(parser_lookahead);
        parser_lookahead = canonical_parser_token(0x75); /*86371, kind-indexed existing table*/
    }
    frontend_data.compiler_flags &= ~3U;
    finish_class_definition();
    break;
case 132: case 133: case 378:
    frontend_data.compiler_flags &= ~1U;
    break;
case 134:
    if (R(-1).declaration.callback)
        (frontend_data.translation_unit->*R(-1).declaration.callback)(R(-1).declaration.object);
    symbol = R(-1).declaration.type->OwnerSymbol(); /*36f41, indexed+8/type owner*/
    if (check_completed_class_option)
        check_completed_class(symbol); /*466b7*/
    if (frontend_bss.browser_filename &&
        (!frontend_bss.declaration_scope_mode || parser_browser_context) &&
        !symbol->SuppressBrowser()) { /*86533*/
        frontend_data.backend->Slot0c();
        if (!frontend_data.backend->Slot54(0, (unsigned)symbol->Name())) /*21427 genuine getter*/
            frontend_data.backend->Slot20(symbol);
        frontend_data.backend->Slot30((unsigned)symbol);
    }
    break;
case 137:
    add_base_class_name(R(-1).access.kind & 3,
        -(int)(R(-1).access.flags & 1), R(0).name); /*2a404*/
    frontend_data.compiler_flags |= 1;
    break;
case 138:
    add_base_class_symbol(R(-1).access.kind & 3,
        -(int)(R(-1).access.flags & 1), R(0).reference->ResolvedSymbol()); /*2a434/21427*/
    frontend_data.compiler_flags |= 1;
    break;
case 139:
    check_qualified_declaration(R(0).reference->Reference()); /*1458f/1d81a*/
    add_qualified_base_class(R(-1).access.kind & 3,
        -(int)(R(-1).access.flags & 1), R(0).reference->Reference()); /*88caf*/
    frontend_data.compiler_flags |= 1;
    break;
case 140:
    ClearRecovery();
    P_CONTEXT.UnreadToken(canonical_parser_token(0x75)); /*86389*/
    break;
case 141:
    RESULT.access.kind &= ~3;
    RESULT.access.flags &= ~7U;
    break;
case 142:
    if (!(RESULT.access.kind & 3))
        RESULT.access.kind = (RESULT.access.kind & ~3) |
            (parser_token_semantic_codes[R(0).token_kind] & 3);
    else if (!(RESULT.access.flags & 2)) {
        compiler_warning(0x1f6, class_scopes[frontend_data.declaration_context_index].symbol);
        RESULT.access.flags |= 2;
    }
    break;
case 143:
    if (!(RESULT.access.flags & 1)) RESULT.access.flags |= 1;
    else if (!(RESULT.access.flags & 4)) {
        compiler_warning(0x1fb, class_scopes[frontend_data.declaration_context_index].symbol);
        RESULT.access.flags |= 4;
    }
    break;
case 146:
    enumerator_value = next_enumerator_value(enumerator_value, R(0).name); /*44b00*/
    add_enumerator(R(0).name, current_enum_type, enumerator_value); /*36f50*/
    frontend_data.compiler_flags |= 1;
    break;
case 147:
    if (frontend_data.source_mode_flags & 0x00100000) {
#ifdef __linux__
        recon_unsupported("enum replay: recorded value representation pending");
#endif
        parser_token = recover_parser_construct(P_CONTEXT.ReadToken(),
            0x7b, 0x37, 1);
        P_CONTEXT.UnreadToken(parser_token);
        saved_parser_value.recorded_type.type = frontend_data.builtin_types.type_0063; /*BA8C8 actualDWORD; a different semantic arm from a declaration*/
        saved_parser_value.recorded_type.kind = 1; /*BA8D0 actualDWORD*/
        saved_parser_value.recorded_type.value = 0; /*BA8D4 actualDWORD*/
        saved_context = (ParserRecordedContext *)allocate_parser_storage(0x30, current_symbol_arena.Reference());
        parser_lookahead = saved_context ? saved_context->Read(0x65, &saved_parser_value.known) : NULL;
    }
    break;
case 148:
    add_enumerator(R(-3).name, current_enum_type, R(0).integer);
    if (!frontend_bss.declaration_depth && !declaration_type_scratch_index &&
        !(frontend_data.compiler_flags & 0x08000000) && !parser_context_depth &&
        !(frontend_data.compiler_mode_flags & 0x40000000) && !(frontend_data.source_mode_flags & 8))
        select_parser_arena(5); /*7363*/
    frontend_data.compiler_flags |= 1;
    break;
case 149:
case 150:
    if (!frontend_bss.declaration_depth && !declaration_type_scratch_index &&
        !(frontend_data.compiler_flags & 0x08000000) && !parser_context_depth &&
        !(frontend_data.compiler_mode_flags & 0x40000000) && !(frontend_data.source_mode_flags & 8))
        select_parser_arena(5);
    frontend_data.compiler_flags &= ~1U;
    parser_token_text_cursor = compiler_token_text;
    (*declaration_control_row(parser_context_depth)).flags &= ~3;
    break;
case 152:
    memcpy(&local_declaration, &R(-1).declaration, 40);
    declare_initialized_symbol(&local_declaration, NULL, NULL, NULL);
    break;
case 154:
#ifdef __linux__
    memcpy(&local_declaration, &R(-2).declaration, 36);
    local_declaration.callback = R(-2).declaration.callback;
#else
    memcpy(&local_declaration, &R(-2).declaration, 40);
#endif
    process_declaration_specifiers(&local_declaration);
    break;
case 156:
    function = (CompilerRuntimeFunction *)R(-1).symbol;
    if (function->function_class == 1 && !(function->type_flags & 0x30) &&
        ((signed char)function->builtin_operation < 1 || (signed char)function->builtin_operation >= 0x11))
        report_language_diagnostic(1, 0xb7, function->Name());
    break;
case 157:
    set_member_access((BYTE)parser_token_semantic_codes[R(-1).token_kind]); /*28e2b*/
    break;
case 158:
    compiler_warning(0x140, parser_lookahead->Kind());
    set_member_access((BYTE)parser_token_semantic_codes[R(0).token_kind]);
    break;
case 160:
case 161:
    compiler_warning(0xee, rule == 160 ? 0x36 : 0x7f);
    if ((frontend_data.compiler_mode_flags & 0x40000000) &&
        P_CONTEXT.PeekToken()->Kind() == 0xf4 && /*bf6e*/
        !P_CONTEXT.IsComplete()) /*86382*/
        report_compiler_error(0xc9);
    ClearRecovery();
    break;
case 164: case 176:
    report_language_diagnostic(1, 0xe4);
    break;
case 165:
case 166:
    parser_lookahead = recover_parser_construct(parser_lookahead, 0x7f, 0x7b, 1);
    if (parser_lookahead->Kind() != 0x7f) {
        defer_parser_token(parser_lookahead);
        parser_lookahead = canonical_parser_token(0x7f);
    }
    ClearRecovery();
    break;
case 167:
    symbol = R(0).declaration_value.symbol;
    if (symbol) {
        symbol->SetKind(3); /*fbed*/
        --declaration_nesting;
        R(0).declaration_value.declarator->ApplyDeclaration(current_declaration()); /*d7ce*/
        R(0).declaration_value.declarator->Resolve();
        if (!declare_initialized_symbol(current_declaration(), symbol,
                R(0).declaration_value.declarator, NULL))
            frontend_data.translation_unit->SetScopeMode(1);
        if (R(0).declaration_value.declarator)
            R(0).declaration_value.declarator->Finish(1); /*dfc6, genuine return-this leaf*/
        RESULT.symbol = symbol;
    }
    break;
case 168:
    require_integral_constant(resolve_parser_expression(R(0).expression)); /*45e2f/79f0*/
    symbol = R(-2).declaration_value.symbol;
    symbol->SetKind(0x13);
    --declaration_nesting;
    R(-2).declaration_value.declarator->ApplyDeclaration(current_declaration());
    R(-2).declaration_value.declarator->Resolve();
    if (!declare_initialized_symbol(current_declaration(), symbol,
            R(-2).declaration_value.declarator, NULL))
        frontend_data.translation_unit->SetScopeMode(1);
    if (R(-2).declaration_value.declarator)
        R(-2).declaration_value.declarator->Finish(1);
    RESULT.symbol = symbol;
    break;
case 169:
    symbol = R(-2).declaration_value.symbol;
    symbol->SetKind(2);
    ((ParserBitfieldSymbol *)symbol)->width = prepare_bitfield(symbol, R(0).integer); /*45170*/
    --declaration_nesting;
    R(-2).declaration_value.declarator->ApplyDeclaration(current_declaration());
    R(-2).declaration_value.declarator->Resolve();
    if (!declare_initialized_symbol(current_declaration(), symbol,
            R(-2).declaration_value.declarator, NULL))
        frontend_data.translation_unit->SetScopeMode(1);
    if (R(-2).declaration_value.declarator)
        R(-2).declaration_value.declarator->Finish(1);
    RESULT.symbol = symbol;
    break;
case 170:
    RESULT.symbol = make_abstract_declarator();
    RESULT.symbol->SetKind(2);
    RESULT.symbol->SetName(frontend_data.builtin_names.alignment_member); /*374b7/cc45c owner*/
    RESULT.symbol->MarkDefined(); /*8653a*/
    ((ParserBitfieldSymbol *)RESULT.symbol)->width = prepare_bitfield(RESULT.symbol, R(0).integer);
    local_declarator.InitializeSymbol(RESULT.symbol);
    local_declarator.ApplyDeclaration(current_declaration());
    local_declarator.Resolve();
    declare_initialized_symbol(current_declaration(), RESULT.symbol, &local_declarator, NULL);
    local_declarator.Reset(); /*real1300 physical alias*/
    break;
case 171:
    frontend_data.translation_unit->SetScopeMode(1);
    RESULT.symbol = R(0).symbol;
    /* Keep the four genuine Alias calls. Its pending body is not silently
     * treated as a pure expression or replaced by a cached result. */
    if (RESULT.symbol->Alias() &&
        ((ParserSymbolView *)RESULT.symbol->Alias())->binding->owner->scope &&
        ((ParserSymbolView *)RESULT.symbol->Alias())->binding)
        process_using_member(RESULT.symbol->Alias()); /*aeb4a*/
    else compiler_warning(0x369, RESULT.symbol);
    break;
case 172:
    process_using_member(R(0).declaration.object);
    break;
case 173:
    if (parser_lookahead->Kind() == 0x36 &&
        is_special_member_name(R(0).declaration_value.symbol->Name())) { /*25699*/
        symbol = R(0).declaration_value.symbol;
        symbol->SetKind(3);
        --declaration_nesting;
        R(0).declaration_value.declarator->ApplyDeclaration(current_declaration());
        R(0).declaration_value.declarator->Resolve();
        parser_lookahead = defer_parser_token(parser_lookahead);
        if (!declare_initialized_symbol(current_declaration(), symbol,
                R(0).declaration_value.declarator, NULL))
            frontend_data.translation_unit->SetScopeMode(1);
        if (R(0).declaration_value.declarator)
            R(0).declaration_value.declarator->Finish(1);
        RESULT.declaration_value.symbol = NULL; /*firstDWORD unchanged*/
    }
    break;
case 177:
    --declaration_nesting;
    R(0).declaration_value.declarator->ApplyDeclaration(current_declaration());
    R(0).declaration_value.declarator->Resolve();
    if (!(frontend_data.compiler_mode_flags & 0x40) &&
        !(frontend_data.source_mode_flags & 0x00200000))
        declare_parameter_symbol(current_declaration(), R(0).declaration_value.symbol,
            R(0).declaration_value.declarator);
    if (R(0).declaration_value.declarator)
        R(0).declaration_value.declarator->Finish(1);
    frontend_data.translation_unit->SetScopeMode(1);
    break;
case 178:
    if (!suppress_parser_codegen && !ignore_initialized_declaration(R(-1).declaration_value.symbol)) { /*44569*/
        --declaration_nesting;
        R(-1).declaration_value.symbol->attributes |= 4;
        R(-1).declaration_value.declarator->ApplyDeclaration(current_declaration());
        R(-1).declaration_value.declarator->Resolve();
        R(-1).declaration_value.symbol = declare_builtin(current_declaration(),
            R(-1).declaration_value.symbol, R(-1).declaration_value.declarator);
        if (R(-1).declaration_value.declarator)
            R(-1).declaration_value.declarator->Finish(1);
        R(-1).declaration_value.symbol = prepare_symbol_initializer(R(-1).declaration_value.symbol); /*1bb94*/
        if (R(-1).declaration_value.symbol) {
            begin_symbol_initializer(R(-1).declaration_value.symbol); /*17d9b*/
            initialize_symbol(R(-1).declaration_value.symbol, R(0).expression); /*1bdf1*/
            frontend_data.translation_unit->SetScopeMode(1);
            finish_symbol_initializer(R(-1).declaration_value.symbol); /*1bcb2*/
        }
    }
    parser_token_text_cursor = compiler_token_text;
    break;
case 179:
    if (!suppress_parser_codegen && R(-1).symbol) {
        if (R(0).expression && R(0).expression->kind == 0xee)
            R(0).expression = expand_initializer_expression(1, R(0).expression, 0); /*9fe00*/
        begin_symbol_initializer(R(-1).symbol);
        frontend_data.compiler_mode_flags |= 0x00080000;
        frontend_data.source_mode_flags |= 0x00080000;
        initialize_symbol(R(-1).symbol, resolve_parser_expression(R(0).expression));
        frontend_data.compiler_mode_flags &= ~0x00080000U;
        frontend_data.source_mode_flags &= ~0x00080000U;
        frontend_data.translation_unit->SetScopeMode(1);
        finish_symbol_initializer(R(-1).symbol);
    }
    parser_token_text_cursor = compiler_token_text;
    break;
case 180:
    if (!suppress_parser_codegen) {
        begin_initializer_list(R(0).symbol, 0); /*2e127*/
        frontend_data.source_mode_flags |= 0x00080000;
        initializer_started = 1; /*CE094 source-only initialized owner*/
    }
    break;
case 181:
    finish_initializer_list(R(-5).symbol); /*2df90*/
    frontend_data.translation_unit->SetScopeMode(1);
    frontend_data.source_mode_flags &= ~0x00080000U;
    if (!suppress_parser_codegen) finish_symbol_initializer(R(-5).symbol);
    break;
case 182:
    R(-1).declaration_value.declarator->ApplyDeclaration(current_declaration());
    R(-1).declaration_value.declarator->Resolve();
    --declaration_nesting;
    if (!suppress_parser_codegen) {
        R(-1).declaration_value.symbol->attributes |= 4;
        symbol = declare_builtin(current_declaration(), R(-1).declaration_value.symbol,
            R(-1).declaration_value.declarator);
        if (R(-1).declaration_value.declarator)
            R(-1).declaration_value.declarator->Finish(1);
        RESULT.symbol = prepare_symbol_initializer(symbol);
        (*declaration_control_row(parser_context_depth)).flags |= 2;
    }
    break;
case 183:
    compiler_warning(0x201, 0x54, current_declaration()->type);
    RESULT.pointer = NULL;
    break;
case 186:
    if (!suppress_parser_codegen)
        process_initializer_expression(resolve_parser_expression(R(0).expression), 0); /*2d904*/
    parser_token_text_cursor = compiler_token_text;
    break;
case 187:
    if (!suppress_parser_codegen) begin_initializer_element(); /*40654*/
    break;
case 188:
    if (!suppress_parser_codegen) finish_initializer_element(); /*2e089*/
    break;
case 189: case 248:
    ++declaration_nesting;
    break;
case 190:
    RESULT.declaration_value.declarator->AddFunction(R(0).parameter_description);
    break;
case 191:
    R(0).declaration_value.declarator->AppendModifier(&R(-1).modifier); /*d739*/
    RESULT.declaration_value = R(0).declaration_value;
    break;
case 192:
    RESULT.declaration_value.declarator->AddArrayBound(&R(-1).integer); /*1eea1*/
    break;
case 193:
    RESULT.declaration_value = R(-1).declaration_value;
    if (parser_context_depth > 0) --parser_context_depth;
    break;
case 194:
    R(0).declaration_value.declarator->ApplyQualifiers(&R(-1).qualifiers);
    RESULT.declaration_value = R(0).declaration_value;
    break;
case 196:
    symbol = R(0).symbol;
    memory = allocate_declarator(72); /*d70f, real always72B allocator, supplied formal retained*/
    declarator = memory ? ((CompilerDeclarator *)memory)->InitializeSymbol(symbol) : NULL;
    if (symbol->type_flags & 0x40)
        declarator->ApplyParserQualifiers(&parser_special_qualifiers); /*45a9a, C0B18 owner*/
    RESULT.declaration_value.declarator = declarator;
    RESULT.declaration_value.symbol = symbol;
    break;
case 197:
    RESULT.symbol = lookup_builtin(R(0).name);
    if (is_current_declaration_class(RESULT.symbol)) RESULT.symbol->type_flags |= 0x10;
    ((CompilerRuntimeFunction *)RESULT.symbol)->function_class = 1;
    break;
case 198:
    RESULT.symbol = lookup_builtin(((ParserNameReference *)R(0).reference)->Identifier());
    if (is_current_declaration_class(RESULT.symbol)) RESULT.symbol->type_flags |= 0x10;
    ((CompilerRuntimeFunction *)RESULT.symbol)->function_class = 1;
    break;
case 200:
    RESULT.symbol = lookup_builtin(R(0).name);
    if (is_current_declaration_class(RESULT.symbol)) RESULT.symbol->type_flags |= 0x20;
    else if (frontend_data.declaration_context_index >= 0) {
        compiler_warning(0x20b, class_scopes[frontend_data.declaration_context_index].symbol, R(0).name);
        RESULT.symbol->SetName(current_class_name()); /*25b87*/
        RESULT.symbol->type_flags |= 0x20;
    } else compiler_warning(0x24c, RESULT.symbol->Name());
    ((CompilerRuntimeFunction *)RESULT.symbol)->function_class = 1;
    break;
case 201:
    RESULT.symbol = lookup_builtin(((ParserNameReference *)R(0).reference)->Identifier());
    if (is_current_declaration_class(RESULT.symbol)) RESULT.symbol->type_flags |= 0x20;
    else if (frontend_data.declaration_context_index >= 0) {
        compiler_warning(0x20b, class_scopes[frontend_data.declaration_context_index].symbol, R(0).pointer);
        RESULT.symbol->SetName(current_class_name());
        RESULT.symbol->type_flags |= 0x20;
    } else compiler_warning(0x24c, RESULT.symbol->Name());
    ((CompilerRuntimeFunction *)RESULT.symbol)->function_class = 1;
    break;
case 202:
    symbol = lookup_builtin(R(0).named_token.name);
    ((CompilerRuntimeFunction *)symbol)->function_class = R(0).named_token.kind;
    /* This one-value reduction aliases RESULT and R(0). Preserve the
     * operator BYTE until its actual consumer, before publishing the pointer. */
    RESULT.symbol = symbol;
    break;
case 203:
    memcpy(&parser_special_qualifiers, &R(0).declaration, 40);
    RESULT.symbol = lookup_builtin(make_conversion_name(parser_special_qualifiers.type)); /*459b1*/
    ((CompilerRuntimeFunction *)RESULT.symbol)->function_class = 0x8f;
    RESULT.symbol->type_flags |= 0x40;
    frontend_data.compiler_flags &= ~4U;
    break;
case 204:
    if (!R(0).reference) break;
    symbol = R(0).reference->Reference();
    owner = P_SYMBOL(symbol)->binding->owner;
    declared_symbol = owner->symbol; /*actual owner+8, source-only view*/
    alias_symbol = lookup_builtin(symbol->Name());
    alias_symbol->SetAlias(symbol); /*20e2c*/
    prepare_qualified_scope(P_SYMBOL(symbol)->binding->owner); /*20e41*/
    if (symbol->type_flags & 0x40) {
        if (declared_symbol->kind == 0x11)
            declared_symbol = select_function_candidate(declared_symbol); /*31d08*/
        parser_special_qualifiers.type = intern_type(get_function_return_type(P_SYMBOL(declared_symbol)->type));
        memcpy(&parser_special_qualifiers.context,
            get_symbol_declaration_context(&local_context, declared_symbol), 28); /*863ab*/
        alias_symbol->type_flags |= 0x40;
    } else if (symbol->type_flags & 0x10) alias_symbol->type_flags |= 0x10;
    else if (symbol->type_flags & 0x20) alias_symbol->type_flags |= 0x20;
    /* The original recognizes this explicit function-like kind set; it
     * has no nested dispatch table for these membership tests. */
    if (!PARSER_FUNCTION_KIND(declared_symbol->kind)) {
        if (declared_symbol->kind != 0x18 ||
            !PARSER_FUNCTION_KIND(((CompilerSymbol *)P_SYMBOL(declared_symbol)->binding->owner)->kind)) {
            RESULT.symbol = alias_symbol;
            goto action_done;
        }
    }
    scope_symbol = P_SYMBOL(symbol)->binding->owner->scope;
    if (scope_symbol) {
        owner_symbol = scope_symbol->symbol; /*actual inline owner+8 read, no helper call*/
        if (owner_symbol->kind == 0x0f)
            note_qualified_scope(P_SYMBOL(owner_symbol)->type_index); /*8639b*/
        if (owner_symbol->kind == 4) {
            alias_symbol->SetDeclarationQualifier(P_SYMBOL(owner_symbol)->class_detail->scope_number); /*20e83*/
            if (frontend_data.source_mode_flags & 0x00200000) alias_symbol->SetKind(3);
        }
    }
    if (declared_symbol->kind == 0x10 || declared_symbol->kind == 0x11 || declared_symbol->kind == 0x19)
        function = (CompilerRuntimeFunction *)select_function_candidate(declared_symbol);
    else function = (CompilerRuntimeFunction *)declared_symbol;
    ((CompilerRuntimeFunction *)alias_symbol)->function_class = function->function_class;
    RESULT.symbol = alias_symbol;
    break;
case 205:
    if (R(-1).reference) {
        symbol = R(-1).reference->Reference();
        memcpy(&parser_special_qualifiers, &R(0).declaration, 40);
        apply_conversion_type(symbol, parser_special_qualifiers.type); /*4590b*/
        if (!P_SYMBOL(symbol)->binding) {
            RESULT.symbol = lookup_builtin(make_anonymous_conversion_name()); /*36d2a*/
        } else {
            alias_symbol = lookup_builtin(symbol->Name());
            alias_symbol->SetAlias(symbol);
            prepare_qualified_scope(P_SYMBOL(symbol)->binding->owner);
            ((CompilerRuntimeFunction *)alias_symbol)->function_class = 0x8f;
            alias_symbol->type_flags |= 0x40;
            if (frontend_data.source_mode_flags & 0x00200000) alias_symbol->SetKind(3);
            RESULT.symbol = alias_symbol;
        }
    }
    frontend_data.compiler_flags &= ~4U;
    break;
case 206:
    initialize_type_specifier(&RESULT.declaration, NULL); /*4594b*/
    RESULT.declaration.context.class_flags &= ~0x80;
    break;
case 207:
    initialize_type_specifier(&RESULT.declaration, R(0).type);
    RESULT.declaration.context.class_flags &= ~0x80;
    break;
case 208:
    R(-2).parameter_description->SetQualifiers(&R(-1).qualifiers); /*e00c*/
    R(-2).parameter_description->SetDeclarationMode(R(0).unsigned_value); /*e01e*/
    break;
case 209:
    RESULT.parameter_description = make_empty_function_parameters(); /*1e271*/
    if (parser_context_depth > 0) --parser_context_depth;
    break;
case 210:
    ++frontend_bss.declaration_depth;
    ++frontend_bss.declaration_scope_mode;
    frontend_data.translation_unit->BeginParameterScope(0); /*d7f3*/
    break;
case 211:
    RESULT.parameter_description = R(-2).parameter_description;
    if (R(-1).symbol)
        RESULT.parameter_description->AppendSymbol(R(-1).symbol);
    --frontend_bss.declaration_depth;
    --frontend_bss.declaration_scope_mode;
    if (parser_context_depth > 0) --parser_context_depth;
    break;
case 212:
    arena_kind = 0;
    if (frontend_bss.declaration_scope_mode && frontend_data.declaration_context_index < 0 &&
        !(frontend_data.compiler_mode_flags & 0x10000000) &&
        !((frontend_data.compiler_mode_flags & 0x40000000) &&
          !(frontend_data.source_mode_flags & 8) && !(frontend_data.compiler_flags & 0x200)) &&
        !(frontend_data.source_mode_flags & 0x50))
        arena_kind = 4;
    symbol = allocate_parser_symbol(11, frontend_data.builtin_names.ellipsis, arena_kind); /*46520/cc478*/
    symbol->MarkParameter(); /*465c9*/
    memory = allocate_parameter_description(28); /*fdb4*/
    RESULT.parameter_description = memory ?
        ((CompilerParameterDescription *)memory)->Initialize(3, NULL) : NULL; /*fdda*/
    RESULT.parameter_description->AppendSymbol(symbol);
    if (parser_context_depth > 0) --parser_context_depth;
    break;
case 214:
    if (R(0).unsigned_value == 2) report_language_diagnostic(3, 0x122);
    RESULT.unsigned_value = R(0).unsigned_value;
    break;
case 215:
    memory = allocate_parameter_description(28);
    RESULT.parameter_description = memory ?
        ((CompilerParameterDescription *)memory)->Initialize(3,
            ((CompilerAuxiliaryScope *)frontend_data.translation_unit->CurrentScope())->ParameterTable()) : NULL; /*e008/fdd6*/
    break;
case 217:
    if (!parser_lookahead) parser_lookahead = P_CONTEXT.ReadToken();
    if (parser_lookahead->Kind() == 0x7f) {
        defer_parser_token(parser_lookahead);
        parser_lookahead = canonical_parser_token(0x7e);
        compiler_warning(0xeb);
    }
    (*declaration_control_row(parser_context_depth)).flags |= 8;
    break;
case 218:
    /* Two encountered deferred-context cells, before indexed addressing. */
    if (R(0).unsigned_value && (parser_context_depth < 1 || parser_context_depth > 2))
        { __asm { int 3 } }
    if (R(0).unsigned_value && deferred_contexts[parser_context_depth - 1]) {
        begin_deferred_input(0); /*45de6*/
        frontend_data.source_mode_flags |= 0x10;
        expression_list = append_expression_list(NULL, NULL, 2);
        memory = allocate_parser_record(20, 0); /*36350*/
        deferred = memory ? ((ParserDeferredListItem *)memory)->Initialize(expression_list, 0, 1) : NULL; /*3637c*/
        deferred_contexts[parser_context_depth - 1]->Prepend(deferred); /*4263f*/
        frontend_data.source_mode_flags &= ~0x10U;
        if (frontend_data.compiler_mode_flags & 0x10000000)
            frontend_data.compiler_mode_flags &= ~0x01000000U;
        finish_deferred_input(); /*363e4*/
    }
    (*declaration_control_row(parser_context_depth)).flags &= ~2;
    break;
case 219:
    if (P_CONTEXT.HasSourceMark()) /*36344*/
        frontend_bss.debug_module->Slota0(P_CONTEXT.Position(), 0);
    if (R(-1).symbol && R(0).expression) {
        expression = resolve_parser_expression(R(0).expression);
        if (frontend_data.compiler_mode_flags & 0x40000000)
            expression = instantiate_parser_expression(expression, 0); /*1a2d8*/
        check_initializer_type(expression, P_SYMBOL(R(-1).symbol)->type); /*21b2e*/
        R(0).expression = convert_typed_expression(P_SYMBOL(R(-1).symbol)->type,
            make_typed_expression(0xcc, P_SYMBOL(R(-1).symbol)->type, expression, 0)); /*12ca7/11ee0*/
        expression_list = append_expression_list(NULL,
            mark_parser_expression(R(0).expression), 2); /*21b51*/
        memory = allocate_parser_record(20, 0);
        deferred = memory ? ((ParserDeferredListItem *)memory)->Initialize(expression_list, 0, 1) : NULL;
        /* Only the two encountered BF4B4/BF4B8 owners are modeled. */
        if (parser_context_depth < 1 || parser_context_depth > 2) { __asm { int 3 } }
        if (deferred_contexts[parser_context_depth - 1])
            deferred_contexts[parser_context_depth - 1]->Prepend(deferred);
        else {
            memory = allocate_parser_record(12, 0);
            deferred_contexts[parser_context_depth - 1] = memory ?
                ((ParserDeferredList *)memory)->Initialize(deferred) : NULL; /*363bc*/
        }
    }
    goto finish_deferred_action;
case 220:
    if (P_CONTEXT.HasSourceMark())
        frontend_bss.debug_module->Slota0(P_CONTEXT.Position(), 0);
    if (!(frontend_data.compiler_mode_flags & 0x10000000))
        compiler_warning(0x3e, P_SYMBOL(R(0).symbol)->payload);
    memory = allocate_parser_record(20, 0);
    deferred = memory ? ((ParserDeferredPointer *)memory)->Initialize(P_SYMBOL(R(0).symbol)->payload, 0) : NULL; /*864c5*/
    if (deferred_contexts[parser_context_depth - 1])
        deferred_contexts[parser_context_depth - 1]->Prepend(deferred);
    else {
        memory = allocate_parser_record(12, 0);
        deferred_contexts[parser_context_depth - 1] = memory ?
            ((ParserDeferredList *)memory)->Initialize(deferred) : NULL;
    }
    finish_deferred_declaration();
    goto finish_deferred_action;
case 221:
    if (P_CONTEXT.HasSourceMark())
        frontend_bss.debug_module->Slota0(P_CONTEXT.Position(), 0);
    expression = (CompilerExpression *)frontend_bss.arenas[frontend_data.constant_arena_index].Allocate(24);
    expression->kind = 0xcc;
    expression->flags = 0;
    expression->value.pointer = R(0).pointer;
    expression_list = append_expression_list(NULL, expression, 2);
    memory = allocate_parser_record(20, 0);
    deferred = memory ? ((ParserDeferredListItem *)memory)->Initialize(expression_list, 0, 1) : NULL;
    if (deferred_contexts[parser_context_depth - 1])
        deferred_contexts[parser_context_depth - 1]->Prepend(deferred);
    else {
        memory = allocate_parser_record(12, 0);
        deferred_contexts[parser_context_depth - 1] = memory ? ((ParserDeferredList *)memory)->Initialize(deferred) : NULL;
    }
    finish_deferred_declaration();
    goto finish_deferred_action;
case 222:
    if (P_CONTEXT.HasSourceMark())
        frontend_bss.debug_module->Slota0(P_CONTEXT.Position(), 0);
    if (parser_lookahead->Kind() == 0x75) {
        compiler_warning(0x117);
        recover_parser_construct(P_CONTEXT.ReadToken(), 0x7b, 1);
        parser_lookahead = NULL;
    } else parser_lookahead = recover_parser_construct(parser_lookahead, 0x7e, 0x37, 1);
    ClearRecovery();
finish_deferred_action:
    frontend_data.source_mode_flags &= ~0x10U;
    if (frontend_data.compiler_mode_flags & 0x10000000)
        frontend_data.compiler_mode_flags &= ~0x01000000U;
    finish_deferred_input();
    (*declaration_control_row(parser_context_depth)).flags &= ~2;
    break;
case 223:
    if (frontend_data.declaration_context_index >= 0 ||
        (frontend_data.compiler_mode_flags & 0x10000000)) {
        frontend_data.compiler_mode_flags |= 0x08000000;
        begin_declaration_scope();
    } else {
        frontend_data.source_mode_flags |= 0x10;
        begin_deferred_input(0);
    }
    prepare_declaration_lookup();
    break;
case 224:
    if (P_CONTEXT.HasSourceMark())
        frontend_bss.debug_module->Slot9c(P_CONTEXT.Position(), 0);
    break;
case 226:
    frontend_data.translation_unit->InsertCurrentScope(R(0).symbol); /*244c0*/
    break;
case 227:
    RESULT.pointer = NULL;
    if (!(frontend_data.compiler_mode_flags & 0x30000000)) {
        parser_lookahead = recover_parser_construct(P_CONTEXT.ReadToken(),
            0x7f, 0x7e, 0x75, 1);
        P_CONTEXT.UnreadToken(parser_lookahead);
    } else {
        if (!parser_lookahead || parser_lookahead->Kind() != 0x3e)
            parser_lookahead = P_CONTEXT.ReadToken();
        parser_lookahead = recover_parser_construct(parser_lookahead, 0x3e, 0x7b, 0x7f, 1);
        P_CONTEXT.UnreadToken(parser_lookahead);
        if (parser_lookahead->Kind() == 0x7f)
            P_CONTEXT.UnreadToken(canonical_parser_token(0x3e));
    }
    ClearRecovery();
    parser_lookahead = NULL;
    break;
case 228:
#ifdef __linux__
    memcpy(&local_declaration, &R(-1).declaration, 36);
    local_declaration.callback = R(-1).declaration.callback;
#else
    memcpy(&local_declaration, &R(-1).declaration, 40);
#endif
    RESULT.symbol = R(0).declaration_value.symbol;
    RESULT.symbol->SetKind(11);
    R(0).declaration_value.declarator->ApplyDeclaration(&local_declaration);
    R(0).declaration_value.declarator->Resolve();
    merge_symbol_declaration(&local_declaration, RESULT.symbol, R(0).declaration_value.declarator);
    if (R(0).declaration_value.declarator) R(0).declaration_value.declarator->Finish(1);
    if (frontend_data.compiler_mode_flags & 0x10000000) {
        type = P_SYMBOL(RESULT.symbol)->type;
        if (((type->flags & 0x408) == 0x408 || (type->flags & 0x402) == 0x402) &&
            !type->shape && !(((CompilerIrSymbol *)RESULT.symbol)->storage_class & 0x80)) {
            compiler_warning(0x3e1, RESULT.symbol);
            RESULT.symbol->scope_flags |= 4;
        }
    }
    --declaration_nesting;
    frontend_data.translation_unit->SetScopeMode(1);
    (*declaration_control_row(parser_context_depth)).flags &= ~3;
    break;
/* Production229 is drafted separately below its shared member-context
 * and formal-parameter naming path; it is not a new source helper. */
case 230: case 237:
    RESULT.qualifiers.Clear(); /*d539*/
    break;
case 233: case 240:
    RESULT.qualifiers.Merge(&R(0).qualifiers); /*864f8*/
    break;
case 234:
    report_language_diagnostic(1, 0xe6);
    break;
case 235: case 241:
    RESULT.qualifiers.Set(parser_token_semantic_codes[R(0).token_kind], 0); /*24526*/
    break;
case 236:
    RESULT.declaration_value = R(-1).declaration_value;
    if (parser_context_depth > 0) --parser_context_depth;
    frontend_data.compiler_flags &= ~0x80000000U;
    if (frontend_data.compiler_mode_flags & 0x10)
        (*declaration_control_row(parser_context_depth)).flags &= ~3;
    break;
case 242:
    RESULT.qualifiers.Set(R(0).word_value, 0);
    break;
case 243:
    if ((R(0).declaration.context.class_flags & 0x30) == 0x30)
        RESULT.qualifiers.Set(R(0).declaration.type->attributes, 0);
    else {
        compiler_warning(0x3e, R(0).declaration.type);
        RESULT.qualifiers.Clear();
    }
    break;
case 244:
    RESULT.qualifiers.Set(3, extract_type_qualifier(R(0).pointer)); /*86609*/
    break;
case 245:
    if (!process_type_qualifier()) RESULT.qualifiers.Clear(); /*2b90d*/
    break;
case 246:
    RESULT.qualifiers.Set(1, 0);
    break;
case 247:
    RESULT.qualifiers.Clear();
    ClearRecovery();
    parser_lookahead = NULL;
    break;
case 250:
    RESULT.declaration_value.symbol = make_abstract_declarator();
    memory = allocate_declarator(72);
    RESULT.declaration_value.declarator = memory ?
        ((CompilerDeclarator *)memory)->InitializeSymbol(RESULT.declaration_value.symbol) : NULL;
    break;
case 229:
    symbol = R(0).declaration_value.symbol;
    renamed_parameter = 0;
    P_SYMBOL(symbol)->encoded_index = (P_SYMBOL(symbol)->encoded_index & 0xff000000U) |
        (allocate_symbol_record_id() & 0x00ffffffU); /*37497*/
    symbol->SetKind(12);
    symbol->SetName(frontend_data.builtin_names.formal_parameter);
    if ((frontend_data.compiler_mode_flags & 0x10000000) &&
        R(-1).declaration.object && R(-1).declaration.object->kind == 4 &&
        ((R(-1).declaration.type->flags & 0x408) == 0x408) &&
        !(R(-1).declaration.context.class_flags & 0x80) &&
        !R(-1).declaration.object->DeclarationQualifier()) { /*8652e*/
        renamed_parameter = 1;
        name = R(-1).declaration.object->Name();
        if (name->flags & 0x02000000) {
            scope_lookup = frontend_data.translation_unit->LookupName(name, 0x20);
            compiler_warning(0x3af, scope_lookup ? scope_lookup->Reference() : NULL);
        }
        symbol->SetName(name);
    }
    symbol->MarkReferenced();
    R(-1).declaration.context.class_flags &= ~0x80;
    set_symbol_declaration_context(symbol, R(-1).declaration.context); /*374c1 genuine28B value*/
    R(0).declaration_value.declarator->ApplyDeclaration(&R(-1).declaration);
    R(0).declaration_value.declarator->Resolve();
    finalize_symbol_declaration(R(0).declaration_value.declarator, symbol);
    if (R(0).declaration_value.declarator) R(0).declaration_value.declarator->Finish(1);
    if (renamed_parameter && P_SYMBOL(symbol)->type->shape) {
        compiler_warning(0x395, symbol->Name());
        symbol->scope_flags |= 4;
    }
    --declaration_nesting;
    (*declaration_control_row(parser_context_depth)).flags &= ~3;
    RESULT.symbol = symbol;
    break;
case 251:
    symbol = make_abstract_declarator();
    memory = allocate_declarator(72);
    declarator = memory ? ((CompilerDeclarator *)memory)->InitializeSymbol(symbol) : NULL;
    declarator->ApplyQualifiers(&R(0).qualifiers);
    RESULT.declaration_value.declarator = declarator;
    RESULT.declaration_value.symbol = symbol;
    break;
case 252:
    symbol = make_abstract_declarator();
    memory = allocate_declarator(72);
    declarator = memory ? ((CompilerDeclarator *)memory)->InitializeSymbol(symbol) : NULL;
    declarator->AddFunction(R(0).parameter_description);
    RESULT.declaration_value.declarator = declarator;
    RESULT.declaration_value.symbol = symbol;
    break;
case 253:
    symbol = make_abstract_declarator();
    memory = allocate_declarator(72);
    declarator = memory ? ((CompilerDeclarator *)memory)->InitializeSymbol(symbol) : NULL;
    declarator->ApplyQualifiers(&R(-1).qualifiers);
    declarator->AddFunction(R(0).parameter_description);
    RESULT.declaration_value.declarator = declarator;
    RESULT.declaration_value.symbol = symbol;
    break;
case 254: case 260: case 443:
    R(0).declaration_value.declarator->AppendModifier(&R(-1).modifier);
    RESULT.declaration_value = R(0).declaration_value;
    break;
case 255:
    RESULT.declaration_value.declarator->AddArrayExpression(R(-1).expression); /*47b83*/
    break;
case 256:
    RESULT.declaration_value.declarator->AddFunction(R(0).parameter_description);
    break;
case 258:
    RESULT.declaration_value = R(-1).declaration_value;
    if (parser_context_depth > 0) --parser_context_depth;
    break;
case 259:
    memory = allocate_declarator(72);
    declarator = memory ? ((CompilerDeclarator *)memory)->InitializeSymbol(NULL) : NULL;
    declarator->AppendModifier(&R(0).modifier);
    RESULT.declaration_value.declarator = declarator;
    RESULT.declaration_value.symbol = NULL;
    break;
case 261:
    memcpy(&local_qualifiers, &R(-2).qualifiers, 8); /*two genuine DWORD loads/stores, including opaque gap*/
    RESULT.modifier.Pointer(&local_qualifiers, &R(0).qualifiers); /*d590*/
    break;
case 262:
    RESULT.modifier.Pointer(local_qualifiers.Initialize(0), &R(0).qualifiers); /*d541*/
    break;
case 263:
    memcpy(&local_qualifiers, &R(-2).qualifiers, 8);
    RESULT.modifier.Reference(&local_qualifiers, &R(0).qualifiers); /*3d241*/
    break;
case 264:
    RESULT.modifier.Reference(local_qualifiers.Initialize(0), &R(0).qualifiers);
    break;
case 265:
    local_qualifiers = R(-2).qualifiers;
    RESULT.modifier.MemberPointer(R(-1).pointer, &local_qualifiers, &R(0).qualifiers); /*40a73*/
    break;
case 267:
case 268:
    arena_kind = 0;
    if (frontend_bss.declaration_scope_mode && frontend_data.declaration_context_index < 0 &&
        !(frontend_data.compiler_mode_flags & 0x10000000) &&
        !((frontend_data.compiler_mode_flags & 0x40000000) &&
          !(frontend_data.source_mode_flags & 8) && !(frontend_data.compiler_flags & 0x200)) &&
        !(frontend_data.source_mode_flags & 0x50))
        arena_kind = 4;
    RESULT.symbol = allocate_parser_symbol(11, frontend_data.builtin_names.ellipsis, arena_kind);
    RESULT.symbol->MarkParameter();
    break;
case 269:
    if (suppress_parser_codegen || (frontend_data.compiler_flags & 0x00800000))
        process_external_recovery(0);
    else {
        if (R(0).token_kind != 0xb0) begin_compound_statement(); /*5052*/
        (*declaration_control_row(parser_context_depth)).flags &= ~3;
        emit_statement_location(); /*5236*/
    }
    break;
case 270:
    if (suppress_parser_codegen || (frontend_data.compiler_flags & 0x00800000))
        frontend_data.compiler_flags &= ~0x00800000U;
    else if (R(-2).token_kind != 0xb0) finish_compound_statement(); /*5558*/
    break;
case 274:
    frontend_data.compiler_flags &= ~1U;
    if (!parser_lookahead) {
        if ((unsigned)parser_context_depth >= 60) { __asm { int 3 } }
        qualification_scopes[parser_context_depth] = NULL; /* same bounded depth as controls */
        (*declaration_control_row(parser_context_depth)).flags &= ~3;
    }
    break;
case 275:
    if (R(-1).expression) {
        if (R(-1).expression->kind == 0xee)
            R(-1).expression = expand_initializer_expression(0x74, R(-1).expression, 0);
        R(-1).expression = convert_parser_rvalue(resolve_parser_expression(R(-1).expression)); /*7bb6*/
        emit_statement_expression(check_statement_expression(R(-1).expression, 2)); /*183d9/181d2*/
    }
    if (!current_recovery_shifts()) select_parser_arena(5);
    parser_token_text_cursor = compiler_token_text;
    break;
case 278:
    finish_conditional_scope(R(-1).expression, 0); /*1c4f9*/
    emit_label(R(-1).expression); /*5c6d*/
    begin_else_clause(1); /*1c52f*/
    finish_conditional_scope(NULL, 1);
    break;
case 279:
    finish_conditional_scope(R(-1).expression, 0);
    emit_label(R(-1).expression);
    finish_if_statement(); /*1d09d*/
    finish_conditional_scope(NULL, 1);
    break;
case 280:
    finish_condition_expression(); /*43804*/
    process_condition_lookahead(parser_lookahead); /*3f8ab*/
    finish_conditional_scope(NULL, R(-1).scope_open);
    break;
case 281:
    if (frontend_data.options.language_extensions) { /*CD094 existing language_extensions owner*/
        finish_conditional_scope(NULL, R(-1).scope_open);
        prepare_loop_condition(); /*43fba*/
    }
    process_condition_lookahead(parser_lookahead);
    if (!frontend_data.options.language_extensions) {
        prepare_loop_condition();
        finish_conditional_scope(NULL, R(-1).scope_open);
    }
    break;
case 282:
    begin_for_statement(); /*46909*/
    begin_for_condition(); /*46981*/
    (*declaration_control_row(parser_context_depth)).flags &= ~3;
    R(0).scope_open = begin_statement_scope(NULL); /*1c255, actual return1*/
    break;
case 283:
    finish_conditional_scope(NULL, R(-4).scope_open);
    finish_for_condition(); /*469c0*/
    (*declaration_control_row(parser_context_depth)).flags |= 3;
    break;
case 284:
    emit_label(continue_labels->expression);
    if (R(-2).expression) {
        R(-2).expression = resolve_parser_expression(R(-2).expression);
        R(-2).expression = convert_condition(R(-2).expression, 0); /*1c2fa*/
        emit_conditional_branch(R(-2).expression, loop_labels->expression, 1); /*1979e*/
    }
    emit_statement_location();
    emit_label(break_labels->expression);
    loop_labels = loop_labels->next;
    continue_labels = continue_labels->next;
    break_labels = break_labels->next;
    label_stack_count -= 3;
    if (label_stack_count == 0) select_parser_arena(6);
    if (!current_recovery_shifts()) select_parser_arena(5);
    parser_token_text_cursor = compiler_token_text;
    finish_for_statement(); /*46a50*/
    if (parser_context_depth > 0) --parser_context_depth;
    break;
case 285:
    emit_statement_location();
    write_ir_record(0x3a, break_labels->expression);
    finish_switch_expression(); /*3f4f4*/
    finish_switch_scope(); /*3f592*/
    finish_conditional_scope(NULL, R(-1).scope_open);
    loop_labels = loop_labels->next;
    break_labels = break_labels->next;
    switch_labels = switch_labels->next;
    default_labels = default_labels->next;
    label_stack_count -= 4;
    if (label_stack_count == 0) select_parser_arena(6);
    break;
case 286:
    prepare_case_statement(); /*8c9a8*/
    break;
case 287:
    if (break_labels) {
        prepare_break_statement(); /*3f72d*/
        write_ir_record(0x3a, break_labels->expression);
    } else compiler_warning(0x2b);
    break;
case 288:
    if (continue_labels) {
        prepare_continue_statement(); /*4e723*/
        write_ir_record(0x3a, continue_labels->expression);
    } else compiler_warning(0x2c);
    break;
case 289:
    prepare_return_statement(); /*165db*/
    emit_return_statement(NULL, 1); /*5241*/
    break;
case 290:
    prepare_return_statement();
    break;
case 291:
    if (R(-1).expression && R(-1).expression->kind == 0xee)
        R(-1).expression = expand_initializer_expression(1, R(-1).expression, 0);
    R(-1).expression = resolve_parser_expression(R(-1).expression);
    emit_return_statement(R(-1).expression, 2);
    if (!current_recovery_shifts()) select_parser_arena(5);
    parser_token_text_cursor = compiler_token_text;
    break;
case 292:
    symbol = find_label_symbol(R(-1).name); /*46b7d*/
    if (symbol) note_label_reference(symbol, 1); /*46cde*/
    else {
        symbol = make_label_symbol(R(-1).name); /*46bd1*/
        emit_label_reference(symbol); /*477e*/
        note_label_reference(symbol, 0);
    }
    write_ir_record(0x3a, symbol_expression(symbol, 0));
    if (!current_recovery_shifts()) select_parser_arena(5);
    parser_token_text_cursor = compiler_token_text;
    break;
case 293:
    emit_statement_location();
    process_empty_statement(); /*496a0*/
    break;
case 294:
    process_try_statement(); /*8cb02, provisional family role*/
    break;
case 295:
    process_except_statement(); /*8ca23*/
    break;
case 296:
    process_finally_statement(); /*8cb22*/
    break;
case 298:
    compiler_warning(0xb5);
    break;
case 301:
    ClearRecovery();
    parser_lookahead = NULL;
    break;
case 302:
    ClearRecovery();
    parser_lookahead = NULL;
    P_CONTEXT.UnreadToken(canonical_parser_token(0x7b));
    break;
case 303:
    if (!parser_lookahead) {
        parser_lookahead = get_next_token();
        if (!parser_lookahead)
            report_compiler_diagnostic(1, ".\\10010613.410\\grammar.y", 0xdc1);
    }
    if (parser_lookahead->Kind() != 0x75) compiler_warning(0x13f, 0xb3);
    begin_parser_extension(); /*ad872*/
    break;
case 304:
    process_parser_extension(); /*ad928*/
    break;
case 305:
    finish_parser_extension(); /*ad950*/
    break;
case 308:
    if (!parser_lookahead) {
        parser_lookahead = get_next_token();
        if (!parser_lookahead)
            report_compiler_diagnostic(1, ".\\10010613.410\\grammar.y", 0xdd6);
    }
    if (parser_lookahead->Kind() != 0x75) compiler_warning(0x13f, 0xb0);
    else if (R(0).unsigned_value) {
        saved_parser_value.declaration.context.class_flags = 0xb0;
        saved_context = (ParserRecordedContext *)allocate_parser_storage(0x30, 4);
        parser_lookahead = saved_context ? saved_context->Read(0x75, &saved_parser_value.known) : NULL;
    }
    break;
case 309:
    set_parser_extension_type(R(-2).type); /*ad838*/
    break;
case 310:
    R(0).parameter_description->Normalize();
    RESULT.pointer = make_parser_extension_parameters(R(0).parameter_description); /*ad688*/
    release_parser_parameter_description(R(0).parameter_description); /*cdecl1/RET0 physical1300*/
    break;
case 311:
    compiler_warning(0x135);
    ClearRecovery();
    parser_lookahead = NULL;
    break;
case 314:
    if (parser_context_depth > 0) --parser_context_depth;
    RESULT.unsigned_value = 1;
    break;
case 315:
    if (parser_context_depth > 0) --parser_context_depth;
    RESULT.unsigned_value = 2;
    break;
case 316:
    begin_exception_handler(); /*8ca3e*/
    break;
case 317:
    begin_finally_handler(); /*8cabc*/
    break;
case 318:
    finish_exception_handler(); /*8cb42*/
    break;
case 319:
    set_exception_filter(R(-1).expression); /*8c9ce*/
    finish_exception_filter(); /*8cb4d*/
    if (parser_context_depth > 0) --parser_context_depth;
    break;
case 320:
    RESULT.token_kind = 0; /*only actual BYTE write*/
    break;
case 322:
    convert_parser_rvalue(R(0).expression); /*7bb6, result ignored*/
    break;
case 323:
    qualification_scopes[parser_context_depth] = NULL;
    RESULT.expression = R(0).declaration_value.symbol ?
        lookup_name_expression(R(0).declaration_value.symbol->Name()) : NULL; /*11b98*/
    break;
case 325:
    case_label = NULL; /*BF490 actual owner*/
    break;
case 326:
    frontend_data.compiler_flags &= ~1U;
    (*declaration_control_row(parser_context_depth)).flags &= ~3;
    break;
case 327:
    process_label_name(R(-1).name); /*46b1d*/
    break;
case 328:
    if (R(-1).expression && switch_labels)
        add_switch_case(resolve_parser_expression(R(-1).expression)); /*3f17c*/
    else compiler_warning(0x2e);
    break;
case 329:
    if (!default_labels) compiler_warning(0x2f);
    else if (default_labels->expression) compiler_warning(0x30);
    else {
        if (!case_label) {
            default_labels->expression = make_internal_label(0x14); /*46f3*/
            case_label = default_labels->expression;
            emit_label(default_labels->expression);
        } else default_labels->expression = case_label;
        finish_default_label(); /*4455e*/
    }
    break;
case 330:
    begin_statement_scope(NULL);
    (*declaration_control_row(parser_context_depth)).secondary_flags |= 2;
    begin_conditional_statement(); /*1c274*/
    break;
case 331:
    RESULT.expression = make_internal_label(0x1e);
    if (R(-1).expression) {
        R(-1).expression = resolve_parser_expression(R(-1).expression);
        R(-1).expression = convert_condition(R(-1).expression, 0);
        emit_conditional_branch(R(-1).expression, RESULT.expression, 0);
    }
    finish_condition_statement(); /*1c464*/
    if (parser_context_depth > 0) --parser_context_depth;
    (*declaration_control_row(parser_context_depth)).flags &= ~1;
    begin_statement_scope(RESULT.expression);
    break;
case 332:
    finish_conditional_scope(R(-2).expression, 0);
    finish_then_clause(); /*1d025*/
    expression = make_internal_label(0x1e);
    write_ir_record(0x3a, expression);
    emit_label(R(-2).expression);
    emit_statement_location();
    (*declaration_control_row(parser_context_depth)).flags &= ~1;
    begin_statement_scope(expression);
    RESULT.expression = expression;
    break;
case 333:
    if (!frontend_data.options.language_extensions)
        R(0).scope_open = begin_statement_scope(NULL);
    (*declaration_control_row(parser_context_depth)).secondary_flags |= 2;
    break;
case 334:
    if (R(-1).expression) {
        R(-1).expression = resolve_parser_expression(R(-1).expression);
        emit_statement_expression(check_statement_expression(R(-1).expression, 2));
    }
    if (!current_recovery_shifts()) select_parser_arena(5);
    parser_token_text_cursor = compiler_token_text;
    (*declaration_control_row(parser_context_depth)).flags |= 2;
    begin_loop_body(); /*43e64*/
    break;
case 335:
    R(-1).expression = convert_condition(R(-1).expression, 0);
    finish_loop_test(); /*43edc*/
    break;
case 336:
    finish_loop_statement(0, R(-5).pointer, R(-2).expression); /*3f7dd*/
    if (R(-1).token_kind) compiler_warning(0x3b, 0x4c, 0x7f);
    if (parser_context_depth > 0) --parser_context_depth;
    (*declaration_control_row(parser_context_depth)).flags &= ~3;
    finish_loop_scope(); /*43f5e*/
    if (frontend_data.options.language_extensions)
        R(-10).scope_open = begin_statement_scope(NULL);
    break;
case 337:
    R(0).scope_open = begin_statement_scope(NULL);
    (*declaration_control_row(parser_context_depth)).secondary_flags |= 2;
    begin_do_statement(); /*4370f*/
    break;
case 338:
    R(-1).expression = convert_parser_rvalue(resolve_parser_expression(R(-1).expression));
    R(-1).expression = convert_condition(R(-1).expression, 0);
    finish_loop_statement(0, R(-1).expression, 0);
    if (parser_context_depth > 0) --parser_context_depth;
    (*declaration_control_row(parser_context_depth)).flags &= ~1;
    finish_do_statement(); /*43787*/
    break;
case 339:
    R(0).scope_open = begin_statement_scope(NULL);
    begin_switch_statement(); /*3ee23*/
    (*declaration_control_row(parser_context_depth)).flags |= 3;
    break;
case 340:
    set_switch_expression(resolve_parser_expression(R(-1).expression)); /*3eebd*/
    if (parser_context_depth > 0) --parser_context_depth;
    (*declaration_control_row(parser_context_depth)).flags &= ~3;
    break;
case 341:
    RESULT.integer = 0;
    break;
case 342:
    if (!(frontend_data.declaration_context_index >= 0 && frontend_data.options.language_extensions) &&
        R(0).integer == 0)
        compiler_warning(0x1d2);
    break;
case 343:
    RESULT.integer = constant_integer_value(convert_parser_rvalue(resolve_parser_expression(R(0).expression))); /*23433*/
    break;
case 345:
    RESULT.expression = convert_parser_rvalue(resolve_parser_expression(R(0).expression));
    break;
case 346:
    RESULT.expression = make_binary_expression(R(-1).token_kind, R(-2).expression, R(0).expression); /*122ba*/
    break;
case 347:
    RESULT.expression = make_parser_extension_expression(); /*ada58*/
    break;
case 348:
    RESULT.expression = make_parser_extension_operand(R(0).expression); /*adaa7*/
    break;
case 349:
    if (R(-2).expression && R(0).expression) {
        expression = convert_assignment_operand(R(-2).expression->type, R(0).expression); /*15a3f*/
        if (!expression) diagnose_parser_operand(0x34, R(0).expression); /*8afa4; return unused*/
        RESULT.expression = make_binary_expression(R(-1).token_kind, R(-2).expression, expression);
    } else RESULT.expression = NULL;
    break;
case 350: case 356: case 357: case 358: case 359: case 360:
case 361: case 362: case 363: case 364: case 365:
    RESULT.expression = make_binary_expression(R(-1).token_kind, R(-2).expression, R(0).expression);
    break;
case 351:
    RESULT.expression = make_conditional_expression(R(-4).expression,
        R(-2).expression, R(0).expression); /*2ccaa*/
    break;
case 352: case 354:
    if (R(-1).expression) prepare_declaration_lookup();
    break;
case 353: case 355:
    RESULT.expression = make_binary_expression(R(-2).token_kind, R(-3).expression, R(0).expression);
    break;
case 366:
    RESULT.expression = make_parser_comparison(R(-2).expression, R(0).expression); /*8e7b6*/
    break;
case 367:
    RESULT.expression = make_binary_expression(R(-1).token_kind, R(-2).expression, R(0).expression);
    break;
case 370:
    frontend_data.compiler_mode_flags |= 1;
    RESULT.expression = make_call_expression(0x64, R(-1).expression, R(0).expression); /*15805*/
    frontend_data.compiler_mode_flags &= ~1U;
    check_call_expression(RESULT.expression); /*15702*/
    break;
case 371:
    allocation_operator = 1; /* existing CD58C BYTE owner */
    break;
case 372:
    R(-1).token_kind = R(-1).token_kind == 0x40 ? 0x55 : 0x54;
    expression = (CompilerExpression *)make_integer_constant(1);
    RESULT.expression = make_binary_expression(R(-1).token_kind, R(0).expression, expression);
    break;
case 373:
    RESULT.expression = make_unary_expression(0x3d, R(0).expression); /*15738*/
    break;
case 374:
    expression = R(0).expression;
    if (!expression) RESULT.expression = NULL;
    else if (expression->kind == 0x3d && expression->value.operands.second)
        RESULT.expression = make_member_pointer_expression(expression->value.operands.second); /*404ef*/
    else if (expression->kind == 0x53 &&
        ((expression->value.operands.second->flags & 7) == 4) &&
        expression->value.operands.second->value.operands.second)
        RESULT.expression = make_unary_expression(0x31, expression->value.operands.second);
    else {
        frontend_data.compiler_mode_flags |= 4;
        RESULT.expression = make_unary_expression(0x31, R(0).expression);
        frontend_data.compiler_mode_flags &= ~4U;
    }
    break;
case 375:
    RESULT.expression = make_unary_expression(0x86, R(0).expression);
    check_call_expression(RESULT.expression);
    break;
case 376:
    RESULT.expression = make_unary_expression(0x5a, R(0).expression);
    break;
case 377:
    RESULT.expression = make_unary_expression(0x3c, R(0).expression);
    break;
case 379:
    RESULT.expression = make_unary_expression(0x59, R(0).expression);
    break;
case 380:
    frontend_bss.debug_module->Slotb8();
    RESULT.expression = R(0).expression ? make_sizeof_expression(R(0).expression) : NULL; /*444ee*/
    --unevaluated_expression_depth; /*CD584 existing source-only suppress_class_completion owner*/
    break;
case 381:
    frontend_bss.debug_module->Slotb8();
    RESULT.expression = make_sizeof_type(P_SYMBOL(R(0).symbol)->type); /*2c3c5*/
    --unevaluated_expression_depth;
    break;
case 387:
    allocation_operator = 1;
    if (R(-1).expression->kind == 0x37) report_language_diagnostic(4, 0x2c5);
    RESULT.expression = make_binary_expression(R(-2).token_kind, R(-3).expression, R(-1).expression);
    break;
case 388:
    (*declaration_control_row(parser_context_depth)).flags |= 3;
    break;
case 389:
    expression = R(-2).expression;
    if (expression && !(expression->flags & 0x88)) {
        if (frontend_data.compiler_mode_flags & 0x00800000) {
            if (R(0).expression) {
                compiler_warning(0x209);
                expression = recover_new_expression(); /*8e143*/
            }
            RESULT.expression = expression;
            frontend_data.compiler_mode_flags &= ~0x00800000U;
        } else RESULT.expression = resolve_function_call(expression, R(0).expression); /*16611*/
    } else RESULT.expression = expression;
    allocation_operator = 1;
    break;
case 390:
    allocation_context = frontend_data.translation_unit->PrepareAllocation(
        R(0).token_kind, R(-1).expression, 0); /*1b0cf actualthiscall3*/
    allocation_operator = R(0).token_kind;
    break;
case 391:
    RESULT.pointer = R(0).pointer;
    break;
case 392:
    allocation_operator = 1;
    expression = (CompilerExpression *)make_integer_constant(1);
    RESULT.expression = make_binary_expression(R(0).token_kind, R(-1).expression, expression);
    break;
case 393:
    allocation_operator = 1;
    RESULT.expression = make_unary_parser_extension(R(-1).token_kind, R(0).expression); /*a9ac9*/
    break;
case 394:
    if (R(0).token_kind != 0x44) compiler_warning(0x2f8, 0x44, R(0).token_kind);
    ++parser_context_depth;
    (*declaration_control_row(parser_context_depth)).flags &= ~3;
    break;
case 395:
    if (R(0).token_kind != 0x3e) compiler_warning(0x2f8, 0x3e, R(0).token_kind);
    (*declaration_control_row(parser_context_depth)).flags &= ~3;
    if (parser_context_depth > 0) --parser_context_depth;
    --declaration_nesting;
    break;
case 396:
    (*declaration_control_row(parser_context_depth)).flags &= ~3;
    if (parser_context_depth > 0) --parser_context_depth;
    if (R(0).expression) {
        frontend_data.compiler_mode_flags |= 1;
        expression = R(0).expression;
        converted = prepare_call_argument_types(&R(-4).declaration,
            &R(-3).declaration); /*12ba6 genuine2args; expression carried on caller stack for15805*/
        RESULT.expression = make_call_expression(R(-7).token_kind, converted, expression);
        frontend_data.compiler_mode_flags &= ~1U;
        check_call_expression(RESULT.expression);
    } else RESULT.expression = NULL;
    break;
case 397:
    RESULT.expression = make_cpp_cast_expression(R(-1).expression); /*8b4ce*/
    (*declaration_control_row(parser_context_depth)).flags &= ~3;
    if (parser_context_depth > 0) --parser_context_depth;
    break;
case 398:
    RESULT.expression = make_cpp_type_expression(&R(-2).declaration, &R(-1).declaration); /*8b624*/
    (*declaration_control_row(parser_context_depth)).flags &= ~3;
    if (parser_context_depth > 0) --parser_context_depth;
    --declaration_nesting;
    break;
case 399:
    RESULT.expression = make_cpp_query_expression(R(-1).expression); /*a02f6*/
    (*declaration_control_row(parser_context_depth)).flags &= ~3;
    if (parser_context_depth > 0) --parser_context_depth;
    break;
case 400:
    RESULT.expression = make_cpp_query_type(&R(-2).declaration, &R(-1).declaration); /*a0716*/
    (*declaration_control_row(parser_context_depth)).flags &= ~3;
    if (parser_context_depth > 0) --parser_context_depth;
    --declaration_nesting;
    break;
case 401:
    RESULT.pointer = R(-1).pointer;
    break;
case 402:
    RESULT.pointer = NULL;
    report_expected_parser_token(parser_lookahead, 0x76); /*85e68*/
    break;
case 403:
    RESULT.expression = make_typed_constant(R(0).literal.type,
        PARSER_LITERAL_DATA(R(0)), frontend_data.constant_arena_index); /*78e0, data is original value+8*/
    break;
case 405:
    process_version_literal((CompilerVersionRecord *)R(0).pointer);
    break;
case 406:
    RESULT.pointer = process_type_qualifier();
    break;
case 407:
    RESULT.expression = make_cast_expression(0x76, R(-1).expression); /*10239*/
    check_call_expression(RESULT.expression);
    if (parser_context_depth > 0) --parser_context_depth;
    break;
case 409:
    RESULT.expression = make_member_access_expression(allocation_operator, (CompilerExpression *)allocation_context, R(0).expression); /*1ad7f*/
    break;
case 410:
    RESULT.expression = make_array_allocation_expression(allocation_operator, allocation_context, R(0).pointer); /*89279*/
    break;
case 411:
    symbol = R(0).reference->Reference();
    RESULT.expression = make_array_allocation_expression(allocation_operator, allocation_context, symbol);
    break;
case 412:
    RESULT.expression = make_qualified_allocation_expression(allocation_operator, allocation_context,
        &R(-3).declaration, &R(0).declaration); /*8922b*/
    break;
case 413:
    RESULT.expression = lookup_name_expression(R(0).name);
    break;
case 414:
    if (!R(0).qualified_name->Context()) /*e008 +4 getter*/
        RESULT.expression = make_symbol_expression(R(0).qualified_name->ResolvedSymbol()); /*11bf9/21427*/
    else {
        expression = make_symbol_expression(R(0).qualified_name->ResolvedSymbol());
        if (expression->value.operands.second)
            report_compiler_diagnostic(1, ".\\10010613.410\\grammar.y", 0x1093);
        expression->value.operands.second = R(0).qualified_name->ContextValue(); /*864f4 +0c getter, distinct body from e008 +4*/
        RESULT.expression = expression;
    }
    break;
case 415:
    RESULT.expression = lookup_name_expression(R(0).named_token.name);
    break;
case 416:
    RESULT.expression = lookup_name_expression(make_conversion_name(R(0).declaration.type));
    frontend_data.compiler_flags &= ~4U;
    break;
case 417:
    if (R(0).reference->Reference()->scope_flags & 4)
        RESULT.expression = NULL;
    else {
        expression = make_symbol_expression(R(0).reference->Reference());
        if (R(0).reference->Context()) {
            if (expression->value.operands.second)
                report_compiler_diagnostic(1, ".\\10010613.410\\grammar.y", 0x10ad);
            expression->value.operands.second = R(0).reference->Context();
        }
        RESULT.expression = expression;
    }
    break;
case 418:
    if (R(-1).reference) {
        apply_conversion_type(R(-1).reference->Reference(), R(0).declaration.type);
        expression = make_symbol_expression(R(-1).reference->Reference());
        if (R(-1).reference->Context()) {
            if (expression->value.operands.second)
                report_compiler_diagnostic(1, ".\\10010613.410\\grammar.y", 0x10c3);
            expression->value.operands.second = R(-1).reference->Context();
        }
        RESULT.expression = expression;
    }
    frontend_data.compiler_flags &= ~4U;
    break;
case 420:
    RESULT.expression = prepare_call_argument_types(&R(-2).declaration, &R(-1).declaration);
    (*declaration_control_row(parser_context_depth)).flags &= ~3;
    if (parser_context_depth > 0) --parser_context_depth;
    --declaration_nesting;
    break;
case 421:
    RESULT.pointer = R(-1).pointer;
    if (parser_context_depth > 0) --parser_context_depth;
    break;
case 422: case 447:
    RESULT.pointer = NULL;
    if (parser_context_depth > 0) --parser_context_depth;
    break;
case 423:
    RESULT.expression = append_expression_list(NULL, R(0).expression, 0);
    break;
case 424:
    RESULT.expression = append_expression_list(R(-2).expression, R(0).expression, 0);
    break;
case 425:
    expression = (CompilerExpression *)frontend_bss.arenas[frontend_data.constant_arena_index].Allocate(24);
    expression->kind = 7;
    expression->flags = 5;
    expression->value.quoted = R(0).quoted;
    RESULT.expression = expression;
    break;
case 426:
    append_parser_string(&R(-1).expression->value, &R(0)); /*4fce3*/
    break;
case 427:
    if (parser_context_depth > 0) --parser_context_depth;
    (*declaration_control_row(parser_context_depth)).flags |= 2;
    frontend_data.compiler_mode_flags &= ~0x10U;
    frontend_data.compiler_flags &= ~0x80000000U;
    break;
case 428: case 430: case 432: case 434:
    prepare_allocation_type(); /*433ec*/
    break;
case 429: case 431:
    type = R(-2).declaration_value.declarator->ResolvedType(); /*43429*/
    RESULT.expression = make_new_expression(R(-3).token_kind, NULL,
        R(-2).declaration_value.symbol, R(0).expression, type); /*4342d*/
    if (R(-2).declaration_value.declarator) R(-2).declaration_value.declarator->Finish(1);
    break;
case 433: case 435:
    type = R(-2).declaration_value.declarator->ResolvedType();
    RESULT.expression = make_new_expression(R(-4).token_kind, R(-3).pointer,
        R(-2).declaration_value.symbol, R(0).expression, type);
    if (R(-2).declaration_value.declarator) R(-2).declaration_value.declarator->Finish(1);
    break;
case 436:
    frontend_data.compiler_mode_flags |= 0x10;
    frontend_data.compiler_flags |= 0x80000000;
    break;
case 437:
    frontend_data.compiler_mode_flags |= 0x10;
    frontend_data.compiler_flags |= 0x80000000;
    ++parser_context_depth;
    (*declaration_control_row(parser_context_depth)).flags &= ~3;
    break;
case 438:
    symbol = R(0).declaration_value.symbol;
    P_SYMBOL(symbol)->encoded_index = (P_SYMBOL(symbol)->encoded_index & 0xff000000U) |
        (allocate_symbol_record_id() & 0x00ffffffU);
    symbol->SetKind(13);
    symbol->SetName(frontend_data.builtin_names.new_target);
    symbol->MarkReferenced();
    R(-1).declaration.context.class_flags &= ~0x80;
    set_symbol_declaration_context(symbol, R(-1).declaration.context);
    R(0).declaration_value.declarator->ApplyDeclaration(&R(-1).declaration);
    R(0).declaration_value.declarator->PrepareAllocationType(); /*433bf*/
    R(0).declaration_value.declarator->Resolve();
    finalize_symbol_declaration(R(0).declaration_value.declarator, symbol);
    if (invalid_allocation_type(symbol)) compiler_warning(0x1d4, P_SYMBOL(symbol)->type); /*433c8*/
    --declaration_nesting;
    (*declaration_control_row(parser_context_depth)).flags &= ~1;
    RESULT.declaration_value = R(0).declaration_value;
    break;
case 439:
    symbol = make_abstract_declarator();
    P_SYMBOL(symbol)->encoded_index = (P_SYMBOL(symbol)->encoded_index & 0xff000000U) |
        (allocate_symbol_record_id() & 0x00ffffffU);
    symbol->SetKind(13);
    symbol->SetName(frontend_data.builtin_names.new_target);
    symbol->MarkReferenced();
    memory = allocate_declarator(72);
    declarator = memory ? ((CompilerDeclarator *)memory)->InitializeSymbol(symbol) : NULL;
    declarator->SetType(R(0).expression->type); /*86525*/
    declarator->Resolve();
    finalize_symbol_declaration(declarator, symbol);
    (*declaration_control_row(parser_context_depth)).flags &= ~1;
    RESULT.declaration_value.declarator = declarator;
    RESULT.declaration_value.symbol = symbol;
    break;
case 440:
    RESULT.declaration_value = R(-1).declaration_value;
    symbol = RESULT.declaration_value.symbol;
    P_SYMBOL(symbol)->encoded_index = (P_SYMBOL(symbol)->encoded_index & 0xff000000U) |
        (allocate_symbol_record_id() & 0x00ffffffU);
    symbol->SetKind(13);
    symbol->SetName(frontend_data.builtin_names.new_target);
    symbol->MarkReferenced();
    R(-2).declaration.context.class_flags &= ~0x80;
    set_symbol_declaration_context(symbol, R(-2).declaration.context);
    RESULT.declaration_value.declarator->ApplyDeclaration(&R(-2).declaration);
    RESULT.declaration_value.declarator->PrepareAllocationType();
    RESULT.declaration_value.declarator->Resolve();
    finalize_symbol_declaration(RESULT.declaration_value.declarator, symbol);
    if (invalid_allocation_type(symbol)) compiler_warning(0x1d4, P_SYMBOL(symbol)->type);
    --declaration_nesting;
    if (parser_context_depth > 0) --parser_context_depth;
    (*declaration_control_row(parser_context_depth)).flags &= ~1;
    break;
case 441:
    RESULT.declaration_value.symbol = make_abstract_declarator();
    memory = allocate_declarator(72);
    RESULT.declaration_value.declarator = memory ?
        ((CompilerDeclarator *)memory)->InitializeSymbol(NULL) : NULL;
    ++declaration_nesting;
    if (parser_lookahead && parser_lookahead->Kind() == 0x76)
        (*declaration_control_row(parser_context_depth)).flags |= 3;
    break;
case 442:
    symbol = make_abstract_declarator();
    memory = allocate_declarator(72);
    declarator = memory ? ((CompilerDeclarator *)memory)->InitializeSymbol(NULL) : NULL;
    declarator->ApplyQualifiers(&R(0).qualifiers);
    ++declaration_nesting;
    RESULT.declaration_value.declarator = declarator;
    RESULT.declaration_value.symbol = symbol;
    break;
case 444:
    if (R(-1).expression && R(-1).expression->kind == 0x37)
        report_language_diagnostic(4, 0x2c5);
    RESULT.declaration_value.declarator->AddArrayExpression(R(-1).expression);
    break;
case 448:
    RESULT.expression = make_delete_expression(R(-1).token_kind, R(0).expression, NULL, 0); /*4546f*/
    break;
case 449:
    if (R(-2).expression && R(-2).expression->kind == 0x37)
        report_language_diagnostic(4, 0x2c5);
    RESULT.expression = make_delete_expression(R(-4).token_kind, R(0).expression,
        R(-2).expression, 1);
    break;
case 450:
    RESULT.token_kind = 1;
    break;

action_done:;
    }

    production = &parser_productions[parser_production]; /* fresh actual global load after action */
    goto_info = &parser_gotos[production->nonterminal];
    state = goto_info->default_state;
    transition = parser_goto_transitions + goto_info->first_transition;
    for (i = 0; i < goto_info->transition_count; ++i, ++transition) {
        if (transition->symbol == (int)states[state_top]) {
            state = transition->destination;
            break;
        }
    }
push_state:
    ++value_top;
    states[++state_top] = state;
    goto read_action;

finished:
    parser_production = saved_production;
    parser_lookahead = saved_lookahead;
    return parser_result;
}

void RECON_THISCALL CompilerParser::SetLookahead(ParserTokenView *token)
{
    parser_lookahead = token;
}
