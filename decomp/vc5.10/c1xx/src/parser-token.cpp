#include <string.h>
#include <stdlib.h>
#include <new.h>
#include "parser.h"
#include "parser-grammar.h"
#include "declaration-grammar.h"
#include "parser-state.h"
#include "keywords.h"
#include "diagnostics.h"
#include "declaration.h"
#include "lexer.h"
#include "pointer-array.h"
#include "hash-table.h"
#include "packing.h"
#include <recon.h>
#pragma intrinsic(memcpy, memset, memcmp)

#define final_parser_token_emitted frontend_data.final_parser_token_emitted
#define parser_input_end_state frontend_data.parser_input_end_state
#define TOKEN_CONTEXT (*(ParserFrontendContextView *)&frontend_bss.saved_context_stack)
#define TOKEN_STREAM ((CompilerTokenStream *)frontend_bss.saved_context_stack.top->object)
#define LEXER_FLAGS frontend_data.compiler_flags
#define DECL_DEPTH frontend_bss.parser_context_depth
#define TOKEN_SEMANTIC(t) (*(ParserSemanticView *)&(t)->value)

extern "C" ParserTokenView *__cdecl get_next_token(void)
{
    ParserTokenView *token = TOKEN_CONTEXT.ReadToken();
    ParserTokenView *result, *looked_ahead;
    CompilerDeclaration declaration;
    CompilerType *type;
    CompilerSymbol *symbol;
    BYTE flags;
    unsigned token_class;
    int depth;
#ifdef RECON_NATIVE_TEST
    /* Encountered token domains, including the const declaration qualifier
     * whose type-method consumer retains its genuine pending entry. */
    if (token->kind != 80 && token->kind != 207 && token->kind != 219 && token->kind != 81 && token->kind != 182 && token->kind != 54 && token->kind != 27 && token->kind != 52 && token->kind != 47 && token->kind != 12 && token->kind != 117 && token->kind != 118 && token->kind != 126 && token->kind != 22 && token->kind != 74 && token->kind != 17 && token->kind != 16 && token->kind != 25 && token->kind != 37 && token->kind != 106 && token->kind != 65 && token->kind != 122 && token->kind != 6 && token->kind != 127 && token->kind != 123 && token->kind != 36 && token->kind != 10 && token->kind != 7 && token->kind != 130 && token->kind != 55 && token->kind != 49 && token->kind != 21 && token->kind != 151 && token->kind != 140 && token->kind != 141 && token->kind != 28 && token->kind != 101 && token->kind != 125 && token->kind != 109)
        recon_unsupported("parser token: declaration-classification domain pending");
#endif
read_token_class:
    token_class = parser_token_classes[token->kind];
    switch (token_class) {
    case PARSER_CLASS_236:
        if (!(frontend_data.compiler_mode_flags & 0x52000000) &&
            !(frontend_data.source_mode_flags & 0x300008)) {
            compiler_warning(0x383);
            goto read_another_token;
        }
        /* The permitted extension shares the same declaration-name path. */
    case PARSER_CLASS_26: case PARSER_CLASS_27:
    case PARSER_CLASS_28: case PARSER_CLASS_29:
        if (LEXER_FLAGS & 2) {
            compiler_warning(0x206, token->kind);
            goto read_another_token;
        }
    case PARSER_CLASS_32:
        looked_ahead = TOKEN_CONTEXT.PeekToken();
        if (parser_token_classes[looked_ahead->kind] == 0xb1)
            return token;
        return (ParserTokenView *)parse_recorded_declaration(token);
    case PARSER_CLASS_126:
        LEXER_FLAGS &= ~1U;
        if (!DECL_DEPTH && !(LEXER_FLAGS & 0x10000000)) {
            compiler_warning(0x3b, 0x4c, token->kind);
            goto read_another_token;
        }
        return token;
    case PARSER_CLASS_0: case 109:
        if (!final_parser_token_emitted) {
            final_parser_token_emitted = 1;
            return (ParserTokenView *)frontend_data.canonical_token_records[4];
        }
        if (parser_input_end_state) parser_input_end_state->reached_end = 1;
        if (has_pending_parser_input()) goto read_another_token;
        finish_token_input();
        return (ParserTokenView *)frontend_data.canonical_token_records[108];
    case PARSER_CLASS_40:
        if (LEXER_FLAGS & 4) return token;
        return (ParserTokenView *)parse_recorded_declaration(token);
    case PARSER_CLASS_15:
        if (LEXER_FLAGS & 2) return token;
        return (ParserTokenView *)parse_recorded_declaration(token);
    case PARSER_CLASS_149:
        if (LEXER_FLAGS & 0x40) return token;
        /* The existing token-stream interface carries the pointer as its
         * actual DWORD argument; the native ABI remains thiscall/RET4. */
        TOKEN_STREAM->Unread((unsigned)token);
        token = read_qualified_parser_token(0, 0);
        if (token->kind != 0x96) {
            result = token;
            goto mark_declaration_token;
        }
    case PARSER_CLASS_6: case PARSER_CLASS_150:
        LEXER_FLAGS |= 0x400;
        result = resolve_identifier_type(token);
        LEXER_FLAGS &= ~0x400U;
        if (result->kind == 0x1e)
            return (ParserTokenView *)parse_recorded_declaration(result);
        goto mark_declaration_token;
    case PARSER_CLASS_124:
        if (((*declaration_control_row(DECL_DEPTH)).flags & 0x40) && token->kind == 0x3e)
            return (ParserTokenView *)frontend_data.canonical_token_records[196];
        return token;
    case 223: /* Explicit original dispatch; no current token-kind maps here. */
        token->kind = 6;
        goto mark_original_token;
    case PARSER_CLASS_49: case PARSER_CLASS_74:
    case PARSER_CLASS_120: case PARSER_CLASS_178:
        goto mark_original_token;
    case PARSER_CLASS_89:
        LEXER_FLAGS |= 1;
    case PARSER_CLASS_60: case PARSER_CLASS_70:
    case PARSER_CLASS_81: case PARSER_CLASS_101:
    case PARSER_CLASS_115: case PARSER_CLASS_140:
    case PARSER_CLASS_141: case PARSER_CLASS_142:
    case PARSER_CLASS_143: case PARSER_CLASS_144:
    case PARSER_CLASS_152:
        goto mark_original_token;
    case PARSER_CLASS_125:
        if (!(LEXER_FLAGS & 0x10000000)) {
            (*declaration_control_row(DECL_DEPTH)).flags |= 2;
            if ((*declaration_control_row(DECL_DEPTH)).secondary_flags & 2) {
                (*declaration_control_row(DECL_DEPTH)).secondary_flags &= ~2;
                (*declaration_control_row(DECL_DEPTH)).flags |= 1;
            }
            (*declaration_control_row(DECL_DEPTH)).flags |= 1;
        }
        return token;
    case PARSER_CLASS_129:
        if (!(LEXER_FLAGS & 0x10000000))
            ++unevaluated_expression_depth;
        goto mark_original_token;
    case PARSER_CLASS_177:
        looked_ahead = TOKEN_CONTEXT.ReadToken();
        if (looked_ahead->kind != 0x44) {
            TOKEN_STREAM->Unread((unsigned)looked_ahead);
            return token;
        }
        looked_ahead = TOKEN_CONTEXT.ReadToken();
        if (looked_ahead->kind == 0x3e)
            return (ParserTokenView *)parse_recorded_declaration(token);
        TOKEN_STREAM->Unread((unsigned)looked_ahead);
        return (ParserTokenView *)frontend_data.canonical_token_records[195];
    case PARSER_CLASS_36:
        flags = (*declaration_control_row(DECL_DEPTH)).flags;
        if ((flags & 2) && (LEXER_FLAGS & 0x08000000) &&
            TOKEN_SEMANTIC(token).declaration.object) {
            symbol = TOKEN_SEMANTIC(token).declaration.object;
            saved_parser_value.name = ((CompilerNamedSymbol *)symbol)->link.name;
            if (!frontend_bss.symbol_arena_kind)
                token = (ParserTokenView *)frontend_bss.pools[7].Allocate(48);
            else token = (ParserTokenView *)frontend_bss.arenas[
                frontend_bss.symbol_arena_kind].Allocate(48);
            if (token) {
                token->kind = 6;
                memcpy(&token->value, &saved_parser_value, 40);
            }
            return token;
        }
        LEXER_FLAGS |= 1;
        goto mark_original_token;
    case PARSER_CLASS_151:
        result = process_parser_declaration();
        goto mark_declaration_token;
    case PARSER_CLASS_170:
        TOKEN_STREAM->Unread((unsigned)token);
        depth = (int)DECL_DEPTH;
        if (depth && !((*declaration_control_row(depth - 1)).flags & 1) &&
            (depth <= 0 || !((*declaration_control_row(depth - 1)).flags & 4)) &&
            (LEXER_FLAGS & 0x20000) && !(LEXER_FLAGS & 0x20))
            return classify_parser_declaration(0);
        return classify_parser_declaration(1);
    case PARSER_CLASS_171:
        if ((*declaration_control_row(DECL_DEPTH)).flags & 2) return token;
        memset(&declaration.context, 0, 28);
        declaration.context.class_flags |= 0x30;
        if (!(LEXER_FLAGS & 0x10000000)) {
            (*declaration_control_row(DECL_DEPTH)).flags |= 2;
            if ((*declaration_control_row(DECL_DEPTH)).secondary_flags & 2) {
                (*declaration_control_row(DECL_DEPTH)).secondary_flags &= ~2;
                (*declaration_control_row(DECL_DEPTH)).flags |= 1;
            }
        }
        type = make_parser_type_scratch(frontend_data.builtin_types.type_0063);
        type->attributes = *(WORD *)&token->value;
        saved_parser_value.declaration.type = intern_type(type);
        memcpy(&saved_parser_value.declaration.context, &declaration.context, 28);
        --declaration_type_scratch_index;
        if (!frontend_bss.symbol_arena_kind)
            token = (ParserTokenView *)frontend_bss.pools[7].Allocate(48);
        else token = (ParserTokenView *)frontend_bss.arenas[
            frontend_bss.symbol_arena_kind].Allocate(48);
        if (token) {
            token->kind = 0x24;
            memcpy(&token->value, &saved_parser_value, 40);
        }
        return token;
    case PARSER_CLASS_118:
        if (!(LEXER_FLAGS & 0x10000000)) {
            if (++DECL_DEPTH >= 60) report_compiler_error(13);
            /* A returning reporter cannot admit the out-of-range write. */
            if (DECL_DEPTH >= 60) { __asm { int 3 } }
            qualification_scopes[DECL_DEPTH] = NULL;
            (*declaration_control_row(DECL_DEPTH)).flags &= 0x70;
        }
        LEXER_FLAGS &= ~1U;
        return token;
    case PARSER_CLASS_225:
        LEXER_FLAGS |= 1;
        return token;
    case PARSER_CLASS_44:
        if (!(LEXER_FLAGS & 0x10000000)) {
            (*declaration_control_row(DECL_DEPTH)).flags |= 2;
            if ((*declaration_control_row(DECL_DEPTH)).secondary_flags & 2) {
                (*declaration_control_row(DECL_DEPTH)).secondary_flags &= ~2;
                (*declaration_control_row(DECL_DEPTH)).flags |= 1;
            }
        }
        LEXER_FLAGS |= 0x80000000;
        return token;
    case PARSER_CLASS_232: case PARSER_CLASS_233:
        saved_parser_value.recorded_type.type = frontend_data.builtin_types.type_0060;
        saved_parser_value.recorded_type.kind = parser_token_semantic_codes[token->kind];
        saved_parser_value.recorded_type.value = 0;
        if (!frontend_bss.symbol_arena_kind)
            token = (ParserTokenView *)frontend_bss.pools[7].Allocate(48);
        else token = (ParserTokenView *)frontend_bss.arenas[
            frontend_bss.symbol_arena_kind].Allocate(48);
        if (token) {
            token->kind = 0x65;
            memcpy(&token->value, &saved_parser_value, 40);
        }
        goto mark_original_token;
    case PARSER_CLASS_31: case PARSER_CLASS_33: case PARSER_CLASS_192:
        return (ParserTokenView *)parse_recorded_declaration(token);
    default:
        return token;
    }
mark_original_token:
    result = token;
mark_declaration_token:
    if (!(LEXER_FLAGS & 0x10000000)) {
        (*declaration_control_row(DECL_DEPTH)).flags |= 2;
        if ((*declaration_control_row(DECL_DEPTH)).secondary_flags & 2) {
            (*declaration_control_row(DECL_DEPTH)).secondary_flags &= ~2;
            (*declaration_control_row(DECL_DEPTH)).flags |= 1;
        }
    }
    return result;
read_another_token:
    token = TOKEN_CONTEXT.ReadToken();
    goto read_token_class;
}


/* Marker tokens carry these two already-known DWORD roles in the semantic
 * prefix. Their remaining32 original bytes are not read by this path. */
struct ParserSourceMarker { void *origin; unsigned position; };
typedef void (__cdecl *ParserContextPopCallback)(void);

ParserTokenView *RECON_THISCALL ParserFrontendContextView::ReadToken(void)
{
    ParserTokenView *token, *result;
    CompilerLexer *lexer;
    CompilerTokenStream *stream;
    FrontendContextFrame *frame;
    ParserSourceMarker *marker;
    ParserContextPopCallback callback;
    CompilerArena *arena;
    CompilerPool *pool;
    BYTE *memory;
    CompilerIdentifier *identifier;
    for (;;) {
        stream = (CompilerTokenStream *)top->object;
        token = (ParserTokenView *)stream->Read();
#ifdef RECON_NATIVE_TEST
        /* The source-position f0 prefix, pragma f2 pointer and recorded-stream
         * f4 completion marker have grounded producers and typed consumers. */
        if (token->kind >= 0xf0 && token->kind != 0xf0 && token->kind != 0xf2 && token->kind != 0xf4)
            recon_unsupported("context token reader: marker/return domain pending");
#endif
        if (token->kind < 0xf0) return token;
        if (token->kind == 0xf0) {
            if (frontend_data.compiler_mode_flags & 0x04000000) return token;
            marker = (ParserSourceMarker *)&token->value;
            position = marker->position;
            origin = marker->origin;
            if (LEXER_FLAGS & 0x200000) return token;
            if ((LEXER_FLAGS & 0x20000) && !(LEXER_FLAGS & 0x20))
                emit_context_transition(0x24);
            continue;
        }
        if (token->kind == 0xf4) {
            if (top->first != 1) return token;
            frame = top;
            if (frame) {
                top = frame->previous;
                origin = frame->saved_origin;
                position = frame->saved_position;
                /* The existing fourth argument cell holds a cdecl0 pop
                 * callback on this variant, without adding another owner. */
                callback = frame->callback;
                if (callback) callback();
            }
            continue;
        }
        if (token->kind == 0xf1) {
            lexer = (CompilerLexer *)token->value.pointer;
            lexer->Reset();
            Push(lexer, 9, 1, 0, &frontend_bss.saved_context_stack);
            lexer = (CompilerLexer *)top->object;
            if (!lexer->marker_emitted) {
                lexer->marker_emitted = 1;
                lexer->Trace((unsigned)frontend_data.canonical_token_records[243]);
            }
            continue;
        }
        if (parser_token_classes[token->kind] == 0xf3) {
            identifier = (CompilerIdentifier *)frontend_data.current_identifier;
            switch (identifier->category) {
            case 1: report_language_diagnostic(1, 0xe2, identifier); break;
            case 2: report_language_diagnostic(1, 0xe9, identifier); break;
            case 3: report_language_diagnostic(1, 0xea, identifier); break;
            case 4: report_language_diagnostic(1, 0xeb, identifier); break;
            case 5: report_language_diagnostic(1, 0xec, identifier); break;
            case 6:
                if (frontend_data.options.language_extensions)
                    report_language_diagnostic(1, 0xed, identifier);
                else compiler_warning(0x148, identifier);
                if (frontend_bss.symbol_arena_kind) {
                    arena = &frontend_bss.arenas[frontend_bss.symbol_arena_kind];
                    memory = arena->cursor;
                    arena->cursor += 48;
                    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
                } else {
                    pool = &frontend_bss.pools[7];
                    if (pool->remaining < 48) pool->Refill(48);
                    memory = pool->cursor;
                    pool->cursor += 48;
                    pool->remaining -= 48;
                }
                result = (ParserTokenView *)memory;
                if (result) {
                    result->kind = 6;
                    result->value.pointer = frontend_data.current_identifier;
                }
                return result;
            }
            continue;
        }
        if (frontend_data.compiler_mode_flags & 0x04000000) return token;
        if (token->kind != 0xf2) return token;
        result = process_context_token(token);
        if (!result) continue;
        if (result->kind == 2 && frontend_bss.forced_source_index &&
            has_saved_source_state()) restore_source_input();
        return result;
    }
}

/* Peeking may retire exhausted nested streams, retaining each real frame's
 * origin, position and optional cdecl pop callback. The token stays unread. */
ParserTokenView *RECON_THISCALL ParserFrontendContextView::PeekToken(void)
{
    ParserTokenView *token = (ParserTokenView *)
        ((CompilerTokenStream *)top->object)->Peek();
    FrontendContextFrame *frame;
    ParserContextPopCallback callback;
    while (token->kind == 0xf4 && top->first == 1) {
        frame = top;
        if (frame) {
            top = frame->previous;
            origin = frame->saved_origin;
            position = frame->saved_position;
            callback = frame->callback;
            if (callback) callback();
        }
        token = (ParserTokenView *)((CompilerTokenStream *)top->object)->Peek();
    }
    return token;
}

extern "C" void __cdecl record_declaration_body(ParserTokenView *);
extern "C" void __cdecl complete_deferred_declaration(ParserTokenView *, CompilerType *);
extern "C" ParserTokenView *__cdecl completed_declaration_token(ParserTokenView *);
extern "C" unsigned __cdecl classify_declaration_tokens(void);
extern "C" int __cdecl keep_declaration_scope(unsigned);
extern "C" void __cdecl complete_declaration_reference(ParserTokenView *);
extern "C" void __cdecl report_declaration_syntax(unsigned);
extern "C" ParserTokenView *__cdecl classify_parser_declaration(unsigned allow_deferred)
{
    ParserTokenView *token, *completed;
    CompilerType *type;
    CompilerRecordedLexer *lexer;
    FrontendContextFrame *frame;
    FrontendContextPopCallback callback;
    unsigned classification, saved_depth;
    int saved_state;
    ++frontend_data.declaration_classification_depth;
    if (declaration_control_row(DECL_DEPTH)->flags & 2) {
        token = TOKEN_CONTEXT.ReadToken();
        type = TOKEN_SEMANTIC(token).declaration.type;
        completed = completed_declaration_token(token);
        if (completed) {
            --frontend_data.declaration_classification_depth;
            return completed;
        }
        goto complete_declaration;
    }
    lexer = (CompilerRecordedLexer *)frontend_bss.arenas[5].Allocate(64);
    if (lexer) lexer = lexer->CopyFrom((CompilerRecordedLexer *)frontend_bss.saved_context_stack.top->object);
    frontend_bss.saved_context_stack.Push(lexer, 5, 0, NULL, &frontend_bss.saved_context_stack);
    token = TOKEN_CONTEXT.ReadToken();
    type = TOKEN_SEMANTIC(token).declaration.type;
    completed = completed_declaration_token(token);
    if (completed) {
        frame = frontend_bss.saved_context_stack.top;
        if (frame) {
            frontend_bss.saved_context_stack.top = frame->previous;
            frontend_bss.saved_context_stack.origin = frame->saved_origin;
            frontend_bss.saved_context_stack.position = frame->saved_position;
            callback = frame->callback;
            if (callback) callback();
        }
        token = TOKEN_CONTEXT.ReadToken();
        --frontend_data.declaration_classification_depth;
        return token;
    }
    saved_depth = DECL_DEPTH;
    saved_state = frontend_data.parser_classification_state;
    TOKEN_STREAM->Unread((unsigned)token);
    if (frontend_data.declaration_classification_depth == 1)
        declaration_control_row(DECL_DEPTH)->flags &= ~4U;
    classification = classify_declaration_tokens();
    DECL_DEPTH = saved_depth;
    frontend_data.parser_classification_state = saved_state;
    frame = frontend_bss.saved_context_stack.top;
    if (frame) {
        frontend_bss.saved_context_stack.top = frame->previous;
        frontend_bss.saved_context_stack.origin = frame->saved_origin;
        frontend_bss.saved_context_stack.position = frame->saved_position;
        callback = frame->callback;
        if (callback) callback();
    }
    token = TOKEN_CONTEXT.ReadToken();
    switch (classification) {
    case 0:
        goto complete_declaration;
    case 1:
        if ((int)DECL_DEPTH > 0 && !keep_declaration_scope(DECL_DEPTH)) goto complete_declaration;
        if (!allow_deferred || frontend_data.parser_classification_state > 0) goto complete_declaration;
        break;
    case 3:
        complete_declaration_reference(token);
        break;
    case 5:
        if ((frontend_data.compiler_mode_flags & 0x10) && !DECL_DEPTH) goto complete_declaration;
        complete_declaration_reference(token);
        break;
    }
    goto finish_declaration;
complete_declaration:
    complete_deferred_declaration(token, type);
finish_declaration:
    LEXER_FLAGS |= 0x40000000;
    if (token->kind == 0x24) {
        ParserDeclarationView *value = declaration_spec_row(DECL_DEPTH);
#ifdef __linux__
        memcpy(value, &((ParserDeclarationToken *)token)->value, 36);
        value->callback = ((ParserDeclarationToken *)token)->value.callback;
#else
        memcpy(value, &token->value, 40);
#endif
        declaration_control_row(DECL_DEPTH)->flags |= 1;
    } else if (token->kind != 0xdf) report_declaration_syntax(0x1e);
    --frontend_data.declaration_classification_depth;
    return token;
}
extern "C" void __cdecl discard_deferred_declaration(ParserTokenView *);
extern "C" ParserTokenView *__cdecl completed_declaration_token(ParserTokenView *token)
{
    if (token->kind != 0xaa) return NULL;
    if (LEXER_FLAGS & 0x40000000) {
        complete_declaration_reference(token);
        return token;
    }
    discard_deferred_declaration(token);
    return NULL;
}
extern "C" void __cdecl discard_deferred_declaration(ParserTokenView *) { __asm { int 3 } }

extern "C" unsigned __cdecl classify_declaration_tokens(void)
{
    CompilerDeclarationParser *previous = frontend_data.active_declaration_parser;
    TokenValue saved;
    memcpy(&saved, &frontend_bss.declaration_token_value, 40);
    void *memory = ::operator new(0x18b8);
    frontend_data.active_declaration_parser = memory ?
        ((CompilerDeclarationParser *)memory)->Initialize() : NULL;
    unsigned result = frontend_data.active_declaration_parser->Run();
    if (frontend_data.active_declaration_parser) {
        frontend_data.active_declaration_parser->Finish();
        ::operator delete(frontend_data.active_declaration_parser);
    }
    frontend_data.active_declaration_parser = previous;
    memcpy(&frontend_bss.declaration_token_value, &saved, 40);
    return result;
}
CompilerDeclarationParser *RECON_THISCALL CompilerDeclarationParser::Initialize(void)
{
    Reset();
    return this;
}
void RECON_THISCALL CompilerDeclarationParser::Reset(void)
{
    lookahead = NULL;
    state = 0;
    recovery_shifts = 0;
    syntax_errors = 0;
    value_top = (TokenValue *)((unsigned)this - 16);
    state_top = (short *)((unsigned)this + 0x1786);
}
extern "C" void __cdecl report_declaration_parser_error(const char *);
unsigned RECON_THISCALL CompilerDeclarationParser::Run(void)
{
    const ParserState *info;
    const ParserTransition *transition;
    const ParserProduction *production;
    const ParserGoto *go;
    TokenValue *right;
    TokenValue &result = frontend_bss.declaration_token_value;
    int state_index = ((int)((unsigned)state_top - (unsigned)this) - 0x1788) / 2;
    int value_index = ((int)((unsigned)value_top - (unsigned)this) - 24) / 40;
    int terminal, rule, i;
    unsigned flags;
    goto push_state;

read_action:
    if ((unsigned)state >= DECLARATION_STATE_COUNT)
        { __asm { int 3 } }
    info = &declaration_states[state];
    if (info->needs_token) {
        if (!lookahead) lookahead = get_next_token();
        if (lookahead->kind >= PARSER_TOKEN_KIND_COUNT)
            { __asm { int 3 } }
        terminal = parser_token_classes[lookahead->kind];
        transition = declaration_shifts + info->first_shift;
        for (i = 0; i < info->shift_count; ++i, ++transition) {
            if (transition->symbol == terminal) {
                memcpy(&result, &lookahead->value, 40);
                state = transition->destination;
                lookahead = NULL;
                if (recovery_shifts > 0) --recovery_shifts;
                goto push_state;
            }
        }
    }
    rule = info->default_production;
    if (info->exception_count) {
        if (!lookahead) lookahead = get_next_token();
        if (lookahead->kind >= PARSER_TOKEN_KIND_COUNT)
            { __asm { int 3 } }
        terminal = parser_token_classes[lookahead->kind];
        transition = declaration_exceptions + info->first_exception;
        for (i = 0; i < info->exception_count; ++i, ++transition) {
            if (transition->symbol == terminal) {
                rule = transition->destination;
                break;
            }
        }
        if (rule < 0) return 0;
    }
    if (!rule && recovery_shifts <= 3) {
        if (recovery_shifts == 3) {
            if (!parser_token_classes[lookahead->kind]) return 1;
            lookahead = NULL;
            goto read_action;
        }
        if (!recovery_shifts) {
            report_declaration_parser_error("syntax error");
            ++syntax_errors;
        }
        recovery_shifts = 3;
        while (state_index >= 0) {
            info = &declaration_states[states[state_index]];
            transition = declaration_shifts + info->first_shift;
            for (i = 0; i < info->shift_count; ++i, ++transition) {
                if (transition->symbol == PARSER_ERROR) {
                    state = transition->destination;
                    goto push_state;
                }
            }
            --state_index;
            --value_index;
            state_top = (short *)((unsigned)this + 0x1788 + state_index * 2);
            value_top = (TokenValue *)((unsigned)this + 24 + value_index * 40);
        }
        return 1;
    }
    if ((unsigned)rule > DECLARATION_PRODUCTION_COUNT)
        { __asm { int 3 } }
    production = &declaration_productions[rule];
    if (value_index < 0 || value_index >= 150 || production->length > value_index + 1)
        { __asm { int 3 } }
    right = &values[value_index];
    state_index -= production->length;
    value_index -= production->length;
    state_top = (short *)((unsigned)this + 0x1788 + state_index * 2);
    value_top = (TokenValue *)((unsigned)this + 24 + value_index * 40);
    /* Authentic default reduction copies all40 bytes, including opaque
     * bytes. Every handwritten action below reads only declaration_flags. */
    memcpy(&result, &values[value_index + 1], 40);
    go = &declaration_gotos[production->nonterminal];
    if (state_index < 0)
        { __asm { int 3 } }
    state = go->default_state;
    transition = declaration_goto_transitions + go->first_transition;
    for (i = 0; i < go->transition_count; ++i, ++transition) {
        if (transition->symbol == states[state_index]) {
            state = transition->destination;
            break;
        }
    }
    switch (rule) {
    case 1:
        if (!DECL_DEPTH) {
            if (right[0].declaration_flags & 2) return 0;
            if (!lookahead) lookahead = get_next_token();
            switch (lookahead->kind) {
            case 117: case 123: case 127: case 172: return 1;
            default: return 0;
            }
        }
        flags = right[0].declaration_flags;
        if ((flags & 1) && !(flags & 8)) return 0;
        if (flags & 4) return 0;
        if (!lookahead) lookahead = get_next_token();
        if (lookahead->kind >= PARSER_TOKEN_KIND_COUNT)
            { __asm { int 3 } }
        return parser_token_classes[lookahead->kind] == 126;
    case 2: return right[0].declaration_flags;
    case 3: return 0;
    case 4:
        result.declaration_flags = right[0].declaration_flags | 8;
        declaration_control_row(DECL_DEPTH)->flags &= ~2;
        break;
    case 5: case 6:
        result.declaration_flags = (result.declaration_flags | right[0].declaration_flags) & ~8U;
        declaration_control_row(DECL_DEPTH)->flags &= ~2;
        break;
    case 7: case 8:
        result.declaration_flags = right[-2].declaration_flags | 2;
        if (result.declaration_flags & 8) result.declaration_flags &= ~1U;
        break;
    case 9:
        result.declaration_flags = right[0].declaration_flags == 5 ? 5 : 1;
        break;
    case 10: case 11: result.declaration_flags = 1; break;
    case 12: return 1;
    case 13: case 46: case 47: case 53: case 64: case 67: case 68:
    case 136: case 137: case 138: case 139: case 140: case 148:
    case 155: case 156: case 157: case 171: case 178:
        if (DECL_DEPTH > 0) --DECL_DEPTH;
        break;
    case 16: case 17: case 22: result.declaration_flags = 0; break;
    case 18:
        if (!right[-1].declaration_flags) result.declaration_flags = right[0].declaration_flags;
        break;
    case 19: case 20: result.declaration_flags = 6; break;
    case 21:
        if (DECL_DEPTH > 0) --DECL_DEPTH;
        declaration_control_row(DECL_DEPTH)->flags |= 4;
        result.declaration_flags = right[-1].declaration_flags;
        break;
    case 25: case 27: case 152: case 154: LEXER_FLAGS &= ~4U; break;
    case 33:
        if (DECL_DEPTH > 0) --DECL_DEPTH;
        result.declaration_flags = 5;
        break;
    case 34: case 35:
        if (DECL_DEPTH > 0) --DECL_DEPTH;
        result.declaration_flags = 3;
        break;
    case 38: case 39: declaration_control_row(DECL_DEPTH)->flags &= ~3; break;
    case 44: case 45: declaration_control_row(DECL_DEPTH)->flags &= ~2; break;
    case 70: case 71: case 72: result.declaration_flags = 2; break;
    case 74: result.declaration_flags = right[0].declaration_flags; break;
    case 79:
        if (DECL_DEPTH > 0) --DECL_DEPTH;
        declaration_control_row(DECL_DEPTH)->flags |= 4;
        break;
    case 121: LEXER_FLAGS &= ~1U; break;
    case 123: case 124: --frontend_data.parser_classification_state; break;
    case 125: LEXER_FLAGS &= 0x7fffffff; break;
    case 134:
        ++DECL_DEPTH;
        declaration_control_row(DECL_DEPTH)->flags &= ~3;
        break;
    case 135:
        declaration_control_row(DECL_DEPTH)->flags &= ~3;
        if (DECL_DEPTH > 0) --DECL_DEPTH;
        break;
    case 162:
        if (DECL_DEPTH > 0) --DECL_DEPTH;
        declaration_control_row(DECL_DEPTH)->flags |= 2;
        frontend_data.compiler_mode_flags &= ~0x10U;
        LEXER_FLAGS &= 0x7fffffff;
        break;
    case 167:
        frontend_data.compiler_mode_flags |= 0x10;
        LEXER_FLAGS |= 0x80000000;
        break;
    case 168: LEXER_FLAGS |= 0x80000000; break;
    case 169:
        frontend_data.compiler_mode_flags |= 0x10;
        LEXER_FLAGS |= 0x80000000;
        ++DECL_DEPTH;
        declaration_control_row(DECL_DEPTH)->flags &= ~3;
        break;
    default:
        /* The remaining recovered productions have no semantic action;
         * 179..182 bypass the original178-case switch in the same way. */
        break;
    }
push_state:
    ++state_index;
    if (state_index > 150) {
        report_declaration_parser_error("yacc stack overflow");
        return 1;
    }
    if (value_index + 1 >= 150)
        { __asm { int 3 } }
    state_top = &states[state_index];
    *state_top = state;
    ++value_index;
    value_top = &values[value_index];
    memcpy(value_top, &result, 40);
    goto read_action;
}
extern "C" void __cdecl report_declaration_parser_error(const char *) { __asm { int 3 } }
void RECON_THISCALL CompilerDeclarationParser::Finish(void)
{
}
extern "C" int __cdecl keep_declaration_scope(unsigned) { __asm { int 3 } return 0; }
extern "C" void __cdecl complete_declaration_reference(ParserTokenView *token)
{
    ParserDeclarationView *declaration = &((ParserDeclarationToken *)token)->value;
    CompilerType *type = declaration->type;
    CompilerSymbol *symbol;
    CompilerNameEntry *name;
    unsigned flags = type->flags;
    int index;
    if (!(flags & 0x9c0) && !type->shape) {
        if (frontend_data.declaration_context_index >= 0) {
            index = (short)type->auxiliary;
            if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
            symbol = (CompilerSymbol *)frontend_data.frontend_pointer_table->items[index];
            token->kind = 0xdf;
            TOKEN_SEMANTIC(token).name = ((CompilerNamedSymbol *)symbol)->link.name;
            return;
        }
        symbol = declaration->object;
        if (symbol->definition_flags & 0x60) {
            complete_deferred_declaration(token, type);
            return;
        }
        if (frontend_data.declaration_classification_depth == 1 && token->kind == 0xaa &&
            !(frontend_data.compiler_mode_flags & 0x100) &&
            !(declaration_control_row(DECL_DEPTH)->flags & 2)) {
            discard_deferred_declaration(token);
            return;
        }
        if ((flags & 0x2fdf) == 0x12 || (flags & 0x2fdf) == 0x11)
            name = ((CompilerNamedSymbol *)symbol)->link.name;
        else {
            index = (short)type->auxiliary;
            if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
            symbol = (CompilerSymbol *)frontend_data.frontend_pointer_table->items[index];
            name = ((CompilerNamedSymbol *)symbol)->link.name;
        }
        token->kind = 0xdf;
        TOKEN_SEMANTIC(token).name = name;
        declaration_control_row(DECL_DEPTH)->flags |= 2;
        return;
    }
    if (!(declaration_control_row(DECL_DEPTH)->flags & 3)) {
        if (token->kind == 0xaa && (LEXER_FLAGS & 0x40000000)) {
            LEXER_FLAGS &= ~0x40000000U;
            TOKEN_STREAM->Unread((unsigned)token);
            classify_parser_declaration(1);
        } else discard_deferred_declaration(token);
        return;
    }
    symbol = declaration->object;
    if (!symbol) {
        complete_deferred_declaration(token, type);
        return;
    }
    token->kind = 0xdf;
    TOKEN_SEMANTIC(token).name = ((CompilerNamedSymbol *)symbol)->link.name;
}
extern "C" void __cdecl report_declaration_syntax(unsigned) { __asm { int 3 } }
extern "C" ParserTokenView *__cdecl parse_declaration_body(ParserTokenView *token)
{
    ParserTokenView *lookahead = TOKEN_CONTEXT.ReadToken();
    if (LEXER_FLAGS & 0x10000000) {
        if (LEXER_FLAGS & 8) {
            token->kind = 0x24;
            LEXER_FLAGS &= ~8U;
        } else token->kind = 0xaa;
        TOKEN_STREAM->Unread((unsigned)lookahead);
        return token;
    }
    if ((LEXER_FLAGS & 0x40000000) && (LEXER_FLAGS & 0x08000000)) {
        complete_deferred_declaration(token, TOKEN_SEMANTIC(token).declaration.type);
        TOKEN_STREAM->Unread((unsigned)lookahead);
        return token;
    }
    record_declaration_body(token);
    LEXER_FLAGS = (LEXER_FLAGS & ~0x40000000U) | 0x08000000;
    return classify_parser_declaration(1);
}
extern "C" void __cdecl record_declaration_tokens(unsigned);
extern "C" void __cdecl finish_recorded_declaration(void);
extern "C" void __cdecl record_declaration_body(ParserTokenView *token)
{
    unsigned kind;
    CompilerArena *arena;
    CompilerSymbolArenaFrame *saved;
    CompilerRecordedLexer *lexer, *recorded;
    FrontendContextFrame *frame;
    FrontendContextPopCallback callback;
    ParserDeclarationToken *copy;
    LEXER_FLAGS = (LEXER_FLAGS & ~8U) | 0x10000000;
    if ((frontend_data.compiler_mode_flags & 0x10000000) ||
        (frontend_data.source_mode_flags & 8)) kind = 0;
    else kind = 2;
    arena = &frontend_bss.arenas[kind];
    saved = (CompilerSymbolArenaFrame *)arena->Allocate(8);
    if (saved) {
        saved->kind = frontend_bss.symbol_arena_kind;
        saved->previous = frontend_bss.saved_symbol_arenas;
    }
    frontend_bss.saved_symbol_arenas = saved;
    frontend_bss.symbol_arena_kind = kind;
    frontend_data.tagged_type_arena_index = frontend_data.constant_arena_index = kind;
    lexer = (CompilerRecordedLexer *)arena->Allocate(64);
    if (lexer) lexer = lexer->Initialize(kind, 1);
    frontend_bss.context_stack.Push(lexer, kind, 0, NULL, &frontend_bss.saved_context_stack);
    if (token) {
#ifdef __linux__
        /* This arm carries the actual typed declaration callback. Ordinary
         * token storage cannot supply its wider native representation. */
        if (token->kind != 0x24)
            recon_unsupported("body recorder: wider token representation pending");
#endif
        if (frontend_bss.symbol_arena_kind) {
            unsigned index = frontend_bss.symbol_arena_kind;
            if (index >= 9) { __asm { int 3 } }
            copy = (ParserDeclarationToken *)frontend_bss.arenas[index].Allocate(sizeof(ParserDeclarationToken));
        } else copy = (ParserDeclarationToken *)frontend_bss.pools[7].Allocate(sizeof(ParserDeclarationToken));
        if (copy) {
#ifdef __linux__
            memcpy(copy, token, 44);
            copy->value.callback = ((ParserDeclarationToken *)token)->value.callback;
#else
            memcpy(copy, token, 48);
#endif
        }
        ((CompilerTokenStream *)frontend_bss.context_stack.top->object)->Trace((unsigned)copy);
        ((CompilerTokenStream *)frontend_bss.context_stack.top->object)->Trace(
            (unsigned)frontend_data.canonical_token_records[117]);
        record_declaration_tokens(1);
    } else record_declaration_tokens(0);
    ((CompilerTokenStream *)frontend_bss.context_stack.top->object)->Trace(
        (unsigned)frontend_data.canonical_token_records[243]);
    frame = frontend_bss.context_stack.top;
    if (frame) {
        recorded = (CompilerRecordedLexer *)frame->object;
        frontend_bss.context_stack.top = frame->previous;
        frontend_bss.context_stack.origin = frame->saved_origin;
        frontend_bss.context_stack.position = frame->saved_position;
        callback = frame->callback;
        if (callback) callback();
    } else recorded = NULL;
    frontend_bss.saved_context_stack.Push(recorded, 2, 1,
        finish_recorded_declaration, &frontend_bss.saved_context_stack);
    LEXER_FLAGS &= ~0x10000000U;
    if ((frontend_data.compiler_mode_flags & 0x32000000) ||
        ((frontend_data.compiler_mode_flags & 0x40000000) &&
         !(frontend_data.source_mode_flags & 8) && !(LEXER_FLAGS & 0x200)) ||
        (frontend_data.source_mode_flags & 0x10) ||
        ((frontend_data.compiler_mode_flags & 0x04000000) && !(LEXER_FLAGS & 0x8000)))
        kind = 0;
    else kind = (frontend_data.source_mode_flags & 1) ? 4 : 5;
    frontend_data.tagged_type_arena_index = frontend_data.constant_arena_index = kind;
    saved = frontend_bss.saved_symbol_arenas;
    frontend_bss.symbol_arena_kind = saved->kind;
    frontend_bss.saved_symbol_arenas = saved->previous;
}
/* This cast-token consumer addresses only the symbol at original+10.
 * The producer's other fields and allocation remain behind its pending entry. */
struct ParserRecordedCastView {
    BYTE unmodeled_00_0f[16];
    CompilerSymbol *symbol;
};
extern "C" CompilerSymbolBinding *__cdecl clone_symbol_binding(CompilerSymbolBinding *, unsigned);
extern "C" void __cdecl resolve_tag_template_id(ParserTokenView **, CompilerSymbol *, unsigned, unsigned);
extern "C" void __cdecl record_declaration_tokens(unsigned initial_parentheses)
{
    ParserTokenView *token, *next, *copy;
    CompilerSymbol *symbol, *enclosing;
    CompilerRecordedLexer *stream;
    CompilerArena *arena;
    CompilerPool *pool;
    unsigned kind, arena_kind, bytes;
    int parentheses = (int)initial_parentheses;
    int angles = 0, questions = 0;
    for (;;) {
        if (frontend_data.compiler_mode_flags & 0x08000000) {
            token = TOKEN_CONTEXT.ReadToken();
            if (token->kind == 6 && !(frontend_data.compiler_mode_flags & 0x10000000)) {
                next = TOKEN_CONTEXT.ReadToken();
                unsigned flags = TOKEN_SEMANTIC(token).name->flags;
                TOKEN_STREAM->Unread((unsigned)next);
                if ((flags & 0x04000000) || ((flags & 0x20000) && next->kind == 0x44)) {
                    int index = frontend_data.declaration_context_index;
                    if (index < 0) enclosing = NULL;
                    else {
                        if ((unsigned)index >= 16) { __asm { int 3 } }
                        enclosing = (CompilerSymbol *)frontend_bss.class_scope_rows[index].symbol;
                    }
                    resolve_tag_template_id(&token, enclosing, 0, 0);
                }
            } else if (token->kind == 0x98) {
                symbol = ((ParserRecordedCastView *)TOKEN_SEMANTIC(token).pointer)->symbol;
                if (symbol->kind == 18) {
                    stream = (CompilerRecordedLexer *)frontend_bss.context_stack.top->object;
                    arena_kind = stream->arena_kind;
                    if (arena_kind == 0 || arena_kind == 2)
                        ((CompilerBoundSymbol *)symbol)->binding = clone_symbol_binding(
                            ((CompilerBoundSymbol *)symbol)->binding, arena_kind);
                }
            }
        } else token = get_next_token();
        kind = token->kind;
        /* The established prototype owner covers kinds1..244. Kind0's
         * preceding DWORD has not been encountered; do not index before it. */
        if (kind == 0 || kind >= 245) { __asm { int 3 } }
        copy = token;
        if (!frontend_data.canonical_token_records[kind - 1]) {
            bytes = 48;
#ifdef __linux__
            if (kind == 0x22 || kind == 0x24 || kind == 0xaa) bytes = sizeof(ParserDeclarationToken);
#endif
            arena_kind = frontend_bss.symbol_arena_kind;
            if (arena_kind) {
                if (arena_kind >= 9) { __asm { int 3 } }
                arena = &frontend_bss.arenas[arena_kind];
                copy = (ParserTokenView *)arena->cursor;
                arena->cursor += bytes;
                if (arena->cursor > arena->committed_limit) arena->CommitExtent();
            } else {
                pool = &frontend_bss.pools[7];
                if (pool->remaining < bytes) pool->Refill(bytes);
                copy = (ParserTokenView *)pool->cursor;
                pool->cursor += bytes; pool->remaining -= bytes;
            }
            if (copy) {
#ifdef __linux__
                if (kind == 0x22 || kind == 0x24 || kind == 0xaa) {
                    memcpy(copy, token, 44);
                    ((ParserDeclarationToken *)copy)->value.callback =
                        ((ParserDeclarationToken *)token)->value.callback;
                } else
#endif
                    memcpy(copy, token, 48);
            }
        }
        ((CompilerTokenStream *)frontend_bss.context_stack.top->object)->Trace((unsigned)copy);
        switch (token->kind) {
        case 0: case 117: case 123: case 127: case 172:
            return;
        case 54:
            if (--questions < 0) return;
            break;
        case 55:
            if (!parentheses && (frontend_data.compiler_mode_flags & 0x08000000)) return;
            break;
        case 62:
            if ((frontend_data.compiler_mode_flags & 0x20000000) && !parentheses) return;
            if (frontend_data.compiler_mode_flags & 0x10000000) {
                if (!angles) return;
                --angles;
            }
            break;
        case 68:
            if (frontend_data.compiler_mode_flags & 0x10000000) ++angles;
            break;
        case 86:
            ++questions;
            break;
        case 118:
            ++parentheses;
            break;
        case 126:
            if (--parentheses < 0) return;
            break;
        }
    }
}
extern "C" CompilerSymbolBinding *__cdecl clone_symbol_binding(CompilerSymbolBinding *, unsigned) { __asm { int 3 } return NULL; }
extern "C" void __cdecl finish_recorded_declaration(void)
{
    LEXER_FLAGS &= 0xb7ffffff;
}

extern "C" CompilerSymbol *__cdecl allocate_tag_symbol(CompilerNameEntry *, WORD, ParserDeclarationCallback *, int);
extern "C" void __cdecl finish_anonymous_union_declaration(CompilerDeclaration *); /*88761*/
extern "C" void __cdecl finish_declaration_specifiers(CompilerDeclaration *declaration)
{
    CompilerType *type = declaration->type;
    CompilerDeclarationContext *context = &declaration->context;
    CompilerGlobalSymbolView *tag;
    CompilerSymbol *symbol, *owner;
    CompilerScopeTable *scope;
    DeclarationClassScope *row;
    ParserDeclarationCallback callback;
    unsigned code = type->flags & 0x2fff;
    int index;
    if (code == 0x41 || code == 0x42 || code == 0x44 || code == 0x80 ||
        (code >= 0x106 && code <= 0x108)) {
        if (!frontend_data.options.language_extensions) {
            compiler_warning(0x3b, 0x4d, 0x177);
            return;
        }
        begin_declaration_diagnostic();
        report_language_diagnostic(2, 0x5b,
            format_declaration_context(describe_declaration_context(0, *context)), type);
        end_declaration_diagnostic();
        return;
    }
    if (code != 0x402 && code != 0x404 && code != 0x408) return;
    if (context->context_flags & 4) return;
    index = (short)type->auxiliary;
    if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
    tag = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[index];
    if (tag->scope_flags & 1) {
        if (LEXER_FLAGS & 0x200) return;
        if ((type->flags & 0x404) == 0x404) {
            finish_anonymous_union_declaration(declaration);
            return;
        }
        if (frontend_data.options.language_extensions)
            report_language_diagnostic(2, 0x5e, type);
        else compiler_warning(0x3b, 0x4d, 0x177);
        return;
    }
    if (!(LEXER_FLAGS & 0x200) &&
        (!(context->class_flags & 0x20) || (context->context_flags & 3) ||
         (context->member_flags & 0xf) || (context->reference_flags & 0x9d) ||
         (context->member_flags & 0x40) || context->function_context_active ||
         context->class_context || (context->context_flags & 0x40))) {
        begin_declaration_diagnostic();
        report_language_diagnostic(1, 0x5b,
            format_declaration_context(describe_declaration_context(0, *context)), type);
        end_declaration_diagnostic();
        return;
    }
    if (frontend_bss.tag_declaration_changed) return;
    callback = &CompilerTranslationUnit::InsertCurrentScope;
    symbol = allocate_tag_symbol(tag->name, type->flags, &callback, 0);
    if (callback) (frontend_data.translation_unit->*callback)(symbol);
    if (!symbol) return;
    index = frontend_data.declaration_context_index;
    if (index >= 0) {
        if (index >= 16) { __asm { int 3 } }
        row = &frontend_bss.class_scope_rows[index];
        symbol->definition_flags ^= (symbol->definition_flags ^ row->access_flags) & 3;
        symbol->declaration_qualifiers = row->symbol->class_detail->scope_number;
    } else {
        scope = frontend_data.translation_unit->current;
        if (!scope->level && scope != frontend_data.translation_unit->root) {
            owner = frontend_data.translation_unit->CurrentDeclarationOwner();
            symbol->declaration_qualifiers =
                ((CompilerNamespaceData *)((CompilerIrSymbol *)owner)->payload)->scope_number;
        }
    }
}
extern "C" void __cdecl finish_anonymous_union_declaration(CompilerDeclaration *)
{ __asm { int 3 } }

extern "C" BYTE *__cdecl allocate_token_storage(unsigned, unsigned); /*40bf7*/
extern "C" CompilerNameEntry *__cdecl find_or_create_identifier(const char *);
extern "C" ParserTokenView *__cdecl process_parser_declaration(void)
{
    /* Tokens whose encountered classification byte admits operator names.
     * Only that semantic bit is represented, not the neighboring flags. */
    static const BYTE operator_name_tokens[] = {
        49,50,51,52,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,
        72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,
        94,97,104,110,115,124,128,129,131,134,140,141,146,147,148,149,
        160,161,166,238
    };
    ParserTokenView *token, *following, *result;
    CompilerNameEntry *name;
    CompilerArena *arena;
    CompilerPool *pool;
    unsigned kind, arena_kind, i;
    BYTE result_kind = 0x8e;
    int declaration_value = 0;
    LEXER_FLAGS = (LEXER_FLAGS & ~1U) | 0x80000010;
    frontend_data.compiler_mode_flags |= 8;
    token = get_next_token();
    frontend_data.compiler_mode_flags &= ~8U;
    LEXER_FLAGS &= ~0x80000010U;
    kind = token->kind;
    switch (kind) {
    case 141: /* delete, with a distinct [] identity when followed by '[' */
        following = TOKEN_CONTEXT.PeekToken();
        if (following->kind == 65) {
            TOKEN_CONTEXT.ReadToken();
            following = TOKEN_CONTEXT.ReadToken();
            if (following->kind != 122) compiler_warning(0x21f);
            name = frontend_data.builtin_names.operator_delete_array;
        } else name = frontend_data.builtin_names.operator_delete;
        result = (ParserTokenView *)allocate_token_storage(48, frontend_bss.symbol_arena_kind);
        if (result) {
            result->kind = 0x8e;
            TOKEN_SEMANTIC(result).named_token.kind = 0x8d;
            TOKEN_SEMANTIC(result).named_token.name = name;
        }
        return result;
    case 140:
        following = TOKEN_CONTEXT.PeekToken();
        if (following->kind == 65) {
            TOKEN_CONTEXT.ReadToken();
            following = TOKEN_CONTEXT.ReadToken();
            if (following->kind != 122) compiler_warning(0x21f);
            name = frontend_data.builtin_names.operator_new_array;
        } else name = frontend_data.builtin_names.operator_new;
        arena_kind = frontend_bss.symbol_arena_kind;
        if (arena_kind >= 9) { __asm { int 3 } }
        result = arena_kind ? (ParserTokenView *)frontend_bss.arenas[arena_kind].Allocate(48) :
            (ParserTokenView *)frontend_bss.pools[7].Allocate(48);
        if (result) {
            result->kind = 0x8e;
            TOKEN_SEMANTIC(result).named_token.kind = 0x8c;
            TOKEN_SEMANTIC(result).named_token.name = name;
        }
        return result;
    case 52:
        saved_parser_value.named_token.kind = 0x34;
        saved_parser_value.named_token.name = find_or_create_identifier(compiler_token_spellings[kind]);
        goto allocate_named_token;
    case 65:
        token = TOKEN_CONTEXT.ReadToken();
        if (token->kind != 122) {
            compiler_warning(0x21f);
            TOKEN_STREAM->Unread((unsigned)token);
        }
        saved_parser_value.named_token.name = find_or_create_identifier("[]");
        saved_parser_value.named_token.kind = 0x41;
        goto allocate_named_token;
    case 118:
        token = TOKEN_CONTEXT.ReadToken();
        if (token->kind != 126) {
            compiler_warning(0x220);
            TOKEN_STREAM->Unread((unsigned)token);
        }
        saved_parser_value.named_token.name = find_or_create_identifier("()");
        saved_parser_value.named_token.kind = 0x76;
        if ((int)DECL_DEPTH > 0) --DECL_DEPTH;
        goto allocate_named_token;
    case 31: case 36:
        LEXER_FLAGS |= 4;
        result_kind = 0x8f;
        declaration_value = 1;
        goto allocate_named_token;
    case 89:
        LEXER_FLAGS &= ~1U;
        break;
    }
    if (kind >= PARSER_TOKEN_KIND_COUNT) { __asm { int 3 } }
    for (i = 0; i < sizeof(operator_name_tokens); ++i)
        if (operator_name_tokens[i] == kind) break;
    if (i == sizeof(operator_name_tokens)) {
        if (kind == 6) compiler_warning(0x341, 0x49, TOKEN_SEMANTIC(token).name);
        else compiler_warning(0x341, 0x4c, kind);
        return (ParserTokenView *)frontend_data.canonical_token_records[0];
    }
    saved_parser_value.named_token.name = find_or_create_identifier(compiler_token_spellings[kind]);
    saved_parser_value.named_token.kind = TOKEN_SEMANTIC(token).named_token.kind;
    /* The common operator-name path originally advances the arena cursor
     * directly; the specialized spellings above call Allocate instead. */
    arena_kind = frontend_bss.symbol_arena_kind;
    if (arena_kind) {
        if (arena_kind >= 9) { __asm { int 3 } }
        arena = &frontend_bss.arenas[arena_kind];
        result = (ParserTokenView *)arena->cursor;
        arena->cursor += 48;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    } else {
        pool = &frontend_bss.pools[7];
        if (pool->remaining < 48) pool->Refill(48);
        result = (ParserTokenView *)pool->cursor;
        pool->cursor += 48;
        pool->remaining -= 48;
    }
    goto copy_value;
allocate_named_token:
    arena_kind = frontend_bss.symbol_arena_kind;
    if (arena_kind >= 9) { __asm { int 3 } }
    i = declaration_value ? sizeof(ParserDeclarationToken) : 48;
    result = arena_kind ? (ParserTokenView *)frontend_bss.arenas[arena_kind].Allocate(i) :
        (ParserTokenView *)frontend_bss.pools[7].Allocate(i);
copy_value:
    if (result) {
        result->kind = result_kind;
#ifdef __linux__
        if (declaration_value) {
            memcpy(&((ParserDeclarationToken *)result)->value, &saved_parser_value, 36);
            ((ParserDeclarationToken *)result)->value.callback = saved_parser_value.declaration.callback;
        } else memcpy(&result->value, &saved_parser_value, 40);
#else
        memcpy(&result->value, &saved_parser_value, 40);
#endif
    }
    return result;
}
extern "C" BYTE *__cdecl allocate_token_storage(unsigned size, unsigned arena_kind)
{
    BYTE *memory;
    CompilerArena *arena;
    CompilerPool *pool;
    if (arena_kind) {
        if (arena_kind >= 9) { __asm { int 3 } }
        arena = &frontend_bss.arenas[arena_kind];
        memory = arena->cursor;
        arena->cursor += size;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    } else {
        pool = &frontend_bss.pools[7];
        if (pool->remaining < size) pool->Refill(size);
        memory = pool->cursor;
        pool->cursor += size;
        pool->remaining -= size;
    }
    return memory;
}
extern "C" void __cdecl complete_deferred_declaration(ParserTokenView *, CompilerType *) { __asm { int 3 } }

extern "C" ParserRecordedContext *__cdecl parse_recorded_declaration(ParserTokenView *token)
{
    ParserDeclarationReader reader;
    ParserTokenView *result, *body;
    unsigned depth;
    BYTE flags;
    memset(&reader, 0, sizeof(reader));
    reader.token = token;
    result = reader.Read();
#ifdef __linux__
    /* MakeToken's ordinary declaration result has the same typed callback
     * representation regardless of the following lexical token. Body and
     * saved-row consumers retain their own guards below. */
    if (!result || !(((result->kind == 0x22 || result->kind == 0x23) &&
        reader.token->kind == result->kind) || result->kind == 0x24 ||
        (result == reader.token && result->kind == 47)))
        recon_unsupported("declaration reader: wider semantic result pending");
#endif
    depth = DECL_DEPTH;
    /* The control rows have the actual depth<60 bound. Semantic-value
     * rows still have their separate transfer guard below. */
    if (depth >= 60) { __asm { int 3 } }
    if (reader.token->kind == 118) {
        if (reader.context.class_flags & 0x10) goto inspect_result;
        if (LEXER_FLAGS & 0x80000000) goto clear_deferred_flag;
        if ((reader.context.class_flags & 7) == 7) goto inspect_result;
        flags = (*declaration_control_row(depth)).flags;
        if (flags & 8) {
            if ((frontend_data.compiler_mode_flags & 0x20000000) && (flags & 0x80))
                goto read_body;
            if (!(frontend_data.source_mode_flags & 0x10)) goto inspect_result;
            return (ParserRecordedContext *)parse_declaration_body(reader.declaration_token);
        }
read_body:
        if (reader.qualifier_flags && reader.copied_class_head) goto inspect_result;
        body = parse_declaration_body(reader.declaration_token);
        if (body->kind == 0xdf && (int)reader.tokens_read > 2) {
            reader.SaveSemantic();
            depth = DECL_DEPTH;
            if (depth != 0) {
#ifdef __linux__
                recon_unsupported("declaration reader: unmodeled saved declaration row");
#else
                __asm { int 3 }
#endif
            }
            memcpy(declaration_spec_row(0), &saved_parser_value, 40);
            (*declaration_control_row(depth)).flags |= 1;
        }
        return (ParserRecordedContext *)body;
    }
    if (LEXER_FLAGS & 0x80000000) {
clear_deferred_flag:
        LEXER_FLAGS &= ~0x80000000U;
    }
inspect_result:
    if ((result->kind == 0x24 || result->kind == 0xeb) && !(LEXER_FLAGS & 0x10000000)) {
        (*declaration_control_row(depth)).flags |= 2;
        if ((*declaration_control_row(depth)).secondary_flags & 2) {
            (*declaration_control_row(depth)).secondary_flags &= ~2;
            (*declaration_control_row(depth)).flags |= 1;
        }
    }
    return (ParserRecordedContext *)result;
}

extern "C" void __cdecl read_declaration_attributes(CompilerDeclarationContext *);
ParserTokenView *RECON_THISCALL ParserDeclarationReader::Read(void)
{
    int state = 0;
    unsigned token_class;
    for (;;) {
        /* The authenticated token-class map ends at the producer's kind244.
         * Do not turn an unsupported kind into a default/zero source row. */
        if (token->kind >= PARSER_TOKEN_KIND_COUNT) {
#ifdef __linux__
            recon_unsupported("declaration reader: unknown token-class row");
#else
            __asm { int 3 }
#endif
        }
        token_class = parser_token_classes[token->kind];
        switch (token_class) {
        case PARSER_CLASS_6: case PARSER_CLASS_150:
            state = ReadIdentifier();
            break;
        case PARSER_CLASS_15: case PARSER_CLASS_33:
            ReadStorageModifier();
            break;
        case PARSER_CLASS_236:
            if (!(frontend_data.compiler_mode_flags & 0x52000000) &&
                !(frontend_data.source_mode_flags & 0x300008)) {
                compiler_warning(0x383);
                break;
            }
            /* FALL THROUGH */
        case PARSER_CLASS_26: case PARSER_CLASS_27:
        case PARSER_CLASS_28: case PARSER_CLASS_29:
            state = ReadTagSpecifier();
            break;
        case PARSER_CLASS_30:
            ReadTypeName();
            break;
        case PARSER_CLASS_31:
            ReadTypeSpecifier();
            break;
        case PARSER_CLASS_32:
            ReadStorageClass();
            break;
        case PARSER_CLASS_34:
            CopyClassHead();
            break;
        case PARSER_CLASS_40:
            state = ReadQualifier();
            break;
        case PARSER_CLASS_149:
            TOKEN_STREAM->Unread((unsigned)token);
            token = read_qualified_parser_token(0, 0);
            state = token->kind == 0x96 ? ReadIdentifier() : 1;
            break;
        case PARSER_CLASS_177:
            context.reference_flags |= 0x40;
            break;
        case PARSER_CLASS_192:
            read_declaration_attributes(&context);
            declaration_marked = 1;
            break;
        default:
            state = 1;
            break;
        }
        ++tokens_read;
        if (state) break;
        token = TOKEN_CONTEXT.ReadToken();
    }
    if (state == 2) return token;
    TOKEN_STREAM->Unread((unsigned)token);
    if (qualifier_flags || !type) FinalizeType();
    return MakeToken(0x24);
}

extern "C" void __cdecl apply_declaration_specifier(CompilerDeclarationContext *, BYTE, unsigned, ...);
void RECON_THISCALL ParserDeclarationReader::ReadStorageClass(void)
{
    BYTE storage;
    if (frontend_data.compiler_mode_flags & 8) {
        compiler_warning(0x2d1, token->kind);
        context.class_flags |= 8;
        return;
    }
    /* These five language-keyword rows are the consumed BYTE role of the
     * semantic table. Internal token32 and other rows remain unsupported. */
    if (token->kind == 8) storage = 4;       /* auto */
    else if (token->kind == 9) storage = 3;  /* static */
    else if (token->kind == 10) storage = 1; /* extern */
    else if (token->kind == 11) storage = 5; /* register */
    else if (token->kind == 12) storage = 7; /* typedef */
    else {
#ifdef __linux__
        recon_unsupported("storage class: unknown semantic keyword row");
#else
        __asm { int 3 }
#endif
        return;
    }
    apply_declaration_specifier(&context, storage, 1);
    declaration_marked = 1;
}

extern "C" int __cdecl parse_linkage_specifier(CompilerDeclaration *declaration, CompilerExpression *string)
{
    char *text = (char *)string->value.quoted.text;
    int length = 0;
    if (string->value.quoted.flags & 1) compiler_warning(2);
    if (frontend_bss.declaration_scope_mode) compiler_warning(0x256);
    if ((declaration->context.class_flags & 7) != 1 ||
        !(declaration->context.class_flags & 0x10))
        report_language_diagnostic(1, 0x1f6);
    if (text[0] == 'C') {
        if (!text[1]) return 2;
        if (text[1] == '+' && text[2] == '+' && !text[3]) return 1;
    }
    while (text[length]) ++length;
    if (length > 32) {
        if (length > 35) length = 35;
        text[length] = 0;
        text[length - 1] = text[length - 2] = text[length - 3] = '.';
    }
    compiler_warning(0x219, text);
    return 0;
}

extern "C" ParserTokenView *__cdecl parse_tag_specifier(WORD, CompilerDeclarationContext *);
extern "C" CompilerType *__cdecl declaration_symbol_type(CompilerSymbol *);
void RECON_THISCALL ParserDeclarationReader::ReadTypeName(void)
{
    type_symbol = TOKEN_SEMANTIC(token).symbol;
    type = declaration_symbol_type(type_symbol);
    context.class_flags |= 0x80;
}

/* A source-only view of the existing class_detail pointee, not an allocation
 * or default claim. This consumer reads exactly its byte at original+3e. */
struct ParserClassCompletionView {
    BYTE unmodeled_00_3d[62];
    BYTE completion_flags;
};
int RECON_THISCALL ParserDeclarationReader::ReadTagSpecifier(void)
{
    WORD tag;
    ParserTokenView *record;
    frontend_data.compiler_flags |= 0x80000;
    /* Actual WORD rows: class0408,enum0200,struct0402,union0404,typename0012. */
    if (token->kind == 26) tag = 0x0408;
    else if (token->kind == 27) tag = 0x0200;
    else if (token->kind == 28) tag = 0x0402;
    else if (token->kind == 29) tag = 0x0404;
    else if (token->kind == 236) tag = 0x0012;
    else {
#ifdef __linux__
        recon_unsupported("tag specifier: unknown semantic keyword row");
#else
        __asm { int 3 }
#endif
        return 0;
    }
    record = parse_tag_specifier(tag, &context);
    frontend_data.compiler_flags &= ~0x80000U;
    if (record->kind == 0xeb) { token = record; return 2; }
    if (record->kind == 1) {
        token = TOKEN_CONTEXT.ReadToken();
        if (frontend_data.compiler_mode_flags & 0x10000000)
            token = recover_parser_construct(token, 0x3e, 0x7f, 1);
        else token = recover_parser_construct(token, 0x7f, 1);
        return 2;
    }
    if (type || type_flags) {
        compiler_warning(0xec, 0x4c, token->kind, TOKEN_SEMANTIC(record).declaration.object);
        type_flags = 0;
    }
#ifdef __linux__
    if (record->kind != 0x22 && record->kind != 0x23 && record->kind != 0x24)
        recon_unsupported("tag specifier: nondeclaration record layout pending");
#endif
    type_symbol = ((ParserDeclarationToken *)record)->value.object;
    declaration_callback = ((ParserDeclarationToken *)record)->value.callback;
    type = declaration_symbol_type(type_symbol);
    if (context.reference_flags & 0x40) {
        ParserSymbolView *symbol = (ParserSymbolView *)type_symbol;
        if (!symbol || symbol->kind != 4 ||
            !(((ParserClassCompletionView *)symbol->class_detail)->completion_flags & 1))
            compiler_warning(0x391, symbol->link.name);
    }
    if (record->kind != 0x22 && record->kind != 0x23) return 0;
    if (record->kind == 0x23) frontend_data.compiler_flags |= 1;
    FinalizeType();
    if (context.context_flags & 4) compiler_warning(0x187, type_symbol);
    token = MakeToken(record->kind);
    return 2;
}

extern "C" CompilerSymbol *__cdecl reference_tag_name(CompilerNameEntry *);
extern "C" void __cdecl resolve_tag_template_id(ParserTokenView **, CompilerSymbol *, unsigned, unsigned);
extern "C" ParserTokenView *__cdecl parse_tag_specifier(WORD tag, CompilerDeclarationContext *parent)
{
    ParserTagDeclaration *declaration = (ParserTagDeclaration *)frontend_bss.arenas[5].Allocate(sizeof(ParserTagDeclaration));
    ParserTokenView *token, *looked_ahead, *result;
    CompilerNameEntry *name;
    CompilerSymbol *symbol;
    CompilerArena *arena;
    CompilerPool *pool;
    unsigned token_class, arena_kind;
    int template_name = 0;
    if (declaration) {
        declaration->parent_context = parent;
        declaration->name = NULL;
        declaration->symbol = NULL;
        declaration->name_token = NULL;
        declaration->tag_flags = tag;
        declaration->qualifier_flags = 0;
        memset(&declaration->context, 0, 28);
        declaration->callback = NULL;
        declaration->initial_state = 1;
    }
    frontend_bss.tag_novtordisp = 0;
    frontend_data.compiler_mode_flags &= ~0x700000U;
    if (frontend_data.compiler_mode_flags & 0x10000000) {
        unsigned kind = tag & 0x2fdf;
        template_name = ((tag & 0x408) == 0x408) || kind == 0x12 || kind == 0x11;
    }
read_next:
    token = TOKEN_CONTEXT.ReadToken();
    if (token->kind >= PARSER_TOKEN_KIND_COUNT) {
#ifdef __linux__
        recon_unsupported("tag parser: unknown token-class row");
#else
        __asm { int 3 }
#endif
    }
    token_class = parser_token_classes[token->kind];
    switch (token_class) {
    case PARSER_CLASS_0:
        report_compiler_error(4);
        return declaration->FinishDeclaration(token);
    case PARSER_CLASS_6:
        name = (CompilerNameEntry *)token->value.pointer;
        looked_ahead = TOKEN_CONTEXT.PeekToken();
        if (template_name) {
            if (looked_ahead->kind == 52 || looked_ahead->kind == 55 ||
                looked_ahead->kind == 62 || looked_ahead->kind == 197) {
                symbol = reference_tag_name(name);
                arena_kind = frontend_bss.symbol_arena_kind;
                if (arena_kind) {
                    if (arena_kind >= 9) {
#ifdef __linux__
                        recon_unsupported("tag parser: unknown reference arena");
#else
                        __asm { int 3 }
#endif
                    }
                    result = (ParserTokenView *)frontend_bss.arenas[arena_kind].Allocate(48);
                } else result = (ParserTokenView *)frontend_bss.pools[7].Allocate(48);
                if (result) { result->kind = 0xeb; result->value.pointer = symbol; }
                return result;
            }
            template_name = 0;
        }
        if (name->flags & 0x04020000) {
            looked_ahead = TOKEN_CONTEXT.PeekToken();
            if (looked_ahead->kind == 0x44)
                resolve_tag_template_id(&token, 0, 0, (parent->reference_flags >> 6) & 1);
        }
        if (declaration->ReadName(token)) goto read_next;
        goto finish;
    case PARSER_CLASS_54: case PARSER_CLASS_117:
        return declaration->FinishTag(token);
    case PARSER_CLASS_124:
        if (token->kind != 62) goto unread_finish;
        /* FALL THROUGH */
    case PARSER_CLASS_52: case PARSER_CLASS_55: case PARSER_CLASS_197:
        if (!template_name) goto unread_finish;
        symbol = reference_tag_name(frontend_data.builtin_names.unnamed);
        TOKEN_STREAM->Unread((unsigned)token);
        arena_kind = frontend_bss.symbol_arena_kind;
        if (arena_kind) {
            if (arena_kind >= 9) {
#ifdef __linux__
                recon_unsupported("tag parser: unknown anonymous-reference arena");
#else
                __asm { int 3 }
#endif
            }
            arena = &frontend_bss.arenas[arena_kind];
            result = (ParserTokenView *)arena->cursor;
            arena->cursor += 48;
            if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        } else {
            pool = &frontend_bss.pools[7];
            if (pool->remaining < 48) pool->Refill(48);
            result = (ParserTokenView *)pool->cursor;
            pool->cursor += 48;
            pool->remaining -= 48;
        }
        if (result) { result->kind = 0xeb; result->value.pointer = symbol; }
        return result;
    case PARSER_CLASS_127:
        goto unread_finish;
    case PARSER_CLASS_149:
        TOKEN_STREAM->Unread((unsigned)token);
        if (declaration->ReadQualifiedName()) goto read_next;
        goto finish;
    case PARSER_CLASS_150:
        if (declaration->ReadScopedName(token)) goto read_next;
        goto finish;
    case PARSER_CLASS_174:
        frontend_bss.tag_novtordisp = 1;
        goto read_next;
    case PARSER_CLASS_183:
        if (token->kind == 184)
            frontend_data.compiler_mode_flags = (frontend_data.compiler_mode_flags & ~0x600000U) | 0x100000;
        else if (token->kind == 185)
            frontend_data.compiler_mode_flags = (frontend_data.compiler_mode_flags & ~0x500000U) | 0x200000;
        else if (token->kind == 186)
            frontend_data.compiler_mode_flags = (frontend_data.compiler_mode_flags & ~0x400000U) | 0x300000;
        else goto finish;
        goto read_next;
    case PARSER_CLASS_192:
        if (declaration->ReadAttributes()) goto read_next;
        goto unread_finish;
    default:
        declaration->initial_state = 0;
        goto unread_finish;
    }
unread_finish:
    TOKEN_STREAM->Unread((unsigned)token);
finish:
    return declaration->FinishDeclaration(token);
}

/* The resolved-name token payload exposes only this first pointer. No other
 * record fields or allocation extent are inferred from the dereference. */
struct ParserTagNameReference { CompilerNameEntry *name; };
int RECON_THISCALL ParserTagDeclaration::ReadName(ParserTokenView *token)
{
    if (!name && !symbol) {
        name_token = token;
        if (token->kind == 6) name = (CompilerNameEntry *)token->value.pointer;
        else name = ((ParserTagNameReference *)token->value.pointer)->name;
        return 1;
    }
    TOKEN_STREAM->Unread((unsigned)token);
    initial_state = 0;
    return 0;
}

extern "C" {
CompilerSymbol *__cdecl create_tag_symbol(CompilerNameEntry *, WORD, ParserDeclarationCallback *);
void __cdecl apply_tag_metadata(CompilerSymbol *, CompilerDeclarationContext *);
ParserTokenView *__cdecl begin_tag_replay(ParserTokenView *, CompilerNameEntry *);
CompilerSymbol *__cdecl complete_tag_symbol(CompilerSymbol *, WORD);
const char *__cdecl format_type_code(WORD);
const char *__cdecl format_diagnostic_symbol(CompilerSymbol *);
CompilerSymbol *__cdecl instantiate_declaration_class(CompilerSymbol *);
CompilerBackend *__cdecl acquire_initial_backend(void);
int __cdecl should_report_diagnostic(unsigned, unsigned);
void __cdecl print_diagnostic_auxiliary(unsigned, ...);
}
/* The following are views of existing tagged pointees. They allocate nothing,
 * initialize no opaque bytes and make no complete-owner equality claim. */
typedef CompilerGlobalClassDetail ParserDefinitionClassView;
struct ParserClassAccessRow {
    CompilerGlobalSymbolView *symbol;
    BYTE unmodeled_04_1c[25]; BYTE access_flags; BYTE unmodeled_1e_1f[2];
};
typedef CompilerNamespaceData ParserOwnerScopeView;
struct ParserAliasReference { BYTE unmodeled_00_07[8]; CompilerSymbol *symbol; };
struct ParserTagAliasPayload { ParserAliasReference *reference; };
#define TAG_SYMBOL ((CompilerGlobalSymbolView *)symbol)
#define TAG_DETAIL ((ParserDefinitionClassView *)TAG_SYMBOL->class_detail)
ParserTokenView *RECON_THISCALL ParserTagDeclaration::FinishTag(ParserTokenView *token)
{
    ParserTokenView *result;
    CompilerSymbol *instantiated, *owner;
    CompilerScopeMatch *lookup;
    CompilerLexerBackend *backend_state;
    CompilerArena *arena;
    CompilerPool *pool;
    CompilerScopeTable *scope;
    const char *old_name;
    unsigned arena_kind;
    int index;
    TOKEN_STREAM->Unread((unsigned)token);
    if (frontend_data.compiler_mode_flags & 0x02000000) {
        token = begin_tag_replay(token, name);
        backend_state = frontend_bss.lexer_backend;
        backend_state->backend = frontend_data.backend;
        frontend_data.backend = acquire_initial_backend();
        backend_state->enabled = 0;
        result = FinishDeclaration(token);
        frontend_data.backend = frontend_bss.lexer_backend->backend;
        frontend_bss.lexer_backend->enabled = 1;
        if (frontend_data.translation_unit->root != frontend_data.translation_unit->current) {
            scope = frontend_data.translation_unit->current;
            if ((!scope->level || scope->level == 0xff) && !TAG_SYMBOL->declaration_qualifiers) {
                index = frontend_data.declaration_context_index;
                if (index >= 0) {
                    /* The shared source-only table still lacks its wider
                     * transfer projection. Both builds stop before reading it. */
#ifdef __linux__
                    recon_unsupported("tag definition: class access row pending");
#else
                    __asm { int 3 }
#endif
                    ParserClassAccessRow *row =
                        &((ParserClassAccessRow *)class_scopes)[index];
                    TAG_SYMBOL->definition_flags = (TAG_SYMBOL->definition_flags & ~3) | (row->access_flags & 3);
                    TAG_SYMBOL->declaration_qualifiers =
                        ((ParserDefinitionClassView *)row->symbol->class_detail)->scope_number;
                } else {
                    owner = frontend_data.translation_unit->CurrentDeclarationOwner();
                    TAG_SYMBOL->declaration_qualifiers =
                        ((ParserOwnerScopeView *)((CompilerIrSymbol *)owner)->payload)->scope_number;
                }
            }
        }
        return result;
    }
    if ((frontend_data.source_mode_flags & 0x100000) && frontend_data.declaration_context_index < 0) {
        lookup = frontend_data.translation_unit->LookupName(name, 0x20);
        symbol = lookup ? lookup->symbol : NULL;
        if (TAG_SYMBOL->kind == 0x12)
            symbol = ((ParserTagAliasPayload *)((CompilerIrSymbol *)symbol)->payload)->reference->symbol;
        TAG_DETAIL->completion_flags |= 0x40;
    }
    if (symbol) {
        old_name = format_diagnostic_symbol(symbol);
        symbol = complete_tag_symbol(symbol, tag_flags);
    } else {
        symbol = create_tag_symbol(name, tag_flags, &callback);
        if (!symbol) {
            if (name->flags & 0x02000000) return (ParserTokenView *)frontend_data.canonical_token_records[0];
            old_name = name->name;
        }
    }
    if (!symbol) {
        compiler_warning(0x289, old_name, format_type_code(tag_flags));
        return (ParserTokenView *)frontend_data.canonical_token_records[0];
    }
    if (TAG_SYMBOL->scope_flags & 4) return (ParserTokenView *)frontend_data.canonical_token_records[0];
    saved_parser_value.declaration.callback = callback;
    apply_tag_metadata(symbol, &context);
    if (tag_flags & 0x600) TAG_DETAIL->completion_flags |= 4;
    if (tag_flags == 0x200) {
        frontend_data.compiler_flags |= 1;
        saved_parser_value.declaration.object = symbol;
        arena_kind = frontend_bss.symbol_arena_kind;
        if (arena_kind) {
            if (arena_kind >= 9) {
#ifdef __linux__
                recon_unsupported("tag definition: unknown enum arena");
#else
                __asm { int 3 }
#endif
            }
            result = (ParserTokenView *)frontend_bss.arenas[arena_kind].Allocate(sizeof(ParserDeclarationToken));
        } else result = (ParserTokenView *)frontend_bss.pools[7].Allocate(sizeof(ParserDeclarationToken));
        if (result) {
            result->kind = 0x23;
#ifdef __linux__
            /* Preserve opaque prefix bytes; the callback is a real native
             * member pointer, never truncated or raw-copied as original4B. */
            memcpy(&((ParserDeclarationToken *)result)->value, &saved_parser_value, 36);
            ((ParserDeclarationToken *)result)->value.callback = saved_parser_value.declaration.callback;
#else
            memcpy(&result->value, &saved_parser_value, 40);
#endif
        }
        return result;
    }
    if (context.member_flags & 0xc) {
        TAG_SYMBOL->member_flags = (TAG_SYMBOL->member_flags & ~4) | (context.member_flags & 4);
        TAG_SYMBOL->member_flags = (TAG_SYMBOL->member_flags & ~8) | (context.member_flags & 8);
    }
    if (context.class_context) TAG_DETAIL->declaration_flags |= 0x80;
    if ((context.context_flags & 7) || (context.member_flags & 3) ||
        (context.reference_flags & 0x9d) || (context.member_flags & 0x40)) {
        begin_declaration_diagnostic();
        report_language_diagnostic(1, 0x26,
            format_declaration_context(describe_declaration_context(0, context)));
        end_declaration_diagnostic();
    }
    if (TAG_DETAIL->completion_flags & 1) {
        if (!(TAG_DETAIL->template_flags & 1)) {
            instantiated = instantiate_declaration_class(symbol);
            if (frontend_data.options.language_extensions) {
                if (should_report_diagnostic(4, 0x297)) {
                    report_language_diagnostic(4, 0x297, ((CompilerNamedSymbol *)instantiated)->link.name);
                    print_diagnostic_auxiliary(0x1903, TAG_SYMBOL->type, TAG_SYMBOL->name);
                }
            } else {
                compiler_warning(0x38a, ((CompilerNamedSymbol *)instantiated)->link.name);
                print_diagnostic_auxiliary(0x1903, TAG_SYMBOL->type, TAG_SYMBOL->name);
            }
            TAG_DETAIL->template_flags |= 1;
        }
        TAG_DETAIL->completion_flags |= 4;
    }
    saved_parser_value.declaration.object = symbol;
    arena_kind = frontend_bss.symbol_arena_kind;
    if (arena_kind) {
        if (arena_kind >= 9) {
#ifdef __linux__
            recon_unsupported("tag definition: unknown class arena");
#else
            __asm { int 3 }
#endif
        }
        arena = &frontend_bss.arenas[arena_kind];
        result = (ParserTokenView *)arena->cursor;
        arena->cursor += sizeof(ParserDeclarationToken);
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    } else {
        pool = &frontend_bss.pools[7];
        if (pool->remaining < sizeof(ParserDeclarationToken)) pool->Refill(sizeof(ParserDeclarationToken));
        result = (ParserTokenView *)pool->cursor;
        pool->cursor += sizeof(ParserDeclarationToken);
        pool->remaining -= sizeof(ParserDeclarationToken);
    }
    if (result) {
        result->kind = 0x22;
#ifdef __linux__
        memcpy(&((ParserDeclarationToken *)result)->value, &saved_parser_value, 36);
        ((ParserDeclarationToken *)result)->value.callback = saved_parser_value.declaration.callback;
#else
        memcpy(&result->value, &saved_parser_value, 40);
#endif
    }
    return result;
}
#undef TAG_SYMBOL
#undef TAG_DETAIL

extern "C" {
CompilerSymbol *__cdecl allocate_tag_symbol(CompilerNameEntry *, WORD, ParserDeclarationCallback *, int);
CompilerSymbol *__cdecl refine_forward_tag(CompilerSymbol *, WORD);
CompilerSymbol *__cdecl find_or_declare_tag_reference(CompilerNameEntry *, WORD, ParserDeclarationCallback *);
void __cdecl validate_tag_declaration_context(CompilerSymbol *, CompilerDeclarationContext *);
void __cdecl prepare_tag_replay_state(void);
CompilerSymbol *__cdecl find_bound_declaration(CompilerSymbol *, CompilerSymbol *);
}

ParserTokenView *RECON_THISCALL ParserTagDeclaration::FinishDeclaration(ParserTokenView *token)
{
    CompilerSymbol *original = NULL, *candidate, *owner;
    CompilerScopeTable *scope;
    CompilerScopeLookup *matches;
    CompilerScopeMatch *match;
    CompilerArena *arena;
    CompilerPool *pool;
    ParserTokenView *result;
    unsigned arena_kind;
    int index;
    const char *type_text, *symbol_text;
    if (frontend_data.compiler_mode_flags & 0x02000000) prepare_tag_replay_state();
    if ((tag_flags & 0x2fdf) != 0x12) {
        if (symbol) {
            original = symbol;
            symbol = refine_forward_tag(symbol, tag_flags);
        } else if (parent_context->context_flags & 4) {
            callback = frontend_bss.declaration_scope_mode ?
                &CompilerTranslationUnit::InsertScopedTag : &CompilerTranslationUnit::InsertTagScope;
            symbol = find_or_declare_tag_reference(name, tag_flags, &callback);
        } else {
            scope = frontend_data.translation_unit->current;
            if (!scope->level && !DECL_DEPTH) {
                if (initial_state) {
                    matches = scope->Find(name, 1);
                    match = matches ? matches->Select(1) : NULL;
                } else {
                    LEXER_FLAGS |= 0x10;
                    match = frontend_data.translation_unit->LookupName(name, 0x132);
                    LEXER_FLAGS &= ~0x10U;
                }
                candidate = match ? match->symbol : NULL;
                if (candidate) {
                    if (candidate->definition_flags & 0x20) {
                        LEXER_FLAGS |= 0x10;
                        candidate = find_bound_declaration(candidate, NULL);
                        LEXER_FLAGS &= ~0x10U;
                    }
                    if (candidate->kind == 4) {
                        if ((frontend_data.compiler_mode_flags & 0x02000000) &&
                            !(((CompilerGlobalSymbolView *)candidate)->class_detail->completion_flags & 2)) {
                            candidate->scope_flags |= 4;
                            compiler_warning(0x3dd, candidate);
                        }
                        symbol = refine_forward_tag(candidate, tag_flags);
                    } else {
                        if (frontend_data.compiler_mode_flags & 0x02000000)
                            compiler_warning(0x388, ((CompilerNamedSymbol *)candidate)->link.name, candidate);
                        callback = &CompilerTranslationUnit::InsertCurrentScope;
                        symbol = allocate_tag_symbol(name, tag_flags, &callback, 0);
                        symbol = refine_forward_tag(symbol, tag_flags);
                        frontend_bss.tag_declaration_changed = 1;
                    }
                } else {
                    callback = &CompilerTranslationUnit::InsertCurrentScope;
                    symbol = allocate_tag_symbol(name, tag_flags, &callback, 0);
                    symbol = refine_forward_tag(symbol, tag_flags);
                    frontend_bss.tag_declaration_changed = 1;
                }
            } else if (frontend_data.declaration_context_index >= 0 && token->kind == 0x7f) {
                callback = &CompilerTranslationUnit::InsertCurrentScope;
                symbol = allocate_tag_symbol(name, tag_flags, &callback, 0);
                symbol = refine_forward_tag(symbol, tag_flags);
                if (symbol && !symbol->declaration_qualifiers) {
                    index = frontend_data.declaration_context_index;
                    if (index >= 0) {
                        if (index >= 16) { __asm { int 3 } }
                        DeclarationClassScope *row = &frontend_bss.class_scope_rows[index];
                        symbol->definition_flags ^= (symbol->definition_flags ^ row->access_flags) & 3;
                        symbol->declaration_qualifiers = row->symbol->class_detail->scope_number;
                    } else if (!frontend_data.translation_unit->current->level) {
                        owner = frontend_data.translation_unit->CurrentDeclarationOwner();
                        symbol->declaration_qualifiers =
                            ((CompilerNamespaceData *)((CompilerIrSymbol *)owner)->payload)->scope_number;
                    }
                }
            } else {
                if (frontend_data.declaration_context_index < 0 &&
                    (frontend_data.compiler_mode_flags & 0x10000000) &&
                    (token->kind == 0x37 || token->kind == 0x3e || token->kind == 0x34 || token->kind == 0xc5)) {
                    if (!name) {
                        compiler_warning(0x3e2);
                        symbol = NULL;
                        return (ParserTokenView *)frontend_data.canonical_token_records[0];
                    }
                    callback = NULL;
                    symbol = allocate_tag_symbol(name, tag_flags, &callback, 0);
                    symbol = refine_forward_tag(symbol, tag_flags);
                    /* The genuine zero/member callback returned by allocation
                     * is retained on this route, unlike the ordinary path. */
                } else callback = &CompilerTranslationUnit::InsertTagScope;
                symbol = find_or_declare_tag_reference(name, tag_flags, &callback);
            }
        }
    }
    saved_parser_value.declaration.object = symbol;
    if (!symbol) {
        type_text = format_type_code(tag_flags);
        symbol_text = format_diagnostic_symbol(original);
        compiler_warning(0x289, symbol_text, type_text);
        return (ParserTokenView *)frontend_data.canonical_token_records[0];
    }
    apply_tag_metadata(symbol, &context);
    validate_tag_declaration_context(symbol, &context);
    saved_parser_value.declaration.callback = callback;
    arena_kind = frontend_bss.symbol_arena_kind;
    if (arena_kind) {
        if (arena_kind >= 9) { __asm { int 3 } }
        arena = &frontend_bss.arenas[arena_kind];
        result = (ParserTokenView *)arena->cursor;
        arena->cursor += sizeof(ParserDeclarationToken);
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    } else {
        pool = &frontend_bss.pools[7];
        if (pool->remaining < sizeof(ParserDeclarationToken)) pool->Refill(sizeof(ParserDeclarationToken));
        result = (ParserTokenView *)pool->cursor;
        pool->cursor += sizeof(ParserDeclarationToken);
        pool->remaining -= sizeof(ParserDeclarationToken);
    }
    if (result) {
        result->kind = 0x24;
#ifdef __linux__
        memcpy(&((ParserDeclarationToken *)result)->value, &saved_parser_value, 36);
        ((ParserDeclarationToken *)result)->value.callback = saved_parser_value.declaration.callback;
#else
        memcpy(&result->value, &saved_parser_value, 40);
#endif
    }
    return result;
}
extern "C" CompilerSymbol *__cdecl resolve_tag_definition(CompilerSymbol *, WORD);
extern "C" CompilerSymbol *__cdecl refine_forward_tag(CompilerSymbol *symbol, WORD flags)
{
    if (!symbol) return NULL;
    symbol = resolve_tag_definition(symbol, flags);
    if (frontend_bss.browser_filename &&
        (!frontend_bss.declaration_scope_mode || publish_browser_local))
        frontend_data.backend->Slot1c((unsigned)symbol);
    return symbol;
}
extern "C" CompilerSymbol *__cdecl find_or_declare_tag_reference(CompilerNameEntry *, WORD, ParserDeclarationCallback *) { __asm { int 3 } return NULL; }
extern "C" void __cdecl validate_tag_declaration_context(
    CompilerSymbol *symbol, CompilerDeclarationContext *context)
{
    CompilerGlobalSymbolView *view = (CompilerGlobalSymbolView *)symbol;
    if (context->member_flags & 0xc) {
        view->member_flags = (view->member_flags & ~4) | (context->member_flags & 4);
        view->member_flags = (view->member_flags & ~8) | (context->member_flags & 8);
    }
    if (context->class_context) view->class_detail->declaration_flags |= 0x80;
}
extern "C" void __cdecl prepare_tag_replay_state(void) { __asm { int 3 } }

extern "C" CompilerSymbol *__cdecl find_or_create_tag_symbol(CompilerNameEntry *, WORD, ParserDeclarationCallback *);
extern "C" CompilerSymbol *__cdecl create_tag_symbol(
    CompilerNameEntry *name, WORD flags, ParserDeclarationCallback *callback)
{
    *callback = &CompilerTranslationUnit::InsertCurrentScope;
    return find_or_create_tag_symbol(name, flags, callback);
}

extern "C" void __cdecl select_code_segment(unsigned);
extern "C" CompilerSymbol *__cdecl allocate_tag_symbol(CompilerNameEntry *, WORD, ParserDeclarationCallback *, int);
extern "C" CompilerSymbol *__cdecl find_or_create_tag_symbol(
    CompilerNameEntry *name, WORD flags, ParserDeclarationCallback *callback)
{
    CompilerScopeMatch *lookup;
    CompilerSymbol *symbol, *owner;
    CompilerGlobalSymbolView *view;
    ParserDefinitionClassView *detail;
    ParserClassAccessRow *row;
    CompilerScopeTable *current;
    unsigned lookup_flags = 0;
    BYTE tag_kind;
    int index;
    if (!(frontend_data.compiler_mode_flags & 0x40000000) && (frontend_data.compiler_flags & 0x20))
        select_code_segment(1);
    if (name) {
        if ((frontend_data.compiler_mode_flags & 0x40000000) &&
            *callback == &CompilerTranslationUnit::InsertTagScope) lookup_flags = 1;
        lookup = frontend_data.translation_unit->FindTagName(name, lookup_flags);
        if (lookup) {
#ifdef __linux__
            recon_unsupported("tag lookup: returned symbol domain pending");
#endif
            symbol = lookup->symbol;
            view = (CompilerGlobalSymbolView *)symbol;
            *callback = NULL;
            if (view->kind == 0x17) {
                compiler_warning(0x365, symbol);
                view->scope_flags |= 4;
                return symbol;
            }
            detail = (ParserDefinitionClassView *)view->class_detail;
            if ((view->definition_flags & 4) || (detail->completion_flags & 2)) {
                if (detail->completion_flags & 2) compiler_warning(0x3de, symbol);
                else compiler_warning(11, symbol, view->type);
                view->scope_flags |= 4;
                return symbol;
            }
            return complete_tag_symbol(symbol, flags);
        }
    }
    if (name && (name->flags & 0x02000000)) {
        lookup = frontend_data.translation_unit->LookupName(name, 0x20);
        symbol = lookup ? lookup->symbol : NULL;
        if ((flags & 0x404) == 0x404) tag_kind = 29;
        else if ((flags & 0x408) == 0x408) tag_kind = 26;
        else tag_kind = ((flags & 0x402) == 0x402) ? 28 : 27;
        if (frontend_bss.declaration_scope_mode) compiler_warning(0x3ad, symbol, tag_kind);
        else {
            index = frontend_data.declaration_context_index;
#ifdef __linux__
            recon_unsupported("tag lookup: class-scope row pending");
#else
            __asm { int 3 }
#endif
            row = &((ParserClassAccessRow *)class_scopes)[index];
            compiler_warning(0x3a6, symbol, tag_kind, row->symbol);
        }
        return NULL;
    }
    symbol = allocate_tag_symbol(name, flags, callback, 1);
    if (!symbol) return NULL;
    view = (CompilerGlobalSymbolView *)symbol;
    index = frontend_data.declaration_context_index;
    if (index >= 0) {
#ifdef __linux__
        recon_unsupported("tag lookup: new-symbol class row pending");
#else
        __asm { int 3 }
#endif
        row = &((ParserClassAccessRow *)class_scopes)[index];
        view->definition_flags = (view->definition_flags & ~3) | (row->access_flags & 3);
        view->declaration_qualifiers = ((ParserDefinitionClassView *)row->symbol->class_detail)->scope_number;
        return complete_tag_symbol(symbol, flags);
    }
    current = frontend_data.translation_unit->current;
    if (!current->level && current != frontend_data.translation_unit->root) {
        owner = frontend_data.translation_unit->CurrentDeclarationOwner();
        view->declaration_qualifiers = ((ParserOwnerScopeView *)((CompilerIrSymbol *)owner)->payload)->scope_number;
        return complete_tag_symbol(symbol, flags);
    }
    if (frontend_bss.current_function) {
        WORD *number = &((CompilerGlobalSymbolView *)frontend_bss.current_function)->declaration_scope_number;
        if (!*number) frontend_bss.current_function->SetDeclarationScopeNumber(7);
        view->declaration_qualifiers = *number;
        if (view->kind == 4)
            ((ParserDefinitionClassView *)view->class_detail)->nested_scope_number = frontend_data.declaration_scope_number;
    }
    return complete_tag_symbol(symbol, flags);
}

/* Special tag scopes reuse their +14 cell as a context pointer. These views
 * describe only the two original operands; no whole allocation or defaults
 * for either special-scope owner are established here. */
struct ParserSpecialTagContext {
    BYTE unmodeled_00_0f[16];
    CompilerSymbol *symbol;
};
struct ParserSpecialTagScope {
    BYTE unmodeled_00_13[20];
    ParserSpecialTagContext *context;
};
CompilerScopeMatch *RECON_THISCALL CompilerTranslationUnit::FindTagName(
    CompilerNameEntry *name, unsigned file_scope)
{
    CompilerScopeTable *scope;
    if (file_scope) return FileScope()->FindOrdinary(name);
    scope = current;
    if (scope->level >= 0xfb) {
#ifdef __linux__
        recon_unsupported("tag-name lookup: special-scope context unmodeled");
#else
        __asm { int 3 }
#endif
        CompilerSymbol *symbol = ((ParserSpecialTagScope *)scope)->context->symbol;
        if (((CompilerGlobalSymbolView *)symbol)->type_flags & 2) return NULL;
    }
    return scope->FindOrdinary(name);
}

extern "C" {
CompilerSymbol *__cdecl allocate_class_symbol(unsigned);
void __cdecl publish_tag_symbol(CompilerSymbol *);
CompilerHashTable *__cdecl create_hash_table(unsigned, unsigned, unsigned, unsigned, int, int);
CompilerSymbol *__cdecl merge_scoped_declaration(CompilerSymbol *, CompilerSymbol *, CompilerDeclarator *);
CompilerType *__cdecl intern_type(CompilerType *);
}
/* A tagged variant of the existing symbol +24 cell. This does not enlarge
 * the symbol or establish any other class/enum payload bytes. */
struct ParserEnumSymbolView {
    BYTE unmodeled_00_23[36];
    CompilerHashTable *members;
};
extern "C" CompilerSymbol *__cdecl allocate_tag_symbol(
    CompilerNameEntry *name, WORD flags, ParserDeclarationCallback *callback, int diagnose)
{
    CompilerSymbol *symbol = allocate_class_symbol(0);
    CompilerGlobalSymbolView *view;
    ParserDefinitionClassView *detail;
    CompilerScopeTable *scope;
    CompilerScopeLookup *matches;
    CompilerScopeMatch *selected, *result;
    CompilerSymbol *merged, *existing;
    CompilerType *type;
    CompilerArena *arena;
    unsigned mode;
    view = (CompilerGlobalSymbolView *)symbol;
    detail = (ParserDefinitionClassView *)view->class_detail;
    if (frontend_data.next_symbol_record_id >= 0x7fffffff) report_compiler_error(0x37);
    view->record_index = (view->record_index & 0xff000000) |
        (frontend_data.next_symbol_record_id++ & 0x00ffffff);
    view->scope_flags = (view->scope_flags & ~8) | (frontend_bss.declaration_scope_mode ? 8 : 0);
    view->definition_flags |= 0x10;
    if (frontend_data.declaration_mode_index < -1 || frontend_data.declaration_mode_index >= 10) {
#ifdef __linux__
        recon_unsupported("tag allocation: declaration mode outside actual ten slots");
#else
        __asm { int 3 }
#endif
    }
    mode = FRONTEND_LINKAGE_CONTEXT();
    view->type_flags = (view->type_flags & ~3) | (mode & 3);
    detail->inheritance_flags &= ~7;
    if (frontend_data.compiler_mode_flags & 0x02000000) detail->completion_flags |= 2;
    scope = frontend_data.translation_unit->current;
    if (!scope->level && scope != frontend_data.translation_unit->root) {
        CompilerSymbol *owner = frontend_data.translation_unit->CurrentDeclarationOwner();
        view->declaration_qualifiers = ((ParserOwnerScopeView *)((CompilerIrSymbol *)owner)->payload)->scope_number;
    }
    symbol->SetDeclarationScopeNumber(4);
    detail->declaration_reference = NULL;
    arena = &frontend_bss.arenas[5];
    type = (CompilerType *)arena->cursor;
    arena->cursor += 16;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    type->flags = flags;
    if (flags & 0x200) {
        type->flags |= 3;
        ((ParserEnumSymbolView *)symbol)->members = create_hash_table(4,
            ((frontend_data.compiler_flags >> 17) & 1) << 2, 0, 0, 1, 1);
    }
    type->auxiliary = detail->scope_number;
    view->type = intern_type(type);
    if (name) {
        view->name = name;
        name->flags |= 0x20000;
        if ((int)frontend_bss.parser_context_depth > 0 && diagnose &&
            (!(frontend_data.compiler_mode_flags & 0x40000000) ||
             *callback != &CompilerTranslationUnit::InsertDeferredTag)) compiler_warning(0xcc, symbol);
        existing = NULL;
        if (frontend_data.compiler_mode_flags & 0x02000000) {
            result = ((CompilerScope *)frontend_data.translation_unit->FileScope())->MatchTag(symbol, 0);
            if (result) existing = result->symbol;
        } else {
            matches = frontend_data.translation_unit->current->Find(view->name, 0);
            if (matches) {
                selected = matches->Select(1);
                merged = selected ? merge_scoped_declaration(selected->symbol, symbol, 0) : NULL;
            } else merged = merge_scoped_declaration(NULL, symbol, 0);
            result = NULL;
            if (merged) {
                result = (CompilerScopeMatch *)frontend_bss.arenas[5].Allocate(12);
                if (result) result = result->Initialize(merged, NULL);
            }
            if (result) existing = result->symbol;
        }
        if (existing) {
            symbol = existing;
            view = (CompilerGlobalSymbolView *)symbol;
            if (view->definition_flags & 0x60) *callback = NULL;
            publish_tag_symbol(symbol);
        } else {
            if (diagnose && is_special_member_name(name)) {
                compiler_warning(0x244, symbol);
                return NULL;
            }
            if (frontend_data.compiler_mode_flags & 0x10) {
                if (!(detail->completion_flags & 1)) compiler_warning(0x1ce, symbol);
            }
            publish_tag_symbol(symbol);
        }
    } else {
        if ((int)frontend_bss.parser_context_depth > 0) {
            if (diagnose) compiler_warning(0x1d1);
            else report_language_diagnostic(1, 0x74);
        }
        if (frontend_data.compiler_mode_flags & 0x10) compiler_warning(0x1cf);
        view->name = frontend_data.builtin_names.unnamed;
        view->name->flags |= 0x20000;
        view->scope_flags |= 1;
    }
    if (frontend_bss.browser_filename &&
        (!frontend_bss.declaration_scope_mode || publish_browser_local)) {
        frontend_data.backend->Slot20(symbol);
        frontend_data.backend->Slot54(1, (unsigned)view->name);
    }
    frontend_bss.tag_declaration_changed = 1;
    return symbol;
}

extern "C" void *__cdecl allocate_symbol_storage(unsigned, unsigned, unsigned);
extern "C" CompilerSymbol *__cdecl allocate_class_symbol(unsigned arena_index)
{
    CompilerNamedSymbol *symbol = NULL;
    CompilerSymbolLink *link;
    CompilerArena *arena;
    void *memory;
    BYTE *detail;
    unsigned size_class;
    if (!arena_index) {
        size_class = frontend_data.symbol_size_class[4];
        if (size_class >= 11) { __asm { int 3 } }
        link = frontend_data.recycled_symbols[size_class];
        if (link) {
            frontend_data.recycled_symbols[size_class] = link->next;
            symbol = (CompilerNamedSymbol *)((BYTE *)link - offsetof(CompilerNamedSymbol, link));
            link->name = NULL;
            link->next = NULL;
            memset(symbol, 0, frontend_data.symbol_record_bytes[4]);
            link->name = NULL;
            symbol->kind = 4;
        }
    }
    if (!symbol) {
        memory = allocate_symbol_storage(48, 4, arena_index);
        symbol = memory ? (CompilerNamedSymbol *)
            ((CompilerSymbol *)memory)->Initialize(4, NULL, arena_index) : NULL;
    }
    if (arena_index >= 9) { __asm { int 3 } }
    arena = &frontend_bss.arenas[arena_index];
    detail = arena->cursor;
    arena->cursor += 72;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    ((CompilerGlobalSymbolView *)symbol)->class_detail = (CompilerGlobalClassDetail *)detail;
    if ((frontend_data.compiler_flags & 0x20000) || frontend_data.pch_separate_debug)
        symbol->definition_flags &= ~8;
    else symbol->definition_flags |= 8;
    return symbol;
}

void RECON_THISCALL CompilerSymbol::SetDeclarationScopeNumber(BYTE kind)
{
    CompilerPointerArray *table = frontend_data.frontend_pointer_table;
    short number = table ? table->count : 0;
    switch (kind) {
    case 4:
        ((ParserDefinitionClassView *)((CompilerGlobalSymbolView *)this)->class_detail)->scope_number = (WORD)number;
        break;
    case 7: case 8:
        ((CompilerGlobalSymbolView *)this)->declaration_scope_number = (WORD)number;
        break;
    case 23:
        ((ParserOwnerScopeView *)((CompilerGlobalSymbolView *)this)->type)->scope_number = (WORD)number;
        break;
    }
    if (number == 0 || number == -1) report_compiler_error(0x41);
    table = frontend_data.frontend_pointer_table;
    if (table->count == table->capacity) table = grow_pointer_array(table);
    table->items[table->count] = this;
    ++table->count;
    frontend_data.frontend_pointer_table = table;
}

extern "C" int __cdecl is_special_member_name(CompilerNameEntry *name)
{
    int index = frontend_data.declaration_context_index;
    if (index < 0) return 0;
    if (index >= 16) { __asm { int 3 } }
    return name == frontend_bss.class_scope_rows[index].symbol->name;
}

/* Diagnostic argument codes for the encountered tag-attribute domain.
 * Larger three-bit encodings remain unsupported before the table read. */
static const unsigned tag_attribute_codes[5] = { 0x133, 0x15b, 0x15c, 0x15d, 0x15d };
extern "C" void __cdecl publish_tag_symbol(CompilerSymbol *symbol)
{
    unsigned flags = frontend_data.compiler_mode_flags;
    ParserDefinitionClassView *detail;
    unsigned previous;
    if (symbol->kind != 4 || !(flags & 0x700000)) return;
    detail = (ParserDefinitionClassView *)((CompilerGlobalSymbolView *)symbol)->class_detail;
    if (!(detail->inheritance_flags & 8)) {
        detail->inheritance_flags = (detail->inheritance_flags & ~7) | ((flags >> 20) & 7);
        detail->inheritance_flags |= 8;
    } else {
        previous = detail->inheritance_flags & 7;
        if (((flags >> 20) & 7) != previous) {
            if (previous >= 5) {
#ifdef __linux__
                recon_unsupported("tag publication: unknown attribute diagnostic code");
#else
                __asm { int 3 }
#endif
            }
            compiler_warning(0x11e, symbol, tag_attribute_codes[previous]);
        }
    }
    frontend_data.compiler_mode_flags &= ~0x700000U;
}

extern "C" CompilerSymbol *__cdecl resolve_tag_definition(CompilerSymbol *, WORD);
/* CF003 is element3 of the existing primitive byte-size table, not a
 * separate enum setting. The enum shares detail+0's size cell. */
struct ParserEnumDefinitionView { int underlying_bytes; };
extern "C" CompilerSymbol *__cdecl complete_tag_symbol(CompilerSymbol *input, WORD flags)
{
    CompilerSymbol *symbol = resolve_tag_definition(input, flags);
    CompilerGlobalSymbolView *view;
    ParserDefinitionClassView *detail;
    int index;
#ifdef __linux__
    if (symbol && symbol->kind != 4)
        recon_unsupported("tag completion: wider returned tag definition pending");
#endif
    if (!symbol || (symbol->scope_flags & 4)) return symbol;
    view = (CompilerGlobalSymbolView *)symbol;
    detail = (ParserDefinitionClassView *)view->class_detail;
    detail->completion_flags = (detail->completion_flags & ~0x80) |
        (((frontend_data.compiler_flags & 0x20000) && !(frontend_data.compiler_flags & 0x20)) ? 0x80 : 0);
    detail->context_flags = (detail->context_flags & ~2) |
        (frontend_data.declaration_context_index >= 0 ? 2 : 0);
    if (flags == 0x200) {
        index = frontend_data.declaration_context_index;
        if (index >= 0) {
            if (index >= 16) { __asm { int 3 } }
            view->definition_flags = (view->definition_flags & ~3) |
                (frontend_bss.class_scope_rows[index].access_flags & 3);
        } else view->definition_flags |= 3;
        enumerator_value = -1;
        ((ParserEnumDefinitionView *)view->class_detail)->underlying_bytes = compiler_primitive_type_bytes[3];
        view->definition_flags |= 4;
        current_enum_type = view->type;
        current_declaration_symbol = symbol;
    }
    return symbol;
}

extern "C" CompilerSymbol *__cdecl instantiate_tag_definition(CompilerSymbol *);
extern "C" CompilerSymbol *__cdecl resolve_tag_definition(CompilerSymbol *input, WORD flags)
{
    CompilerSymbol *symbol = input;
    CompilerGlobalSymbolView *view;
    CompilerType *type;
    unsigned previous, requested;
#ifdef __linux__
    if (symbol->kind != 4 || !(symbol->definition_flags & 0x10))
        recon_unsupported("tag resolver: alias or instantiated record pending");
#endif
    if (symbol->kind == 0x12) {
        ParserTagAliasPayload *payload = (ParserTagAliasPayload *)((CompilerIrSymbol *)symbol)->payload;
        if (!payload) return NULL;
        symbol = payload->reference->symbol;
        if (!symbol) return NULL;
    }
    if (!(symbol->definition_flags & 0x10)) {
        symbol = instantiate_tag_definition(symbol);
        if (!symbol) return NULL;
    }
    view = (CompilerGlobalSymbolView *)symbol;
    type = view->type;
    previous = type->flags;
    requested = flags;
    if ((previous ^ requested) & 0x2fdf) {
        if ((requested & 0x2fdf) == 0x12) return symbol;
        if (!(flags == 0x200 && (previous & 0x200))) {
            if (((previous & 0x408) == 0x408 || (previous & 0x402) == 0x402) &&
                ((requested & 0x408) == 0x408 || (requested & 0x402) == 0x402)) {
                if ((previous & 0x408) == 0x408)
                    report_language_diagnostic(2, 0x63, symbol, 0x1a, 0x1c);
                else report_language_diagnostic(2, 0x63, symbol, 0x1c, 0x1a);
                view->qualifiers |= 0x20;
                return symbol;
            }
            compiler_warning(11, symbol, type);
            view->scope_flags |= 4;
        }
    }
    publish_tag_symbol(symbol);
    return symbol;
}

extern "C" CompilerDeclarationMetadata *__cdecl get_declaration_metadata(CompilerSymbol *);
extern "C" void __cdecl set_declaration_metadata(CompilerSymbol *, CompilerDeclarationMetadata *);
extern "C" void __cdecl apply_tag_metadata(CompilerSymbol *symbol, CompilerDeclarationContext *context)
{
    CompilerGlobalSymbolView *view = (CompilerGlobalSymbolView *)symbol;
    CompilerDeclarationMetadata *copy;
    CompilerArena *arena;
    if (!(context->context_flags & 0x40)) return;
#ifdef __linux__
    recon_unsupported("tag metadata: nonempty metadata owner pending");
#endif
    if (view->symbol_flags & 0x40) {
        if (memcmp(get_declaration_metadata(symbol), context->metadata, 16))
            compiler_warning(0x16f, symbol);
        return;
    }
    if (!context->metadata) { view->symbol_flags &= ~0x40; return; }
    arena = &frontend_bss.arenas[0];
    copy = (CompilerDeclarationMetadata *)arena->cursor;
    arena->cursor += 16;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (copy) memcpy(copy, context->metadata, 16);
    set_declaration_metadata(symbol, copy);
}

extern "C" CompilerType *__cdecl get_tag_type(CompilerSymbol *);
extern "C" CompilerType *__cdecl declaration_symbol_type(CompilerSymbol *input)
{
    CompilerSymbol *symbol = input;
    CompilerGlobalSymbolView *view;
    CompilerType *type;
    unsigned references;
#ifdef __linux__
    if (symbol->kind != 4 && symbol->kind != 10 && symbol->kind != 0x12)
        recon_unsupported("declaration symbol type: other symbol variant pending");
#endif
    if (symbol->kind == 0x12) {
        ParserTagAliasPayload *payload = (ParserTagAliasPayload *)((CompilerIrSymbol *)symbol)->payload;
        symbol = payload->reference->symbol;
    }
    if (symbol->kind == 0x18)
        symbol = *(CompilerSymbol **)((CompilerIrSymbol *)symbol)->payload;
#ifdef __linux__
    if (symbol->kind != 4 && symbol->kind != 10)
        recon_unsupported("declaration symbol type: resolved symbol variant pending");
#endif
    view = (CompilerGlobalSymbolView *)symbol;
    type = view->type;
    if (symbol->kind == 4) return get_tag_type(symbol);
    references = view->references;
    if ((references & 3) < 3)
        view->references = (references & ~3U) | ((references + 1) & 3);
    references = view->references;
    view->references = (references & ~4U) | ((frontend_data.reference_context & 1) << 2);
    return type;
}

extern "C" CompilerType *__cdecl get_tag_type(CompilerSymbol *symbol)
{
    return ((CompilerGlobalSymbolView *)symbol)->type;
}

extern "C" CompilerType *__cdecl normalize_parsed_type(CompilerType *);
void RECON_THISCALL ParserDeclarationReader::FinalizeType(void)
{
    CompilerType *scratch;
    CompilerTypeShape *shape;
    ParserTokenView *record;
    CompilerArena *arena;
    CompilerPool *pool;
    unsigned arena_kind;
    if (type) {
        if (!qualifier_flags) { resolved_type = type; return; }
        shape = type->shape;
        if (shape && (((shape->flags & 0x77) == 4) || (shape->flags & 0x40))) {
            report_language_diagnostic(1, (shape->flags & 0x77) == 4 ? 0xb4 : 0xb5);
            qualifier_flags = 0;
            resolved_type = type;
            return;
        }
        resolved_type = type->ApplyTypeQualifiers(qualifier_flags);
        return;
    }
    if (type_flags) goto temporary_type;
    if (!qualifier_flags) context.class_flags |= 0x10;
    else if (frontend_data.compiler_flags & 0x10000000) {
#ifdef __linux__
        recon_unsupported("declaration finalizer: wider type-token copy pending");
#endif
        saved_parser_value.word_value = qualifier_flags;
        arena_kind = frontend_bss.symbol_arena_kind;
        if (arena_kind) {
            if (arena_kind >= 9) { __asm { int 3 } }
            arena = &frontend_bss.arenas[arena_kind];
            record = (ParserTokenView *)arena->cursor;
            arena->cursor += 48;
            if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        } else {
            pool = &frontend_bss.pools[7];
            if (pool->remaining < 48) pool->Refill(48);
            record = (ParserTokenView *)pool->cursor;
            pool->cursor += 48;
            pool->remaining -= 48;
        }
        if (record) { record->kind = 0xab; memcpy(&record->value, &saved_parser_value, 40); }
        declaration_token = record;
        if (declaration_marked) compiler_warning(0x3e, frontend_data.builtin_types.type_0063);
        return;
    }
    if (qualifier_flags) { type_flags = 0x63; goto temporary_type; }
    resolved_type = frontend_data.builtin_types.type_0063;
    return;
temporary_type:
    /* Initial index0 derives the actual row1 at c04c8. This supports one
     * concrete frame, without interpreting row0 or asserting total capacity. */
    if (declaration_type_scratch_index != 0) {
#ifdef __linux__
        recon_unsupported("declaration finalizer: deeper scratch frame unmodeled");
#else
        __asm { int 3 }
#endif
    }
    ++declaration_type_scratch_index;
    scratch = &frontend_bss.first_type_scratch_frame;
    memset(scratch, 0, 16);
    scratch->flags = type_flags | 0x1000;
    scratch->attributes = qualifier_flags;
    resolved_type = normalize_parsed_type(scratch);
}

ParserTokenView *RECON_THISCALL ParserDeclarationReader::MakeToken(unsigned kind)
{
    ParserTokenView *record;
    CompilerArena *arena;
    CompilerPool *pool;
    CompilerPointerArray *table;
    CompilerGlobalSymbolView *owner;
    unsigned arena_kind;
    int index;
    if (declaration_token) {
        declaration_token->kind = (BYTE)kind;
        return declaration_token;
    }
    if (!resolved_type) {
        resolved_type = type;
        if (declaration_marked && (context.class_flags & 7) == 1 && (resolved_type->flags & 0x600)) {
            table = frontend_data.frontend_pointer_table;
            index = (short)resolved_type->auxiliary;
            if (index < 0 || index >= table->count) { __asm { int 3 } }
            owner = (CompilerGlobalSymbolView *)table->items[index];
            if (((ParserDefinitionClassView *)owner->class_detail)->completion_flags & 0x80)
                compiler_warning(0x270, owner);
        }
    }
    if (!(context.class_flags & 7)) context.class_flags |= 0x20;
    saved_parser_value.declaration.callback = declaration_callback;
    saved_parser_value.declaration.object = type_symbol;
    saved_parser_value.declaration.type = resolved_type;
    memcpy(&saved_parser_value.declaration.context, &context, 28);
    arena_kind = frontend_bss.symbol_arena_kind;
    if (arena_kind) {
        if (arena_kind >= 9) { __asm { int 3 } }
        arena = &frontend_bss.arenas[arena_kind];
        record = (ParserTokenView *)arena->cursor;
        arena->cursor += sizeof(ParserDeclarationToken);
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    } else {
        pool = &frontend_bss.pools[7];
        if (pool->remaining < sizeof(ParserDeclarationToken)) pool->Refill(sizeof(ParserDeclarationToken));
        record = (ParserTokenView *)pool->cursor;
        pool->cursor += sizeof(ParserDeclarationToken);
        pool->remaining -= sizeof(ParserDeclarationToken);
    }
    if (record) {
        record->kind = (BYTE)kind;
#ifdef __linux__
        memcpy(&((ParserDeclarationToken *)record)->value, &saved_parser_value, 36);
        ((ParserDeclarationToken *)record)->value.callback = saved_parser_value.declaration.callback;
#else
        memcpy(&record->value, &saved_parser_value, 40);
#endif
    }
    declaration_token = record;
    return record;
}

/* Shift the complete original semantic record. A declaration callback has
 * its actual native representation; opaque prefix bytes remain uninterpreted. */
extern "C" void __fastcall copy_token_value(ParserSemanticView *destination, ParserTokenView *token)
{
#ifdef __linux__
    if (token->kind == 0x22 || token->kind == 0x23 || token->kind == 0x24) {
        ParserDeclarationView *value = &((ParserDeclarationToken *)token)->value;
        memcpy(destination, value, 36);
        destination->declaration.callback = value->callback;
    } else if (token->kind == 80 || token->kind == 81 || token->kind == 54 || token->kind == 207 || token->kind == 219 || token->kind == 47 || token->kind == 0xb6 || token->kind == 0xdf || token->kind == 52 || token->kind == 117 || token->kind == 118 || token->kind == 126 || token->kind == 74 || token->kind == 65 || token->kind == 122 || token->kind == 6 || token->kind == 127 || token->kind == 123 || token->kind == 37 || token->kind == 106 || token->kind == 7 || token->kind == 130 || token->kind == 55 || token->kind == 49 || token->kind == 142 || token->kind == 101 || token->kind == 125 || token->kind == 5) {
        /* These ordinary tokens retain their complete opaque40-byte storage;
         * no declaration callback is read or synthesized for this arm. */
        memcpy(destination, &token->value, 40);
    } else recon_unsupported("parser shift: other semantic representation pending");
#else
    memcpy(destination, &token->value, 40);
#endif
}

/* Enter the class-definition state; return the same declared symbol. */
extern "C" CompilerSymbol *__cdecl finish_class_declaration(CompilerSymbol *input, CompilerType *type)
{
    CompilerGlobalSymbolView *symbol = (CompilerGlobalSymbolView *)input;
    ParserDefinitionClassView *detail = (ParserDefinitionClassView *)symbol->class_detail;
    DeclarationClassScope *row;
    CompilerArena *arena;
    CompilerPool *pool;
    CompilerTranslationUnit *unit;
    CompilerSavedLookup *saved;
    ParserClassDefinitionLink *link;
    CompilerClassBinding *binding;
    BYTE *memory;
    unsigned type_code;
    int index, special = 0;
    BYTE classification;
    if (frontend_data.options.incremental_rebuild)
        frontend_data.dependency->RecordClass(input);
    if (frontend_data.class_completion_checks)
        detail->template_flags |= 8;
    if (((CompilerRecordedLexer *)frontend_bss.saved_context_stack.top->object)->active) {
        if (!frontend_bss.debug_module->Slot38(frontend_bss.saved_context_stack.position,
                (unsigned)detail->declaration_reference))
            report_compiler_diagnostic(0x49, ".\\esu.c", 0x132);
    }
    if (frontend_bss.browser_filename &&
        (!frontend_bss.declaration_scope_mode || publish_browser_local) &&
        !(symbol->scope_flags & 1)) {
        frontend_data.backend->Slot0c();
        if (!frontend_data.backend->Slot54(0, (unsigned)symbol->name))
            frontend_data.backend->Slot20(input);
        frontend_data.backend->Slot30((unsigned)input);
    }
    index = frontend_data.declaration_context_index;
    if (index >= 0) {
        if ((unsigned)index >= 16) { __asm { int 3 } }
        row = &frontend_bss.class_scope_rows[index];
        symbol->definition_flags = (symbol->definition_flags & ~3) | (row->access_flags & 3);
        special = (row->access_flags >> 4) & 1;
        if (frontend_data.source_mode_flags & 8) {
            arena = &frontend_bss.arenas[0];
            link = (ParserClassDefinitionLink *)arena->cursor;
            arena->cursor += 12;
            if (arena->cursor > arena->committed_limit) arena->CommitExtent();
            ParserClassDefinitionLink **head = frontend_data.class_definition_head;
            if (!head) { __asm { int 3 } }
            if (link) { link->symbol = input; link->definitions = NULL; link->next = *head; }
            else link = NULL;
            *head = link;
        }
    }
    index = ++frontend_data.declaration_context_index;
    if (index == 16) report_compiler_diagnostic(0x2e, symbol->type);
    if ((unsigned)index >= 16) { __asm { int 3 } }
    row = &frontend_bss.class_scope_rows[index];
    if (!special) special = symbol->name == frontend_data.bool_identifier;
    if (special) row->access_flags |= 0x10;
    else row->access_flags &= ~0x10;
    type_code = type->flags & 0x408;
    classification = symbol->qualifiers;
    if (type_code != 0x408) classification = (BYTE)~classification;
    row->access_flags = (row->access_flags & ~2) | (((classification & 0x20) | 0x10) >> 4);
    row->reset_10 = 0;
    detail->context_flags &= ~0x50;
    arena = &frontend_bss.arenas[1]; /* actual c6600 */
    memory = arena->cursor;
    arena->cursor += 36;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    unit = frontend_data.translation_unit;
    saved = memory ? ((CompilerSavedLookup *)memory)->Initialize(unit, unit->saved_lookup, 0) : NULL;
    unit->saved_lookup = saved;
    if (!frontend_bss.parser_context_depth) {
        if (symbol->declaration_qualifiers) {
            if (unit->current != unit->root) unit->PrepareScopeSymbol(input);
            int scope = (short)symbol->declaration_qualifiers;
            unit->SelectScopeSymbol((CompilerSymbol *)frontend_data.frontend_pointer_table->items[scope]);
        }
        frontend_data.translation_unit->SetEnclosingClass(input, 0xff);
    }
    index = frontend_data.declaration_context_index;
    if ((unsigned)index >= 16) { __asm { int 3 } }
    row = &frontend_bss.class_scope_rows[index];
    row->access_flags &= ~4;
    detail->declaration_flags |= 1;
    type_code = type->flags;
    if ((type_code & 0x408) == 0x408 || (type_code & 0x402) == 0x402)
        row->access_flags |= 8;
    else row->access_flags &= ~8;
    row->symbol = symbol;
    row->reset_04 = NULL;
    row->reset_14 = 0;
    row->reset_1c = 0;
    row->access_flags &= ~0x20;
    row->reset_18 = 0;
    row->type = type;
    row->state = 1;
    pool = &frontend_bss.pools[8]; /* actual c0b00 */
    if (pool->remaining < 12) pool->Refill(12);
    binding = (CompilerClassBinding *)pool->cursor;
    pool->cursor += 12;
    pool->remaining -= 12;
    if (binding) binding->reset_a = binding->reset_b = binding->reset_c = 0;
    index = frontend_data.declaration_context_index;
    if ((unsigned)index >= 16) { __asm { int 3 } }
    ((CompilerKind4Symbol *)frontend_bss.class_scope_rows[index].symbol)->class_binding = binding;
    return input;
}

CompilerSavedLookup *RECON_THISCALL CompilerSavedLookup::Initialize(
    CompilerTranslationUnit *unit, CompilerSavedLookup *prior, int copy_scopes)
{
    current = unit->current;
    depth = unit->depth;
    retired = unit->retired_scopes;
    lookup_context = unit->lookup_context;
    suppress_lookup_errors = unit->suppress_lookup_errors;
    opaque_lookup_state = unit->opaque_lookup_state;
    previous = prior;
    lookup_override = unit->lookup_override;
    if (depth && copy_scopes) {
        /* The original depth setter bounds the128-slot owner. Do not
         * interpret a slot outside that owner if an unsupported value arrives. */
        if (depth >= 128) { __asm { int 3 } }
        CompilerArena *arena = &frontend_bss.arenas[1];
        unsigned bytes = (depth * 4 + 3) & ~3U;
        CompilerScopeTable **saved = (CompilerScopeTable **)arena->cursor;
        arena->cursor += bytes;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        scopes = saved;
        for (unsigned i = 1; i <= depth; ++i)
            saved[i - 1] = unit->scopes[i];
    } else scopes = NULL;
    return this;
}

extern "C" int __cdecl is_scope_declaration(CompilerSymbol *);
CompilerScopeTable *RECON_THISCALL CompilerTranslationUnit::SetEnclosingClass(CompilerSymbol *symbol, unsigned level)
{
    if (!symbol) return NULL;
    if (symbol->kind == 0x12) {
        ParserTagAliasPayload *alias = (ParserTagAliasPayload *)
            ((CompilerGlobalSymbolView *)symbol)->type;
        symbol = alias->reference->symbol;
    }
    if (!is_scope_declaration(symbol)) return NULL;
    if (!current->HasBuckets() && current->level < 0xfb)
        ((CompilerScope *)current)->EnsureBuckets();
    if (level == 0xfd) return CreateClassScope(0, 0xfd, symbol);
    return CreateClassScope(4, level, symbol);
}

extern "C" int __cdecl is_scope_declaration(CompilerSymbol *symbol)
{
    if (symbol->kind == 4 || symbol->kind == 23) return 1;
    CompilerType *type = ((CompilerTypedSymbol *)symbol)->type;
    if (!type) return 0;
    return (symbol->definition_flags & 0x10) && (type->flags & 0x200) ? 1 : 0;
}

extern "C" {
void *__cdecl get_class_template_context(CompilerSymbol *);
ParserDeferredList *__cdecl get_class_template_arguments(CompilerSymbol *);
void *__cdecl get_class_context_key(void *);
CompilerScopeTable *__cdecl make_class_lookup_scope(void *, ParserDeferredList *, CompilerSymbol *, int, int, int);
}
CompilerScopeTable *RECON_THISCALL CompilerTranslationUnit::CreateClassScope(
    unsigned arena_kind, unsigned level, CompilerSymbol *symbol)
{
    if (arena_kind >= 9) { __asm { int 3 } }
    CompilerArena *arena = &frontend_bss.arenas[arena_kind];
    BYTE *memory = arena->cursor;
    arena->cursor += 32;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    CompilerClassScope *scope = memory ? new(memory) CompilerClassScope(level, symbol) : NULL;
    if (scope && symbol->kind == 4 &&
        (((ParserDefinitionClassView *)((CompilerGlobalSymbolView *)symbol)->class_detail)->completion_flags & 1)) {
        void *value = get_class_template_context(symbol);
        if (value) {
            arena = &frontend_bss.arenas[3]; /* actual c6560 */
            memory = arena->cursor;
            arena->cursor += 24;
            if (arena->cursor > arena->committed_limit) arena->CommitExtent();
            scope->auxiliary = memory ? new(memory) CompilerAuxiliaryScope(value) : NULL;
        } else {
            void *context = instantiate_declaration_class(scope->symbol);
            ParserDeferredList *arguments = get_class_template_arguments(scope->symbol);
            context = get_class_context_key(context);
            scope->auxiliary = make_class_lookup_scope(context, arguments, scope->symbol, 0, 0, 0);
        }
    }
    ActivateClassScope(scope);
    return scope;
}

extern "C" {
void __cdecl complete_class_members(CompilerSymbol *);
void __cdecl update_class_emission_state(CompilerSymbol *);
void __cdecl emit_completed_class(CompilerSymbol *);
}
extern "C" void __cdecl finish_class_definition(void)
{
    int index = frontend_data.declaration_context_index;
    if ((unsigned)index >= 16) { __asm { int 3 } }
    CompilerGlobalSymbolView *symbol = frontend_bss.class_scope_rows[index].symbol;
    ParserDefinitionClassView *detail = (ParserDefinitionClassView *)symbol->class_detail;
    detail->declaration_flags |= 4;
    if (!(frontend_data.source_mode_flags & 0x100000)) {
        complete_class_members((CompilerSymbol *)symbol);
        index = frontend_data.declaration_context_index;
        if ((unsigned)index >= 16) { __asm { int 3 } }
        update_class_emission_state((CompilerSymbol *)frontend_bss.class_scope_rows[index].symbol);
        index = frontend_data.declaration_context_index;
        if ((unsigned)index >= 16) { __asm { int 3 } }
        emit_completed_class((CompilerSymbol *)frontend_bss.class_scope_rows[index].symbol);
    }
}

/* Source-only partial view of the dependency record owner. No allocation,
 * default or interpretation of its first12 bytes is implied. */
extern "C" CompilerSymbol *__cdecl complete_member_declaration(CompilerSymbol *, int *, CompilerSymbol *);
extern "C" void __cdecl complete_class_members(CompilerSymbol *input)
{
    CompilerClassMemberIterator iterator;
    int scratch = 0;
    int pass = 1;
    do {
        iterator.Initialize(&((CompilerNamedSymbol *)input)->link, 4);
        while (iterator.current) {
            CompilerSymbolLink *link = iterator.current;
            if (iterator.chain) iterator.chain = iterator.chain->next;
            iterator.current = iterator.chain;
            CompilerSymbol *symbol = (CompilerSymbol *)((BYTE *)link - 8);
            if (symbol->kind == 3) {
                if (link->name == frontend_data.builtin_names.vbptr || link->name == frontend_data.builtin_names.vfptr)
                    continue;
                break;
            }
            CompilerClassMemberSymbol *member = (CompilerClassMemberSymbol *)symbol;
            if ((frontend_data.source_mode_flags & 0x100000) &&
                symbol->kind == 15 && (member->flags & 2)) continue;
            if (symbol->kind != 15) break;
            CompilerPointerArray *table = frontend_data.frontend_pointer_table;
            int index = member->owner_index;
            if (index < 0 || index >= table->count) { __asm { int 3 } }
            CompilerGlobalSymbolView *owner = (CompilerGlobalSymbolView *)table->items[index];
            if ((frontend_data.source_mode_flags & 0x100000) &&
                ((CompilerNamedSymbol *)input)->link.name == owner->name) continue;
            ParserDefinitionClassView *detail = (ParserDefinitionClassView *)owner->class_detail;
            if (((detail->member_phase_flags >> 6) & 1) != pass) continue;
            if (!(member->flags & 1))
                member->resolved = complete_member_declaration((CompilerSymbol *)owner, &scratch, input);
            if (frontend_data.options.incremental_rebuild &&
                ((ClassDependencyRecordsView *)frontend_data.dependency->records)->record_members) {
                table = frontend_data.frontend_pointer_table;
                index = member->owner_index;
                if (index < 0 || index >= table->count) { __asm { int 3 } }
                frontend_data.dependency->RecordMember((CompilerSymbol *)table->items[index]);
            }
        }
    } while (pass--);
}

CompilerClassMemberIterator *RECON_THISCALL CompilerClassMemberIterator::Initialize(
    CompilerSymbolLink *owner_link, unsigned search_mode)
{
    CompilerKind4Symbol *symbol = (CompilerKind4Symbol *)((BYTE *)owner_link - 8);
    CompilerSymbolLink *head = symbol->class_binding ? symbol->class_binding->members : NULL;
    owner = owner_link;
    chain = head;
    valid = head != NULL;
    reset_value = 0;
    current = NULL;
    mode = search_mode;
    if (valid) current = head;
    return this;
}

extern "C" void __cdecl update_class_emission_state(CompilerSymbol *)
{
    frontend_bss.tag_novtordisp |= !frontend_data.current_vtordisp;
}

extern "C" int __cdecl requested_member_pointer_model(unsigned *);
extern "C" int __cdecl class_member_pointer_model(CompilerSymbol *);
extern "C" void __cdecl emit_completed_class(CompilerSymbol *input)
{
    CompilerGlobalSymbolView *symbol = (CompilerGlobalSymbolView *)input;
    ParserDefinitionClassView *detail = (ParserDefinitionClassView *)symbol->class_detail;
    if (!(detail->declaration_flags & 4) || !(detail->inheritance_flags & 8) ||
        (detail->declaration_flags & 0x20)) return;
    unsigned requested;
    int explicit_model = requested_member_pointer_model(&requested);
    detail = (ParserDefinitionClassView *)symbol->class_detail;
    int model = class_member_pointer_model(input);
    int declared = detail->inheritance_flags & 7;
    if (explicit_model ? model > declared : model != declared) {
        detail->declaration_flags |= 0x20;
        model = class_member_pointer_model(input);
        /* The encountered diagnostic-code table has five entries. The flag
         * mask alone does not establish meanings for indices5..7. */
        static const unsigned inheritance_diagnostic_codes[] = {
            0x133, 0x15b, 0x15c, 0x15d, 0x15d
        };
        if ((unsigned)model >= 5) { __asm { int 3 } }
        unsigned actual_code = inheritance_diagnostic_codes[model];
        detail = (ParserDefinitionClassView *)symbol->class_detail;
        declared = detail->inheritance_flags & 7;
        if ((unsigned)declared >= 5) { __asm { int 3 } }
        compiler_warning(explicit_model ? 0x11f : 0x124, input,
            inheritance_diagnostic_codes[declared], actual_code);
    }
}

extern "C" void __cdecl select_parser_arena(unsigned index)
{
    if (index >= 9) { __asm { int 3 } }
    frontend_bss.arenas[index].ResetStorage();
}

extern "C" WORD __cdecl combine_type_specifiers(WORD first, WORD second)
{
    WORD first_sign = first & 0xc000;
    WORD second_sign = second & 0xc000;
    WORD next = second & 0x2fff;
    WORD base;
    int incompatible = 0;
    if (first_sign && second_sign) {
        if (first_sign & second_sign)
            report_language_diagnostic(1, 0x72);
        else {
            compiler_warning(0x2a);
            second &= 0x3fff;
        }
        return first | second;
    }
    if (next == 0x60 && first_sign && (first & 0xefff) == first_sign) {
        report_language_diagnostic(1, 0x2b9, compiler_token_spellings[231]);
        return 0x60;
    }
    base = first & 0x2fff;
    if (base == 0x60 && second_sign) {
        report_language_diagnostic(1, 0x2b9, compiler_token_spellings[231]);
        return 0x60;
    }
    if (second_sign || (second & 0x4000) || (first & 0xefff) == first_sign)
        return first | second;
    switch (base) {
    case 0x41: case 0x42: case 0x43: case 0x4c: case 0x4d:
    case 0x60: case 0x61:
    case 0x108: case 0x402: case 0x404: case 0x408:
    case 0x2062: case 0x2064:
        incompatible = 1;
        break;
    case 0x62:
        if (next != 0x63) incompatible = 1;
        base = 0x2062;
        break;
    case 0x63:
        if (next == 0x62) base = 0x2062;
        else if (next == 0x64) base = 0x2064;
        else incompatible = 1;
        break;
    case 0x64:
        switch (next) {
        case 0x63: base = 0x2064; break;
        case 0x106:
            report_language_diagnostic(1, 0xd7);
            base = 0x107;
            break;
        case 0x107: base = 0x108; break;
        default: incompatible = 1; break;
        }
        break;
    case 0x80:
        compiler_warning(0x78);
        break;
    case 0x106:
        if (next == 0x64) {
            report_language_diagnostic(1, 0xd8);
            base = 0x107;
        } else incompatible = 1;
        break;
    case 0x107:
        if (next == 0x64) base = 0x108;
        else incompatible = 1;
        break;
    default:
        if (base & 0x200) incompatible = 1;
        break;
    }
    if (incompatible) {
        const char *second_name = format_type_code(next);
        const char *first_name = format_type_code(base);
        compiler_warning(0x278, first_name, second_name);
    }
    return base | first_sign;
}
void RECON_THISCALL ParserDeclarationReader::ReadTypeSpecifier(void)
{
    unsigned kind = token->kind;
    if (type) {
        if (kind >= 246) { __asm { int 3 } }
        compiler_warning(0x274, type_symbol, compiler_token_spellings[kind]);
    } else {
        if (kind >= PARSER_TOKEN_KIND_COUNT) { __asm { int 3 } }
        WORD code = (WORD)parser_token_semantic_codes[kind];
        if (!type_flags) type_flags = code;
        else type_flags = combine_type_specifiers(type_flags, code);
    }
}

extern "C" CompilerSymbol *__cdecl current_declarator_context(void);
int RECON_THISCALL ParserDeclarationReader::ReadIdentifier(void)
{
    ParserTokenView *resolved, *copy;
    CompilerNameEntry *name;
    CompilerSymbol *symbol;
    unsigned diagnostic;
    void *argument;
    if (!type && !type_flags) {
        frontend_data.compiler_flags |= 0x400;
        token = resolve_identifier_type(token);
        frontend_data.compiler_flags &= ~0x400U;
        if (token->kind == 0x1e) {
            ReadTypeName();
            return 0;
        }
        return declaration_token ? 1 : 2;
    }
    if (frontend_bss.parser_context_depth || frontend_bss.declaration_scope_mode ||
        frontend_data.declaration_context_index < 0 ||
        (frontend_data.compiler_flags & 0x10000000)) goto accept_identifier;
    if (token->kind == 6) {
        name = TOKEN_SEMANTIC(token).name;
        if (current_class_name() != name) goto accept_identifier;
        if (frontend_bss.symbol_arena_kind) {
            unsigned arena = frontend_bss.symbol_arena_kind;
            if (arena >= 9) { __asm { int 3 } }
            copy = (ParserTokenView *)frontend_bss.arenas[arena].Allocate(48);
        } else copy = (ParserTokenView *)frontend_bss.pools[7].Allocate(48);
        if (copy) memcpy(copy, token, 48);
        resolved = read_qualified_parser_token(copy, 0);
        if (resolved->kind == 0x96) goto check_class_reference;
        if (resolved->kind == 6 && current_class_name() == TOKEN_SEMANTIC(resolved).name) {
            diagnostic = 0x49;
            argument = name;
            goto diagnose_class_name;
        }
        token = resolved;
        goto accept_identifier;
    }
    if (token->kind != 0x96) goto accept_identifier;
    resolved = token;
check_class_reference:
    token = resolved;
    symbol = ((CompilerScopeMatch *)TOKEN_SEMANTIC(token).pointer)->symbol;
    if (!(symbol->type_flags & 0x10)) goto accept_identifier;
    name = ((CompilerBoundSymbol *)symbol)->binding->owner->named_scope->link.name;
    if (current_class_name() != name) goto accept_identifier;
    diagnostic = 0x53;
    argument = symbol;
diagnose_class_name:
    compiler_warning(0x17c, diagnostic, argument);
    type_flags = 0;
    type_symbol = current_declarator_context();
    type = declaration_symbol_type(type_symbol);
    return 0;
accept_identifier:
    frontend_data.compiler_flags |= 1;
    return 1;
}

extern "C" CompilerNameEntry *__cdecl current_class_name(void)
{
    int index = frontend_data.declaration_context_index;
    if (index < 0) return NULL;
    if (index >= 16) { __asm { int 3 } }
    return frontend_bss.class_scope_rows[index].symbol->name;
}

extern "C" CompilerType *__cdecl intern_type(CompilerType *);
extern "C" CompilerType *__cdecl normalize_parsed_type(CompilerType *description)
{
    if (!description) return frontend_data.builtin_types.type_0063;
    WORD flags = description->flags;
    if (!(flags & 0x2fdf)) flags |= 0x63;
    flags &= 0xcfff;
    if (flags & 0xc000) {
        if (!(flags & 0x40) && !(flags & 0x800)) {
            report_language_diagnostic(1, 0x4c, ((flags >> 14) & 1) | 0x18);
            flags &= 0x3fff;
        } else if ((flags & 0x2fdf) != 0x41) flags &= 0x7fff;
    }
    description->flags = flags;
    CompilerType *result = intern_type(description);
    --frontend_bss.type_scratch_index;
    return result;
}

/* Name lookup and conversion to the type-name token arm. These partial
 * pointees describe existing allocations; they create no parallel owners. */
struct ParserQualifiedLookupValue {
    CompilerNameEntry *name;
    unsigned form; /* literal1 selects the following context */
    CompilerSymbol *symbol;
    void *context;
};
typedef char ParserQualifiedLookup_offsets[(
    offsetof(ParserQualifiedLookupValue, form) == 4 &&
    offsetof(ParserQualifiedLookupValue, symbol) == 8 &&
    offsetof(ParserQualifiedLookupValue, context) == 12) ? 1 : -1];
struct ParserIndirectSymbol : CompilerNamedSymbol {
    CompilerSymbol **target;
};
struct ParserResolvedNameLink {
    CompilerSymbol *symbol;
    union { ParserResolvedNameLink *next; void *context; };
};
typedef char ParserResolvedNameLink_layout[(sizeof(ParserResolvedNameLink) == 8 &&
    offsetof(ParserResolvedNameLink, context) == 4) ? 1 : -1];
extern "C" {
void __cdecl select_temporary_symbol_arena(unsigned);
void __cdecl restore_temporary_symbol_arena(void);
CompilerSymbolBinding *__cdecl copy_symbol_binding(CompilerSymbol *);
CompilerSymbol *__cdecl prepare_qualified_symbol(CompilerSymbol *, unsigned);
ParserTokenView *__cdecl make_declaration_type_token(CompilerType *, BYTE,
    const CompilerDeclarationContext *, CompilerSymbol *);
}
extern "C" ParserTokenView *__cdecl resolve_identifier_type(ParserTokenView *token)
{
    CompilerSymbol *symbol = NULL, *reference_symbol = NULL, *result_symbol;
    CompilerGlobalSymbolView *browser_symbol;
    CompilerNameEntry *name;
    CompilerScopeMatch *match;
    CompilerSymbolBinding *binding;
    ParserQualifiedLookupValue *qualified;
    ParserResolvedNameLink *link;
    CompilerArena *arena;
    CompilerPool *pool;
    ParserTokenView *result;
    BYTE *memory;
    unsigned flags, kind, arena_kind, bytes;
    int referenced = 0, index;
    BYTE result_kind;
    if (token->kind == 6) token = read_qualified_parser_token(token, 0);
    kind = token->kind;
    if (kind == 6) {
        name = TOKEN_SEMANTIC(token).name;
        if (!(name->flags & 0x20000)) goto ordinary_token;
    } else if (kind == 0x96) {
        referenced = 1;
        symbol = ((CompilerScopeMatch *)TOKEN_SEMANTIC(token).pointer)->symbol;
    } else if (kind == 0xea) {
        qualified = (ParserQualifiedLookupValue *)TOKEN_SEMANTIC(token).pointer;
        if (!(qualified->name->flags & 0x20000)) goto ordinary_token;
    } else goto ordinary_token;
    flags = frontend_data.compiler_flags;
    if (flags & 0x40) {
        match = frontend_data.translation_unit->LookupName(
            (CompilerNameEntry *)TOKEN_SEMANTIC(token).pointer, 0x132);
        symbol = match ? match->symbol : NULL;
        if (!symbol) compiler_warning(0x41, TOKEN_SEMANTIC(token).pointer);
        else if (!(symbol->definition_flags & 0x10))
            compiler_warning(0x3d, TOKEN_SEMANTIC(token).pointer);
        else return make_declaration_type_token(((CompilerIrSymbol *)symbol)->type, 0, NULL, NULL);
        return token;
    }
    if (!(flags & 0x80000000)) {
        if (flags & 1) goto ordinary_token;
        if ((*declaration_control_row(frontend_bss.parser_context_depth)).flags & 2) return token;
        if (frontend_data.translation_unit->opaque_lookup_state) return token;
    }
    if (kind == 6) {
        match = frontend_data.translation_unit->LookupName(TOKEN_SEMANTIC(token).name, 0x132);
        symbol = match ? match->symbol : NULL;
        if (!symbol) return token;
    } else if (kind == 0xea) {
        symbol = ((ParserQualifiedLookupValue *)TOKEN_SEMANTIC(token).pointer)->symbol;
    }
    if (symbol->kind == 0x12) {
        reference_symbol = symbol;
        symbol = ((CompilerBoundSymbol *)symbol)->binding->owner->named_scope;
    }
    if (symbol->kind == 15 && !(((CompilerClassMemberSymbol *)symbol)->flags & 2)) {
        index = ((CompilerClassMemberSymbol *)symbol)->owner_index;
        if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
        symbol = (CompilerSymbol *)frontend_data.frontend_pointer_table->items[index];
    } else if (symbol->kind == 24) {
        symbol = *((ParserIndirectSymbol *)symbol)->target;
    }
    if (!(symbol->definition_flags & 0x10)) return token;
    if (!frontend_bss.parser_context_depth && (frontend_data.compiler_flags & 0x20000) &&
        !(frontend_data.compiler_flags & 0x18000000) &&
        !(frontend_data.compiler_mode_flags & 0x18)) {
        ParserTokenView *following = TOKEN_CONTEXT.PeekToken();
        if (following->kind == 0x36) return token;
    }
    if (symbol->definition_flags & 0x20) {
        CompilerSymbol *instantiated = instantiate_tag_definition(symbol);
        if (!referenced) return token;
        ((CompilerBoundSymbol *)reference_symbol)->binding->owner->named_scope =
            (CompilerNamedSymbol *)instantiated;
        result_symbol = reference_symbol;
        goto make_reference;
    }
    if (!referenced) {
        if (frontend_bss.parser_context_depth >= 60) { __asm { int 3 } }
        frontend_bss.parser_qualification_scopes[frontend_bss.parser_context_depth] = NULL;
    } else if (frontend_data.options.language_extensions) {
        CompilerSymbol *prepared = prepare_qualified_symbol(reference_symbol, 4);
        if (frontend_bss.parser_context_depth >= 60) { __asm { int 3 } }
        frontend_bss.parser_qualification_scopes[frontend_bss.parser_context_depth] = prepared;
    } else check_qualified_declaration(reference_symbol);
    if (symbol->kind == 4 &&
        ((*declaration_control_row(frontend_bss.parser_context_depth)).flags & 1) &&
        !(frontend_data.compiler_mode_flags & 8)) return token;
    if (frontend_bss.browser_filename &&
        (!frontend_bss.declaration_scope_mode || publish_browser_local)) {
        if (symbol->kind == 0x12)
            browser_symbol = (CompilerGlobalSymbolView *)((CompilerBoundSymbol *)symbol)->binding->owner->named_scope;
        else browser_symbol = (CompilerGlobalSymbolView *)symbol;
        if (!(browser_symbol->record_index & 0xffffff)) {
            if (frontend_data.next_symbol_record_id >= 0x7fffffff) report_compiler_error(0x37);
            browser_symbol->record_index = (browser_symbol->record_index & 0xff000000) |
                (frontend_data.next_symbol_record_id++ & 0xffffff);
        }
        frontend_data.backend->Slot1c((unsigned)browser_symbol);
    }
    result_symbol = reference_symbol ? reference_symbol : symbol;
    if ((frontend_data.compiler_mode_flags & 0x100) &&
        !(frontend_data.compiler_mode_flags & 0x20000000) && result_symbol->kind == 10 &&
        ((((CompilerGlobalSymbolView *)result_symbol)->storage_class & 7) == 7)) {
        CompilerType *type = ((CompilerIrSymbol *)result_symbol)->type;
        index = (short)type->auxiliary;
        if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
        CompilerSymbol *qualified_symbol = (CompilerSymbol *)frontend_data.frontend_pointer_table->items[index];
        CompilerSymbol *owner = qualified_symbol;
        if (owner && owner->declaration_qualifiers) {
            index = (short)owner->declaration_qualifiers;
            if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
            owner = (CompilerSymbol *)frontend_data.frontend_pointer_table->items[index];
            if (owner->kind == 4) {
                select_temporary_symbol_arena(5);
                binding = copy_symbol_binding(qualified_symbol);
                restore_temporary_symbol_arena();
                bytes = (frontend_data.symbol_record_bytes[18] + 3) & ~3U;
                CompilerNameEntry *copy_name = ((CompilerNamedSymbol *)qualified_symbol)->link.name;
                memory = frontend_bss.arenas[5].Allocate(bytes);
                CompilerSymbol *copy = memory ? ((CompilerSymbol *)memory)->Initialize(18,
                    copy_name, 5) : NULL;
                ((CompilerBoundSymbol *)copy)->binding = binding;
                result_symbol = copy;
                goto make_reference;
            }
        }
    }
    saved_parser_value.pointer = result_symbol;
    result_kind = 0x1e;
    goto allocate_token;
make_reference:
    arena = &frontend_bss.arenas[4]; /* actual c6580 */
    memory = arena->cursor;
    arena->cursor += 8;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    link = (ParserResolvedNameLink *)memory;
    if (link) { link->symbol = result_symbol; link->next = NULL; }
    saved_parser_value.pointer = link;
    result_kind = 0x96;
allocate_token:
    arena_kind = frontend_bss.symbol_arena_kind;
    if (arena_kind) {
        if (arena_kind >= 9) { __asm { int 3 } }
        arena = &frontend_bss.arenas[arena_kind];
        memory = arena->cursor;
        arena->cursor += 48;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        result = (ParserTokenView *)memory;
    } else {
        pool = &frontend_bss.pools[7];
        if (pool->remaining < 48) pool->Refill(48);
        result = (ParserTokenView *)pool->cursor;
        pool->cursor += 48;
        pool->remaining -= 48;
    }
    if (result) {
        result->kind = result_kind;
        memcpy(&result->value, &saved_parser_value, 40);
    }
    return result;
ordinary_token:
    frontend_data.compiler_flags &= ~1U;
    return token;
}

/* Qualified-name reader, including dependent template and destructor syntax. */
extern "C" WORD __cdecl classify_template_type(CompilerType *, unsigned, unsigned);
extern "C" CompilerSymbolBinding *__cdecl make_symbol_binding(CompilerSymbolBinding *, CompilerSymbolBindingOwner *, CompilerSymbol *);
extern "C" CompilerSymbolBinding *__cdecl extend_qualified_binding(CompilerSymbolBinding *, CompilerNameEntry *, unsigned, unsigned);
extern "C" CompilerSymbol *__cdecl materialize_qualified_binding(CompilerSymbolBinding *, CompilerNameEntry *, unsigned);
extern "C" void __cdecl read_dependent_template_id(ParserTokenView **, CompilerSymbol *, unsigned);
extern "C" CompilerSymbol *__cdecl find_bound_declaration(CompilerSymbol *, CompilerSymbol *);
extern "C" WORD __cdecl get_type_classification(CompilerType *);
/* The scopes and bindings here are existing owners. The native domains of
 * those pointees still stop at their named pending constructors/consumers. */
#define QUALIFIED_CLASS(b) ((CompilerSymbol *)(b)->owner->named_scope)
#define QUALIFIED_TYPE(s) (((CompilerIrSymbol *)(s))->type)
#define QUALIFIED_ALIAS(s) ((CompilerSymbol *)((CompilerBoundSymbol *)(s))->binding->owner->named_scope)
/* This is ordinary inline arena/pool allocation, without another App body. */
#define GET_QUALIFIED_TOKEN(destination) do { \
    unsigned a_ = frontend_bss.symbol_arena_kind; \
    if (a_) { \
        if (a_ >= 9) { __asm { int 3 } } \
        CompilerArena *arena_ = &frontend_bss.arenas[a_]; \
        BYTE *memory_ = arena_->cursor; \
        arena_->cursor += 48; \
        if (arena_->cursor > arena_->committed_limit) arena_->CommitExtent(); \
        destination = (ParserTokenView *)memory_; \
    } else { \
        CompilerPool *pool_ = &frontend_bss.pools[7]; \
        if (pool_->remaining < 48) pool_->Refill(48); \
        destination = (ParserTokenView *)pool_->cursor; \
        pool_->cursor += 48; pool_->remaining -= 48; \
    } \
} while (0)
extern "C" ParserTokenView *__cdecl read_qualified_parser_token(ParserTokenView *input, unsigned mode)
{
    ParserTokenView *token = TOKEN_CONTEXT.ReadToken(), *next, *result;
    CompilerSymbolBinding *binding = NULL, *previous_binding;
    CompilerSymbol *symbol = NULL, *scope_symbol, *candidate, *owner;
    CompilerGlobalSymbolView *view;
    CompilerScopeMatch *match;
    CompilerScopeLookup *matches;
    CompilerNameEntry *name, *special_name;
    CompilerType *type;
    CompilerTranslationUnit *unit;
    CompilerSymbol *saved_retired;
    int saved_suppression;
    DWORD saved_opaque;
    BYTE saved_lookup_context;
    int errors = 0, leading_global = 0, constructor = 0, destructor = 0;
    int scope_marker = 0, scope_marker_second = 0, mismatch, found_locally, hidden;
    unsigned flags, index, name_flags, materialize_flags;
    BYTE operator_form;
    void *context = NULL;
    ParserQualifiedLookupValue *qualified;
    ParserResolvedNameLink *link;
    CompilerArena *arena;
    BYTE *memory;
    if (input) {
        name_flags = TOKEN_SEMANTIC(input).name->flags;
        if ((name_flags & 0x4000000) ||
            ((name_flags & 0x20000) && token->kind == 0x44)) {
            TOKEN_STREAM->Unread((unsigned)token);
            resolve_tag_template_id(&input, 0, 0, 0);
            token = TOKEN_CONTEXT.ReadToken();
            goto check_scope_separator;
        }
        if ((name_flags & 0x8000000) && token->kind == 0x44) {
            TOKEN_STREAM->Unread((unsigned)token);
            read_dependent_template_id(&input, NULL, 0);
            token = TOKEN_CONTEXT.ReadToken();
        }
    }
check_scope_separator:
    if (token->kind != 0x95) {
        TOKEN_STREAM->Unread((unsigned)token);
        return input;
    }
    if (!input) {
        leading_global = 1;
        select_temporary_symbol_arena(4);
        binding = make_symbol_binding(NULL, NULL, frontend_data.translation_unit->global_namespace);
        restore_temporary_symbol_arena();
        goto read_segment;
    }
    name = input->kind == 0xea ?
        ((ParserQualifiedLookupValue *)TOKEN_SEMANTIC(input).pointer)->name : TOKEN_SEMANTIC(input).name;
    frontend_data.compiler_flags |= 0x10;
    match = frontend_data.translation_unit->FindDirectName(name); /*2142b*/
    frontend_data.compiler_flags &= ~0x10U;
    if (!match) {
        match = frontend_data.translation_unit->FindHiddenName(name); /*3252d*/
        candidate = match ? match->symbol : NULL;
        symbol = NULL;
        if (candidate && candidate->kind != 22) {
            if (candidate->kind == 18) candidate = QUALIFIED_ALIAS(candidate);
            type = QUALIFIED_TYPE(candidate);
            if ((short)type->auxiliary < 0) symbol = match->symbol;
            else if (((((CompilerGlobalSymbolView *)candidate)->storage_class & 7) == 7) && !type->shape) {
                index = (short)type->auxiliary;
                if (index >= (unsigned)frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
                owner = (CompilerSymbol *)frontend_data.frontend_pointer_table->items[index];
                if (owner && owner->kind == 4) {
                    hidden = match->scope && match->scope->level == 0xfa;
                    match = frontend_data.translation_unit->FindContextName(((CompilerNamedSymbol *)owner)->link.name); /*9a331*/
                    symbol = match ? match->symbol : NULL;
                    if (symbol && symbol->kind == 18) {
                        if (!hidden) report_language_diagnostic(4, 0x61, 0x53, candidate, 0x53, symbol);
                        match = frontend_data.translation_unit->LookupName(name, 0x132);
                        owner = match ? match->symbol : NULL;
                        if (owner->kind == 18) owner = QUALIFIED_ALIAS(owner);
                        index = (short)QUALIFIED_TYPE(owner)->auxiliary;
                        if (index >= (unsigned)frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
                        owner = (CompilerSymbol *)frontend_data.frontend_pointer_table->items[index];
                        match = frontend_data.translation_unit->FindDirectName(((CompilerNamedSymbol *)owner)->link.name);
                        symbol = match ? match->symbol : NULL;
                    } else symbol = NULL;
                }
            }
        }
    } else symbol = match->symbol;
    if (!symbol) {
        match = frontend_data.translation_unit->LookupName(name, 0x132);
        symbol = match ? match->symbol : NULL;
        if (symbol && symbol->kind != 22) {
            if (symbol->kind == 18) symbol = QUALIFIED_ALIAS(symbol);
            if (!(symbol->definition_flags & 0x10)) symbol = NULL;
            else if (get_type_classification(QUALIFIED_TYPE(symbol)) & 0x80) {
                index = (short)QUALIFIED_TYPE(symbol)->auxiliary;
                if (index >= (unsigned)frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
                symbol = (CompilerSymbol *)frontend_data.frontend_pointer_table->items[index];
            } else {
                next = TOKEN_CONTEXT.PeekToken();
                TOKEN_STREAM->Unread((unsigned)token);
                if (next->kind == 0x59) {
                    frontend_data.compiler_flags |= 0x40;
                    return make_declaration_type_token(QUALIFIED_TYPE(symbol), 0, NULL, NULL);
                }
                return input;
            }
        }
        if (!symbol) {
            compiler_warning(0x28d, 0x49, name);
            errors = 1;
            goto read_segment;
        }
    }
    if (symbol->kind == 18) binding = ((CompilerBoundSymbol *)symbol)->binding;
    else {
        select_temporary_symbol_arena(4);
        if (symbol->kind != 23 && (frontend_data.source_mode_flags & 0x200000) &&
            classify_template_type(QUALIFIED_TYPE(symbol), 0, 0) && symbol->kind != 22 &&
            !(((ParserDefinitionClassView *)((CompilerGlobalSymbolView *)symbol)->class_detail)->completion_flags & 2))
            symbol = instantiate_declaration_class(symbol);
        binding = make_symbol_binding(NULL, NULL, symbol);
        restore_temporary_symbol_arena();
    }
    if (!frontend_data.translation_unit->opaque_lookup_state) {
        scope_symbol = QUALIFIED_CLASS(binding);
        if (match->scope->level == 0xff && ((CompilerClassScope *)match->scope)->reset_value) {
            if (!(scope_symbol->definition_flags & 0x10) ||
                ((get_type_classification(QUALIFIED_TYPE(scope_symbol)) & 0x80) &&
                 ((CompilerClassScope *)match->scope)->symbol == scope_symbol)) scope_marker = 1;
        }
        if (match->scope->level == 0xfc) scope_marker_second = 1;
    }
read_segment:
    special_name = NULL;
    name = NULL;
    operator_form = 1;
    token = TOKEN_CONTEXT.ReadToken();
dispatch_segment:
    switch (token->kind) {
    case 0:
        report_compiler_error(4);
        goto extend_name;
    case 6: case 234:
        goto ordinary_name;
    case 89:
        goto destructor_name;
    case 151:
        token = process_parser_declaration();
        if (token->kind >= PARSER_TOKEN_KIND_COUNT) { __asm { int 3 } }
        if (parser_token_classes[token->kind] == 1) goto read_separator;
        if (parser_token_classes[token->kind] == 142) {
            name = TOKEN_SEMANTIC(token).named_token.name;
            operator_form = TOKEN_SEMANTIC(token).named_token.kind;
            goto resolve_name;
        }
        if (parser_token_classes[token->kind] == 143) {
            frontend_data.compiler_flags |= 1;
            goto finish_name;
        }
        goto ordinary_name;
    case 177:
        goto template_keyword;
    case 74:
        if (!binding) compiler_warning(0x285);
        GET_QUALIFIED_TOKEN(result);
        if (result) {
            result->kind = binding ? 0x9f : 0x4a;
            memcpy(&result->value, &saved_parser_value, 40);
        }
        goto materialize_name;
    case 140: case 141:
        if (!leading_global) compiler_warning(0x342, token->kind);
        if (frontend_data.compiler_mode_flags & 8) {
            mode = (mode & ~0xffU) | token->kind;
            compiler_warning(0x2d2, token->kind, (BYTE)mode);
        }
        saved_parser_value.token_kind = 2;
        GET_QUALIFIED_TOKEN(result);
        if (result) {
            result->kind = token->kind;
            memcpy(&result->value, &saved_parser_value, 40);
        }
        return result;
    default:
        compiler_warning(0x24d, token->kind);
        binding = NULL;
        goto finish_name;
    }
template_keyword:
    next = TOKEN_CONTEXT.ReadToken();
    if (next->kind == 6) {
        name_flags = TOKEN_SEMANTIC(next).name->flags;
        if ((name_flags & 0x4000000) && TOKEN_CONTEXT.PeekToken()->kind == 0x44) {
            resolve_tag_template_id(&next, binding ? QUALIFIED_CLASS(binding) : NULL,
                leading_global, 0);
        } else if ((name_flags & 0x8000000) && TOKEN_CONTEXT.PeekToken()->kind == 0x44) {
            read_dependent_template_id(&next, binding ? QUALIFIED_CLASS(binding) : NULL, leading_global);
        } else compiler_warning(0x387, TOKEN_SEMANTIC(next).name);
    } else compiler_warning(0x386, next->kind);
    TOKEN_STREAM->Unread((unsigned)next);
    token = TOKEN_CONTEXT.ReadToken();
    goto dispatch_segment;
destructor_name:
    token = TOKEN_CONTEXT.ReadToken();
    if (token->kind != 6) {
        compiler_warning(0x263, token->kind);
        return token;
    }
    name = TOKEN_SEMANTIC(token).name;
    name_flags = name->flags;
    if ((name_flags & 0x4000000) ||
        ((name_flags & 0x20000) && TOKEN_CONTEXT.PeekToken()->kind == 0x44)) {
        resolve_tag_template_id(&token, binding ? QUALIFIED_CLASS(binding) : NULL,
            leading_global, 0);
        name = token->kind == 0xea ?
            ((ParserQualifiedLookupValue *)TOKEN_SEMANTIC(token).pointer)->name : TOKEN_SEMANTIC(token).name;
    }
    if (!binding) {
        if (!errors) { compiler_warning(0x24c, name); errors = 1; }
        goto read_separator;
    }
    scope_symbol = QUALIFIED_CLASS(binding);
    if (scope_symbol && scope_symbol->kind == 4 &&
        (((ParserDefinitionClassView *)((CompilerGlobalSymbolView *)scope_symbol)->class_detail)->completion_flags & 1)) {
        flags = ((ParserDefinitionClassView *)((CompilerGlobalSymbolView *)scope_symbol)->class_detail)->completion_flags;
        if ((frontend_data.source_mode_flags & 0x200000) && (flags & 2) &&
            ((CompilerNamedSymbol *)scope_symbol)->link.name == name) {
            name = ((CompilerNamedSymbol *)scope_symbol)->link.name;
            saved_parser_value.pointer = name;
            if (frontend_bss.symbol_arena_kind) {
                index = frontend_bss.symbol_arena_kind;
                if (index >= 9) { __asm { int 3 } }
                result = (ParserTokenView *)frontend_bss.arenas[index].Allocate(48);
            } else result = (ParserTokenView *)frontend_bss.pools[7].Allocate(48);
            if (result) { result->kind = token->kind; memcpy(&result->value, &saved_parser_value, 40); }
            token = result;
        } else {
            owner = instantiate_declaration_class(scope_symbol);
            if (((CompilerNamedSymbol *)owner)->link.name == name) {
                name = ((CompilerNamedSymbol *)scope_symbol)->link.name;
                saved_parser_value.pointer = name;
                if (frontend_bss.symbol_arena_kind) {
                    index = frontend_bss.symbol_arena_kind;
                    if (index >= 9) { __asm { int 3 } }
                    result = (ParserTokenView *)frontend_bss.arenas[index].Allocate(48);
                } else result = (ParserTokenView *)frontend_bss.pools[7].Allocate(48);
                if (result) { result->kind = token->kind; memcpy(&result->value, &saved_parser_value, 40); }
                token = result;
            }
        }
    }
    scope_symbol = QUALIFIED_CLASS(binding);
    if (scope_symbol->kind == 23) { compiler_warning(0x24c, name); return NULL; }
    if (((CompilerNamedSymbol *)scope_symbol)->link.name != name) {
        mismatch = 1;
        found_locally = 1;
        match = frontend_data.translation_unit->FindHiddenName(name);
        candidate = match ? match->symbol : NULL;
        if (!candidate) {
            found_locally = 0;
            unit = frontend_data.translation_unit;
            saved_retired = unit->retired_scopes;
            saved_lookup_context = unit->lookup_context;
            saved_opaque = unit->opaque_lookup_state;
            saved_suppression = unit->suppress_lookup_errors;
            unit->suppress_lookup_errors = 0; unit->retired_scopes = NULL;
            unit->lookup_context = 1; unit->opaque_lookup_state = 0;
            match = frontend_data.translation_unit->FindHiddenName(name);
            candidate = match ? match->symbol : NULL;
            unit = frontend_data.translation_unit;
            if (saved_opaque) {
                unit->retired_scopes = saved_retired;
                unit->lookup_context = saved_lookup_context;
                unit->opaque_lookup_state = 1;
                unit->suppress_lookup_errors = saved_suppression;
            } else {
                unit->retired_scopes = NULL; unit->lookup_context = 1;
                unit->opaque_lookup_state = 0; unit->suppress_lookup_errors = 0;
            }
        }
        if (candidate) {
            if (candidate->kind == 18) candidate = QUALIFIED_ALIAS(candidate);
            if ((((CompilerGlobalSymbolView *)candidate)->storage_class & 7) == 7) {
                type = QUALIFIED_TYPE(candidate);
                index = (short)type->auxiliary;
                if (index >= (unsigned)frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
                owner = (CompilerSymbol *)frontend_data.frontend_pointer_table->items[index];
                if (owner && !type->shape && owner->kind == 4 && owner == QUALIFIED_CLASS(binding)) {
                    hidden = match->scope && match->scope->level == 0xfa;
                    if (!hidden) report_language_diagnostic(4, 0x61, 0x49, name, 0x53, owner);
                    unit = frontend_data.translation_unit;
                    saved_retired = unit->retired_scopes;
                    saved_lookup_context = unit->lookup_context;
                    saved_opaque = unit->opaque_lookup_state;
                    saved_suppression = unit->suppress_lookup_errors;
                    if (!found_locally) {
                        unit->suppress_lookup_errors = 0; unit->retired_scopes = NULL;
                        unit->lookup_context = 1; unit->opaque_lookup_state = 0;
                    }
                    frontend_data.translation_unit->LookupName(name, 0x132);
                    mismatch = 0;
                    unit = frontend_data.translation_unit;
                    if (saved_opaque) {
                        unit->retired_scopes = saved_retired; unit->lookup_context = saved_lookup_context;
                        unit->opaque_lookup_state = 1; unit->suppress_lookup_errors = saved_suppression;
                    } else {
                        unit->retired_scopes = NULL; unit->lookup_context = 1;
                        unit->opaque_lookup_state = 0; unit->suppress_lookup_errors = 0;
                    }
                }
            }
        }
        if (mismatch) compiler_warning(0x20b, QUALIFIED_CLASS(binding), name);
    }
    if (destructor) compiler_warning(0x254, name);
    if (constructor) compiler_warning(0x253, name);
    destructor = 1;
    special_name = name;
    name = frontend_data.builtin_names.destructor;
    goto extend_name;
ordinary_name:
    if (token->kind == 6) {
        name = TOKEN_SEMANTIC(token).name;
        name_flags = name->flags;
        if ((name_flags & 0x4000000) && TOKEN_CONTEXT.PeekToken()->kind == 0x44) {
            resolve_tag_template_id(&token, binding ? QUALIFIED_CLASS(binding) : NULL,
                leading_global, 0);
        } else if ((name_flags & 0x8000000) && TOKEN_CONTEXT.PeekToken()->kind == 0x44) {
            read_dependent_template_id(&token, binding ? QUALIFIED_CLASS(binding) : NULL, leading_global);
        }
    }
    if (token->kind == 0xea) {
        qualified = (ParserQualifiedLookupValue *)TOKEN_SEMANTIC(token).pointer;
        name = qualified->name;
        if (qualified->form == 1) context = qualified->context;
    }
resolve_name:
    if (!binding) {
        if (errors) goto read_separator;
        matches = frontend_data.translation_unit->root->Find(name, 1);
        match = matches ? matches->Select(1) : NULL;
        symbol = match ? match->symbol : NULL;
        if (!symbol) {
            if (token->kind == 142) {
                saved_parser_value.token_kind = TOKEN_SEMANTIC(token).named_token.kind;
                GET_QUALIFIED_TOKEN(result);
                if (result) { result->kind = 0x97; memcpy(&result->value, &saved_parser_value, 40); }
                return result;
            }
            compiler_warning(0x205, name);
            errors = 1;
            binding = NULL;
            goto read_separator;
        }
        if (symbol->kind == 10 && (symbol->definition_flags & 0x10) &&
            (get_type_classification(QUALIFIED_TYPE(symbol)) & 0x80)) {
            index = (short)QUALIFIED_TYPE(symbol)->auxiliary;
            if (index >= (unsigned)frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
            symbol = (CompilerSymbol *)frontend_data.frontend_pointer_table->items[index];
            if ((frontend_data.compiler_flags & 0x80000) && TOKEN_CONTEXT.PeekToken()->kind != 0x95)
                compiler_warning(0xf2);
        }
        select_temporary_symbol_arena(4);
        binding = make_symbol_binding(NULL, NULL, symbol);
        restore_temporary_symbol_arena();
        goto read_separator;
    }
    scope_symbol = QUALIFIED_CLASS(binding);
    if (scope_symbol && scope_symbol->kind == 4 &&
        (((ParserDefinitionClassView *)((CompilerGlobalSymbolView *)scope_symbol)->class_detail)->completion_flags & 1)) {
        owner = instantiate_declaration_class(scope_symbol);
        if (((CompilerNamedSymbol *)owner)->link.name == name) name = ((CompilerNamedSymbol *)scope_symbol)->link.name;
    }
    scope_symbol = QUALIFIED_CLASS(binding);
    if (scope_symbol->kind != 23 && ((CompilerNamedSymbol *)scope_symbol)->link.name == name) {
        if (constructor) compiler_warning(0x253, name);
        if (destructor) compiler_warning(0x254, name);
        constructor = 1;
        special_name = name;
        name = frontend_data.builtin_names.constructor;
    }
extend_name:
    previous_binding = binding;
    binding = extend_qualified_binding(binding, name, 0, mode);
    if (!binding) {
        if (frontend_data.declaration_context_index < 0) constructor = destructor = 0;
        if (constructor || destructor) {
            symbol = lookup_builtin(special_name);
            symbol->type_flags |= destructor ? 0x20 : 0x10;
            GET_QUALIFIED_TOKEN(result);
            if (result) { result->kind = 0xad; TOKEN_SEMANTIC(result).pointer = symbol; }
            return result;
        }
        scope_symbol = QUALIFIED_CLASS(previous_binding);
        if (!(scope_symbol->definition_flags & 4) &&
            ((CompilerNamedSymbol *)scope_symbol)->link.name == current_class_name()) {
            if (operator_form != 1) {
                GET_QUALIFIED_TOKEN(result);
                if (result) {
                    result->kind = 0x8e;
                    TOKEN_SEMANTIC(result).named_token.kind = operator_form;
                    TOKEN_SEMANTIC(result).named_token.name = name;
                }
                return result;
            }
            if (token->kind == 6) {
                GET_QUALIFIED_TOKEN(result);
                if (result) { result->kind = 6; TOKEN_SEMANTIC(result).name = name; }
                return result;
            }
        }
        ++errors;
    }
read_separator:
    token = TOKEN_CONTEXT.ReadToken();
    if (token->kind == 0x95) goto read_segment;
finish_name:
    TOKEN_STREAM->Unread((unsigned)token);
    GET_QUALIFIED_TOKEN(result);
    if (input) {
        if (result) memcpy(result, input, 48);
        result->kind = 0x96;
    } else if (result) {
        result->kind = 0x96;
        memcpy(&result->value, &saved_parser_value, 40);
    }
materialize_name:
    if (!binding) {
        if (special_name) { result->kind = 6; TOKEN_SEMANTIC(result).name = special_name; return result; }
        if (name) { result->kind = 6; TOKEN_SEMANTIC(result).name = name; return result; }
        return result->kind == 0x4a ? result : (ParserTokenView *)frontend_data.canonical_token_records[148];
    }
    materialize_flags = 0;
    if (frontend_bss.declaration_scope_mode && frontend_data.declaration_context_index < 0 &&
        !(frontend_data.compiler_mode_flags & 0x10000000) &&
        (!(frontend_data.compiler_mode_flags & 0x40000000) ||
         (frontend_data.source_mode_flags & 8) || (frontend_data.compiler_flags & 0x200)) &&
        !(frontend_data.source_mode_flags & 0x50)) materialize_flags = 4;
    symbol = materialize_qualified_binding(binding, name, materialize_flags);
    if (result->kind == 0x9f) TOKEN_SEMANTIC(result).pointer = symbol;
    else {
        result->kind = 0x96;
        arena = &frontend_bss.arenas[4];
        memory = arena->cursor;
        arena->cursor += 8;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        link = (ParserResolvedNameLink *)memory;
        if (link) { link->symbol = symbol; link->context = context; }
        TOKEN_SEMANTIC(result).pointer = link;
    }
    if (leading_global) {
        if (frontend_data.translation_unit->opaque_lookup_state) {
            compiler_warning(0x12b, symbol, frontend_data.translation_unit->lookup_context);
            symbol->scope_flags |= 4;
        }
        symbol->scope_flags |= 0x40;
    }
    if ((frontend_data.compiler_flags & 0x80000) && !mode) {
        owner = ((CompilerBoundSymbol *)symbol)->binding->owner->named_scope;
        if (owner->kind == 10 && (owner->definition_flags & 0x10) &&
            (get_type_classification(QUALIFIED_TYPE(owner)) & 0x80)) compiler_warning(0xf2);
    }
    symbol = result->kind == 0x96 ? ((ParserResolvedNameLink *)TOKEN_SEMANTIC(result).pointer)->symbol :
        (CompilerSymbol *)TOKEN_SEMANTIC(result).pointer;
    find_bound_declaration(symbol, NULL);
    if (special_name) symbol->type_flags |= 0x10;
    if (destructor) symbol->type_flags |= 0x20;
    if (scope_marker) symbol->type_flags |= 0x80;
    if (scope_marker_second) symbol->scope_flags |= 1;
    return result;

}
#undef GET_QUALIFIED_TOKEN
#undef QUALIFIED_CLASS
#undef QUALIFIED_TYPE
#undef QUALIFIED_ALIAS

BYTE RECON_THISCALL ParserTokenView::Kind(void)
{
    return kind;
}

extern "C" CompilerDeclaration *__cdecl current_declaration(void)
{
    unsigned depth = frontend_bss.parser_context_depth;
    if (declaration_control_row(depth)->flags & 1)
        return declaration_spec_row(depth);
    return default_declaration(0);
}

/* Class member publication. The local context is the same seven-DWORD
 * typed owner used by declaration merging; no declaration callback is copied. */
extern "C" {
CompilerSymbol *__cdecl find_declared_member(CompilerNameEntry *); /*2643a*/
void __cdecl merge_declaration_classes(CompilerSymbol *, CompilerSymbol *);
CompilerSymbol *__cdecl prepare_function_symbol(CompilerSymbol *);
void __cdecl define_member_function(CompilerDeclarationContext, CompilerSymbol *, unsigned); /*26ede*/
void __cdecl record_member_declaration(CompilerSymbol *); /*263a6*/
void *__cdecl make_member_storage(CompilerSymbol *); /*2648e*/
void __cdecl register_static_member(CompilerSymbol *, CompilerDeclarator *); /*3654f*/
CompilerGlobalSymbolView *__cdecl enclosing_declaration_class(CompilerSymbol *); /*7321*/
int __cdecl check_member_type(CompilerSymbol *, CompilerSymbol *); /*464be*/
void __cdecl set_member_type_flags(CompilerSymbol *, unsigned, unsigned); /*451a0*/
void __cdecl initialize_function_symbol(CompilerDeclaration *, CompilerSymbol *); /*4b6a6*/
void __cdecl finish_dependent_member(CompilerSymbol *, CompilerSymbol *); /*88eed*/
void __cdecl declare_qualified_member(CompilerDeclaration *, CompilerSymbol *, CompilerDeclarator *,
    CompilerSymbol *, CompilerSymbol *, CompilerDeclarationContext, unsigned); /*45d3f*/
void __cdecl prepare_type_declaration(CompilerDeclaration *); /*460cc*/
void __cdecl complete_member_type(CompilerType *); /*461d5*/
void __cdecl declare_friend_type(CompilerSymbol *, CompilerDeclaration *, unsigned); /*3cf44*/
void __cdecl compiler_member_diagnostic(unsigned, ...); /*48caa*/
CompilerSymbol *__cdecl merge_scoped_declaration(CompilerSymbol *, CompilerSymbol *, CompilerDeclarator *);
int __cdecl is_class_scope_symbol(CompilerSymbol *);
unsigned __cdecl get_function_calling_convention(CompilerType *);
CompilerSymbol *__cdecl resolve_symbol_reference(CompilerSymbol *);
CompilerDeclarationMetadata *__cdecl get_declaration_metadata(CompilerSymbol *);
CompilerDeclarationAttachment *__cdecl get_declaration_attachment(CompilerSymbol *);
CompilerFunctionContext *__cdecl get_declaration_function_context(CompilerSymbol *);
}
/* An alternate tagged symbol points its +c link at the name-link portion
 * of a bound symbol. Its binding at+10 uses the already existing owner. */
struct MemberBoundSymbol : CompilerNamedSymbol { CompilerSymbolBinding *binding; };
struct CompilerPendingMemberNode;
struct CompilerMemberQueueState {
    CompilerPendingMemberNode *head, *current;
    void *secondary; /* only its presence is consumed here */
};
typedef char CompilerMemberQueueState_offsets[(
    offsetof(CompilerMemberQueueState, head) == 0 &&
    offsetof(CompilerMemberQueueState, current) == 4 &&
    offsetof(CompilerMemberQueueState, secondary) == 8) ? 1 : -1];
struct CompilerMemberQueue {
    CompilerMemberQueueState *data; /* same actual c0578 pointer cell */
    void RECON_THISCALL Add(CompilerSymbol *); /*44809*/
};
typedef char CompilerMemberQueue_cell[sizeof(CompilerMemberQueue) == 4 ? 1 : -1];
static RECON_FORCEINLINE DeclarationClassScope &member_class_row(void)
{
    int index = frontend_data.declaration_context_index;
    if ((unsigned)index >= 16) { __asm { int 3 } }
    return frontend_bss.class_scope_rows[index];
}
#define MEMBER_CLASS (member_class_row().symbol)
#define MEMBER_DETAIL ((ParserDefinitionClassView *)MEMBER_CLASS->class_detail)
#define MEMBER_SYMBOL ((CompilerGlobalSymbolView *)symbol)
extern "C" int __cdecl declare_initialized_symbol(CompilerDeclaration *declaration,
    CompilerSymbol *symbol, CompilerDeclarator *declarator, CompilerSymbol **replacement)
{
    CompilerDeclarationState state;
    CompilerDeclarationContext checked_context;
    CompilerGlobalSymbolView *enclosing;
    CompilerSymbol *function = NULL, *previous = NULL;
    MemberBoundSymbol *bound = NULL;
    CompilerScopeMatch *match;
    CompilerType *type;
    CompilerNameEntry *name;
    unsigned special = 0, classification;
    if (frontend_data.declaration_context_index < 0) {
        compiler_warning(0x1c);
        return 0;
    }
    if (!symbol) {
        type = declaration->type;
        if (declaration->context.context_flags & 4) {
            if ((short)type->auxiliary < 0) return 0;
            declare_friend_type((CompilerSymbol *)MEMBER_CLASS, declaration, 0);
            if (frontend_bss.browser_filename) {
                int index = (short)declaration->type->auxiliary;
                if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
                frontend_data.backend->Slot50((unsigned)frontend_data.frontend_pointer_table->items[index]);
            }
            return 0;
        }
        if (type->flags & 0x400) {
            int index = (short)type->auxiliary;
            if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
            symbol = (CompilerSymbol *)frontend_data.frontend_pointer_table->items[index];
        }
        if (type->flags & 0x400 && symbol->scope_flags & 1) {
            CompilerGlobalSymbolView *tag = (CompilerGlobalSymbolView *)symbol;
            ParserDefinitionClassView *detail = (ParserDefinitionClassView *)tag->class_detail;
            if ((type->flags & 0x404) != 0x404) {
                compiler_member_diagnostic(0xc9, 0x1d3, type);
            }
            if ((detail->completion_flags & 1) && !(detail->template_flags & 1) &&
                !(tag->definition_flags & 4) && !(detail->completion_flags & 4) &&
                (frontend_data.declaration_context_index < 0 ||
                 MEMBER_CLASS->name != tag->name) && !(tag->scope_flags & 4) &&
                !(frontend_data.source_mode_flags & 0x200020)) {
                CompilerGlobalSymbolView *instantiated = (CompilerGlobalSymbolView *)instantiate_declaration_class(symbol);
                if (((ParserDefinitionClassView *)instantiated->class_detail)->completion_flags & 8)
                    merge_declaration_classes((CompilerSymbol *)instantiated, symbol);
            }
            if (!(tag->definition_flags & 4)) {
                if (!suppress_parser_codegen)
                    compiler_warning(0x1b, symbol);
                return 0;
            }
            prepare_type_declaration(declaration);
            complete_member_type(type);
            return 0;
        }
        if (type->auxiliary != 0 &&
            (type->flags & 0x2fdf) != 0x12) {
            if (declaration->context.class_flags & 0x80) compiler_warning(0xd0, type);
            return 0;
        }
        ParserDeclarationView *parsed = (ParserDeclarationView *)declaration;
        if (parsed->object && parsed->object->kind == 0x12) process_using_member(parsed->object);
        else compiler_warning(0xd0, type);
        return 0;
    }
    if (symbol->kind == 0x13) { special = 1; symbol->kind = 3; }
    declaration->context.class_flags &= 0x7f;
    if (frontend_bss.declaration_scope_mode && !(frontend_data.compiler_flags & 0x20000)) {
        unsigned kind = symbol->kind;
        if (kind >= 26) { __asm { int 3 } }
        BYTE *memory = frontend_bss.arenas[0].Allocate((frontend_data.symbol_record_bytes[kind] + 3) & ~3U);
        symbol = memory ? ((CompilerSymbol *)memory)->CopyFrom(symbol, 0) : NULL;
    }
    CompilerSymbolLink *qualified = ((CompilerNamedSymbol *)symbol)->link.next;
    MemberBoundSymbol *candidate = qualified ? (MemberBoundSymbol *)((BYTE *)qualified - 8) : NULL;
    if (candidate && candidate->binding->owner->scope->entries && candidate->binding)
        bound = candidate;
    memcpy(&state.context, &declaration->context, 28);
    if (symbol->type_flags & 0x20) {
        ((CompilerNamedSymbol *)symbol)->link.name = frontend_data.builtin_names.destructor;
    } else if (is_special_member_name(((CompilerNamedSymbol *)symbol)->link.name)) {
        if (!(declarator->flags & 8)) {
            compiler_warning(0x244, symbol);
            symbol->scope_flags |= 4;
            return 0;
        }
        ((CompilerNamedSymbol *)symbol)->link.name = frontend_data.builtin_names.constructor;
        symbol->type_flags |= 0x10;
        symbol->declaration_qualifiers = MEMBER_DETAIL->scope_number;
        if (declaration_control_row(frontend_bss.parser_context_depth)->flags & 1) {
            if (!(declaration->context.class_flags & 0x10) &&
                member_class_row().type->auxiliary != declaration->type->auxiliary) {
                compiler_warning(0x215, (CompilerSymbol *)MEMBER_CLASS);
                goto merge_declaration;
            }
            memcpy(&checked_context, &declaration->context, 28);
            if ((checked_context.context_flags & 6) || (checked_context.class_flags & 7))
                compiler_warning(0x279, (CompilerSymbol *)MEMBER_CLASS);
        }
        goto merge_declaration;
    } else if (!(state.context.class_flags & 7) && !(state.context.context_flags & 4)) {
        name = ((CompilerNamedSymbol *)symbol)->link.name;
        if (name == frontend_data.builtin_names.operator_new || name == frontend_data.builtin_names.operator_delete ||
            name == frontend_data.builtin_names.operator_new_array || name == frontend_data.builtin_names.operator_delete_array) {
            state.context.class_flags = (state.context.class_flags & ~4) | 3;
            declaration->context.class_flags = (declaration->context.class_flags & ~4) | 3;
        }
    }
merge_declaration:
    if (state.context.context_flags & 4) {
        declare_qualified_member(declaration, symbol, declarator,
            (CompilerSymbol *)bound, (CompilerSymbol *)MEMBER_CLASS, state.context, special);
        return 0;
    }
    if (bound) {
        if ((declaration->context.class_flags & 0x30) == 0x30 && (declarator->flags & 4)) {
            frontend_data.translation_unit->SetScopeMode(1);
            finish_dependent_member((CompilerSymbol *)MEMBER_CLASS, (CompilerSymbol *)bound);
            return 1;
        }
        if (bound->binding->owner->scope->symbol != (CompilerSymbol *)MEMBER_CLASS) {
            compiler_warning(0x346);
            frontend_data.translation_unit->SetScopeMode(1);
            return 1;
        }
    }
    if (MEMBER_CLASS->member_flags & 0xc) {
        BYTE mode = state.context.class_flags & 7;
        if (mode == 3 || declarator->flags & 8 ||
            ((declarator->flags & 4) && declaration->type->shape && (declaration->type->shape->flags & 0x77) == 4)) {
            name = ((CompilerNamedSymbol *)symbol)->link.name;
            if (name != frontend_data.builtin_names.deleting_destructor &&
                name != frontend_data.builtin_names.vector_deleting_destructor &&
                name != frontend_data.builtin_names.local_vftable_constructor_closure)
                initialize_function_symbol(declaration, symbol);
        }
    }
    if (symbol->kind != 2) symbol->declaration_qualifiers = MEMBER_DETAIL->scope_number;
    if (bound && !bound->binding) ((CompilerNamedSymbol *)symbol)->link.next = NULL;
    symbol = merge_symbol_declaration(declaration, symbol, declarator);
    if (!symbol) return 0;
    *(DWORD *)&state.context.class_flags = MEMBER_SYMBOL->storage_class |
        ((DWORD)MEMBER_SYMBOL->symbol_flags << 8) | ((DWORD)MEMBER_SYMBOL->member_flags << 16) |
        ((DWORD)MEMBER_SYMBOL->reference_flags << 24);
    state.context.reference = NULL;
    state.context.reference_flags &= ~1;
    if (MEMBER_SYMBOL->reference_flags & 1) {
        state.context.reference = resolve_symbol_reference(symbol);
        if (state.context.reference) state.context.reference_flags |= 1;
    }
    state.context.metadata = NULL;
    state.context.context_flags &= ~0x40;
    if (MEMBER_SYMBOL->symbol_flags & 0x40) {
        state.context.metadata = get_declaration_metadata(symbol);
        if (state.context.metadata) state.context.context_flags |= 0x40;
    }
    if ((symbol->kind == 7 || symbol->kind == 8) && (MEMBER_SYMBOL->queue_flags & 0x80)) {
        CompilerFunctionContext *context = get_declaration_function_context(symbol);
        if (!context) { state.context.function_context_active = 0; state.context.function_context = NULL; }
        else { state.context.function_context_active = 1; state.context.function_context = context; }
    } else state.SetFunctionContext(NULL);
    state.context.attachment = MEMBER_SYMBOL->reference_flags & 0x80 ? get_declaration_attachment(symbol) : NULL;
    state.context.class_context = symbol->kind == 4 && (((ParserDefinitionClassView *)MEMBER_SYMBOL->class_detail)->declaration_flags & 0x80) ? 1 : 0;
    name = ((CompilerNamedSymbol *)symbol)->link.name;
    ((CompilerNamedSymbol *)symbol)->link.next = NULL;
    if (symbol->kind == 7 || symbol->kind == 8) {
        if ((symbol->type_flags & 0x20) && !(MEMBER_SYMBOL->type->shape->flags & 0x4000)) {
            compiler_warning(0x20c, (CompilerSymbol *)MEMBER_CLASS);
            return 0;
        }
        if (name->flags & 0x02000000) {
            match = frontend_data.translation_unit->LookupName(name, 0x20);
            compiler_warning(0x3a3, match ? match->symbol : NULL, (CompilerSymbol *)MEMBER_CLASS);
            return 0;
        }
        symbol->type_flags |= 4;
    } else {
        if (symbol->type_flags & 0x20) { compiler_warning(0x20c, (CompilerSymbol *)MEMBER_CLASS); return 0; }
        if (symbol->type_flags & 0x10) { compiler_warning(0x1cd, (CompilerSymbol *)MEMBER_CLASS); return 0; }
        if ((declaration->context.class_flags & 0x30) == 0x30) return 0;
        if (special) { compiler_warning(0xfc, symbol); return 0; }
        if (!(symbol->scope_flags & 1) && name->name) {
            MEMBER_DETAIL->declaration_flags |= 0x40;
            if (name->flags & 0x02000000) {
                match = frontend_data.translation_unit->LookupName(name, 0x20);
                compiler_warning(0x3a4, match ? match->symbol : NULL, (CompilerSymbol *)MEMBER_CLASS);
                return 0;
            }
            if (frontend_data.compiler_mode_flags & 0x1000000) {
                ((CompilerMemberQueue *)&frontend_bss.pending_member_queue)->Add(symbol);
                frontend_data.compiler_mode_flags &= ~0x1000000U;
            }
        }
    }
    symbol->definition_flags = (symbol->definition_flags & ~3) | (member_class_row().access_flags & 3);
    if ((member_class_row().access_flags & 3) <= 2) member_class_row().access_flags |= 4;
    function = find_declared_member(name);
    if (function) {
        if (function->kind == 15) { compiler_warning(0x56, function); return 0; }
        previous = merge_scoped_declaration(function, symbol, declarator);
        if ((MEMBER_SYMBOL->linkage_flags & 0x40) && previous != symbol) {
            *replacement = previous;
            return 0;
        }
        if ((!previous && (symbol->scope_flags & 4)) ||
            (previous && ((previous->scope_flags & 4) || (symbol->scope_flags & 4)))) return 0;
    }
    classification = state.context.class_flags & 7;
    if (classification == 7) {
        symbol->kind = 10;
        if (!previous) record_member_declaration(symbol);
        goto publish_browser;
    }
    if ((get_function_calling_convention(MEMBER_SYMBOL->type) & 0x100) &&
        symbol->kind != 7 && symbol->kind != 8 && classification != 3)
        MEMBER_DETAIL->declaration_flags |= 0x10;
    if (MEMBER_SYMBOL->reference_flags & 4) MEMBER_DETAIL->template_flags |= 0x10;
    if (symbol->kind == 2) {
        set_member_type_flags(symbol, MEMBER_SYMBOL->type->flags, previous == NULL);
    } else if (symbol->kind == 7 || symbol->kind == 8) {
        if (!function) function = prepare_function_symbol(symbol);
        if (symbol->type_flags & 0x10) {
            MEMBER_CLASS->type_flags |= 0x10;
            ((CompilerGlobalSymbolView *)function)->type_flags |= 0x10;
        } else if (symbol->type_flags & 0x20) {
            MEMBER_CLASS->type_flags |= 0x20;
            ((CompilerGlobalSymbolView *)function)->type_flags |= 0x20;
        }
        define_member_function(state.context, symbol, special);
        if (!special) record_member_declaration(function);
    } else {
        if ((MEMBER_CLASS->type->flags & 0x404) == 0x404 &&
            !check_member_type((CompilerSymbol *)MEMBER_CLASS, symbol)) return 0;
        if (MEMBER_CLASS->member_flags & 0xc) {
            enclosing = enclosing_declaration_class(symbol);
            if (enclosing && !(enclosing->member_flags & 0xc) &&
                !(((ParserDefinitionClassView *)enclosing->class_detail)->declaration_flags & 1))
                report_language_diagnostic(2, 0xfb, symbol, enclosing->type,
                    (CompilerSymbol *)enclosing, MEMBER_CLASS->type, (CompilerSymbol *)MEMBER_CLASS);
        }
        if (name != frontend_data.builtin_names.alignment_member) {
            if (MEMBER_SYMBOL->reference_flags & 0x80) MEMBER_SYMBOL->symbol_value = 0;
            else if (is_class_scope_symbol(symbol)) {
                MEMBER_SYMBOL->symbol_value = 0;
                if (frontend_bss.declaration_scope_mode) compiler_warning(0xf6, symbol);
            } else MEMBER_SYMBOL->alternate_name = (CompilerNameEntry *)make_member_storage(symbol);
        }
        if (classification == 3) register_static_member(symbol, declarator);
        if (!previous) record_member_declaration(symbol);
        if (classification != 3) {
            MEMBER_CLASS->qualifiers = (MEMBER_CLASS->qualifiers & ~1) | (symbol->qualifiers & 1);
            member_class_row().reset_18 = 0;
        }
    }
publish_browser:
    if (frontend_bss.browser_filename &&
        (!frontend_bss.declaration_scope_mode || publish_browser_local) &&
        !(MEMBER_CLASS->scope_flags & 1) &&
        ((symbol->kind != 7 && symbol->kind != 8) || !(MEMBER_SYMBOL->record_index & 0xff000000)))
        frontend_data.backend->Slot20(symbol);
    return 0;
}
#undef MEMBER_CLASS
#undef MEMBER_DETAIL
#undef MEMBER_SYMBOL

/* Genuine pending children of member publication. */
extern "C" CompilerSymbol *__cdecl find_declared_member(CompilerNameEntry *name)
{
    int index = frontend_data.declaration_context_index;
    if (index < 0 || !name || name == frontend_data.builtin_names.alignment_member ||
        !(name->flags & 0x40000)) return NULL;
    if (index >= 16) { __asm { int 3 } }
    CompilerKind4Symbol *owner = (CompilerKind4Symbol *)frontend_bss.class_scope_rows[index].symbol;
    CompilerClassBinding *binding = owner->class_binding;
    CompilerSymbolLink *entry = binding ? binding->members : NULL;
    while (entry) {
        if (entry->name == name)
            return (CompilerSymbol *)((BYTE *)entry - offsetof(CompilerNamedSymbol, link));
        entry = entry->next;
    }
    return NULL;
}
extern "C" {
void __cdecl validate_constructor_declaration(CompilerSymbol *); /*2728e*/
void __cdecl finish_member_function(CompilerSymbol *); /*27757*/
void __cdecl finalize_special_member_declaration(CompilerSymbol *, CompilerSymbol *, DWORD); /*45e67*/
CompilerType *__cdecl map_function_parameter_symbols(CompilerType *, void (__cdecl *)(CompilerSymbol *)); /*3643f*/
void __cdecl mark_function_parameter_symbol(CompilerSymbol *); /*36546*/
void __cdecl record_local_member_function(CompilerSymbol *, CompilerSymbol *); /*3e9f7*/
void __cdecl classify_assignment_operator(CompilerSymbol *); /*2b428*/
void __cdecl bind_parameter_declarations(CompilerParameterLink *, unsigned, CompilerFunctionType *, unsigned, CompilerSymbol *);
void __cdecl validate_function_declaration(CompilerSymbol *, unsigned);
int __cdecl can_inherit_special_member(CompilerSymbolLink *, CompilerSymbol *, unsigned);
}
extern "C" void __cdecl define_member_function(CompilerDeclarationContext context,
    CompilerSymbol *input, unsigned special)
{
    CompilerRuntimeFunction *symbol = (CompilerRuntimeFunction *)input;
    CompilerNameEntry *name;
    CompilerType *result;
    symbol->definition_flags = (symbol->definition_flags & ~3) | (member_class_row().access_flags & 3);
    if (!(symbol->encoded_index & 0xffffff)) {
        if (frontend_data.next_symbol_record_id >= 0x7fffffff) report_compiler_error(0x37);
        unsigned index = frontend_data.next_symbol_record_id++;
        symbol->encoded_index = (symbol->encoded_index & 0xff000000) | (index & 0xffffff);
    }
    symbol->type_flags |= 4;
    symbol->declaration_qualifiers = member_class_row().symbol->class_detail->scope_number;
    if (symbol->type_flags & 0x40) member_class_row().symbol->class_detail->template_flags |= 2;
    name = symbol->link.name;
    if (name == frontend_data.builtin_names.operator_new ||
        name == frontend_data.builtin_names.operator_delete ||
        name == frontend_data.builtin_names.operator_new_array ||
        name == frontend_data.builtin_names.operator_delete_array ||
        (context.class_flags & 7) == 3) {
        symbol->builtin_flags &= ~1;
        symbol->type = map_function_parameter_symbols(symbol->type, mark_function_parameter_symbol);
    } else symbol->builtin_flags |= 1;

    if (symbol->type_flags & 0x10) {
        validate_constructor_declaration(input);
        result = symbol->type->shape->payload.function->type;
        if (result->attributes & 0x100) compiler_warning(0x247, input, 0x2f);
        result = symbol->type->shape->payload.function->type;
        if (result->attributes & 0x200) compiler_warning(0x247, input, 0x30);
    } else if (symbol->type_flags & 0x20) {
        result = symbol->type->shape->payload.function->type;
        if (result->attributes & 0x100) compiler_warning(0x247, input, 0x2f);
        result = symbol->type->shape->payload.function->type;
        if (result->attributes & 0x200) compiler_warning(0x247, input, 0x30);
        if ((context.context_flags & 2) && !(member_class_row().access_flags & 8))
            compiler_warning(0x23b, (CompilerSymbol *)member_class_row().symbol, input);
    } else {
        if (symbol->link.name == frontend_data.builtin_names.assignment_operator) {
            if ((context.class_flags & 7) == 3)
                compiler_warning(0x245, (CompilerSymbol *)member_class_row().symbol);
            else classify_assignment_operator(input);
        }
        if ((context.context_flags & 2) && !(member_class_row().access_flags & 8))
            compiler_warning(0x23b, (CompilerSymbol *)member_class_row().symbol, input);
    }
    if (symbol->function_flags & 2) symbol->link.name->flags |= 0x100000;
    if ((symbol->builtin_flags & 1) &&
        can_inherit_special_member(&symbol->link, (CompilerSymbol *)member_class_row().symbol, 1)) {
        symbol->function_flags |= 2;
        symbol->link.name->flags |= 0x100000;
    }
    if (special)
        finalize_special_member_declaration(input, (CompilerSymbol *)member_class_row().symbol, symbol->declaration_state);
    validate_function_declaration(input, 0);
    finish_member_function(input);
    if (!(frontend_data.compiler_flags & 0x4000000)) {
        process_merged_declaration(input, (symbol->function_flags >> 2) & 1);
    } else if (!(member_class_row().access_flags & 0x10) && frontend_bss.current_function &&
               !(symbol->linkage_flags & 0x10)) {
        symbol->builtin_flags &= ~8;
        record_local_member_function((CompilerSymbol *)member_class_row().symbol, input);
    }
}
extern "C" void __cdecl validate_constructor_declaration(CompilerSymbol *input)
{
    CompilerRuntimeFunction *symbol = (CompilerRuntimeFunction *)input;
    CompilerTypeShape *shape = symbol->type->shape;
    CompilerFunctionType *function;
    CompilerParameterTypes *parameters;
    CompilerType *type;
    unsigned count;
    if (shape->flags & 0x4000) {
        symbol->builtin_flags |= 0x10;
        return;
    }
    function = shape->payload.function;
    parameters = function->parameters;
    if ((shape->flags & 0x8000) && parameters->count == 1) {
        symbol->builtin_flags |= 0x30;
        return;
    }
    count = parameters->count - function->symbol_count;
    if (!count) symbol->builtin_flags |= 0x10;
    else if (count != 1) return;
    type = parameters->types[0];
    if (!(type->flags & 0x400) || type->auxiliary != member_class_row().type->auxiliary) {
        symbol->builtin_flags |= 0x40;
        frontend_data.source_mode_flags |= 0x80;
        return;
    }
    shape = type->shape;
    if (!shape) {
        CompilerSymbol *owner = (CompilerSymbol *)frontend_data.frontend_pointer_table->items[(short)type->auxiliary];
        compiler_warning(0x28c, owner, owner);
        symbol->scope_flags |= 4;
        symbol->builtin_flags |= 0x20;
        return;
    }
    if ((shape->flags & 0x40) && !shape->base) {
        symbol->builtin_flags |= 0x20;
        symbol->queue_flags = (symbol->queue_flags & 0x9f) | ((type->attributes & 0x300) >> 3);
    }
}
extern "C" void __cdecl finish_member_function(CompilerSymbol *input)
{
    if (!input || (input->kind != 7 && input->kind != 8)) return;
    CompilerType *type = ((CompilerIrSymbol *)input)->type;
    if (!type || !type->shape) return;
    CompilerFunctionType *function = type->shape->payload.function;
    unsigned count = function->symbol_count;
    bind_parameter_declarations(function->symbols, count, function,
        function->parameters->count - count, input);
}
extern "C" void __cdecl finalize_special_member_declaration(CompilerSymbol *, CompilerSymbol *, DWORD) { __asm { int 3 } }
extern "C" CompilerType *__cdecl map_function_parameter_symbols(CompilerType *, void (__cdecl *)(CompilerSymbol *)) { __asm { int 3 } return NULL; }
extern "C" void __cdecl mark_function_parameter_symbol(CompilerSymbol *) { __asm { int 3 } }
extern "C" void __cdecl record_local_member_function(CompilerSymbol *, CompilerSymbol *) { __asm { int 3 } }
extern "C" CompilerTypeShape *__cdecl strip_declaration_shape(CompilerTypeShape *);
extern "C" void __cdecl classify_assignment_operator(CompilerSymbol *input)
{
    CompilerRuntimeFunction *symbol = (CompilerRuntimeFunction *)input;
    if (symbol->link.name != frontend_data.builtin_names.assignment_operator) return;
    CompilerTypeShape *shape = symbol->type->shape;
    CompilerFunctionType *function = shape->payload.function;
    if ((shape->flags & 0x4000) || function->parameters->count != 1 || function->symbol_count) return;
    CompilerType *parameter = function->parameters->types[0];
    if (!(parameter->flags & 0x400) || parameter->auxiliary != member_class_row().type->auxiliary) return;
    if (strip_declaration_shape(parameter->shape)) return;
    symbol->builtin_flags |= 0x80;
    symbol->queue_flags = (symbol->queue_flags & 0x9f) | ((parameter->attributes & 0x300) >> 3);
}
extern "C" void __cdecl record_member_declaration(CompilerSymbol *symbol)
{
    CompilerSymbolLink *entry = &((CompilerNamedSymbol *)symbol)->link;
    if (entry->name) entry->name->flags |= 0x40000;
    CompilerKind4Symbol *owner = (CompilerKind4Symbol *)member_class_row().symbol;
    CompilerClassBinding *binding = owner->class_binding;
    entry->next = NULL;
    if (binding->last_member) binding->last_member->next = entry;
    else binding->members = entry;
    binding->last_member = entry;
}
extern "C" void __cdecl register_static_member(CompilerSymbol *, CompilerDeclarator *) { __asm { int 3 } }
extern "C" CompilerGlobalSymbolView *__cdecl enclosing_declaration_class(CompilerSymbol *symbol)
{
    CompilerType *type = ((CompilerIrSymbol *)symbol)->type;
    if (!type || !(type->flags & 0x400)) return NULL;
    if (!(symbol->definition_flags & 0x80) && type->shape) return NULL;
    int index = (short)type->auxiliary;
    if (index < 0) return NULL;
    CompilerPointerArray *table = frontend_data.frontend_pointer_table;
    if (index >= table->count) { __asm { int 3 } }
    return (CompilerGlobalSymbolView *)table->items[index];
}
extern "C" int __cdecl check_member_type(CompilerSymbol *, CompilerSymbol *) { __asm { int 3 } return 0; }
extern "C" void __cdecl set_member_type_flags(CompilerSymbol *, unsigned, unsigned) { __asm { int 3 } }
extern "C" void __cdecl initialize_function_symbol(CompilerDeclaration *, CompilerSymbol *) { __asm { int 3 } }
extern "C" void __cdecl finish_dependent_member(CompilerSymbol *, CompilerSymbol *) { __asm { int 3 } }
extern "C" void __cdecl declare_qualified_member(CompilerDeclaration *, CompilerSymbol *, CompilerDeclarator *,
    CompilerSymbol *, CompilerSymbol *, CompilerDeclarationContext, unsigned) { __asm { int 3 } }
extern "C" void __cdecl prepare_type_declaration(CompilerDeclaration *) { __asm { int 3 } }
extern "C" void __cdecl complete_member_type(CompilerType *) { __asm { int 3 } }
extern "C" void __cdecl declare_friend_type(CompilerSymbol *, CompilerDeclaration *, unsigned) { __asm { int 3 } }
extern "C" void __cdecl compiler_member_diagnostic(unsigned, ...) { __asm { int 3 } }
void RECON_THISCALL CompilerMemberQueue::Add(CompilerSymbol *) { __asm { int 3 } }
CompilerSymbol *RECON_THISCALL CompilerSymbol::CopyFrom(CompilerSymbol *, unsigned) { __asm { int 3 } return NULL; }

extern "C" {
unsigned __cdecl global_type_size(CompilerType *);
void __cdecl apply_member_alignment(unsigned); /*2678f*/
void __cdecl finish_member_layout(void); /*453a3*/
int __cdecl validate_member_offset(__int64, __int64, WORD, WORD); /*23478*/
void *__cdecl make_member_location(unsigned, unsigned); /*26806*/
}
#define LAYOUT_CLASS (member_class_row().symbol)
#define LAYOUT_DETAIL ((ParserDefinitionClassView *)LAYOUT_CLASS->class_detail)
extern "C" void *__cdecl make_member_storage(CompilerSymbol *symbol)
{
    CompilerGlobalSymbolView *member = (CompilerGlobalSymbolView *)symbol;
    CompilerGlobalSymbolView *owner;
    CompilerType *type;
    unsigned bytes, alignment, offset;
    int index;
    if (frontend_data.source_mode_flags & 0x100000) return NULL;
    if ((member_class_row().access_flags & 8) && (LAYOUT_CLASS->qualifiers & 1)) {
        compiler_warning(0xe5, LAYOUT_CLASS->type, (CompilerSymbol *)LAYOUT_CLASS);
        LAYOUT_CLASS->qualifiers &= ~1;
    }
    if (symbol->kind == 4)
        bytes = ((ParserDefinitionClassView *)member->class_detail)->member_size;
    else {
        bytes = global_type_size(member->type);
        if (!bytes) {
            type = member->type;
            index = (short)type->auxiliary;
            if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
            owner = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[index];
            if (owner && !(owner->definition_flags & 4) &&
                (!type->shape || (type->shape->flags & 0x77) != 2 || type->shape->payload.value)) {
                compiler_warning(0x4f, symbol, type, (CompilerSymbol *)owner);
                return NULL;
            }
            if (!frontend_data.options.language_extensions && !(frontend_data.compiler_mode_flags & 0x20000)) {
                compiler_warning(0x85, symbol);
                return NULL;
            }
            report_language_diagnostic(2, 0xc8);
            bytes = global_type_size(get_function_return_type(member->type));
            symbol->qualifiers |= 1;
        }
    }
    if ((member_class_row().access_flags & 8) && member_class_row().reset_18) {
        finish_member_layout();
        member_class_row().access_flags &= ~0x20;
        member_class_row().reset_1c = 0;
    }
    type = member->type;
    while (type->shape && (type->shape->flags & 0x77) == 2)
        type = get_function_return_type(type);
    if (!type->shape && (type->flags & 0x140)) {
        unsigned flags = type->flags;
        if (flags & 0x340) alignment = compiler_primitive_type_bytes[flags & 15];
        else {
            /* The original retains this alternate encoding branch. */
            alignment = flags;
            if (alignment & ~15U) alignment &= 15;
            else alignment = compiler_primitive_type_bytes[alignment];
        }
        if (alignment >= frontend_data.options.packing_stack->top->alignment)
            alignment = global_type_size(type);
    } else if (!type->shape && (type->flags & 0x400)) {
        index = (short)type->auxiliary;
        if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
        owner = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[index];
        alignment = ((ParserDefinitionClassView *)owner->class_detail)->member_phase_flags & 0x3f;
    } else alignment = global_type_size(type);
    apply_member_alignment(alignment);
    if (member_class_row().access_flags & 8) {
        offset = member_class_row().member_bytes;
        if (!(symbol->qualifiers & 1)) {
            if (bytes && validate_member_offset((__int64)(LONG)offset,
                    (__int64)(bytes - 1), 1, (WORD)compiler_primitive_type_bytes[3])) {
                compiler_warning(0x59, (CompilerSymbol *)LAYOUT_CLASS, LAYOUT_CLASS->type);
                return NULL;
            }
            member_class_row().member_bytes += bytes;
        }
    } else {
        offset = 0;
        if (member_class_row().member_bytes < bytes) member_class_row().member_bytes = bytes;
    }
    type = member->type;
    if (!type->shape && (type->flags & 0x400) && symbol->kind != 4) {
        index = (short)type->auxiliary;
        if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
        owner = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[index];
        symbol->qualifiers = (symbol->qualifiers & ~1) | (owner->qualifiers & 1);
        index = (short)type->auxiliary;
        if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
        owner = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[index];
        LAYOUT_CLASS->qualifiers |= owner->qualifiers & 2;
        type = member->type;
        int has_class_layout = (type->flags & 0x404) == 0x404;
        if (!has_class_layout) {
            index = (short)type->auxiliary;
            if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
            owner = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[index];
            has_class_layout = (((ParserDefinitionClassView *)owner->class_detail)->context_flags & 1) != 0;
        }
        LAYOUT_DETAIL->context_flags |= has_class_layout ? 1 : 0;
    }
    return make_member_location(offset, 0);
}
#undef LAYOUT_CLASS
#undef LAYOUT_DETAIL
extern "C" unsigned __cdecl member_alignment_padding(unsigned);
extern "C" void __cdecl insert_alignment_member(unsigned);
extern "C" void __cdecl apply_member_alignment(unsigned requested)
{
    unsigned packing = frontend_data.options.packing_stack->top->alignment;
    unsigned capped = requested > packing ? packing : requested;
    DeclarationClassScope &row = member_class_row();
    if (row.alignment <= (WORD)capped) row.alignment = (WORD)capped;
    unsigned padding = member_alignment_padding(requested);
    if (!padding) return;
    packing = frontend_data.options.packing_stack->top->alignment;
    if (packing < requested)
        report_language_diagnostic(4, 0x79, (CompilerSymbol *)member_class_row().symbol);
    insert_alignment_member(padding);
}
extern "C" unsigned __cdecl calculate_alignment_padding(unsigned, unsigned);
extern "C" unsigned __cdecl member_alignment_padding(unsigned alignment)
{
    return calculate_alignment_padding(alignment, member_class_row().member_bytes);
}
extern "C" unsigned __cdecl calculate_alignment_padding(unsigned requested, unsigned position)
{
    unsigned packing = frontend_data.options.packing_stack->top->alignment;
    unsigned alignment = requested > packing ? packing : requested;
    unsigned remainder = position % alignment;
    return remainder ? alignment - remainder : 0;
}
extern "C" void __cdecl insert_alignment_member(unsigned) { __asm { int 3 } }
extern "C" void __cdecl finish_member_layout(void) { __asm { int 3 } }
extern "C" int __cdecl validate_member_offset(__int64 start, __int64 span,
    WORD unsigned_range, WORD bytes)
{
    unsigned __int64 ones = (unsigned __int64)-1;
    /* Microsoft integer helpers consume CL, not the full count DWORD.
     * Explicit clipping also avoids undefined C shifts at counts>=64. */
    unsigned bound_shift = (bytes * 8U - 1U) & 0xff;
    unsigned __int64 lower = bound_shift < 64 ? ones << bound_shift : 0;
    unsigned __int64 left = (unsigned __int64)start;
    unsigned __int64 right = (unsigned __int64)span;
    if (unsigned_range) {
        unsigned shift = ((8U - bytes) * 8U) & 0xff;
        unsigned __int64 maximum = ~lower;
        maximum |= maximum << 1;
        if (shift < 64) {
            left = (left << shift) >> shift;
            right = (right << shift) >> shift;
        } else left = right = 0;
        return right > maximum - left;
    }
    if ((start >= 0) != (span >= 0)) return 0;
    if (start > 0) {
        __int64 remaining = (__int64)(~lower - left);
        return span > remaining;
    }
    __int64 remaining = (__int64)(lower - left);
    return span < remaining;
}
extern "C" void *__cdecl make_member_location(unsigned offset, unsigned base)
{
    return (void *)(offset + base);
}

CompilerDeclarator *RECON_THISCALL CompilerDeclarator::Finish(unsigned)
{
    return this;
}

extern "C" {
void __cdecl prepare_class_members(CompilerSymbol *); /*269a6*/
void __cdecl analyze_class_special_members(CompilerSymbol *); /*26c6f*/
void __cdecl finalize_class_functions(CompilerSymbol *); /*27c62*/
void __cdecl finalize_class_offsets(CompilerSymbol *); /*284f8*/
void __cdecl finalize_class_layout(CompilerSymbol *); /*28738*/
void __cdecl finalize_class_metadata(CompilerSymbol *); /*2879f*/
void __cdecl prepare_specialized_class(CompilerSymbol *); /*36859*/
}
extern "C" CompilerType *__cdecl finish_class_specifier(void)
{
    DeclarationClassScope &row = member_class_row();
    row.access_flags |= 3;
    CompilerGlobalSymbolView *symbol = row.symbol;
    CompilerType *type = row.type;
    if (!(symbol->scope_flags & 1) && frontend_bss.browser_filename &&
        (!frontend_bss.declaration_scope_mode || !publish_browser_local))
        frontend_data.backend->Slot08();
    if (!frontend_bss.declaration_depth && !(frontend_data.compiler_mode_flags & 0x20000000)) {
        ParserDefinitionClassView *detail = (ParserDefinitionClassView *)symbol->class_detail;
        if (detail->template_flags & 6) prepare_specialized_class((CompilerSymbol *)symbol);
        if (!(frontend_data.source_mode_flags & 0x100000)) {
            frontend_data.compiler_flags |= 0x04000000;
            prepare_class_members((CompilerSymbol *)symbol);
            analyze_class_special_members((CompilerSymbol *)symbol);
            finalize_class_functions((CompilerSymbol *)symbol);
            finalize_class_offsets((CompilerSymbol *)symbol);
            finalize_class_layout((CompilerSymbol *)symbol);
            frontend_data.compiler_flags &= ~0x04000000U;
        }
    }
    ParserDefinitionClassView *detail = (ParserDefinitionClassView *)symbol->class_detail;
    detail->member_phase_flags = (detail->member_phase_flags & ~0x3f) | (member_class_row().alignment & 0x3f);
    symbol->definition_flags |= 4;
    --frontend_data.declaration_context_index;
    detail->completion_flags &= ~4;
    frontend_bss.tag_declaration_changed = 1;
    frontend_data.translation_unit->RestoreSavedLookup(0);
    frontend_data.translation_unit->PopSavedLookup();
    finalize_class_metadata((CompilerSymbol *)symbol);
    unsigned recorded = 0;
    if (((CompilerRecordedLexer *)frontend_bss.saved_context_stack.top->object)->active) {
        detail = (ParserDefinitionClassView *)symbol->class_detail;
        if (!frontend_bss.debug_module->Slot3c(frontend_bss.saved_context_stack.position,
                (unsigned)detail->declaration_reference, &recorded))
            report_compiler_diagnostic(0x49, ".\\esu.c", 0x41d);
    }
    if (frontend_data.options.incremental_rebuild && recorded &&
        !frontend_data.dependency->CompleteClass((CompilerSymbol *)symbol))
        compiler_warning(0x1d7, frontend_data.options.database_path);
    return type;
}
extern "C" void __cdecl initialize_class_member_lists(CompilerSymbol *); /*269bf*/
extern "C" void __cdecl classify_class_members(CompilerSymbol *); /*26a17*/
extern "C" void __cdecl prepare_class_members(CompilerSymbol *symbol)
{
    initialize_class_member_lists(symbol);
    classify_class_members(symbol);
}
extern "C" CompilerVector *__cdecl new_option_vector(unsigned, int);
extern "C" void __cdecl initialize_class_member_lists(CompilerSymbol *input)
{
    CompilerGlobalSymbolView *symbol = (CompilerGlobalSymbolView *)input;
    symbol->class_detail->member_phase_flags &= 0x7f;
    symbol->class_detail->member_phase_flags &= 0xbf;
    for (unsigned group = 0; group != 2; ++group) {
        CompilerVector *members = new_option_vector(0, 5);
        symbol->class_detail->member_groups[group] = members;
    }
    symbol->class_detail->member_list_state = 0;
    CompilerVector *members = new_option_vector(0, 5);
    symbol->class_detail->additional_members = members;
}
extern "C" {
void __cdecl classify_inherited_member(CompilerSymbol *, CompilerClassMemberSymbol *, unsigned *); /*2a9cc*/
CompilerSymbol *__cdecl create_special_member(CompilerSymbol *, unsigned); /*44b23*/
int __cdecl can_inherit_special_member(CompilerSymbolLink *, CompilerSymbol *, unsigned); /*27349*/
CompilerSymbol *__cdecl make_special_member_declaration(unsigned, CompilerSymbol *, unsigned, CompilerSymbol *); /*44ed0*/
CompilerSymbol *__cdecl finish_special_member_declaration(CompilerSymbol *, CompilerSymbol *); /*4b973*/
void __cdecl complete_class_member_layout(CompilerSymbol *); /*26c14*/
void __cdecl append_option_vector(CompilerVector *, void *);
}
extern "C" void __cdecl classify_class_members(CompilerSymbol *input)
{
    CompilerKind4Symbol *owner = (CompilerKind4Symbol *)input;
    CompilerGlobalSymbolView *symbol = (CompilerGlobalSymbolView *)input;
    unsigned inherited[3] = {0, 0, 0};
    CompilerClassMemberIterator members;
    members.InitializeHead(owner->class_binding ? owner->class_binding->members : NULL);
    members.owner = &owner->link;
    members.reset_value = 0;
    members.current = members.valid ? members.chain : NULL;
    members.mode = 4;
    while (members.current) {
        members.entry = members.current;
        if (members.chain) members.chain = members.chain->next;
        members.current = members.chain;
        CompilerSymbol *member = (CompilerSymbol *)((BYTE *)members.entry - 8);
        if (member->kind == 3 && (members.entry->name == frontend_data.builtin_names.vbptr ||
                members.entry->name == frontend_data.builtin_names.vfptr)) continue;
        if ((frontend_data.source_mode_flags & 0x100000) && member->kind == 15 &&
            (((CompilerClassMemberSymbol *)member)->flags & 2)) continue;
        if (member->kind != 15) break;
        classify_inherited_member(input, (CompilerClassMemberSymbol *)member, inherited);
    }
    CompilerSymbol *selected = input;
    unsigned generate_selected = inherited[1];
    if (!(symbol->class_detail->member_phase_flags & 0x80) && inherited[0]) {
        selected = create_special_member(input, 1);
        generate_selected = 1;
    }
    unsigned generate_default = inherited[2];
    CompilerSymbol *default_member = input;
    if (!(symbol->class_detail->member_phase_flags & 0x40)) {
        CompilerClassMemberIterator declarations;
        declarations.Initialize(&owner->link, 2);
        CompilerMemberVariantIterator variants(NULL);
        for (;;) {
            if (variants.Advance()) {
                CompilerSymbol *member = (CompilerSymbol *)((BYTE *)variants.current - 8);
                if (member->kind == 24) member = ((CompilerAliasSymbol *)member)->target->symbol;
                if (((CompilerIrSymbol *)member)->function_flags & 2) {
                    if (!can_inherit_special_member(variants.current, input, 0)) {
                        default_member = create_special_member(input, 0);
                        generate_default = 1;
                        break;
                    }
                }
            } else {
                if (!declarations.Next()) break;
                variants.SetLink(declarations.entry);
            }
        }
    }
    if (generate_default) {
        CompilerSymbol *member = make_special_member_declaration(0, input, 0, default_member);
        append_option_vector(symbol->class_detail->member_groups[0], member);
    }
    if (generate_selected) {
        CompilerSymbol *member = finish_special_member_declaration(input, selected);
        append_option_vector(symbol->class_detail->member_groups[1], member);
    }
    complete_class_member_layout(input);
}
extern "C" void __cdecl classify_inherited_member(CompilerSymbol *, CompilerClassMemberSymbol *, unsigned *) { __asm { int 3 } }
extern "C" CompilerSymbol *__cdecl create_special_member(CompilerSymbol *, unsigned) { __asm { int 3 } return NULL; }
/* Borrowed group views on the inherited-member path. The untouched header
 * and record tails are storage only, with no default or allocation claim. */
struct InheritedMemberCursor {
    CompilerVector *vector;
    short index;
    void *current;
};
typedef char ClassMemberFunctionTable_offsets[(
    offsetof(ClassMemberFunctionTable, count) == 0x10 &&
    offsetof(ClassMemberFunctionTable, capacity) == 0x14 &&
    offsetof(ClassMemberFunctionTable, records) == 0x18 &&
    offsetof(ClassMemberFunctionTable, storage_bytes) == 0x1c &&
    offsetof(ClassMemberFunctionTable, merge_value) == 0x30 &&
    offsetof(ClassMemberFunctionTable, merge_targets) == 0x34 &&
    offsetof(ClassMemberFunctionTable, function) == 4 &&
    offsetof(ClassMemberFunctionTable, secondary_function) == 0x3c &&
    offsetof(ClassMemberFunctionTable, secondary_function_state) == 0x40 &&
    sizeof(InheritedMemberRecord) == 16 &&
    offsetof(InheritedMemberRecord, flags) == 12 &&
    offsetof(InheritedMemberGroup, functions) == 0x14 &&
    offsetof(InheritedMemberGroup, secondary_state) == 0x20) ? 1 : -1];
extern "C" int __cdecl next_inherited_member_group(InheritedMemberCursor *); /*27ce0*/
extern "C" int __cdecl compare_inherited_member_signatures(CompilerSymbolLink *,
    CompilerSymbolLink *, CompilerSymbol *, unsigned); /*3671e*/
extern "C" int __cdecl can_inherit_special_member(CompilerSymbolLink *input,
    CompilerSymbol *owner, unsigned record)
{
    CompilerClassMemberIterator members;
    CompilerClassMemberSymbol *base;
    CompilerGlobalSymbolView *base_owner;
    InheritedMemberCursor primary, additional;
    unsigned index;
    int found = 0;
    if (!(input->name->flags & 0x100000)) return 0;
    members.InitializeBinding(((CompilerKind4Symbol *)owner)->class_binding);
    members.owner = &((CompilerNamedSymbol *)owner)->link;
    members.entry = NULL;
    members.current = members.valid ? members.chain : NULL;
    members.mode = 4;
    while (members.current) {
        members.entry = members.current;
        if (members.chain) members.chain = members.chain->next;
        members.current = members.chain;
        CompilerSymbol *member = (CompilerSymbol *)((BYTE *)members.entry - 8);
        if (member->kind == 3 && (members.entry->name == frontend_data.builtin_names.vbptr ||
            members.entry->name == frontend_data.builtin_names.vfptr)) continue;
        if ((frontend_data.source_mode_flags & 0x100000) && member->kind == 15 &&
            (((CompilerClassMemberSymbol *)member)->flags & 2)) continue;
        if (member->kind != 15) break;
        base = (CompilerClassMemberSymbol *)member;
        base_owner = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[base->owner_index];
        primary.vector = base_owner->class_detail->member_groups[0];
        primary.index = 0;
        additional.vector = base_owner->class_detail->additional_members;
        additional.index = 0;
        for (;;) {
            if (!next_inherited_member_group(&primary)) {
                do {
                    if (!next_inherited_member_group(&additional)) goto next_base;
                    primary.vector = ((InheritedMemberGroup *)additional.current)->functions[0];
                    primary.index = 0;
                } while (!next_inherited_member_group(&primary));
            }
            ClassMemberFunctionTable *group = (ClassMemberFunctionTable *)primary.current;
            for (index = 0; (int)index < group->count; ++index) {
                CompilerSymbolLink *candidate = group->records[index].member;
                if (input->name != candidate->name) continue;
                if (compare_inherited_member_signatures(input, candidate, owner, record)) {
                    if (!record) return 1;
                    found = 1;
                }
            }
        }
next_base:;
    }
    return found;
}
CompilerClassMemberIterator *RECON_THISCALL CompilerClassMemberIterator::InitializeBinding(CompilerClassBinding *binding)
{
    chain = binding ? binding->members : NULL;
    valid = chain != NULL;
    return this;
}
extern "C" int __cdecl next_inherited_member_group(InheritedMemberCursor *cursor)
{
    if (!cursor->vector || cursor->index >= cursor->vector->count) return 0;
    cursor->current = cursor->vector->items[cursor->index++];
    return 1;
}
extern "C" int __cdecl compare_inherited_member_signatures(CompilerSymbolLink *, CompilerSymbolLink *, CompilerSymbol *, unsigned) { __asm { int 3 } return 0; }
extern "C" CompilerSymbol *__cdecl make_special_member_declaration(unsigned, CompilerSymbol *, unsigned, CompilerSymbol *) { __asm { int 3 } return NULL; }
extern "C" CompilerSymbol *__cdecl finish_special_member_declaration(CompilerSymbol *, CompilerSymbol *) { __asm { int 3 } return NULL; }
extern "C" void __cdecl complete_class_member_layout(CompilerSymbol *input)
{
    if (member_class_row().reset_18 && member_class_row().reset_1c)
        finish_member_layout();
    apply_member_alignment(member_class_row().alignment);
    ((CompilerGlobalSymbolView *)input)->class_detail->member_size = member_class_row().member_bytes;
}
/* Five original28-byte local records. Only the directly initialized/read
 * control fields are named; the opaque intervals have storage, not semantics.
 * This owner is actually zeroed in full by the original35-DWORD writer. */
struct CompilerSpecialMemberTraits {
    DWORD initial_flags, condition;
    DWORD declared, seen; /* +8/+c, explicit special-member selection */
    DWORD secondary_flags;
    DWORD duplicate_reported; /* +14 */
    BYTE active;
    BYTE unmodeled_19_1b[3];
};
typedef char CompilerSpecialMemberTraits_layout[(sizeof(CompilerSpecialMemberTraits) == 28 &&
    offsetof(CompilerSpecialMemberTraits, condition) == 4 &&
    offsetof(CompilerSpecialMemberTraits, secondary_flags) == 0x10 &&
    offsetof(CompilerSpecialMemberTraits, active) == 0x18) ? 1 : -1];
extern "C" {
void __cdecl collect_class_member_traits(CompilerSymbol *, CompilerSpecialMemberTraits *); /*26e22*/
void __cdecl collect_data_member_traits(CompilerSymbol *, CompilerSpecialMemberTraits *); /*2719e*/
void __cdecl select_class_special_members(CompilerSymbol *, CompilerSpecialMemberTraits *, int *, unsigned *); /*27553*/
void __cdecl complete_class_special_member(CompilerSymbol *, CompilerSpecialMemberTraits *, unsigned, unsigned, unsigned, unsigned); /*277df*/
}
extern "C" void __cdecl analyze_class_special_members(CompilerSymbol *input)
{
    CompilerSpecialMemberTraits traits[5];
    int first_requirement;
    unsigned second_requirement;
    memset(traits, 0, sizeof(traits));
    for (unsigned kind = 1; kind != 5; ++kind) {
        traits[kind].initial_flags = 1;
        traits[kind].active = 1;
        traits[kind].secondary_flags = 1;
    }
    collect_class_member_traits(input, traits);
    collect_data_member_traits(input, traits);
    select_class_special_members(input, traits, &first_requirement, &second_requirement);
    CompilerGlobalSymbolView *symbol = (CompilerGlobalSymbolView *)input;
    ParserDefinitionClassView *detail = (ParserDefinitionClassView *)symbol->class_detail;
    if (detail->member_phase_flags & 0xc0) {
        traits[1].condition = traits[2].condition = traits[3].condition = 1;
        detail->declaration_flags &= ~1;
    }
    unsigned plain = !(first_requirement > 0 || traits[1].condition || traits[2].condition ||
        (member_class_row().access_flags & 4));
    symbol->class_detail->declaration_flags &= (BYTE)(plain | 0xfe);
    complete_class_special_member(input, traits, 1, 0x1fe, first_requirement == 0, 0);
    complete_class_special_member(input, traits, 2, 0x1ff, 1, (unsigned)first_requirement);
    complete_class_special_member(input, traits, 3, 0x200, 1, second_requirement);
    complete_class_special_member(input, traits, 4, 0x201, 1, 0);
    detail = (ParserDefinitionClassView *)symbol->class_detail;
    if (!first_requirement && !(detail->generated_member_flags & 2) && !(detail->inherited_member_flags & 2)) {
        report_language_diagnostic(4, 0x262, symbol->type, input);
        return;
    }
    if ((detail->completion_flags & 1) && !(detail->template_flags & 1) &&
        !(symbol->definition_flags & 4) && !(detail->completion_flags & 4) &&
        (frontend_data.declaration_context_index < 0 || member_class_row().symbol->name != symbol->name) &&
        !(symbol->scope_flags & 4) && !(frontend_data.source_mode_flags & 0x200020)) {
        CompilerGlobalSymbolView *instantiated = (CompilerGlobalSymbolView *)instantiate_declaration_class(input);
        if (((ParserDefinitionClassView *)instantiated->class_detail)->completion_flags & 8)
            merge_declaration_classes((CompilerSymbol *)instantiated, input);
    }
    detail = (ParserDefinitionClassView *)symbol->class_detail;
    if (!(detail->generated_member_flags & 0x10) && !(detail->declared_member_flags & 0x10))
        report_language_diagnostic(4, 0x20f, symbol->type, input);
}
extern "C" int __cdecl collect_inherited_class_traits(CompilerSymbol *, CompilerSymbol *, CompilerSpecialMemberTraits *); /*2791b*/
extern "C" void __cdecl collect_class_member_traits(CompilerSymbol *input, CompilerSpecialMemberTraits *traits)
{
    CompilerKind4Symbol *symbol = (CompilerKind4Symbol *)input;
    CompilerSymbolLink *chain = symbol->class_binding ? symbol->class_binding->members : NULL;
    CompilerSymbolLink *current = chain;
    int collected = 0;
    while (current) {
        CompilerSymbolLink *link = current;
        chain = chain ? chain->next : NULL;
        current = chain;
        CompilerSymbol *member = (CompilerSymbol *)((BYTE *)link - 8);
        if (member->kind == 3 && (link->name == frontend_data.builtin_names.vbptr ||
                link->name == frontend_data.builtin_names.vfptr)) continue;
        if ((frontend_data.source_mode_flags & 0x100000) && member->kind == 15 &&
            (((CompilerClassMemberSymbol *)member)->flags & 2)) continue;
        if (member->kind != 15) return;
        int index = ((CompilerClassMemberSymbol *)member)->owner_index;
        CompilerPointerArray *table = frontend_data.frontend_pointer_table;
        if (index < 0 || index >= table->count) { __asm { int 3 } }
        collected += collect_inherited_class_traits(input, (CompilerSymbol *)table->items[index], traits);
        ((CompilerGlobalSymbolView *)input)->class_detail->declaration_flags &= ~1;
    }
}
extern "C" int __cdecl collect_inherited_special_member(CompilerSymbol *, CompilerSymbol *, CompilerSpecialMemberTraits *, unsigned);
extern "C" int __cdecl collect_inherited_class_traits(CompilerSymbol *input,
    CompilerSymbol *inherited, CompilerSpecialMemberTraits *traits)
{
    int count = 0;
    for (unsigned kind = 1; kind <= 4; ++kind) {
        if (!(((CompilerGlobalSymbolView *)inherited)->class_detail->generated_member_flags & (1U << kind)))
            count += collect_inherited_special_member(input, inherited, traits, kind);
    }
    return count;
}
extern "C" int __cdecl collect_inherited_special_member(CompilerSymbol *, CompilerSymbol *, CompilerSpecialMemberTraits *, unsigned) { __asm { int 3 } return 0; }

extern "C" int __cdecl member_requires_initialization(CompilerSymbol *); /*27523*/
extern "C" void __cdecl collect_data_member_traits(CompilerSymbol *input, CompilerSpecialMemberTraits *traits)
{
    CompilerKind4Symbol *symbol = (CompilerKind4Symbol *)input;
    CompilerSymbolLink *chain = symbol->class_binding ? symbol->class_binding->members : NULL;
    CompilerSymbolLink *current = chain;
    while (current) {
        CompilerSymbolLink *link = current;
        chain = chain ? chain->next : NULL;
        current = chain;
        CompilerSymbol *member = (CompilerSymbol *)((BYTE *)link - 8);
        if ((member->kind != 3 && member->kind != 2) || (member->scope_flags & 1)) continue;
        CompilerGlobalSymbolView *owner = enclosing_declaration_class(member);
        if (is_class_scope_symbol(member) || (((CompilerIrSymbol *)member)->reference_flags & 0x80)) continue;
        if (owner) {
            collect_inherited_class_traits(input, (CompilerSymbol *)owner, traits);
            if (!(owner->class_detail->declaration_flags & 1))
                ((CompilerGlobalSymbolView *)input)->class_detail->declaration_flags &= ~1;
            ParserDefinitionClassView *detail = (ParserDefinitionClassView *)((CompilerGlobalSymbolView *)input)->class_detail;
            detail->context_flags = (detail->context_flags & ~8) | (owner->class_detail->context_flags & 8);
        }
        if (member_requires_initialization(member)) {
            traits[1].initial_flags = traits[3].initial_flags = 0;
            traits[1].condition = traits[3].condition = 1;
        }
    }
}
extern "C" int __cdecl member_requires_initialization(CompilerSymbol *symbol)
{
    if (get_function_calling_convention(((CompilerIrSymbol *)symbol)->type) & 0x100) return 1;
    CompilerTypeShape *shape = ((CompilerIrSymbol *)symbol)->type->shape;
    return shape && (shape->flags & 0x40) ? 1 : 0;
}
extern "C" void __cdecl select_class_special_members(CompilerSymbol *input,
    CompilerSpecialMemberTraits *traits, int *constructor_count, unsigned *assignment_count)
{
    CompilerClassMemberIterator declarations;
    declarations.Initialize(&((CompilerNamedSymbol *)input)->link, 2);
    CompilerMemberVariantIterator variants(NULL);
    CompilerMemberVariantIterator *cursor = &variants;
    *assignment_count = 0;
    *constructor_count = 0;
    for (;;) {
        if (!cursor->Next()) {
            if (!declarations.Next()) break;
            variants.SetLink(declarations.entry);
            continue;
        }
        if (!variants.IsPrimary()) continue;
        unsigned accessible = (variants.GetSymbol()->definition_flags & 3) != 1;
        *constructor_count = (int)((unsigned)*constructor_count + ((variants.GetSymbol()->type_flags >> 4) & 1));
        CompilerSymbol *raw = (CompilerSymbol *)((BYTE *)variants.current - 8);
        CompilerRuntimeFunction *member = (CompilerRuntimeFunction *)(raw->kind == 24 ?
            ((CompilerAliasSymbol *)raw)->target->symbol : raw);
        *assignment_count += member->link.name == frontend_data.builtin_names.assignment_operator;
        raw = (CompilerSymbol *)((BYTE *)variants.current - 8);
        member = (CompilerRuntimeFunction *)(raw->kind == 24 ? ((CompilerAliasSymbol *)raw)->target->symbol : raw);
        if (member->builtin_flags & 0x10) {
            traits[1].declared = 1;
            if (traits[1].seen) {
                if (!traits[1].duplicate_reported) report_language_diagnostic(3, 0x208, input);
                traits[1].duplicate_reported = 1;
            }
            traits[1].secondary_flags &= accessible;
            traits[1].seen = 1;
        }
        raw = (CompilerSymbol *)((BYTE *)variants.current - 8);
        member = (CompilerRuntimeFunction *)(raw->kind == 24 ? ((CompilerAliasSymbol *)raw)->target->symbol : raw);
        if (member->builtin_flags & 0x20) {
            traits[2].declared = 1;
            if (traits[2].seen) {
                if (!traits[2].duplicate_reported) report_language_diagnostic(3, 0x209, input);
                traits[2].duplicate_reported = 1;
            }
            traits[2].secondary_flags &= accessible;
            traits[2].seen = 1;
            member = (CompilerRuntimeFunction *)variants.GetSymbol();
            ParserDefinitionClassView *detail = (ParserDefinitionClassView *)((CompilerGlobalSymbolView *)input)->class_detail;
            detail->inheritance_flags = (detail->inheritance_flags & 0xcf) | ((member->queue_flags >> 1) & 0x30);
            continue;
        }
        raw = (CompilerSymbol *)((BYTE *)variants.current - 8);
        member = (CompilerRuntimeFunction *)(raw->kind == 24 ? ((CompilerAliasSymbol *)raw)->target->symbol : raw);
        if (member->builtin_flags & 0x80) {
            traits[3].declared = 1;
            if (traits[3].seen) {
                if (!traits[3].duplicate_reported) report_language_diagnostic(3, 0x20a, input);
                traits[3].duplicate_reported = 1;
            }
            traits[3].secondary_flags &= accessible;
            traits[3].seen = 1;
            member = (CompilerRuntimeFunction *)variants.GetSymbol();
            ParserDefinitionClassView *detail = (ParserDefinitionClassView *)((CompilerGlobalSymbolView *)input)->class_detail;
            detail->inheritance_flags = (detail->inheritance_flags & 0x3f) | ((member->queue_flags << 1) & 0xc0);
            continue;
        }
        raw = (CompilerSymbol *)((BYTE *)variants.current - 8);
        member = (CompilerRuntimeFunction *)(raw->kind == 24 ? ((CompilerAliasSymbol *)raw)->target->symbol : raw);
        if (member->type_flags & 0x20) {
            traits[4].declared = 1;
            if (traits[4].seen) {
                if (!traits[4].duplicate_reported) report_language_diagnostic(3, 0x20b, input);
                traits[4].duplicate_reported = 1;
            }
            traits[4].secondary_flags &= accessible;
            traits[4].seen = 1;
        }
    }
}
extern "C" CompilerSymbol *__cdecl create_implicit_special_member(CompilerSymbol *, unsigned, unsigned, unsigned); /*27964*/
extern "C" void __cdecl complete_class_special_member(CompilerSymbol *input,
    CompilerSpecialMemberTraits *traits, unsigned kind, unsigned diagnostic,
    unsigned required, unsigned)
{
    /* Only the five actual local rows are modeled; callers here select1..4. */
    if (kind < 1 || kind > 4) { __asm { int 3 } }
    CompilerSpecialMemberTraits &record = traits[kind];
    CompilerGlobalSymbolView *symbol = (CompilerGlobalSymbolView *)input;
    ParserDefinitionClassView *detail;
    BYTE mask;
    if (record.declared) {
        detail = (ParserDefinitionClassView *)symbol->class_detail;
        detail->declared_member_flags |= (BYTE)((BYTE)record.seen << kind);
        detail = (ParserDefinitionClassView *)symbol->class_detail;
        detail->accessible_member_flags |= (BYTE)((BYTE)record.secondary_flags << kind);
        detail = (ParserDefinitionClassView *)symbol->class_detail;
        detail->duplicate_member_flags |= (BYTE)((BYTE)record.duplicate_reported << kind);
        return;
    }
    if (!required) return;
    if (!record.condition) {
        mask = (BYTE)(1U << kind);
        detail = (ParserDefinitionClassView *)symbol->class_detail;
        detail->generated_member_flags |= mask;
        detail = (ParserDefinitionClassView *)symbol->class_detail;
        detail->accessible_member_flags |= mask;
        if (kind == 2) detail->inheritance_flags = (detail->inheritance_flags & 0xdf) | 0x10;
        else if (kind == 3) detail->inheritance_flags = (detail->inheritance_flags & 0x3f) | 0x40;
        create_implicit_special_member(input, kind, kind == 3, 1);
        return;
    }
    if (!record.initial_flags) {
        report_language_diagnostic(4, diagnostic, input);
        return;
    }
    mask = (BYTE)(1U << kind);
    detail = (ParserDefinitionClassView *)symbol->class_detail;
    detail->inherited_member_flags |= mask;
    detail = (ParserDefinitionClassView *)symbol->class_detail;
    detail->declared_member_flags |= mask;
    detail = (ParserDefinitionClassView *)symbol->class_detail;
    detail->accessible_member_flags |= mask;
    if (kind == 2)
        detail->inheritance_flags = (detail->inheritance_flags & 0xcf) | ((traits[2].active & 3) << 4);
    else if (kind == 3)
        detail->inheritance_flags = (detail->inheritance_flags & 0x3f) | (BYTE)(traits[3].active << 6);
    create_implicit_special_member(input, kind, 1, 0);
}
extern "C" {
CompilerNameEntry *__cdecl special_member_name(unsigned); /*27bc8*/
CompilerParameterDescription *__cdecl make_special_member_parameters(CompilerSymbol *, unsigned); /*247d3*/
CompilerParameterDescription *__cdecl make_empty_function_parameters(void); /*1e271*/
CompilerSymbol *__cdecl recycle_symbol(unsigned, CompilerNameEntry *, unsigned);
void __cdecl emit_implicit_function(CompilerSymbol *); /*15c4c*/
}
extern "C" CompilerSymbol *__cdecl create_implicit_special_member(CompilerSymbol *input,
    unsigned kind, unsigned emit, unsigned generated)
{
    CompilerNameEntry *name = special_member_name(kind);
    CompilerRuntimeFunction *symbol = (CompilerRuntimeFunction *)recycle_symbol(3, name, 0);
    if (!symbol) {
        BYTE *memory = frontend_bss.arenas[0].Allocate((frontend_data.symbol_record_bytes[3] + 3) & ~3U);
        symbol = memory ? (CompilerRuntimeFunction *)((CompilerSymbol *)memory)->Initialize(3, name, 0) : NULL;
    }
    symbol->scope_flags |= 1;
    symbol->linkage_flags = (symbol->linkage_flags & ~0x10) | ((generated & 1) << 4);
    symbol->builtin_flags = (symbol->builtin_flags & ~8) | ((emit & 1) << 3) | 1;
    symbol->function_class = 1;
    symbol->definition_flags |= 3;
    symbol->builtin_operation = (BYTE)kind;
    CompilerParameterDescription *parameters = make_special_member_parameters(input, kind);
    switch (kind) {
    case 1:
        symbol->builtin_flags |= 0x10;
        symbol->link.name = ((CompilerNamedSymbol *)input)->link.name;
        symbol->type_flags |= 0x10;
        break;
    case 2:
        symbol->link.name = ((CompilerNamedSymbol *)input)->link.name;
        symbol->builtin_flags |= 0x20;
        symbol->type_flags |= 0x10;
        break;
    case 3:
        symbol->function_class = 0x34;
        symbol->builtin_flags |= 0x80;
        break;
    case 4:
        symbol->link.name = ((CompilerNamedSymbol *)input)->link.name;
        symbol->type_flags |= 0x20;
        break;
    }
    if (!parameters) parameters = make_empty_function_parameters();
    CompilerDeclarator declarator;
    declarator.InitializeSymbol(symbol);
    declarator.AddFunction(parameters);
    ParserDeclarationView declaration;
    if (kind == 3) {
        CompilerDeclaratorQualifiers qualifier;
        qualifier.flags = 0;
        qualifier.value = 0;
        declarator.AddReference(&qualifier);
        declaration.type = member_class_row().type;
    } else {
        if (kind > 5 && kind <= 7) {
            CompilerDeclaratorQualifiers first, second;
            first.flags = second.flags = 0;
            first.value = second.value = 0;
            declarator.AddPointer(&first, &second);
        }
        declaration.type = frontend_data.builtin_types.type_0080;
    }
    /* Only the original28-byte context and object cell are written here.
     * The unused declaration callback stays untouched in both layouts. */
    memset(&declaration.context, 0, 28);
    declaration.context.class_flags = 0x10;
    declaration.object = NULL;
    declaration_control_row(frontend_bss.parser_context_depth)->flags |= 1;
    declarator.ApplyDeclaration(&declaration);
    declarator.Resolve();
    CompilerSymbol *replacement = NULL;
    declare_initialized_symbol(&declaration, symbol, &declarator, &replacement);
    symbol->function_flags |= 1;
    symbol->definition_flags |= 4;
    if ((symbol->member_flags & 4) && emit) emit_implicit_function(symbol);
    return symbol;
}
extern "C" CompilerNameEntry *__cdecl special_member_name(unsigned kind)
{
    switch (kind) {
    case 1: return frontend_data.builtin_names.constructor;
    case 2: return frontend_data.builtin_names.constructor;
    case 3: return frontend_data.builtin_names.assignment_operator;
    case 4: return frontend_data.builtin_names.destructor;
    case 5: return frontend_data.builtin_names.vbase_destructor;
    case 6: return frontend_data.builtin_names.deleting_destructor;
    case 7: return frontend_data.builtin_names.vector_deleting_destructor;
    case 8: return frontend_data.builtin_names.copy_constructor_closure;
    case 9: return frontend_data.builtin_names.local_vftable_constructor_closure;
    case 10: return frontend_data.builtin_names.default_constructor_closure;
    case 11: return frontend_data.builtin_names.vector_constructor;
    case 12: return frontend_data.builtin_names.vector_vbase_constructor;
    case 13: return frontend_data.builtin_names.vector_destructor;
    case 14: return frontend_data.builtin_names.eh_vector_constructor;
    case 15: return frontend_data.builtin_names.eh_vector_vbase_constructor;
    case 16: return frontend_data.builtin_names.eh_vector_destructor;
    default: return NULL;
    }
}
extern "C" CompilerParameterDescription *__cdecl make_class_reference_parameters(CompilerSymbol *, WORD); /*2b263*/
extern "C" CompilerType *__cdecl make_parser_type_scratch(CompilerType *type)
{
    /* Only the actual first frame at c04c8 is modeled; deeper indexed
     * storage is unsupported before increment or address calculation. */
    if (frontend_bss.type_scratch_index != 0) { __asm { int 3 } }
    ++frontend_bss.type_scratch_index;
    CompilerType *scratch = &frontend_bss.first_type_scratch_frame;
    memcpy(scratch, type, 16);
    return scratch;
}
extern "C" CompilerParameterDescription *__cdecl make_special_member_parameters(CompilerSymbol *input, unsigned kind)
{
    CompilerParameterDescription *parameters = NULL;
    switch (kind) {
    case 2: case 8:
        parameters = make_class_reference_parameters(input,
            (WORD)((((CompilerGlobalSymbolView *)input)->class_detail->inheritance_flags >> 4 & 3) << 8));
        break;
    case 3:
        parameters = make_class_reference_parameters(input,
            (WORD)((((CompilerGlobalSymbolView *)input)->class_detail->inheritance_flags >> 6) << 8));
        break;
    case 6: case 7: {
        CompilerArena *arena = &frontend_bss.arenas[5];
        BYTE *memory = arena->cursor;
        arena->cursor += 28;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        parameters = (CompilerParameterDescription *)memory;
        if (parameters) {
            parameters->kind = 3;
            parameters->scope_open = 0;
            parameters->parameters = NULL;
        }
        parameters->Append(frontend_data.builtin_names.flags_parameter,
            frontend_data.builtin_types.type_4063_first);
        break;
    }
    }
    if (!parameters) parameters = make_empty_function_parameters();
    return parameters;
}
extern "C" CompilerParameterDescription *__cdecl make_class_reference_parameters(CompilerSymbol *input, WORD attributes)
{
    CompilerNameEntry *name = frontend_data.builtin_names.that_parameter;
    CompilerParameterSymbol *symbol = (CompilerParameterSymbol *)recycle_symbol(11, name, 4);
    if (!symbol) {
        BYTE *memory = frontend_bss.arenas[4].Allocate((frontend_data.symbol_record_bytes[11] + 3) & ~3U);
        symbol = memory ? (CompilerParameterSymbol *)((CompilerSymbol *)memory)->Initialize(11, name, 4) : NULL;
    }
    symbol->type = NULL;
    if (frontend_data.next_symbol_record_id >= 0x7fffffff) report_compiler_error(0x37);
    unsigned index = frontend_data.next_symbol_record_id++;
    symbol->encoded_index = (symbol->encoded_index & 0xff000000) | (index & 0xffffff);
    symbol->storage_class = (symbol->storage_class & ~7) | 5;
    symbol->declaration_state = 0;
    symbol->storage_class = (symbol->storage_class & ~7) | 5;
    CompilerType *type = ((CompilerIrSymbol *)input)->type;
    if (attributes) {
        CompilerType *scratch = make_parser_type_scratch(type);
        scratch->attributes = attributes;
        type = intern_type(scratch);
        --frontend_bss.type_scratch_index;
    }
    CompilerDeclarator declaration;
    CompilerDeclaratorQualifiers qualifiers;
    declaration.InitializeSymbol((CompilerSymbol *)symbol);
    declaration.base_type = type;
    qualifiers.flags = 0;
    qualifiers.value = 0;
    declaration.AddReference(&qualifiers);
    declaration.Resolve();
    finalize_symbol_declaration(&declaration, (CompilerSymbol *)symbol);
    if ((symbol->emission_state & 3) < 3)
        symbol->emission_state = (symbol->emission_state & ~3U) | ((symbol->emission_state + 1) & 3);
    symbol->emission_state = (symbol->emission_state & ~4U) | ((frontend_data.reference_context & 1) << 2);
    CompilerArena *arena = &frontend_bss.arenas[5];
    BYTE *memory = arena->cursor;
    arena->cursor += 28;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    CompilerParameterDescription *parameters = (CompilerParameterDescription *)memory;
    if (parameters) {
        parameters->kind = 3;
        parameters->scope_open = 0;
        parameters->parameters = NULL;
    }
    parameters->AppendSymbol((CompilerSymbol *)symbol);
    return parameters;
}
extern "C" void __cdecl emit_implicit_function(CompilerSymbol *) { __asm { int 3 } }
extern "C" CompilerTypeShape *__cdecl create_type_shape(unsigned, unsigned, const unsigned *);
void RECON_THISCALL CompilerDeclarator::AddReference(CompilerDeclaratorQualifiers *qualifiers)
{
    WORD attributes = ValidateReferenceQualifiers(qualifiers->flags);
    CompilerTypeShape *next = create_type_shape(0x40, attributes, &qualifiers->value);
    ValidateReferenceShape(next);
    if (last_shape) {
        last_shape->base = next;
        last_shape = next;
    } else {
        last_shape = next;
        shape = next;
    }
}
WORD RECON_THISCALL CompilerDeclarator::ValidateReferenceQualifiers(WORD attributes)
{
    if (attributes & 1) {
        compiler_warning(0x280, symbol);
        attributes &= 0xfffc;
        flags |= 1;
    }
    if (attributes & 0x70) shape_flags |= 1;
    return attributes;
}
extern "C" void __cdecl prepare_class_function_tables(CompilerSymbol *); /*27c7b*/
extern "C" void __cdecl finalize_class_member_groups(CompilerSymbol *); /*28416*/
extern "C" void __cdecl finalize_class_functions(CompilerSymbol *symbol)
{
    prepare_class_function_tables(symbol);
    finalize_class_member_groups(symbol);
}
extern "C" {
void __cdecl reset_class_layout_reference(void);
void __cdecl collect_base_class_layout(CompilerSymbol *);
void __cdecl finalize_class_size(CompilerSymbol *);
void __cdecl validate_additional_member_groups(CompilerSymbol *);
void __cdecl reconcile_class_members(CompilerSymbol *);
void __cdecl reconcile_member_group_layout(CompilerSymbol *);
void __cdecl merge_inherited_member_groups(CompilerSymbol *);
void __cdecl update_class_member_groups(CompilerSymbol *);
int __cdecl has_flagged_class_members(CompilerSymbol *); /*2838b*/
}
extern "C" void __cdecl prepare_class_function_tables(CompilerSymbol *input)
{
    reset_class_layout_reference();
    collect_base_class_layout(input);
    finalize_class_size(input);
    validate_additional_member_groups(input);
    reconcile_class_members(input);
    reconcile_member_group_layout(input);
    merge_inherited_member_groups(input);
    update_class_member_groups(input);
    unsigned flagged = (BYTE)has_flagged_class_members(input);
    CompilerGlobalClassDetail *detail = ((CompilerGlobalSymbolView *)input)->class_detail;
    detail->context_flags = (detail->context_flags & 0x7f) | (BYTE)(flagged << 7);
}
extern "C" void __cdecl reset_class_layout_reference(void)
{
    frontend_data.class_layout_reference = NULL;
}
extern "C" void __cdecl collect_base_member_layout(CompilerClassMemberSymbol *, CompilerSymbol *); /*2aa86*/
extern "C" void __cdecl collect_base_class_layout(CompilerSymbol *input)
{
    CompilerKind4Symbol *symbol = (CompilerKind4Symbol *)input;
    CompilerSymbolLink *chain = symbol->class_binding ? symbol->class_binding->members : NULL;
    CompilerSymbolLink *current = chain;
    while (current) {
        CompilerSymbolLink *link = current;
        chain = chain ? chain->next : NULL;
        current = chain;
        CompilerSymbol *member = (CompilerSymbol *)((BYTE *)link - 8);
        if (member->kind == 3 && (link->name == frontend_data.builtin_names.vbptr ||
                link->name == frontend_data.builtin_names.vfptr)) continue;
        if ((frontend_data.source_mode_flags & 0x100000) && member->kind == 15 &&
            (((CompilerClassMemberSymbol *)member)->flags & 2)) continue;
        if (member->kind != 15) return;
        collect_base_member_layout((CompilerClassMemberSymbol *)member, input);
    }
}
extern "C" void __cdecl collect_base_member_layout(CompilerClassMemberSymbol *, CompilerSymbol *) { __asm { int 3 } }
extern "C" void __cdecl finalize_class_size(CompilerSymbol *input)
{
    CompilerGlobalClassDetail *detail = ((CompilerGlobalSymbolView *)input)->class_detail;
    detail->size = member_class_row().member_bytes;
    if (!detail->size) {
        unsigned minimum = (int)compiler_primitive_type_bytes[1];
        member_class_row().member_bytes = minimum;
        detail->size = minimum;
        detail->context_flags |= 8;
        detail->context_flags |= 4;
    }
}
extern "C" int __cdecl check_class_member_group(InheritedMemberGroup *, CompilerSymbol *); /*4be4b*/
extern "C" void __cdecl complete_class_member_group(InheritedMemberGroup *, CompilerSymbol *); /*4bec7*/
extern "C" void __cdecl validate_additional_member_groups(CompilerSymbol *input)
{
    CompilerVector *groups = ((CompilerGlobalSymbolView *)input)->class_detail->additional_members;
    short index = 0;
    while (groups && index < groups->count) {
        InheritedMemberGroup *group = (InheritedMemberGroup *)groups->items[index++];
        if (!check_class_member_group(group, input)) complete_class_member_group(group, input);
    }
}
extern "C" int __cdecl check_class_member_group(InheritedMemberGroup *, CompilerSymbol *) { __asm { int 3 } return 0; }
extern "C" void __cdecl complete_class_member_group(InheritedMemberGroup *, CompilerSymbol *) { __asm { int 3 } }
extern "C" void __cdecl reconcile_class_member_function(CompilerSymbolLink *, CompilerSymbol *); /*27f3f*/
extern "C" void __cdecl reconcile_class_members(CompilerSymbol *input)
{
    CompilerKind4Symbol *owner = (CompilerKind4Symbol *)input;
    CompilerClassMemberIterator members;
    members.InitializeHead(owner->class_binding ? owner->class_binding->members : NULL);
    members.entry = NULL;
    members.current = members.valid ? members.chain : NULL;
    members.owner = &owner->link;
    members.mode = 2;
    CompilerMemberVariantIterator variants;
    for (;;) {
        if (!variants.source || !variants.chain) {
            if (!members.Next()) return;
            variants.current = NULL;
            variants.source = members.entry;
            CompilerSymbol *member = (CompilerSymbol *)((BYTE *)members.entry - 8);
            variants.chain = member->IsFunction() || member->kind == 24 ? members.entry :
                ((CompilerMemberGroupSymbol *)member)->variants;
        }
        while (variants.chain && ((CompilerSymbol *)((BYTE *)variants.chain - 8))->kind == 24)
            variants.chain = variants.chain->next;
        if (!variants.chain) continue;
        variants.current = variants.chain;
        CompilerSymbol *source = (CompilerSymbol *)((BYTE *)variants.source - 8);
        variants.chain = source->IsFunction() ? NULL : variants.chain->next;
        reconcile_class_member_function(variants.current, input);
    }
}
extern "C" void __cdecl match_class_member_group(CompilerSymbolLink *, ClassMemberFunctionTable *, CompilerSymbol *, unsigned); /*2adcb*/
extern "C" ClassMemberFunctionTable *__cdecl first_class_member_table(CompilerSymbol *); /*3d273*/
extern "C" void __cdecl initialize_member_function_record(InheritedMemberRecord *, CompilerSymbolLink *); /*3d283*/
extern "C" void __cdecl append_member_function_record(ClassMemberFunctionTable *, const InheritedMemberRecord *); /*3d2a6*/
extern "C" unsigned __cdecl allocate_member_function_storage(ClassMemberFunctionTable *, unsigned); /*3d2f2*/
extern "C" void __cdecl reconcile_class_member_function(CompilerSymbolLink *input, CompilerSymbol *owner)
{
    CompilerGlobalClassDetail *detail = ((CompilerGlobalSymbolView *)owner)->class_detail;
    InheritedMemberCursor primary, additional;
    InheritedMemberGroup *outer = NULL;
    primary.vector = detail->member_groups[0];
    primary.index = 0;
    additional.vector = detail->additional_members;
    additional.index = 0;
    for (;;) {
        if (!primary.vector || primary.index >= primary.vector->count) {
            if (!next_inherited_member_group(&additional)) break;
            outer = (InheritedMemberGroup *)additional.current;
            primary.vector = outer->functions[0];
            primary.index = 0;
            continue;
        }
        ClassMemberFunctionTable *table = (ClassMemberFunctionTable *)primary.vector->items[primary.index++];
        primary.current = table;
        match_class_member_group(input, table, owner, outer && outer->secondary_state ? 1 : 0);
    }
    CompilerRuntimeFunction *symbol = (CompilerRuntimeFunction *)((BYTE *)input - 8);
    if (symbol->builtin_flags & 2) {
        symbol->function_flags |= 2;
        input->name->flags |= 0x100000;
    } else if (symbol->function_flags & 2) {
        ClassMemberFunctionTable *table = first_class_member_table(owner);
        InheritedMemberRecord record;
        initialize_member_function_record(&record, input);
        append_member_function_record(table, &record);
        unsigned bytes = global_type_size(symbol->type);
        symbol->member_offset = allocate_member_function_storage(table, bytes);
        symbol->member_scope_number = detail->scope_number;
    }
}
extern "C" void __cdecl match_class_member_group(CompilerSymbolLink *, ClassMemberFunctionTable *, CompilerSymbol *, unsigned) { __asm { int 3 } }
extern "C" ClassMemberFunctionTable *__cdecl first_class_member_table(CompilerSymbol *) { __asm { int 3 } return NULL; }
extern "C" void __cdecl initialize_member_function_record(InheritedMemberRecord *, CompilerSymbolLink *) { __asm { int 3 } }
extern "C" void __cdecl append_member_function_record(ClassMemberFunctionTable *, const InheritedMemberRecord *) { __asm { int 3 } }
extern "C" unsigned __cdecl allocate_member_function_storage(ClassMemberFunctionTable *, unsigned) { __asm { int 3 } return 0; }
extern "C" void __cdecl complete_member_function_record(InheritedMemberRecord *); /*4c57e*/
extern "C" void __cdecl reconcile_member_group_layout(CompilerSymbol *input)
{
    CompilerGlobalClassDetail *detail = ((CompilerGlobalSymbolView *)input)->class_detail;
    InheritedMemberCursor primary, additional;
    primary.vector = detail->member_groups[0];
    primary.index = 0;
    additional.vector = detail->additional_members;
    additional.index = 0;
    for (;;) {
        if (!next_inherited_member_group(&primary)) {
            do {
                if (!next_inherited_member_group(&additional)) return;
                primary.vector = ((InheritedMemberGroup *)additional.current)->functions[0];
                primary.index = 0;
            } while (!next_inherited_member_group(&primary));
        }
        ClassMemberFunctionTable *table = (ClassMemberFunctionTable *)primary.current;
        if (table->kind != 2) continue;
        for (int index = 0; index < table->count; ++index) {
            InheritedMemberRecord *record = &table->records[index];
            if (record->flags & 4) {
                CompilerVector *members = record->members;
                short member_index = 0;
                while (members && member_index < members->count) {
                    CompilerSymbol *member = (CompilerSymbol *)((BYTE *)members->items[member_index++] - 8);
                    short scope = member->declaration_qualifiers;
                    if (scope < 0 || scope >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
                    compiler_warning(0xfa, input, frontend_data.frontend_pointer_table->items[scope], member);
                }
                complete_member_function_record(record);
            } else if (record->flags & 2) {
                CompilerSymbol *member = (CompilerSymbol *)((BYTE *)record->member - 8);
                short scope = member->declaration_qualifiers;
                if (scope < 0 || scope >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
                report_language_diagnostic(2, 0xfa, input, frontend_data.frontend_pointer_table->items[scope], member);
            }
        }
    }
}
extern "C" void __cdecl complete_member_function_record(InheritedMemberRecord *) { __asm { int 3 } }
extern "C" int __cdecl compare_member_merge_targets(CompilerVector *, CompilerVector *); /*4c595*/
extern "C" void __cdecl merge_inherited_member_groups(CompilerSymbol *input)
{
    CompilerGlobalClassDetail *detail = ((CompilerGlobalSymbolView *)input)->class_detail;
    for (unsigned group = 0; group < 2; ++group) {
        CompilerVector *primary = detail->member_groups[group];
        CompilerVector *additional = detail->additional_members;
        short primary_index = 0, additional_index = 0;
        for (;;) {
            if (!primary || primary_index >= primary->count) {
                if (!additional || additional_index >= additional->count) break;
                InheritedMemberGroup *outer = (InheritedMemberGroup *)additional->items[additional_index++];
                primary = outer->functions[group];
                primary_index = 0;
                continue;
            }
            ClassMemberFunctionTable *table = (ClassMemberFunctionTable *)primary->items[primary_index++];
            if (!table->merge_value) continue;
            int merged = 0;
            CompilerVector *other_primary = detail->member_groups[group];
            CompilerVector *other_additional = detail->additional_members;
            short other_index = 0, other_additional_index = 0;
            for (;;) {
                if (!other_primary || other_index >= other_primary->count) {
                    if (!other_additional || other_additional_index >= other_additional->count) break;
                    InheritedMemberGroup *outer = (InheritedMemberGroup *)other_additional->items[other_additional_index++];
                    other_primary = outer->functions[group];
                    other_index = 0;
                    continue;
                }
                ClassMemberFunctionTable *other = (ClassMemberFunctionTable *)other_primary->items[other_index++];
                if (!other->merge_value || other == table) continue;
                if (compare_member_merge_targets(table->merge_targets, other->merge_targets)) {
                    append_option_vector(other->merge_targets, other->merge_value);
                    other->merge_value = NULL;
                    merged = 1;
                }
            }
            if (merged) {
                append_option_vector(table->merge_targets, table->merge_value);
                table->merge_value = NULL;
            }
        }
    }
}
extern "C" int __cdecl compare_member_merge_targets(CompilerVector *, CompilerVector *) { __asm { int 3 } return 0; }
typedef void *(__cdecl *MemberGroupCopyFunction)(void *, unsigned);
extern "C" CompilerVector *__cdecl copy_member_group_vector(CompilerVector *, unsigned, MemberGroupCopyFunction); /*2831f*/
extern "C" void *__cdecl copy_member_function_table(void *, unsigned); /*2aed2*/
extern "C" void *__cdecl copy_inherited_member_group(void *, unsigned); /*4bf27*/
extern "C" void __cdecl update_class_member_groups(CompilerSymbol *input)
{
    for (unsigned group = 0; group < 2; ++group) {
        CompilerGlobalClassDetail *detail = ((CompilerGlobalSymbolView *)input)->class_detail;
        CompilerVector *copy = copy_member_group_vector(detail->member_groups[group], 0, copy_member_function_table);
        ((CompilerGlobalSymbolView *)input)->class_detail->member_groups[group] = copy;
    }
    CompilerGlobalClassDetail *detail = ((CompilerGlobalSymbolView *)input)->class_detail;
    CompilerVector *copy = copy_member_group_vector(detail->additional_members, 0, copy_inherited_member_group);
    ((CompilerGlobalSymbolView *)input)->class_detail->additional_members = copy;
}
extern "C" CompilerVector *__cdecl copy_member_group_vector(CompilerVector *input, unsigned arena, MemberGroupCopyFunction copy)
{
    if (!input || !input->count) return NULL;
    CompilerVector *result = new_option_vector(input ? input->count : 0, arena);
    short index = 0;
    while (input && index < input->count) {
        void *item = copy(input->items[index++], arena);
        append_option_vector(result, item);
    }
    return result;
}
extern "C" void *__cdecl copy_member_function_table(void *, unsigned) { __asm { int 3 } return NULL; }
extern "C" void *__cdecl copy_inherited_member_group(void *, unsigned) { __asm { int 3 } return NULL; }
extern "C" int __cdecl has_flagged_class_members(CompilerSymbol *input)
{
    CompilerGlobalClassDetail *detail = ((CompilerGlobalSymbolView *)input)->class_detail;
    CompilerVector *primary = detail->member_groups[0];
    CompilerVector *additional = detail->additional_members;
    short index = 0, additional_index = 0;
    for (;;) {
        if (!primary || index >= primary->count) {
            if (!additional || additional_index >= additional->count) return 0;
            InheritedMemberGroup *outer = (InheritedMemberGroup *)additional->items[additional_index++];
            primary = outer->functions[0];
            index = 0;
            continue;
        }
        ClassMemberFunctionTable *table = (ClassMemberFunctionTable *)primary->items[index++];
        for (int member = 0; member < table->count; ++member) {
            InheritedMemberRecord *record = &table->records[member];
            if (!record) break;
            CompilerRuntimeFunction *symbol = (CompilerRuntimeFunction *)((BYTE *)record->member - 8);
            if (symbol->builtin_flags & 4) return 1;
        }
    }
}
extern "C" CompilerRuntimeFunction *__cdecl create_member_group_function(unsigned, ClassMemberFunctionTable *, CompilerSymbol *); /*2afac*/
extern "C" void __cdecl finalize_class_member_groups(CompilerSymbol *input)
{
    for (unsigned group = 0; group < 2; ++group) {
        CompilerGlobalClassDetail *detail = ((CompilerGlobalSymbolView *)input)->class_detail;
        CompilerVector *primary = detail->member_groups[group];
        CompilerVector *additional = detail->additional_members;
        short index = 0, additional_index = 0;
        for (;;) {
            if (!primary || index >= primary->count) {
                if (!additional || additional_index >= additional->count) break;
                InheritedMemberGroup *outer = (InheritedMemberGroup *)additional->items[additional_index++];
                primary = outer->functions[group];
                index = 0;
                continue;
            }
            ClassMemberFunctionTable *table = (ClassMemberFunctionTable *)primary->items[index++];
            CompilerRuntimeFunction *function = create_member_group_function(group, table, input);
            table->function = function;
            if (function && (table->kind == 0 || table->kind == 2)) {
                if (function->member_flags & 8)
                    table->secondary_function = create_member_group_function(5, table, input);
                else table->secondary_function = NULL;
                table->secondary_function_state = 0;
            }
        }
    }
}
extern "C" CompilerRuntimeFunction *__cdecl create_member_group_function(unsigned, ClassMemberFunctionTable *, CompilerSymbol *) { __asm { int 3 } return NULL; }
extern "C" CompilerSymbol *__cdecl find_class_member_by_name(CompilerSymbol *, CompilerNameEntry *); /*1e785*/
extern "C" void __cdecl prepare_class_constructor(CompilerSymbol *, CompilerRuntimeFunction *); /*4265c*/
extern "C" void __cdecl complete_class_constructor(CompilerSymbol *, CompilerRuntimeFunction *); /*36e00*/
extern "C" void __cdecl prepare_secondary_class_constructor(CompilerSymbol *, CompilerRuntimeFunction *); /*b3fcf*/
extern "C" void __cdecl finalize_class_offsets(CompilerSymbol *input)
{
    CompilerGlobalClassDetail *detail = ((CompilerGlobalSymbolView *)input)->class_detail;
    if (!(detail->generated_member_flags & 2)) {
        CompilerSymbol *constructors = find_class_member_by_name(input, frontend_data.builtin_names.constructor);
        if (constructors) {
            CompilerSymbolLink *source = &((CompilerNamedSymbol *)constructors)->link;
            CompilerSymbolLink *chain = constructors->IsFunction() || constructors->kind == 24 ? source :
                ((CompilerMemberGroupSymbol *)constructors)->variants;
            while (chain) {
                CompilerSymbolLink *current = chain;
                chain = constructors->IsFunction() || constructors->kind == 24 ? NULL : chain->next;
                CompilerSymbol *raw = (CompilerSymbol *)((BYTE *)current - 8);
                CompilerRuntimeFunction *member = (CompilerRuntimeFunction *)(raw->kind == 24 ?
                    ((CompilerAliasSymbol *)raw)->target->symbol : raw);
                if ((member->builtin_flags & 0x10) && member->type->shape->payload.function->symbol_count)
                    prepare_class_constructor(input, member);
            }
            detail = ((CompilerGlobalSymbolView *)input)->class_detail;
            if (!(detail->generated_member_flags & 0x10)) {
                CompilerSymbol *destructors = find_class_member_by_name(input, frontend_data.builtin_names.destructor);
                if (destructors) {
                    CompilerSymbolLink *destructor = destructors->IsFunction() || destructors->kind == 24 ?
                        &((CompilerNamedSymbol *)destructors)->link : ((CompilerMemberGroupSymbol *)destructors)->variants;
                    if (destructor) {
                        destructors->IsFunction(); /* original inline advance, only its first result is consumed */
                        CompilerSymbol *raw = (CompilerSymbol *)((BYTE *)destructor - 8);
                        CompilerRuntimeFunction *member = (CompilerRuntimeFunction *)(raw->kind == 24 ?
                            ((CompilerAliasSymbol *)raw)->target->symbol : raw);
                        if (member->function_flags & 2) {
                            chain = constructors->IsFunction() || constructors->kind == 24 ? source :
                                ((CompilerMemberGroupSymbol *)constructors)->variants;
                            while (chain) {
                                CompilerSymbolLink *current = chain;
                                chain = constructors->IsFunction() || constructors->kind == 24 ? NULL : chain->next;
                                raw = (CompilerSymbol *)((BYTE *)current - 8);
                                member = (CompilerRuntimeFunction *)(raw->kind == 24 ?
                                    ((CompilerAliasSymbol *)raw)->target->symbol : raw);
                                if (member->builtin_flags & 0x10) {
                                    complete_class_constructor(input, member);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    detail = ((CompilerGlobalSymbolView *)input)->class_detail;
    if (!(detail->generated_member_flags & 4)) {
        CompilerSymbol *constructors = find_class_member_by_name(input, frontend_data.builtin_names.constructor);
        if (!constructors) return;
        CompilerMemberVariantIterator variants(&((CompilerNamedSymbol *)constructors)->link);
        while (variants.Next()) {
            CompilerSymbol *raw = (CompilerSymbol *)((BYTE *)variants.current - 8);
            CompilerRuntimeFunction *member = (CompilerRuntimeFunction *)(raw->kind == 24 ?
                ((CompilerAliasSymbol *)raw)->target->symbol : raw);
            if ((member->builtin_flags & 0x20) && member->type->shape->payload.function->symbol_count) {
                prepare_secondary_class_constructor(input, member);
                return;
            }
        }
    }
}
extern "C" CompilerSymbol *__cdecl find_class_member_by_name(CompilerSymbol *input, CompilerNameEntry *name)
{
    CompilerGlobalSymbolView *symbol = (CompilerGlobalSymbolView *)input;
    CompilerGlobalClassDetail *detail = symbol->class_detail;
    CompilerSymbolLink *found = NULL;
    if (detail->member_index) found = detail->member_names.Find(name);
    else {
        CompilerClassMemberIterator members;
        members.InitializeBinding(((CompilerKind4Symbol *)input)->class_binding);
        members.entry = NULL;
        members.owner = &((CompilerNamedSymbol *)input)->link;
        members.current = members.valid ? members.chain : NULL;
        members.mode = 0;
        while ((members.entry = members.current) != NULL) {
            members.current = members.AdvanceChain() ? members.chain : NULL;
            if (members.MatchesMode() && members.entry->name == name) {
                found = members.entry;
                break;
            }
        }
    }
    if (!found) return NULL;
    CompilerSymbol *result = (CompilerSymbol *)((BYTE *)found - 8);
    return result->kind == 17 || result->kind == 8 ? result : NULL;
}
/* 1128b..112e2, 5a56e..5a583. FinalizeClassMetadata creates this
 * nonempty index in the same unsigned name-address order. */
CompilerSymbolLink *RECON_THISCALL CompilerMemberNameIndex::Find(CompilerNameEntry *name)
{
    int low = 0, high = count - 1;
    int middle;
    unsigned sought = (unsigned)name, candidate;
    if (sought > (unsigned)items[high]->name || sought < (unsigned)items[0]->name)
        return NULL;
    do {
        middle = (unsigned)(high + low) >> 1;
        candidate = (unsigned)items[middle]->name;
        if (sought < candidate) high = middle - 1;
        else low = middle + 1;
        if (candidate == sought) break;
    } while (low <= high);
    if (candidate != sought) return NULL;
    CompilerSymbolLink *entry = items[middle];
    CompilerSymbol *symbol = (CompilerSymbol *)((BYTE *)entry - 8);
    if (symbol->definition_flags & 0x40)
        entry = &((CompilerNamedSymbol *)instantiate_tag_definition(symbol))->link;
    return entry;
}
extern "C" CompilerHashTable *__cdecl get_constructor_alias_table(void);
extern "C" void __cdecl prepare_class_constructor(CompilerSymbol *owner, CompilerRuntimeFunction *source)
{
    CompilerRuntimeFunction *generated = (CompilerRuntimeFunction *)
        create_implicit_special_member(owner, 10, 1, 0);
    if ((source->member_flags & 0x0c) && !(generated->member_flags & 0x0c)) {
        generated->member_flags = (generated->member_flags & ~4) | (source->member_flags & 4);
        generated->member_flags = (generated->member_flags & ~8) | (source->member_flags & 8);
        if (generated->member_flags & 4) emit_implicit_function(generated);
    }
    insert_hash_entry(get_constructor_alias_table(), source, generated);
}
extern "C" CompilerHashTable *__cdecl get_constructor_alias_table(void)
{
    if (!frontend_data.constructor_aliases)
        frontend_data.constructor_aliases = create_hash_table(31, 0, 0, 0, 1, 1);
    return frontend_data.constructor_aliases;
}
extern "C" void __cdecl complete_class_constructor(CompilerSymbol *, CompilerRuntimeFunction *) { __asm { int 3 } }
extern "C" void __cdecl prepare_secondary_class_constructor(CompilerSymbol *, CompilerRuntimeFunction *) { __asm { int 3 } }
extern "C" void __cdecl finalize_class_destructor(CompilerSymbol *, CompilerSymbol *); /*35ec9*/
extern "C" void __cdecl finalize_class_layout(CompilerSymbol *input)
{
    CompilerGlobalClassDetail *detail = ((CompilerGlobalSymbolView *)input)->class_detail;
    if (detail->generated_member_flags & 0x10) return;
    CompilerSymbol *destructors = find_class_member_by_name(input, frontend_data.builtin_names.destructor);
    if (!destructors) return;
    CompilerSymbolLink *member = destructors->kind == 7 || destructors->kind == 8 ||
        destructors->kind == 6 || destructors->kind == 24 ? &((CompilerNamedSymbol *)destructors)->link :
        ((CompilerMemberGroupSymbol *)destructors)->variants;
    if (member) {
        CompilerSymbol *symbol = (CompilerSymbol *)((BYTE *)member - 8);
        if (symbol->kind == 24) symbol = ((CompilerAliasSymbol *)symbol)->target->symbol;
        finalize_class_destructor(input, symbol);
    }
}
extern "C" void __cdecl finalize_class_destructor(CompilerSymbol *, CompilerSymbol *) { __asm { int 3 } }
extern "C" int __cdecl compare_class_member_names(const void *, const void *); /*28835*/
extern "C" void __cdecl finalize_class_metadata(CompilerSymbol *input)
{
    CompilerKind4Symbol *owner = (CompilerKind4Symbol *)input;
    CompilerSymbolLink *member = owner->class_binding ? owner->class_binding->members : NULL;
    unsigned count = 0;
    while (member) {
        ++count;
        member = member->next;
    }
    if (!count) return;
    CompilerArena *arena = &frontend_bss.arenas[0]; /* actual c6500 */
    CompilerSymbolLink **index = (CompilerSymbolLink **)arena->cursor;
    arena->cursor += (count * 4 + 3) & ~3U;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    member = owner->class_binding ? owner->class_binding->members : NULL;
    CompilerSymbolLink **slot = index;
    while (member) {
        *slot++ = member;
        member = member->next;
    }
    qsort(index, count, 4, compare_class_member_names);
    CompilerGlobalClassDetail *detail = ((CompilerGlobalSymbolView *)input)->class_detail;
    detail->member_index = index;
    detail->member_count = count;
}
extern "C" int __cdecl compare_class_member_names(const void *first, const void *second)
{
    unsigned left = (unsigned)(*(CompilerSymbolLink *const *)first)->name;
    unsigned right = (unsigned)(*(CompilerSymbolLink *const *)second)->name;
    if (left < right) return -1;
    return left > right ? 1 : 0;
}
extern "C" void __cdecl start_pending_member_queue(void); /*448f2, distinct tail target*/
extern "C" void __cdecl finish_class_scope(void)
{
    if (frontend_data.declaration_context_index >= 0 || (int)frontend_bss.parser_context_depth > 0) return;
    CompilerMemberQueueState *queue = frontend_bss.pending_member_queue;
    if (!queue || !queue->head) return;
    if (frontend_bss.pending_member_error_count || suppress_parser_codegen) {
        frontend_bss.pending_member_error_count = 0;
        queue->current = queue->head = NULL;
        if (!frontend_bss.pending_member_queue->secondary)
            frontend_bss.pending_member_queue = NULL;
        return;
    }
    queue->current = queue->head;
    start_pending_member_queue();
}
extern "C" void __cdecl start_pending_member_queue(void) { __asm { int 3 } }
extern "C" void __cdecl restore_class_scope(void)
{
    if (frontend_data.declaration_context_index >= 0 || (int)frontend_bss.parser_context_depth > 0 ||
        frontend_bss.current_function) return;
    frontend_data.declaration_lookup_owner = NULL;
    frontend_bss.tag_declaration_changed = 1;
}
extern "C" ParserTokenView *__cdecl defer_parser_token(ParserTokenView *token)
{
    if (token) {
        if (token->kind == 0x76 && (int)frontend_bss.parser_context_depth > 0)
            --frontend_bss.parser_context_depth;
        ((CompilerTokenStream *)frontend_bss.saved_context_stack.top->object)->Unread((unsigned)token);
    }
    return NULL;
}
extern "C" void *__cdecl allocate_parser_storage(unsigned bytes, unsigned arena_kind)
{
    if (arena_kind) {
        if (arena_kind >= 9) { __asm { int 3 } }
        CompilerArena *arena = &frontend_bss.arenas[arena_kind];
        BYTE *result = arena->cursor;
        arena->cursor += bytes;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        return result;
    }
    CompilerPool *pool = &frontend_bss.pools[7]; /* actual bf498 */
    if (bytes > pool->remaining) pool->Refill(bytes);
    BYTE *result = pool->cursor;
    pool->cursor += bytes;
    pool->remaining -= bytes;
    return result;
}
ParserTokenView *RECON_THISCALL ParserRecordedContext::Read(BYTE kind, const TokenValue *value)
{
#ifdef __linux__
    /* This reached allocation is the declaration-token variant. Other
     * native arms need their own representation before this transfer. */
    if (kind != 0x22) recon_unsupported("recorded token: native semantic arm pending");
    ParserDeclarationToken *token = (ParserDeclarationToken *)this;
    const ParserDeclarationView *declaration = (const ParserDeclarationView *)value;
    token->kind = kind;
    memcpy(&token->value, declaration, 36);
    token->value.callback = declaration->callback;
#else
    ((ParserTokenView *)this)->kind = kind;
    memcpy((BYTE *)this + 8, value, 40);
#endif
    return (ParserTokenView *)this;
}
void RECON_THISCALL ParserDeclarationReader::CopyClassHead(void)
{
    const ParserDeclarationView *head = &((ParserDeclarationToken *)token)->value;
    copied_class_head = 1;
    type = head->type;
    type_symbol = head->object;
    declaration_callback = head->callback;
    qualifier_flags = type->attributes;
    memcpy(&context, &head->context, 28);
}
ParserDeclarationView *RECON_THISCALL ParserRecordedContext::GetDeclaration(void)
{
    return &((ParserDeclarationToken *)this)->value;
}
extern "C" CompilerSymbol *__cdecl check_symbol_definition(CompilerSymbol *);
extern "C" CompilerSymbol *__cdecl declare_parameter_symbol(CompilerDeclaration *declaration,
    CompilerSymbol *input, CompilerDeclarator *declarator)
{
    CompilerSymbol *symbol = declare_builtin(declaration, input, declarator);
    if (!symbol) return NULL;
    if (symbol->scope_flags & 4) return NULL;
    if ((((CompilerIrSymbol *)symbol)->storage_class & 7) == 1) return symbol;
    if (!check_symbol_definition(symbol) || suppress_parser_codegen ||
        (frontend_data.source_mode_flags & 0x00200000)) return symbol;
    if (symbol->definition_flags & 4) {
        begin_symbol_initializer(symbol);
        symbol = prepare_symbol_initializer(symbol);
        initialize_symbol(symbol, NULL);
    }
    finish_symbol_initializer(symbol);
    return symbol;
}
extern "C" void __cdecl prepare_specialized_class(CompilerSymbol *) { __asm { int 3 } }
void RECON_THISCALL CompilerTranslationUnit::RestoreSavedLookup(unsigned copy_scopes)
{
    if (!saved_lookup) return;
    current = saved_lookup->current;
    depth = saved_lookup->depth;
    if (!copy_scopes) return;
    if (depth >= 128) { __asm { int 3 } }
    for (unsigned index = 1; index <= depth; ++index)
        scopes[index] = saved_lookup->scopes[index - 1];
    retired_scopes = saved_lookup->retired;
    lookup_context = saved_lookup->lookup_context;
    opaque_lookup_state = saved_lookup->opaque_lookup_state;
    suppress_lookup_errors = saved_lookup->suppress_lookup_errors;
    lookup_override = saved_lookup->lookup_override;
}
void RECON_THISCALL CompilerTranslationUnit::PopSavedLookup(void)
{
    saved_lookup = saved_lookup->previous;
}
int RECON_THISCALL CompilerDependency::CompleteClass(CompilerSymbol *) { __asm { int 3 } return 0; }

extern "C" CompilerSymbol *__cdecl make_parameter_sentinel(void);
extern "C" CompilerParameterDescription *__cdecl make_empty_function_parameters(void)
{
    CompilerArena *arena = &frontend_bss.arenas[5];
    BYTE *memory = arena->cursor;
    arena->cursor += 28;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    CompilerParameterDescription *parameters = (CompilerParameterDescription *)memory;
    if (parameters) {
        parameters->kind = 5;
        parameters->scope_open = 0;
        parameters->parameters = NULL;
    }
    if (!frontend_data.parameter_sentinel)
        frontend_data.parameter_sentinel = make_parameter_sentinel();
    parameters->AppendEmptySymbol(frontend_data.parameter_sentinel);
    return parameters;
}
void RECON_THISCALL CompilerParameterDescription::AppendEmptySymbol(CompilerSymbol *symbol)
{
    CompilerParameterSet *set;
    CompilerSymbolLink *link;
    CompilerArena *arena;
    CompilerPool *pool;
    BYTE *memory;
    unsigned arena_index;
    if (!parameters) {
        arena_index = (frontend_data.source_mode_flags & 0x48) ? 0 :
            (frontend_data.declaration_context_index < 0) + 3;
        if (!arena_index && frontend_data.free_parameter_sets) {
            set = frontend_data.free_parameter_sets;
            frontend_data.free_parameter_sets = set->next_free;
            set->arena_index = 0;
            set->head = set->tail = NULL;
        } else {
            if (arena_index) {
                arena = &frontend_bss.arenas[arena_index];
                memory = arena->cursor;
                arena->cursor += 12;
                if (arena->cursor > arena->committed_limit) arena->CommitExtent();
            } else {
                pool = &frontend_bss.pools[8]; /* actual c0b00 */
                if (pool->remaining < 12) pool->Refill(12);
                memory = pool->cursor;
                pool->cursor += 12;
                pool->remaining -= 12;
            }
            set = (CompilerParameterSet *)memory;
            if (set) {
                set->arena_index = arena_index;
                set->head = set->tail = NULL;
            }
        }
        parameters = set;
    }
    set = parameters;
    link = &((CompilerNamedSymbol *)symbol)->link;
    if (link) {
        link->name = link->name;
        link->next = NULL;
        ((CompilerParameterSymbol *)symbol)->previous_parameter = set->tail;
    }
    if (!set->tail) set->head = set->tail = link;
    else {
        if (!frontend_data.parameter_sentinel)
            frontend_data.parameter_sentinel = make_parameter_sentinel();
        if ((CompilerSymbol *)((BYTE *)set->tail - offsetof(CompilerNamedSymbol, link)) ==
            frontend_data.parameter_sentinel) set->head = set->tail = link;
        else { set->tail->next = link; set->tail = link; }
    }
}

extern "C" void __cdecl print_compiler_statistics(void);
extern "C" void __cdecl flush_declaration_work(void); /*f139*/

extern "C" void __cdecl finish_external_declaration(void)
{
    CompilerTranslationUnit *unit;
    CompilerTokenStream *stream;
    if (frontend_data.report_statistics) print_compiler_statistics();
    if (!current_recovery_shifts()) frontend_bss.arenas[5].ResetStorage();
    frontend_bss.declaration_controls[0].flags &= ~3U;
    frontend_data.compiler_mode_flags &= ~2U;
    frontend_data.compiler_flags &= 0xfe7ffdfeU;
    frontend_data.token_text_cursor = compiler_token_text;
    frontend_bss.current_parameters = NULL;
    frontend_bss.active_parameters = NULL;
    frontend_bss.current_parameter_kind = 0;
    frontend_bss.active_parameter_kind = 0;
    frontend_data.declaration_cleanup_state[0] = 0;
    frontend_data.declaration_cleanup_state[1] = 0;
    frontend_data.declaration_cleanup_state[2] = 0;
    frontend_bss.parser_context_depth = 0;
    frontend_bss.parser_qualification_scopes[0] = NULL;
    if (suppress_parser_codegen) return;
    if ((frontend_data.compiler_flags & 0x20000) &&
        !(frontend_data.compiler_flags & 0x20)) return;
    unit = frontend_data.translation_unit;
    if (unit->current != unit->root) {
        if (unit->current->level) {
            do {
                unit->current = unit->current->next;
                --unit->depth;
            } while (unit->current->level);
        }
        unit->retired_scopes = NULL;
        unit->lookup_context = 1;
        unit->opaque_lookup_state = 0;
        unit->suppress_lookup_errors = 0;
        unit->lookup_override = 0;
    }
    stream = (CompilerTokenStream *)frontend_bss.saved_context_stack.top->object;
    if (!stream->Valid() || frontend_data.recursion_depth) return;
    if (frontend_data.compiler_flags & 0x20) select_code_segment(1);
    if (!has_pending_parser_input()) {
        frontend_bss.arenas[3].ResetStorage();
        frontend_bss.arenas[2].ResetStorage();
        if (!current_recovery_shifts()) frontend_bss.arenas[4].ResetStorage();
    }
    flush_declaration_work();
}

extern "C" int __cdecl current_recovery_shifts(void)
{
    return frontend_data.active_parser->recovery_shifts;
}

extern "C" int __cdecl process_pending_parser_input(void); /*dba7*/
extern "C" int __cdecl has_pending_parser_input(void)
{
    int result;
    if (suppress_parser_codegen || frontend_bss.current_function) return 0;
    result = process_pending_parser_input();
    if (!result) return 0;
    if (result == 2000) {
        report_compiler_error(0xca);
        return 0;
    }
    return 1;
}

extern "C" {
int __cdecl prepare_pending_parser_input(void); /*dec1*/
int __cdecl prepare_deferred_parser_input(void); /*defb*/
void __cdecl initialize_pending_input_state(void); /*3d511*/
void __cdecl finalize_pending_input_state(void); /*3e8c9*/
void __cdecl flush_pending_translation_symbols(void); /*47f5d*/
void __cdecl resume_pending_parser_input(void); /*2254f*/
}

extern "C" int __cdecl process_pending_parser_input(void)
{
    FrontendContextStack *stack;
    FrontendContextFrame *frame;
    CompilerTokenStream *stream;
    FrontendContextPopCallback callback;
    CompilerBackend *saved_backend;
    void *saved_origin;
    unsigned saved_position;
    int saved_processing, saved_mode, result, i;
    if (frontend_bss.error_count) {
        stack = &frontend_bss.nested_context_stacks[3];
        while (stack->top) {
            frame = stack->top;
            stack->top = frame->previous;
            stack->origin = frame->saved_origin;
            stack->position = frame->saved_position;
            callback = frame->callback;
            if (callback) callback();
        }
        stack = &frontend_bss.nested_context_stacks[2];
        while (stack->top) {
            frame = stack->top;
            stack->top = frame->previous;
            stack->origin = frame->saved_origin;
            stack->position = frame->saved_position;
            callback = frame->callback;
            if (callback) callback();
        }
        return 0;
    }
    stack = &frontend_bss.nested_context_stacks[3];
    if (stack->top) {
        ((CompilerTokenStream *)stack->top->object)->Trace(
            (unsigned)frontend_data.canonical_token_records[243]);
        frame = stack->top;
        stream = NULL;
        if (frame) {
            stack->top = frame->previous;
            stream = (CompilerTokenStream *)frame->object;
            stack->origin = frame->saved_origin;
            stack->position = frame->saved_position;
            callback = frame->callback;
            if (callback) callback();
        }
        frontend_bss.saved_context_stack.Push(stream, 3, 1,
            resume_pending_parser_input, &frontend_bss.saved_context_stack);
        return 1;
    }
    stack = &frontend_bss.nested_context_stacks[2];
    if (stack->top) {
        ((CompilerTokenStream *)stack->top->object)->Trace(
            (unsigned)frontend_data.canonical_token_records[243]);
        frame = stack->top;
        stream = NULL;
        if (frame) {
            stack->top = frame->previous;
            stream = (CompilerTokenStream *)frame->object;
            stack->origin = frame->saved_origin;
            stack->position = frame->saved_position;
            callback = frame->callback;
            if (callback) callback();
        }
        frontend_bss.saved_context_stack.Push(stream, 2, 1,
            resume_pending_parser_input, &frontend_bss.saved_context_stack);
        return ++frontend_data.deferred_input_depth;
    }
    stack = &frontend_bss.nested_context_stacks[0];
    if (stack->top) {
        ((CompilerTokenStream *)stack->top->object)->Trace(
            (unsigned)frontend_data.canonical_token_records[243]);
        frame = stack->top;
        stream = NULL;
        if (frame) {
            stack->top = frame->previous;
            stream = (CompilerTokenStream *)frame->object;
            stack->origin = frame->saved_origin;
            stack->position = frame->saved_position;
            callback = frame->callback;
            if (callback) callback();
        }
        frontend_bss.saved_context_stack.Push(stream, 0, 1,
            resume_pending_parser_input, &frontend_bss.saved_context_stack);
        return 1;
    }
    saved_origin = frontend_bss.saved_context_stack.origin;
    saved_position = frontend_bss.saved_context_stack.position;
    saved_backend = frontend_data.backend;
    frontend_data.backend = acquire_initial_backend();
    saved_processing = frontend_bss.processing_pending_parser_input;
    saved_mode = (frontend_data.compiler_mode_flags >> 7) & 1;
    frontend_bss.processing_pending_parser_input = 1;
    frontend_data.compiler_mode_flags |= 0x80;
    if ((frontend_data.translation_symbols && frontend_data.translation_symbols->count) ||
        (frontend_data.secondary_translation_symbols && frontend_data.secondary_translation_symbols->count) ||
        frontend_bss.pending_parser_work) {
        flush_pending_translation_symbols();
        if (frontend_data.translation_symbols) {
            frontend_data.translation_symbols->count = 0;
            for (i = frontend_data.translation_symbols->capacity - 1; i >= 0; --i)
                frontend_data.translation_symbols->items[i] = NULL;
        }
        if (frontend_data.secondary_translation_symbols) {
            frontend_data.secondary_translation_symbols->count = 0;
            for (i = frontend_data.secondary_translation_symbols->capacity - 1; i >= 0; --i)
                frontend_data.secondary_translation_symbols->items[i] = NULL;
        }
    }
    frontend_bss.pending_parser_work = 0;
    if (prepare_pending_parser_input()) {
        initialize_pending_input_state();
        finalize_pending_input_state();
        result = process_pending_parser_input();
    } else if (prepare_deferred_parser_input()) result = process_pending_parser_input();
    else result = 0;
    frontend_data.backend = saved_backend;
    frontend_bss.processing_pending_parser_input = saved_processing;
    if (!saved_mode) frontend_data.compiler_mode_flags &= ~0x80U;
    frontend_bss.saved_context_stack.origin = saved_origin;
    frontend_bss.saved_context_stack.position = saved_position;
    return result;
}

extern "C" void __cdecl initialize_pending_declaration_cursor(PendingDeclarationCursor *, void *); /*18ee7*/
extern "C" int __cdecl next_pending_declaration(PendingDeclarationCursor *); /*18ef8*/
extern "C" int __cdecl prepare_pending_parser_input(void)
{
    PendingDeclarationCursor cursor;
    if (frontend_bss.error_count || !frontend_data.pending_declarations) return 0;
    if (frontend_data.translation_unit->current == frontend_data.translation_unit->root) return 1;
    initialize_pending_declaration_cursor(&cursor, frontend_data.pending_declarations);
    while (next_pending_declaration(&cursor)) {
        switch ((signed char)((CompilerRuntimeFunction *)cursor.current)->builtin_operation) {
        case 0:
        case 11: case 12: case 13:
        case 14: case 15: case 16:
            break;
        default:
            return 1;
        }
    }
    return 0;
}

extern "C" int __cdecl prepare_deferred_parser_input(void)
{
    ParserInputEndState *state = parser_input_end_state;
    ParserClassDefinitionLink *class_link;
    ParserDeferredDefinition *definition;
    CompilerGlobalSymbolView *owner;
    int class_recorded, any_recorded = 0;
    unsigned complete;
    if (!state || !state->reached_end) return 0;
    if (frontend_data.class_definition_head) {
        class_link = *frontend_data.class_definition_head;
        while (class_link) {
            class_recorded = 0;
            definition = class_link->definitions;
            while (definition) {
                owner = (CompilerGlobalSymbolView *)class_link->symbol;
                complete = (((ParserDefinitionClassView *)owner->class_detail)->completion_flags >> 4) & 1;
                if (!(definition->symbol->definition_flags & 4) &&
                    ((definition->symbol->emission_state & 3) ||
                     (definition->symbol->function_flags & 2) || complete)) {
                    frontend_bss.nested_context_stacks[0].Push(definition->stream,
                        0, 1, NULL, &frontend_bss.saved_context_stack);
                    class_recorded = 1;
                }
                definition = definition->next;
            }
            if (class_recorded) any_recorded = 1;
            class_link = class_link->next;
        }
        state = parser_input_end_state;
    }
    if (any_recorded) return 1;
    state->Finish();
    parser_input_end_state = NULL;
    return 1;
}

extern "C" void __cdecl reset_expression_work(void); /*654f*/
extern "C" void __cdecl flush_declaration_work(void)
{
    if (frontend_data.expression_work_current || frontend_data.expression_work_head)
        reset_expression_work();
}

typedef char CompilerDeclaratorQualifiers_layout[(sizeof(CompilerDeclaratorQualifiers) == 8 &&
    offsetof(CompilerDeclaratorQualifiers, value) == 4) ? 1 : -1];
void RECON_THISCALL CompilerDeclaratorQualifiers::Set(WORD kind, unsigned argument)
{
    if (kind == 3 && !argument) kind = 0;
    value = argument;
    flags = kind;
}
void RECON_THISCALL CompilerDeclaratorQualifiers::Clear(void)
{
    /* The two original DWORD stores clear all eight storage bytes, including
     * the uninterpreted WORD gap between flags and value. */
    memset(this, 0, sizeof(*this));
}

CompilerDeclaratorQualifiers *RECON_THISCALL CompilerDeclaratorQualifiers::Initialize(WORD initial_flags)
{
    flags = initial_flags;
    value = 0;
    return this;
}
void RECON_THISCALL ParserTypeModifier::Pointer(
    CompilerDeclaratorQualifiers *left, CompilerDeclaratorQualifiers *right)
{
    kind = 1;
    flags = left->flags;
    qualifiers = right->flags;
    value = left->value;
}

void RECON_THISCALL ParserTypeModifier::Reference(
    CompilerDeclaratorQualifiers *left, CompilerDeclaratorQualifiers *right)
{
    if (right->flags) report_language_diagnostic(1, 0xe3);
    kind = 0x40;
    qualifiers = 0;
    flags = left->flags;
    value = 0;
}

void RECON_THISCALL CompilerDeclarator::AppendModifier(ParserTypeModifier *modifier)
{
    WORD attributes = 0;
    CompilerTypeShape *next;
    switch (modifier->kind) {
    case 1:
        if (modifier->flags & 0x70) shape_flags |= 1;
        attributes = modifier->flags | modifier->qualifiers;
        break;
    case 16:
        attributes = MergeModifierQualifiers(modifier->flags, modifier->qualifiers);
        break;
    case 64:
        attributes = ValidateReferenceQualifiers(modifier->flags);
        break;
    }
    /* The actual shape constructor consumes the low WORDs of kind/attributes
     * and the DWORD value only, never the modifier's uninterpreted gap. */
    next = create_type_shape(modifier->kind, attributes, &modifier->value);
    if (modifier->kind == 64) ValidateReferenceShape(next);
    if (last_shape) {
        last_shape->base = next;
        last_shape = next;
    } else {
        last_shape = next;
        shape = next;
    }
}

void RECON_THISCALL CompilerTranslationUnit::BeginParameterScope(CompilerParameterSet *parameters)
{
    CompilerArena *arena = &frontend_bss.arenas[4]; /* actual c6580 */
    BYTE *memory = arena->cursor;
    arena->cursor += 24;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    CompilerAuxiliaryScope *scope = memory ? new(memory) CompilerAuxiliaryScope(parameters, 1) : NULL;
    if (parameters) frontend_bss.active_parameters = parameters;
    ActivateClassScope(scope);
}

void RECON_THISCALL CompilerDeclarator::AddArrayShape(CompilerTypeShape *next)
{
    CompilerDeclaration *declaration;
    unsigned last_kind;
    if (!shape) return;
    if (!next->payload.value) {
        last_kind = last_shape->flags & 0x77;
        if (last_kind == 2 || (last_shape->flags & 0x40)) {
            if (last_kind != 2) compiler_warning(0x109, symbol);
            else {
                unsigned depth = frontend_bss.parser_context_depth;
                declaration = (declaration_control_row(depth)->flags & 1) ?
                    declaration_spec_row(depth) : default_declaration(0);
                if (declaration->context.reference_flags & 0x80) return;
                if (!(frontend_data.source_mode_flags & 0x100000))
                    compiler_warning(0x57, symbol);
            }
            flags |= 1;
            next->payload.value = 1;
        }
    }
    if ((last_shape->flags & 0x77) == 4) {
        compiler_warning(0x5a, symbol);
        flags |= 1;
        next->flags = 1;
        next->payload.value = 0;
    }
    if ((last_shape->flags & 0x40) && (next->flags & 0x1000)) {
        compiler_warning(0x10a, symbol);
        flags |= 1;
    }
}

extern "C" unsigned __cdecl merge_declaration_qualifiers(unsigned first, unsigned second)
{
    if (first & second & 0x1f00) {
        if (frontend_data.options.language_extensions)
            report_language_diagnostic(1, 0x72);
        else compiler_warning(0x121);
    }
    unsigned difference = first ^ second;
    if ((difference & 3) && (first & 3) && (second & 3))
        compiler_warning(0x121);
    if ((difference & 0x70) && (first & 0x70) && (second & 0x70)) {
        compiler_warning(0x97);
        first &= 0xff8f;
    }
    return first | second;
}
int RECON_THISCALL ParserDeclarationReader::ReadQualifier(void)
{
    if (!(frontend_data.compiler_mode_flags & 8) &&
        ((declaration_control_row(frontend_bss.parser_context_depth)->flags & 2) ||
         (frontend_data.compiler_flags & 4)) && !copied_class_head)
        return 2;
    unsigned kind = token->kind;
    if (kind >= PARSER_TOKEN_KIND_COUNT) { __asm { int 3 } }
    qualifier_flags = merge_declaration_qualifiers(qualifier_flags,
        (WORD)parser_token_semantic_codes[kind]);
    return 0;
}

/* Kind5 has its own40-byte allocation. The opaque DWORD between the common
 * header and low32-bit enumerator value is retained without interpretation. */
struct CompilerEnumeratorSymbol : CompilerIrSymbol {
    BYTE unmodeled_20_23[4];
    int value;
};
typedef char CompilerEnumeratorSymbol_layout[(sizeof(CompilerEnumeratorSymbol) == 40 &&
    offsetof(CompilerEnumeratorSymbol, value) == 36) ? 1 : -1];
extern "C" CompilerSymbol *__cdecl recycle_symbol(unsigned, CompilerNameEntry *, unsigned);
extern "C" void __cdecl add_enumerator(CompilerNameEntry *name, CompilerType *type, __int64 value)
{
    CompilerIrSymbol *symbol;
    CompilerSymbol *previous;
    CompilerScopeLookup *found;
    CompilerScopeMatch *selected, *merged = NULL;
    CompilerArena *arena;
    CompilerHashTable *members;
    BYTE *memory;
    unsigned arena_kind = 0, width;
    union { __int64 whole; struct { unsigned low; int high; } words; } integer;
    int index;
    if (!type) return;
    integer.whole = value;
    if (name->flags & 0x02000000) {
        selected = frontend_data.translation_unit->LookupName(name, 0x20);
        compiler_warning(0x3a2, selected ? selected->symbol : NULL, current_declaration_symbol);
        return;
    }
    if ((int)frontend_bss.parser_context_depth <= 0 || (LEXER_FLAGS & 0x20000)) {
        if (frontend_bss.declaration_scope_mode && frontend_data.declaration_context_index < 0 &&
            !(frontend_data.compiler_mode_flags & 0x10000000) &&
            (!(frontend_data.compiler_mode_flags & 0x40000000) ||
             (frontend_data.source_mode_flags & 8) || (LEXER_FLAGS & 0x200)) &&
            !(frontend_data.source_mode_flags & 0x50)) arena_kind = 4;
    }
    symbol = (CompilerIrSymbol *)recycle_symbol(5, name, arena_kind);
    if (!symbol) {
        arena = &frontend_bss.arenas[arena_kind];
        memory = arena->Allocate((frontend_data.symbol_record_bytes[5] + 3) & ~3U);
        symbol = memory ? (CompilerIrSymbol *)((CompilerSymbol *)memory)->Initialize(5, name, arena_kind) : NULL;
    }
    symbol->type = type;
    if ((LEXER_FLAGS & 0x20000) || frontend_data.pch_separate_debug ||
        (current_declaration_symbol->scope_flags & 1)) symbol->definition_flags &= ~8;
    else symbol->definition_flags |= 8;
    found = frontend_data.translation_unit->current->Find(symbol->link.name, 0);
    if (found) {
        selected = found->Select(1);
        if (selected) {
            previous = merge_scoped_declaration(selected->symbol, symbol, 0);
            if (previous) {
                merged = (CompilerScopeMatch *)frontend_bss.arenas[5].Allocate(12);
                if (merged) merged = merged->Initialize(previous, NULL);
            }
        }
    } else {
        previous = merge_scoped_declaration(NULL, symbol, 0);
        if (previous) {
            merged = (CompilerScopeMatch *)frontend_bss.arenas[5].Allocate(12);
            if (merged) merged = merged->Initialize(previous, NULL);
        }
    }
    previous = merged ? merged->symbol : NULL;
    if (previous) {
        compiler_warning(0x155, previous);
        symbol = (CompilerIrSymbol *)previous;
    } else {
        if (is_special_member_name(symbol->link.name)) compiler_warning(0x1ca, symbol);
        else frontend_data.translation_unit->current->Insert(symbol);
    }
    members = ((ParserEnumSymbolView *)current_declaration_symbol)->members;
    if (!members) return;
    insert_hash_entry(members, name, symbol);
    if (!(symbol->encoded_index & 0xffffff)) {
        if (frontend_data.next_symbol_record_id >= 0x7fffffff) report_compiler_error(0x37);
        symbol->encoded_index = (symbol->encoded_index & 0xff000000U) |
            (frontend_data.next_symbol_record_id++ & 0xffffff);
    }
    width = value < (-2147483647 - 1) || value > 2147483647 ? 8 : 4;
    if ((int)width > compiler_primitive_type_bytes[3] &&
        integer.words.high != 0)
        report_language_diagnostic(frontend_data.options.language_extensions ? 3 : 1, 0x155, symbol);
    if (symbol->kind != 5 || frontend_data.symbol_record_bytes[5] < 40) { __asm { int 3 } }
    ((CompilerEnumeratorSymbol *)symbol)->value = (int)integer.words.low;
    enumerator_value = value;
    index = frontend_data.declaration_context_index;
    if (index >= 0) {
        if (index >= 16) { __asm { int 3 } }
        symbol->definition_flags = (symbol->definition_flags & ~3) |
            (frontend_bss.class_scope_rows[index].access_flags & 3);
    }
    if (frontend_bss.browser_filename &&
        (!frontend_bss.declaration_scope_mode || publish_browser_local))
        frontend_data.backend->Slot20(symbol);
}

extern "C" CompilerType *__cdecl current_enumerator_type(void)
{
    CompilerType *type = current_enum_type;
    CompilerPointerArray *owners = frontend_data.frontend_pointer_table;
    CompilerSymbol *owner;
    int index;
    if (!type || !owners) { __asm { int 3 } }
    index = (short)type->auxiliary;
    if (index < 0 || index >= owners->count) { __asm { int 3 } }
    owner = (CompilerSymbol *)owners->items[index];
    if (!(owner->scope_flags & 1) && frontend_bss.browser_filename &&
        (!frontend_bss.declaration_scope_mode || publish_browser_local))
        frontend_data.backend->Slot08();
    /* Browser callbacks may change the active enum; this is a fresh load. */
    return current_enum_type;
}

extern "C" int __cdecl scan_recorded_declaration_body(void);
extern "C" ParserTokenView *__cdecl prepare_deferred_declaration_token(void);
extern "C" void __cdecl begin_declaration_scope(void)
{
    record_declaration_body(NULL);
    LEXER_FLAGS |= 0x08000008;
    if (!scan_recorded_declaration_body()) {
        frontend_data.compiler_mode_flags &= ~0x08000000U;
        frontend_data.source_mode_flags |= 0x10;
        frontend_data.tagged_type_arena_index = 0;
        frontend_data.constant_arena_index = 0;
        return;
    }
    ParserTokenView *token = prepare_deferred_declaration_token();
    TOKEN_STREAM->Unread((unsigned)token);
}

/* Look ahead in a copied recorded stream. The original cursor is restored
 * on every return; only literal tokens deliberately rewritten in shared
 * storage survive the temporary context. */
extern "C" int __cdecl scan_recorded_declaration_body(void)
{
    CompilerRecordedLexer *lexer;
    ParserTokenView *token;
    CompilerNameEntry *name;
    CompilerSymbol *symbol;
    FrontendContextFrame *frame;
    FrontendContextPopCallback callback;
    int parentheses = 0, brackets = 0, found = 0, before_separator = 1;
    unsigned kind;
    lexer = (CompilerRecordedLexer *)frontend_bss.arenas[5].Allocate(64);
    if (lexer) lexer = lexer->CopyFrom((CompilerRecordedLexer *)frontend_bss.saved_context_stack.top->object);
    frontend_bss.saved_context_stack.Push(lexer, 5, 0, NULL, &frontend_bss.saved_context_stack);
    while (!found && parentheses >= 0) {
        token = TOKEN_CONTEXT.ReadToken();
        kind = token->kind;
        if (kind >= PARSER_TOKEN_KIND_COUNT) { __asm { int 3 } }
        if (parser_token_classes[kind] == 0x65) {
            /* Numeric-token production supplies the compatible literal arm:
             * type at+0, opaque DWORD+4 and QWORD value at+8. */
            saved_parser_value.expression = make_typed_constant(
                TOKEN_SEMANTIC(token).literal.type,
                PARSER_LITERAL_DATA(TOKEN_SEMANTIC(token)), 0);
            token->kind = 0xb6;
            memcpy(&token->value, &saved_parser_value, 40);
            continue;
        }
        switch (kind) {
        case 1: case 244:
            parentheses = -1;
            break;
        case 6: case 223: case 234:
            if (before_separator) {
                name = kind == 234 ? TOKEN_SEMANTIC(token).indirect_name->name : TOKEN_SEMANTIC(token).name;
                symbol = find_declared_member(name);
                if (!symbol || symbol->kind != 5) found = 1;
            }
            break;
        case 30: case 150: case 152: case 170:
            if (before_separator) found = 1;
            break;
        case 52:
            before_separator = 1;
            break;
        case 55:
            if (!parentheses && !brackets) before_separator = 0;
            break;
        case 65:
            ++brackets;
            break;
        case 118:
            ++parentheses;
            break;
        case 122:
            --brackets;
            break;
        case 126:
            --parentheses;
            break;
        default:
            break;
        }
    }
    frame = frontend_bss.saved_context_stack.top;
    if (frame) {
        frontend_bss.saved_context_stack.top = frame->previous;
        frontend_bss.saved_context_stack.origin = frame->saved_origin;
        frontend_bss.saved_context_stack.position = frame->saved_position;
        callback = frame->callback;
        if (callback) callback();
    }
    return found;
}

int RECON_THISCALL ParserFrontendContextView::HasSourceMark(void)
{
    return ((CompilerRecordedLexer *)top->object)->active;
}

extern "C" BYTE *__cdecl allocate_parser_record(unsigned bytes, unsigned arena_kind)
{
    CompilerArena *arena;
    BYTE *memory;
    if (arena_kind >= 9) { __asm { int 3 } }
    arena = &frontend_bss.arenas[arena_kind];
    memory = arena->cursor;
    arena->cursor += (bytes + 3) & ~3U;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    return memory;
}

extern "C" void __cdecl finish_deferred_input(void)
{
    unsigned flags = frontend_data.compiler_mode_flags;
    unsigned mode = frontend_data.source_mode_flags;
    unsigned compiler_flags = frontend_data.compiler_flags;
    if ((flags & 0x32000000) ||
        ((flags & 0x40000000) && !(mode & 8) && !(compiler_flags & 0x200)) ||
        (mode & 0x10) || ((flags & 0x4000000) && !(compiler_flags & 0x8000))) {
        frontend_data.tagged_type_arena_index = 0;
        frontend_data.constant_arena_index = 0;
    } else {
        frontend_data.tagged_type_arena_index = 5;
        frontend_data.constant_arena_index = 5;
        if (mode & 1) {
            frontend_data.tagged_type_arena_index = 4;
            frontend_data.constant_arena_index = 4;
        }
    }
}

extern "C" int __cdecl merge_default_declaration_context(CompilerDeclarationContext *, CompilerDeclarationContext);
extern "C" CompilerDeclaration *__cdecl default_declaration(const CompilerDeclarationContext *context)
{
    ParserDeclarationView *declaration = declaration_spec_row(DECL_DEPTH);
    declaration->type = frontend_data.builtin_types.type_0063;
    memset(&declaration->context, 0, 28);
    declaration->context.class_flags |= 0x30;
    if (context) merge_default_declaration_context(&declaration->context, *context);
    declaration_control_row(DECL_DEPTH)->flags |= 1;
    return declaration_spec_row(DECL_DEPTH);
}

extern "C" {
ParserTokenView *__cdecl current_declaration_lookahead(void);
void __cdecl prepare_function_source(CompilerSymbol *);
void __cdecl record_function_definition(CompilerSymbol *, CompilerTokenStream *, unsigned);
void __cdecl set_declaration_lookahead(ParserTokenView *);
unsigned __cdecl make_arena_token(unsigned);
unsigned __cdecl debug_function_name(CompilerNameEntry *, CompilerGlobalSymbolView *);
}
extern "C" void __cdecl process_merged_declaration(CompilerSymbol *input, unsigned allow_missing_body)
{
    CompilerIrSymbol *symbol = (CompilerIrSymbol *)input;
    ParserTokenView *lookahead, *last;
    CompilerRecordedLexer *stream;
    CompilerSymbolArenaFrame *saved;
    CompilerArena *arena;
    FrontendContextStack *stack;
    ParserClassDefinitionLink *class_link;
    ParserDeferredDefinition *definition;
    unsigned arena_kind, options, debug_result = 0;
    BYTE *memory;
    if (input->scope_flags & 4) {
        frontend_data.compiler_mode_flags &= ~0x01000000U;
        return;
    }
    if (frontend_data.compiler_mode_flags & 0x01000000) {
        if (!(frontend_data.source_mode_flags & 0x100008))
            ((CompilerMemberQueue *)&frontend_bss.pending_member_queue)->Add(input);
        frontend_data.compiler_mode_flags &= ~0x01000000U;
    }
    if (frontend_data.source_mode_flags & 0x200040) return;
    if (!current_declaration_lookahead())
        set_declaration_lookahead(TOKEN_CONTEXT.ReadToken());
    lookahead = current_declaration_lookahead();
    switch (lookahead->kind) {
    case 52:
        return;
    case 55: case 127:
        if (frontend_bss.current_function && frontend_data.declaration_context_index >= 0 &&
            !(symbol->function_flags & 4) && !allow_missing_body)
            compiler_warning(0x257, input);
        return;
    case 54: case 117:
        break;
    default:
        compiler_warning(0x3b, 0x4c, lookahead->kind);
        return;
    }
    if (frontend_data.source_mode_flags & 0x100000) {
        process_external_recovery(lookahead->kind == 54);
        TOKEN_CONTEXT.ReadToken();
        input->definition_flags |= 4;
        set_declaration_lookahead(parser_semicolon);
        return;
    }
    if (!frontend_bss.current_function) symbol->function_flags |= 1;
    else if (frontend_data.declaration_context_index < 0) compiler_warning(0x259, input);
    else symbol->function_flags &= ~1U;
    prepare_function_source(input);
    TOKEN_STREAM->Unread((unsigned)lookahead);
    if (frontend_data.source_mode_flags & 8) {
        arena_kind = 0;
        arena = &frontend_bss.arenas[0];
        memory = arena->cursor;
        arena->cursor += 64;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        stream = (CompilerRecordedLexer *)memory;
        if (stream) new(stream) CompilerRecordedLexer(frontend_bss.saved_context_stack.origin,
            frontend_bss.saved_context_stack.position, 0);
        memory = arena->cursor;
        arena->cursor += 8;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        saved = (CompilerSymbolArenaFrame *)memory;
        if (saved) {
            saved->kind = frontend_bss.symbol_arena_kind;
            saved->previous = frontend_bss.saved_symbol_arenas;
        }
        frontend_bss.saved_symbol_arenas = saved;
        frontend_bss.symbol_arena_kind = 0;
    } else {
        /* The original bit arithmetic selects arena2 or3. */
        arena_kind = (frontend_data.compiler_mode_flags & 0x40000000) ? 2 : 3;
        arena = &frontend_bss.arenas[arena_kind];
        memory = arena->cursor;
        arena->cursor += 8;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        saved = (CompilerSymbolArenaFrame *)memory;
        if (saved) {
            saved->kind = frontend_bss.symbol_arena_kind;
            saved->previous = frontend_bss.saved_symbol_arenas;
        }
        frontend_bss.saved_symbol_arenas = saved;
        frontend_bss.symbol_arena_kind = arena_kind;
        stack = &frontend_bss.nested_context_stacks[arena_kind];
        if (!stack->top) {
            stream = (CompilerRecordedLexer *)arena->Allocate(64);
            if (stream) stream = stream->Initialize(arena_kind, 0);
            stack->Push(stream, arena_kind, 0, NULL, &frontend_bss.saved_context_stack);
        }
        stream = (CompilerRecordedLexer *)stack->top->object;
    }
    stream->Trace(make_arena_token(arena_kind));
    record_function_definition(input, stream, arena_kind);
    if (symbol->function_flags & 4)
        input->declaration_qualifiers = member_class_row().symbol->class_detail->scope_number;
    if (input->kind != 7 && input->kind != 8) compiler_warning(0x3f, input);
    options = ((input->type_flags & 0x10) && lookahead->kind == 54) ? 0x242 : 0x2c2;
    if (((CompilerRecordedLexer *)frontend_bss.saved_context_stack.top->object)->active) {
        if (!frontend_bss.debug_module->Slot8c(frontend_bss.saved_context_stack.position,
                frontend_bss.lexical_skip_mode))
            report_compiler_diagnostic(0x49, ".\\funcs.c", 0x9e7);
    }
    last = stream->RecordUntil(options, 1);
    if (((CompilerRecordedLexer *)frontend_bss.saved_context_stack.top->object)->active) {
        if (!frontend_bss.debug_module->Slot90(frontend_bss.saved_context_stack.position,
                frontend_bss.lexical_skip_mode, &debug_result))
            report_compiler_diagnostic(0x49, ".\\funcs.c", 0x9ef);
    }
    if (frontend_data.options.incremental_rebuild && debug_result) {
        CompilerGlobalSymbolView *owner = member_class_row().symbol;
        unsigned name = debug_function_name(symbol->link.name, owner);
        if (!frontend_bss.debug_module->Slot94((unsigned)owner->class_detail->declaration_reference, name))
            report_compiler_diagnostic(0x49, ".\\funcs.c", 0x9f8);
    }
    if (last->kind != 123)
        compiler_warning((frontend_data.compiler_mode_flags & 0x40000000) ? 0x3c2 : 0x3c9, last->kind);
    if (frontend_data.compiler_mode_flags & 0x40000000)
        stream->Trace((unsigned)parser_end_marker);
    saved = frontend_bss.saved_symbol_arenas;
    frontend_bss.symbol_arena_kind = saved->kind;
    frontend_bss.saved_symbol_arenas = saved->previous;
    if (frontend_data.source_mode_flags & 8) {
        CompilerSymbol *owner = (CompilerSymbol *)member_class_row().symbol;
        class_link = *frontend_data.class_definition_head;
        while (class_link && class_link->symbol != owner) class_link = class_link->next;
        if (!class_link) { __asm { int 3 } }
        memory = frontend_bss.arenas[0].cursor;
        frontend_bss.arenas[0].cursor += 12;
        if (frontend_bss.arenas[0].cursor > frontend_bss.arenas[0].committed_limit)
            frontend_bss.arenas[0].CommitExtent();
        definition = (ParserDeferredDefinition *)memory;
        if (definition) {
            definition->symbol = symbol;
            definition->stream = stream;
            definition->next = class_link->definitions;
        }
        class_link->definitions = definition;
    }
    set_declaration_lookahead(parser_semicolon);
}

extern "C" void __cdecl set_declaration_lookahead(ParserTokenView *token)
{
    frontend_data.active_parser->SetLookahead(token);
}

extern "C" void __cdecl prepare_function_source(CompilerSymbol *symbol)
{
    int was_defined;
    if (!frontend_bss.browser_filename) return;
    if (frontend_bss.declaration_scope_mode && !publish_browser_local) return;
    if (member_class_row().symbol->scope_flags & 1) return;
    was_defined = (symbol->definition_flags >> 2) & 1;
    symbol->definition_flags |= 4;
    frontend_data.backend->Slot20(symbol);
    if (!was_defined) symbol->definition_flags &= ~4;
}

struct CompilerRecordedDeclarationState {
    DWORD cleanup[3];
    DWORD reset_value;
    CompilerRecordedParameterState parameter_state;
};
extern "C" void __cdecl set_function_parameters(CompilerRecordedParameterState *state)
{
    if (frontend_data.compiler_flags & 0x20) select_code_segment(1);
    frontend_bss.current_parameters = state->parameters;
    frontend_bss.current_parameter_kind = state->kind;
    frontend_data.compiler_flags |= 0x300;
}
typedef char CompilerRecordedDeclarationState_layout[(sizeof(CompilerRecordedParameterState) == 8 &&
    sizeof(CompilerRecordedDeclarationState) == 24 &&
    offsetof(CompilerRecordedDeclarationState, parameter_state) == 16) ? 1 : -1];
extern "C" void __cdecl record_function_definition(CompilerSymbol *input,
        CompilerTokenStream *stream, unsigned arena_kind)
{
    CompilerArena *arena;
    ParserTokenView *token;
    CompilerSymbol *copy = input;
    CompilerRecordedParameterState *parameters;
    CompilerRecordedDeclarationState *declaration;
    BYTE *memory;
    unsigned bytes, kind;
    if (arena_kind >= 9) { __asm { int 3 } }
    arena = &frontend_bss.arenas[arena_kind];
    if (!(frontend_data.compiler_mode_flags & 0x40000000)) {
        parameters = (CompilerRecordedParameterState *)arena->cursor;
        arena->cursor += 8;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        saved_parser_value.pointer = parameters;
        parameters->parameters = frontend_bss.current_parameters;
        parameters->kind = frontend_bss.current_parameter_kind;
        if (arena_kind) {
            token = (ParserTokenView *)arena->cursor;
            arena->cursor += 48;
            if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        } else token = (ParserTokenView *)frontend_bss.pools[7].Allocate(48);
        if (token) {
            token->kind = 0xdb;
            memcpy(&token->value, &saved_parser_value, 40);
        }
    } else {
        declaration = (CompilerRecordedDeclarationState *)arena->cursor;
        arena->cursor += 24;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        saved_parser_value.pointer = declaration;
        declaration->cleanup[0] = frontend_data.declaration_cleanup_state[0];
        declaration->cleanup[1] = frontend_data.declaration_cleanup_state[1];
        declaration->cleanup[2] = frontend_data.declaration_cleanup_state[2];
        declaration->reset_value = 0;
        declaration->parameter_state.parameters = frontend_bss.current_parameters;
        declaration->parameter_state.kind = frontend_bss.current_parameter_kind;
        memory = arena_kind ? arena->Allocate(48) : frontend_bss.pools[7].Allocate(48);
        token = (ParserTokenView *)memory;
        if (token) {
            token->kind = 0xd0;
            memcpy(&token->value, &saved_parser_value, 40);
        }
    }
    stream->Trace((unsigned)token);
    if (frontend_bss.current_function) {
        copy = (CompilerSymbol *)frontend_bss.arenas[0].cursor;
        frontend_bss.arenas[0].cursor += 48;
        if (frontend_bss.arenas[0].cursor > frontend_bss.arenas[0].committed_limit)
            frontend_bss.arenas[0].CommitExtent();
        if (copy) {
            kind = input->kind;
            if (kind >= 26 || frontend_data.symbol_record_bytes[kind] > 48) { __asm { int 3 } }
            bytes = frontend_data.symbol_record_bytes[kind];
            /* Symbol records have compatible plain-pointer storage; opaque
             * record bytes are retained, without assigning them defaults. */
            memcpy(copy, input, bytes);
            CompilerSymbolLink *link = &((CompilerNamedSymbol *)copy)->link;
            const CompilerSymbolLink *original = &((CompilerNamedSymbol *)input)->link;
            if (kind == 4) {
                link->CopyClassLink(original, copy);
                link->SetClassContext(((CompilerKind4Symbol *)input)->class_binding);
            } else if (kind == 16 || kind == 17) link->CopySmallLink(original, copy);
            else {
                link->name = original->name;
                link->next = NULL;
            }
        }
    }
    memory = arena_kind ? arena->Allocate(48) : frontend_bss.pools[7].Allocate(48);
    token = (ParserTokenView *)memory;
    if (token) {
        token->kind = 0xcf;
        token->value.pointer = copy;
    }
    stream->Trace((unsigned)token);
}

/* Delimiter marks are actual12-byte arena4 allocations. Their source pair
 * shares the established origin/position prefix of FrontendContextStack;
 * FileName reads only that prefix, not a context-frame pointer. */
struct CompilerRecordedDelimiter {
    CompilerRecordedDelimiter *previous;
    FrontendSourcePosition source;
};
typedef char CompilerRecordedDelimiter_layout[(sizeof(CompilerRecordedDelimiter) == 12 &&
    offsetof(CompilerRecordedDelimiter, source) == 4) ? 1 : -1];
extern "C" const char *__cdecl get_compiler_message(unsigned);
#define RECORDED_DELIMITER_MESSAGE(mark, diagnostic, message) \
    diagnostic(0x3be, get_compiler_message(message), \
        ((FrontendContextStack *)&(mark)->source)->FileName(), (mark)->source.position)
#define RECORDED_UNCLOSED_DELIMITER(mark, message) \
    report_compiler_diagnostic(0x4b, get_compiler_message(message), \
        ((FrontendContextStack *)&(mark)->source)->FileName(), (mark)->source.position)

ParserTokenView *RECON_THISCALL CompilerRecordedLexer::RecordUntil(unsigned stops,
        unsigned include_terminal)
{
    CompilerRecordedDelimiter *parentheses = NULL, *braces = NULL;
    CompilerRecordedDelimiter *brackets = NULL, *angles = NULL, *mark;
    int saw_parenthesis = 0, saw_brace = 0, saw_bracket = 0, saw_angle = 0;
    int parenthesis_count = 0, brace_count = 0, bracket_count = 0, angle_count = 0;
    int done = 0, previous_identifier = 0, copy_tokens = 0;
    ParserTokenView *token = NULL, *copy;
    CompilerRecordedLexer *source;
    CompilerSymbol *bound, *last_class, *candidate;
    CompilerSymbolBindingOwner *chain, *owner;
    unsigned flags, mode, options, kind, arena;

    frontend_data.compiler_mode_flags |= 0x04000000;
    flags = frontend_data.compiler_mode_flags;
    mode = frontend_data.source_mode_flags;
    options = frontend_data.compiler_flags;
    if ((flags & 0x32000000) ||
        ((flags & 0x40000000) && !(mode & 8) && !(options & 0x200)) ||
        (mode & 0x10) || ((flags & 0x04000000) && !(options & 0x8000))) {
        frontend_data.tagged_type_arena_index = frontend_data.constant_arena_index = 0;
    } else {
        frontend_data.tagged_type_arena_index = frontend_data.constant_arena_index = 5;
        if (mode & 1)
            frontend_data.tagged_type_arena_index = frontend_data.constant_arena_index = 4;
    }
    source = (CompilerRecordedLexer *)frontend_bss.saved_context_stack.top->object;
    if (!source->active && arena_kind < source->arena_kind) copy_tokens = 1;
    Trace(make_arena_token(arena_kind));
    for (;;) {
        if (done && !parentheses && !braces && !brackets) break;
        token = TOKEN_CONTEXT.ReadToken();
        kind = token->kind;
        /* The prototype-pointer owner is established only for kinds1..244. */
        if (kind == 0 || kind >= 245) { __asm { int 3 } }
        if (!frontend_data.canonical_token_records[kind - 1] && copy_tokens) {
#ifdef __linux__
            /* A declaration callback has a wider native representation.
             * This ordinary48-byte token copy does not reinterpret that arm. */
            if (kind == 0x22 || kind == 0x23 || kind == 0x24 || kind == 0xaa)
                recon_unsupported("recorded delimiter scan: declaration token copy pending");
#endif
            copy = (ParserTokenView *)allocate_token_storage(48, frontend_bss.symbol_arena_kind);
            if (copy) memcpy(copy, token, 48);
            token = copy;
        }
        switch (token->kind) {
        case 117:
            if ((stops & 1) && !braces) done = 1;
            else {
                mark = (CompilerRecordedDelimiter *)frontend_bss.arenas[4].Allocate(12);
                if (mark) {
                    mark->previous = braces;
                    mark->source.origin = frontend_bss.saved_context_stack.origin;
                    mark->source.position = frontend_bss.saved_context_stack.position;
                }
                braces = mark;
                saw_brace = 1;
                ++brace_count;
            }
            break;
        case 123:
            if (braces) {
                braces = braces->previous;
                --brace_count;
                if (braces) goto clear_other_delimiters;
            }
            if ((stops & 2) && (include_terminal ? saw_brace : !saw_brace)) done = 1;
            saw_brace = 0;
clear_other_delimiters:
            if (parentheses) {
                RECORDED_DELIMITER_MESSAGE(parentheses, compiler_warning, 0x169);
                parentheses = NULL;
                parenthesis_count = 0;
            }
            saw_parenthesis = 0;
            if (brackets) {
                RECORDED_DELIMITER_MESSAGE(brackets, compiler_warning, 0x16a);
                brackets = NULL;
                bracket_count = 0;
            }
            saw_bracket = 0;
            break;
        case 118:
            if ((stops & 4) && !parentheses && !braces && !brackets) done = 1;
            else {
                mark = (CompilerRecordedDelimiter *)frontend_bss.arenas[4].Allocate(12);
                if (mark) {
                    mark->previous = parentheses;
                    mark->source.origin = frontend_bss.saved_context_stack.origin;
                    mark->source.position = frontend_bss.saved_context_stack.position;
                }
                parentheses = mark;
                saw_parenthesis = 1;
                ++parenthesis_count;
            }
            break;
        case 126:
            if (parentheses) {
                parentheses = parentheses->previous;
                --parenthesis_count;
                if (parentheses) break;
            }
            if ((stops & 8) && !braces && !brackets &&
                (include_terminal ? saw_parenthesis : !saw_parenthesis)) done = 1;
            saw_parenthesis = 0;
            break;
        case 65:
            if ((stops & 0x10) && !brackets && !parentheses && !braces) done = 1;
            else {
                mark = (CompilerRecordedDelimiter *)frontend_bss.arenas[4].Allocate(12);
                if (mark) {
                    mark->previous = brackets;
                    mark->source.origin = frontend_bss.saved_context_stack.origin;
                    mark->source.position = frontend_bss.saved_context_stack.position;
                }
                brackets = mark;
                saw_bracket = 1;
                ++bracket_count;
            }
            break;
        case 122:
            if (brackets) {
                brackets = brackets->previous;
                --bracket_count;
                if (brackets) break;
            }
            if ((stops & 0x20) && !parentheses && !braces &&
                (include_terminal ? saw_bracket : !saw_bracket)) done = 1;
            saw_bracket = 0;
            break;
        case 68:
            if (frontend_data.compiler_mode_flags & 0x10000000) {
                mark = (CompilerRecordedDelimiter *)frontend_bss.arenas[4].Allocate(12);
                if (mark) {
                    mark->previous = angles;
                    mark->source.origin = frontend_bss.saved_context_stack.origin;
                    mark->source.position = frontend_bss.saved_context_stack.position;
                }
                angles = mark;
                saw_angle = 1;
                ++angle_count;
            }
            break;
        case 62:
            if (frontend_data.compiler_mode_flags & 0x10000000) {
                if (angles) {
                    angles = angles->previous;
                    --angle_count;
                    if (angles) break;
                }
                if ((stops & 0x400) && (include_terminal ? saw_angle : !saw_angle)) done = 1;
                saw_angle = 0;
            }
            break;
        case 127:
            if ((stops & 0x40) && !braces && !parentheses) done = 1;
            if (brackets) {
                RECORDED_DELIMITER_MESSAGE(brackets, compiler_warning, 0x16a);
                brackets = NULL;
                bracket_count = 0;
            }
            saw_bracket = 0;
            break;
        case 54: case 55:
            if ((stops & (token->kind == 54 ? 0x100 : 0x80)) &&
                !braces && !parentheses && !brackets && !angles) done = 1;
            break;
        case 52:
            if ((stops & 0x200) && !braces && !parentheses && !brackets && !previous_identifier)
                done = 1;
            break;
        case 150:
            bound = TOKEN_SEMANTIC(token).indirect_name->symbol;
            chain = ((CompilerBoundSymbol *)bound)->binding->owner;
            last_class = NULL;
            if (bound->scope_flags & 0x40)
                Trace((unsigned)frontend_data.canonical_token_records[148]);
            for (;;) {
                candidate = NULL;
                for (owner = chain; owner; owner = owner->inner) {
                    if (owner->symbol->kind == 4) {
                        if (owner->symbol == last_class) break;
                        candidate = owner->symbol;
                    }
                }
                arena = frontend_bss.symbol_arena_kind;
                if (arena >= 9) { __asm { int 3 } }
                copy = (ParserTokenView *)(arena ? frontend_bss.arenas[arena].Allocate(48) :
                    frontend_bss.pools[7].Allocate(48));
                if (copy) {
                    copy->kind = 6;
                    copy->value.pointer = ((CompilerNamedSymbol *)(candidate ? candidate : chain->symbol))->link.name;
                }
                Trace((unsigned)copy);
                if (!candidate) break;
                Trace((unsigned)frontend_data.canonical_token_records[148]);
                last_class = candidate;
            }
            continue;
        case 42:
            Trace((unsigned)token);
            frontend_data.compiler_flags |= 0x200000;
            RecordAssembly();
            frontend_data.compiler_flags &= ~0x200000U;
            continue;
        }
        previous_identifier = token->kind == 151;
        if (!done || include_terminal) Trace((unsigned)token);
        else TOKEN_STREAM->Unread((unsigned)token);
        if (done) {
            if (parentheses) {
                RECORDED_DELIMITER_MESSAGE(parentheses, compiler_warning, 0x169);
                parentheses = NULL; saw_parenthesis = 0; parenthesis_count = 0;
            }
            if (braces) {
                RECORDED_DELIMITER_MESSAGE(braces, compiler_warning, 0x168);
                braces = NULL; saw_brace = 0; brace_count = 0;
            }
            if (brackets) {
                RECORDED_DELIMITER_MESSAGE(brackets, compiler_warning, 0x16a);
                brackets = NULL; saw_bracket = 0; bracket_count = 0;
            }
        } else if (token->kind == 109) break;
    }
    Trace(make_arena_token(arena_kind));
    frontend_data.compiler_mode_flags &= ~0x04000000U;
    flags = frontend_data.compiler_mode_flags;
    mode = frontend_data.source_mode_flags;
    options = frontend_data.compiler_flags;
    if ((flags & 0x32000000) ||
        ((flags & 0x40000000) && !(mode & 8) && !(options & 0x200)) ||
        (mode & 0x10) || ((flags & 0x04000000) && !(options & 0x8000))) {
        frontend_data.tagged_type_arena_index = frontend_data.constant_arena_index = 0;
    } else {
        frontend_data.tagged_type_arena_index = frontend_data.constant_arena_index = 5;
        if (mode & 1)
            frontend_data.tagged_type_arena_index = frontend_data.constant_arena_index = 4;
    }
    if (angle_count > 0) { RECORDED_UNCLOSED_DELIMITER(angles, 0x16c); }
    if (bracket_count > 0) { RECORDED_UNCLOSED_DELIMITER(brackets, 0x16a); }
    if (parenthesis_count > 0) { RECORDED_UNCLOSED_DELIMITER(parentheses, 0x169); }
    if (brace_count > 0) { RECORDED_UNCLOSED_DELIMITER(braces, 0x168); }
    return token;
}
#undef RECORDED_DELIMITER_MESSAGE
#undef RECORDED_UNCLOSED_DELIMITER
