/* Deferred inline member text and generated special-member definitions. */
#include "inline-capture.h"
#include "source-input.h"
#include "ir.h"
#include "parameters.h"
#include "parser.h"
#include <recon.h>

int pending_class_name_index; /* original 0x9ecd8 */
typedef char PendingClassFlagsOffsetMustBe29[
    (offsetof(ClassContext, flags) == 29) ? 1 : -1];

#define GENERATING_MEMBER (token_allocation_flags & 0x08000000UL)

/* Original 0x34ea0..0x354ea. The class parser defers inline definitions
   through the paged text stack. Generation kinds 0..10 synthesize the same
   text stream without scanning a user body. All four stack slots remain. */
void __stdcall capture_inline_member_declaration(CppSymbol *symbol, int generated_kind,
    int generation_context, unsigned short pointer_kind)
{
    int emit = !(symbol->declaration_flags & 2), needed = 1;
    int body = 0, braces = 0, parentheses = 0, initial_character = 0, defaults = 0;
    int generated_text = 0;
    unsigned saved_line, length;
    unsigned char token;
    unsigned short distance_kind;
    short diagnostic;
    TypeShape *shape;
    char *name, *distance, *flags, *formatted, *generated_name, *text;

    token_allocation_flags |= 0x200;
    if (!GENERATING_MEMBER) {
        if (parser_lookahead == -1) parser_lookahead = read_parser_token();
        switch (parser_lookahead) {
        case 63: /* constructor member initializers */
            initial_character = ':';
            body = 1;
            symbol->storage[1] |= 1;
            break;
        case 127:
            initial_character = '{';
            braces = body = 1;
            symbol->storage[1] |= 1;
            break;
        case 61: case 64: case 138:
            break;
        default:
            report_syntax_error(59, token_spellings[(unsigned char)parser_lookahead]);
            token_allocation_flags &= ~0x200UL;
            return;
        }
        if (cpp_parser_flags & 0x80000) {
            shape = symbol->type->related;
            if (shape && !(shape->flags & 0x77)) shape = shape->related;
            for (; shape; shape = shape->related) {
                if ((shape->flags & 0x77) == 4 && shape->data.parameters->default_count) {
                    defaults = 1;
                    break;
                }
            }
        }
        needed = body || defaults;
    }
    saved_line = source_line_number;
    if (!needed) {
        token_allocation_flags &= ~0x200UL;
        return;
    }
    if (emit && (!GENERATING_MEMBER || (generated_kind >= 0 && generated_kind < 11))) {
        if (!compiler_stack || *(unsigned *)(compiler_stack->read + 0x7fc)) {
            initialize_work_stack(&compiler_stack);
            write_work_character(';', compiler_stack);
            write_work_character('\n', compiler_stack);
        }
        preprocessor_stream = 0;
        begin_pending_class_names();
        while ((name = next_pending_class_name()) != 0)
            format_work_text(compiler_stack, "%Fs %Fs;\n", token_spellings[183], name);
        --source_line_number;
        emit_line_directive();
        source_line_number = saved_line;
        write_work_character('\n', compiler_stack);
    }
    if (GENERATING_MEMBER && generated_kind >= 0 && generated_kind <= 7) {
        distance_kind = generated_member_pointer_kind(generation_context);
        if (generated_kind == 0)
            emit_generated_default_constructor(symbol, generation_context, distance_kind);
        else if (generated_kind == 1)
            emit_generated_copy_constructor(symbol, generation_context, distance_kind);
        else if (generated_kind == 2)
            emit_generated_assignment(symbol, generation_context, distance_kind);
        else {
            distance = pointer_kind_spelling(distance_kind);
            flags = pointer_kind_spelling((unsigned short)(pointer_kind & 7));
            begin_type_diagnostic();
            diagnostic = reserve_type_diagnostic();
            format_diagnostic_symbol(diagnostic, symbol);
            formatted = type_diagnostic_text(diagnostic);
            if (generated_kind == 3)
                format_work_text(compiler_stack, "inline %Fs %Fs::~%Fs(void)%Fs{}\n",
                    flags, formatted, symbol->name, distance);
            else if (generated_kind == 4)
                format_work_text(compiler_stack, "inline void %Fs %Fs::%Fs(void)%Fs{}\n",
                    flags, formatted, name_vbase_dtor, distance);
            else if (generated_kind == 7)
                format_work_text(compiler_stack, "inline void %Fs %Fs::%Fs(void)%Fs{}\n",
                    flags, formatted, name_default_ctor_closure, distance);
            else {
                generated_name = generated_member_name(generated_kind);
                format_work_text(compiler_stack, "inline void %Fs* %Fs %Fs::%Fs(unsigned %Fs)%Fs{}\n",
                    distance, flags, formatted, generated_name, name_flags, distance);
            }
            end_type_diagnostic();
        }
        generated_text = 1;
    } else if (GENERATING_MEMBER && generated_kind >= 8 && generated_kind <= 10) {
        if (generated_kind == 8)
            format_work_text(compiler_stack,
                "inline void __stdcall %Fs(void*__t,unsigned __s,int __n,void(__thiscall *__f)(void*))"
                "{while(--__n>=0){(*__f)(__t);__t=(char*)__t+__s;}}", name_vector_ctor);
        else if (generated_kind == 9)
            format_work_text(compiler_stack,
                "inline void __stdcall %Fs(void*__t,unsigned __s,int __n,void(__thiscall *__f)(void*))"
                "{while(--__n>=0){(*(void(__thiscall*)(void*,int))__f)(__t,1);__t=(char*)__t+__s;}}", name_vector_ctor_vb);
        else
            format_work_text(compiler_stack,
                "inline void __stdcall %Fs(void*__t,unsigned __s,int __n,void(__thiscall *__f)(void*))"
                "{__t=(char*)__t+__s*__n;while(--__n>=0){__t=(char*)__t-__s;(*__f)(__t);}}", name_vector_dtor);
        generated_text = 1;
    }
    if (generated_text) {
        write_work_character(';', compiler_stack);
        write_work_character('\n', compiler_stack);
        preprocessor_stream = stdout;
    }
    if (GENERATING_MEMBER) {
        token_allocation_flags &= ~0x200UL;
        return;
    }
    if (emit) {
        begin_type_diagnostic();
        print_inline_declaration(symbol, 0);
        end_type_diagnostic();
        options.option_E = 1;
    }
    if (initial_character && emit) write_work_character(initial_character, compiler_stack);
    if (!body) {
        if (emit) {
            write_work_character(';', compiler_stack);
            write_work_character('\n', compiler_stack);
            preprocessor_stream = stdout;
            options.option_E = 0;
        }
        token_allocation_flags &= ~0x200UL;
        return;
    }
    for (;;) {
        token = read_parser_token();
        switch (token) {
        case 0:
            source_line_number = saved_line;
            fatal_error(502);
            return;
        case 1: case 147:
            continue;
        case 127:
            ++braces;
            if (emit) write_work_character('{', compiler_stack);
            continue;
        case 133:
            --braces;
            if (emit) write_work_character('}', compiler_stack);
            if (braces) continue;
            if (function_symbol) report_syntax_error(601);
            goto captured;
        case 128:
            ++parentheses;
            break;
        case 136:
            if (--parentheses < 0) report_syntax_error(284);
            break;
        case 138:
            if (braces >= 1) {
                if (emit) write_work_character(';', compiler_stack);
                continue;
            }
            report_syntax_error(353, symbol->name);
            if (emit) {
                write_work_character('{', compiler_stack);
                write_work_character('}', compiler_stack);
                write_work_character(';', compiler_stack);
            }
            parser_lookahead = 138;
            goto captured;
        }
        if (emit) {
            text = token_spellings[current_token_code];
            for (length = 0; text[length]; ++length) ;
            write_work_bytes(text, length, 1, compiler_stack);
        }
    }
captured:
    if (emit) {
        write_work_character('\n', compiler_stack);
        options.option_E = 0;
        preprocessor_stream = stdout;
    }
    token_allocation_flags &= ~0x200UL;
    /* Even semicolon recovery finishes with the captured-body grammar token. */
    parser_lookahead = 127;
}

int captured_class_error_count; /* original 0x91884 */
void __stdcall write_storage_specification(StorageSpec storage, FILE *file);
void __stdcall finish_work_stack(WorkingStack *stack);
void reset_pushed_tokens(void);

/* Original 0x1e7a0..0x1e942. Replay deferred inline text only outside the
   class/declaration scopes, sealing its paged work stack as a synthetic input.
   Parser errors within the captured class instead discard that text. */
void __stdcall flush_scanned_declaration(DeclarationType *type)
{
    StorageSpec storage;
    CppSymbol *tag;
    char *name, *destination, *source;
    if (macro_expansion_active) {
        cpp_parser_flags |= 8;
        return;
    }
    cpp_parser_flags &= ~8UL;
    if (!compiler_stack || *(unsigned *)(compiler_stack->read + 0x7fc)) return;
    if (token_allocation_flags & 0x40) flush_current_function(1);
    if ((token_allocation_flags & 0x80000) || current_class_context >= 0 || declaration_depth > 0)
        return;
    if (!captured_class_error_count) {
        if (cpp_parser_flags & 0x00100000UL) token_allocation_flags |= 0x08000000UL;
        if (!options.prototypes_only) {
            if (type) {
                begin_type_diagnostic();
                storage.flags = type->storage[0] | (type->storage[1] << 8);
                storage.extended = type->storage[2];
                write_storage_specification(storage, 0);
                tag = cpp_context_stack->items[type->type->field08];
                format_work_text(compiler_stack, " %Fs %Fs ", type_kind_name(type->type->flags), tag->name);
                end_type_diagnostic();
            }
            token_allocation_flags |= 0x400;
            finish_work_stack(compiler_stack);
            name = destination = source_buffer_cursor;
            source = options.intermediate_prefix;
            for (;;) {
                *destination = *source;
                if (!*source++) break;
                ++destination;
            }
            source = "if";
            do {
                source_buffer_cursor = destination + 1;
                *destination++ = *source;
            } while (*source++);
            open_source_file(name, 2);
            reset_pushed_tokens();
            text_work_cursor = expression_storage;
            return;
        }
    }
    cpp_parser_flags &= ~0x00100000UL;
    captured_class_error_count = 0;
    finish_work_stack(compiler_stack);
    text_work_cursor = expression_storage;
    reset_work_stack(compiler_stack);
}

/* Original 0x33260..0x3328d. Skip only the leading class contexts whose
   opening names have already been written into the deferred text stream. */
void begin_pending_class_names(void)
{
    pending_class_name_index = 0;
    while (pending_class_name_index <= current_class_context) {
        if (!(class_contexts[pending_class_name_index].flags & 8)) return;
        ++pending_class_name_index;
    }
}

/* Original 0x33290..0x332be. Mark and return the next enclosing class
   name in declaration order; the begin helper handles the leading skip. */
char *next_pending_class_name(void)
{
    char *name;
    if (pending_class_name_index > current_class_context) return 0;
    class_contexts[pending_class_name_index].flags |= 8;
    name = class_contexts[pending_class_name_index].symbol->name;
    ++pending_class_name_index;
    return name;
}

/* Original 0x35570..0x3559f. Only the two nondefault distance kinds
   need explicit keywords in synthesized member declarations. */
char *__stdcall pointer_kind_spelling(unsigned short kind)
{
    kind &= 7;
    if (kind == 2) return "__far";
    if (kind == 3) return "__far16";
    return "";
}
