#include <string.h>
#include <stdarg.h>
#include "frontend.h"
#include "keywords.h"
#include "pragma.h"
#pragma intrinsic(memcpy)
#include <recon.h>

struct CompilerDeclarationFlagView {
    unsigned flags;
    unsigned RECON_THISCALL QualifierCode(void);
};
struct CompilerDeclarationContext {
    unsigned flags;
    void *context;
    void RECON_THISCALL SetContext(void *);
};
/* Only the name field is used by this source path; its producer/extent and
 * the preceding bytes are still outside the supported native domain. */
struct DeclarationTagNameView {
    BYTE unmodeled_00[8];
    CompilerNameEntry *name;
};
/* Encountered pointer-to-cdecl-function shape. The parameter-list pointer
 * remains an identity here; this predicate never consumes its contents. */
#define CDECL_INT_CALLBACK(t) ((t) && (t)->flags == 0x63 && !(t)->attributes && \
    (t)->shape && (t)->shape->flags == 3 && (t)->shape->attributes == 0x10 && \
    (t)->shape->base && (t)->shape->base->flags == 0x404 && \
    (t)->shape->base->attributes == 0x10 && !(t)->shape->base->base)

/* Encountered local pointer to a two-element array of cdecl callbacks. The
 * function parameter list is retained as identity, never interpreted here. */
#define CDECL_CALLBACK_TABLE_POINTER(t) ((t) && (t)->flags == 0x63 && !(t)->attributes && \
    (t)->shape && (t)->shape->flags == 1 && !(t)->shape->attributes && \
    (t)->shape->base && (t)->shape->base->flags == 2 && !(t)->shape->base->attributes && \
    (t)->shape->base->value == 2 && (t)->shape->base->base && \
    (t)->shape->base->base->flags == 3 && (t)->shape->base->base->attributes == 0x10 && \
    (t)->shape->base->base->base && (t)->shape->base->base->base->flags == 0x404 && \
    (t)->shape->base->base->base->attributes == 0x10 && !(t)->shape->base->base->base->base)

/* Encountered stdcall/fastcall callbacks use convention0x60/0x40 on both
 * shapes, retaining the parameter-list identity without reading its owner. */
#define WIN32_INT_CALLBACK(t) ((t) && (t)->flags == 0x63 && !(t)->attributes && \
    (t)->shape && (t)->shape->flags == 3 && \
    ((t)->shape->attributes == 0x40 || (t)->shape->attributes == 0x60) && \
    (t)->shape->base && (t)->shape->base->flags == 0x404 && \
    (t)->shape->base->attributes == (t)->shape->attributes && !(t)->shape->base->base)
#define DECLARED_INT_CALLBACK(t) (CDECL_INT_CALLBACK(t) || WIN32_INT_CALLBACK(t))

#define INT_ARRAY_TYPE(t) ((t) && (t)->flags == 0x63 && !(t)->attributes && \
    (t)->shape && (t)->shape->flags == 2 && !(t)->shape->attributes && !(t)->shape->base)
#define CONST_INT_ARRAY_TYPE(t) ((t) && (t)->flags == 0x63 && (t)->attributes == 0x100 && \
    (t)->shape && (t)->shape->flags == 2 && !(t)->shape->attributes && \
    !(t)->shape->base && (t)->shape->value)

#define CHAR_ARRAY_TYPE(t) ((t) && (t)->flags == 0x61 && !(t)->attributes && \
    (t)->shape && (t)->shape->flags == 2 && !(t)->shape->attributes && !(t)->shape->base)

#define INT_MATRIX_TYPE(t) ((t) && (t)->flags == 0x63 && !(t)->attributes && \
    (t)->shape && (t)->shape->flags == 2 && !(t)->shape->attributes && \
    (t)->shape->base && (t)->shape->base->flags == 2 && \
    !(t)->shape->base->attributes && !(t)->shape->base->base)

#define FIXED_INT_ARRAY(t) ((t) && (t)->flags == 0x63 && !(t)->attributes && \
    (t)->shape && (t)->shape->flags == 2 && !(t)->shape->attributes && \
    !(t)->shape->base && (t)->shape->value)

#define CDECL_INT_FUNCTION(t) ((t) && (t)->flags == 0x63 && (t)->attributes == 0x10 && \
    (t)->shape && ((t)->shape->flags & 7) == 4 && \
    (t)->shape->attributes == 0x10 && !(t)->shape->base)

#define STDCALL_INT_FUNCTION(t) ((t) && (t)->flags == 0x63 && (t)->attributes == 0x60 && \
    (t)->shape && ((t)->shape->flags & 7) == 4 && \
    (t)->shape->attributes == 0x60 && !(t)->shape->base)

#define WIDE_INTEGER_TYPE(t) ((t) == c1_data.builtin_types.type_004c || \
    (t) == c1_data.builtin_types.type_404c)
#define SCALAR_ARITHMETIC_TYPE(t) (WIDE_INTEGER_TYPE(t) || \
    (t) == c1_data.builtin_types.type_0044 || (t) == c1_data.builtin_types.type_4044 || \
    (t) == c1_data.builtin_types.type_0106 || (t) == c1_data.builtin_types.type_0107 || \
    ((t) && (((t)->flags & ~0x4000U) == 0x64 || ((t)->flags & ~0x4000U) == 0x62 || (t)->flags == 0x8061 || (t)->flags == 0x61) && \
     !(t)->attributes && !(t)->shape && !(t)->auxiliary))
#define VOLATILE_UNSIGNED_INT(t) ((t) && (t)->flags == 0x4063 && (t)->attributes == 0x200 && \
    !(t)->shape && !(t)->auxiliary)
#define VOLATILE_SHORT_POINTER(t) ((t) && (t)->flags == 0x62 && (t)->attributes == 0x200 && \
    (t)->shape && (t)->shape->flags == 1 && !(t)->shape->attributes && !(t)->shape->base)
#define VOLATILE_SHORT_ARRAY(t) ((t) && (t)->flags == 0x62 && (t)->attributes == 0x200 && \
    (t)->shape && (t)->shape->flags == 2 && !(t)->shape->attributes && !(t)->shape->base)
#define FLOATING_ARRAY(t) ((t) && ((t)->flags == 0x106 || (t)->flags == 0x107) && \
    !(t)->attributes && (t)->shape && (t)->shape->flags == 2 && \
    !(t)->shape->attributes && !(t)->shape->base)

#define CDECL_INT_CALLBACK_ARRAY(t) ((t) && (t)->flags == 0x63 && !(t)->attributes && \
    (t)->shape && (t)->shape->flags == 2 && !(t)->shape->attributes && \
    (t)->shape->base && (t)->shape->base->flags == 3 && \
    (t)->shape->base->attributes == 0x10 && (t)->shape->base->base && \
    (t)->shape->base->base->flags == 0x404 && \
    (t)->shape->base->base->attributes == 0x10 && !(t)->shape->base->base->base)

#define COMPLETE_AGGREGATE_MEMBER(t) ((t) && ((t)->flags == 0x402 || (t)->flags == 0x404) && \
    !(t)->attributes && !(t)->shape && (t)->symbol_owner && \
    (t)->symbol_owner->kind == 4 && ((t)->symbol_owner->flags & 1))

/* Additional member alternatives have the same existing interned type/shape
 * producers: an int matrix, a const-char pointer, or a cdecl callback. */
#define DECLARED_RECORD_MEMBER_TYPE(t) (SCALAR_ARITHMETIC_TYPE(t) || INT_ARRAY_TYPE(t) || \
    COMPLETE_AGGREGATE_MEMBER(t) || CDECL_INT_CALLBACK(t) || \
    ((t) && (t)->flags == 0x63 && !(t)->attributes && (t)->shape && \
     (t)->shape->flags == 2 && !(t)->shape->attributes && (t)->shape->value && \
     (t)->shape->base && (t)->shape->base->flags == 2 && \
     !(t)->shape->base->attributes && (t)->shape->base->value && !(t)->shape->base->base) || \
    ((t) && (t)->flags == 0x61 && (t)->attributes == 0x100 && (t)->shape && \
     (t)->shape->flags == 1 && !(t)->shape->attributes && !(t)->shape->base))

#define COMPLETE_RECORD_VALUE(t) ((t) && (t)->flags == 0x402 && !(t)->attributes && \
    !(t)->shape && (t)->symbol_owner && (t)->symbol_owner->kind == 4 && \
    ((t)->symbol_owner->flags & 1))

/* Const-int pointer parameters retain the ordinary pointer shape, with no
 * array/function adjustment or pointee read in declaration validation. */
#define CONST_INT_POINTER(t) ((t) && (t)->flags == 0x63 && (t)->attributes == 0x100 && \
    !(t)->auxiliary && (t)->shape && (t)->shape->flags == 1 && \
    !(t)->shape->attributes && !(t)->shape->base)

#define CONST_CHAR_POINTER(t) ((t) && (t)->flags == 0x61 && (t)->attributes == 0x100 && \
    (t)->shape && (t)->shape->flags == 1 && !(t)->shape->attributes && !(t)->shape->base)

#define CONST_CHAR_POINTER_ARRAY(t) ((t) && (t)->flags == 0x61 && (t)->attributes == 0x100 && \
    (t)->shape && (t)->shape->flags == 2 && !(t)->shape->attributes && \
    (t)->shape->base && (t)->shape->base->flags == 1 && \
    !(t)->shape->base->attributes && !(t)->shape->base->base)

#define CONST_INT_ROW_POINTER(t) ((t) && (t)->flags == 0x63 && (t)->attributes == 0x100 && \
    (t)->shape && (t)->shape->flags == 1 && !(t)->shape->attributes && \
    (t)->shape->base && (t)->shape->base->flags == 2 && \
    !(t)->shape->base->attributes && !(t)->shape->base->base)

#define VOID_POINTER(t) ((t) && (t)->flags == 0x80 && !(t)->attributes && \
    (t)->shape && (t)->shape->flags == 1 && !(t)->shape->attributes && !(t)->shape->base)

#define COMPLETE_RECORD_POINTER(t) ((t) && (t)->flags == 0x402 && !((t)->attributes & ~0x100U) && \
    (t)->shape && (t)->shape->flags == 1 && !(t)->shape->attributes && \
    !(t)->shape->base && (t)->symbol_owner && (t)->symbol_owner->kind == 4 && \
    ((t)->symbol_owner->flags & 1))

#define CONST_INT_MATRIX(t) ((t) && (t)->flags == 0x63 && (t)->attributes == 0x100 && \
    (t)->shape && (t)->shape->flags == 2 && !(t)->shape->attributes && \
    (t)->shape->base && (t)->shape->base->flags == 2 && \
    !(t)->shape->base->attributes && !(t)->shape->base->base)

extern "C" {
void __cdecl compiler_warning(int, ...);
CompilerIrSymbol *__cdecl prepare_function_declarator(CompilerIrSymbol *, CompilerDeclarator *);
void __cdecl declare_symbol(CompilerDeclarator *, CompilerIrSymbol *);
void __cdecl validate_declaration_type(CompilerIrSymbol *, int);
CompilerIrSymbol *__cdecl lookup_ir_symbol(CompilerIrSymbol *);
CompilerIrSymbol *__cdecl find_named_symbol(CompilerSymbolBinding *, CompilerNameEntry *);
void __cdecl report_extension_diagnostic(unsigned, unsigned, ...);
CompilerIrSymbol *__cdecl merge_scoped_declaration(CompilerIrSymbol *, CompilerIrSymbol *, int);
int __cdecl check_scoped_declaration(CompilerIrSymbol *, CompilerIrSymbol *, int);
void __cdecl attach_declaration_context(CompilerIrSymbol *, void *);
void __cdecl discard_declaration_symbol(CompilerIrSymbol *);
void __cdecl record_scoped_declaration(CompilerIrSymbol *);
void __cdecl report_language_diagnostic(unsigned, unsigned, ...);
}

extern "C" void __cdecl declare_symbol(CompilerDeclarator *declarator,
    CompilerIrSymbol *symbol)
{
    CompilerDeclarationContext original;
    unsigned flags, storage_class, modifier;
    void *context;
    CompilerType *type;
    original.flags = symbol->intrinsic_flags;
    if (symbol->function_flags & 1) {
        original.context = lookup_ir_symbol(symbol);
        if (original.context) original.flags |= 0x1000000;
        else original.flags &= ~0x1000000;
    } else original.SetContext(NULL);
    if (!(symbol->kind == 13 && !original.flags && !original.context &&
          declarator && !(declarator->flags & 1) &&
          (SCALAR_ARITHMETIC_TYPE(declarator->resolved_type) || declarator->resolved_type == c1_data.builtin_types.type_0063 ||
           declarator->resolved_type == c1_data.builtin_types.type_4063_first ||
           CDECL_INT_CALLBACK(declarator->resolved_type) || CDECL_CALLBACK_TABLE_POINTER(declarator->resolved_type) || CONST_INT_ROW_POINTER(declarator->resolved_type) ||
           COMPLETE_RECORD_POINTER(declarator->resolved_type) ||
           (declarator->resolved_type && !declarator->resolved_type->shape &&
            (declarator->resolved_type->flags == 0x402 || declarator->resolved_type->flags == 0x404) &&
            !(declarator->resolved_type->attributes & ~0x100U) &&
            declarator->resolved_type->symbol_owner && declarator->resolved_type->symbol_owner->kind == 4 &&
            (declarator->resolved_type->symbol_owner->flags & 1)))) &&
        ((original.flags != 0x20 && original.flags != 7 && !(original.flags == 3 && symbol->kind == 10) &&
          !(original.flags == 0x40020 && symbol->kind == 10 && declarator &&
            STDCALL_INT_FUNCTION(declarator->resolved_type)) &&
          !(original.flags == 0x20003 && symbol->kind == 10 && declarator &&
            declarator->resolved_type == c1_data.builtin_types.type_0063) &&
          !(original.flags == 0x1001 && symbol->kind == 10 && declarator &&
            (CDECL_INT_FUNCTION(declarator->resolved_type) || INT_ARRAY_TYPE(declarator->resolved_type)))) ||
        original.context || (symbol->kind != 10 && symbol->kind != 2 && symbol->kind != 3 && symbol->kind != 11 &&
        !(symbol->kind == 12 && declarator && declarator->resolved_type &&
          (CDECL_INT_CALLBACK(declarator->resolved_type) || SCALAR_ARITHMETIC_TYPE(declarator->resolved_type) ||
           VOLATILE_SHORT_POINTER(declarator->resolved_type) || CONST_CHAR_POINTER(declarator->resolved_type) ||
           CONST_INT_POINTER(declarator->resolved_type) ||
           COMPLETE_RECORD_VALUE(declarator->resolved_type) || COMPLETE_RECORD_POINTER(declarator->resolved_type) ||
           declarator->resolved_type == c1_data.builtin_types.type_0080 ||
           declarator->resolved_type == c1_data.builtin_types.type_0063 ||
           declarator->resolved_type == c1_data.builtin_types.type_4063_first ||
           (declarator->resolved_type->shape && (declarator->resolved_type->shape->flags & 7) == 1 &&
            declarator->resolved_type->flags == 0x402 && declarator->resolved_type->attributes == 0x100 &&
            declarator->resolved_type->symbol_owner && declarator->resolved_type->symbol_owner->kind == 4 &&
            (declarator->resolved_type->symbol_owner->flags & 1))))) ||
        !declarator || (declarator->flags & 1) || !declarator->resolved_type)) {
#ifdef __linux__
        recon_unsupported("symbol declaration context or declarator domain is not yet modeled");
#else
        __asm { int 3 }
#endif
    }
    flags = original.flags;
    context = original.context;
    if (symbol->kind == 2 || symbol->kind == 3) {
        storage_class = original.flags & 7;
        if ((storage_class >= 4 && storage_class <= 6) || storage_class == 3 ||
            storage_class == 1 || storage_class == 7 || (original.flags & 0xc0000)) {
            compiler_warning(71, symbol);
            symbol->scope_bits |= 1;
            flags |= 8;
        }
    }
    if (declarator) {
        if (declarator->flags & 1) symbol->scope_bits |= 1;
        type = declarator->resolved_type;
        symbol->value_type = type;
        storage_class = flags & 7;
        validate_declaration_type(symbol, storage_class == 7);
#ifdef RECON_NATIVE_TEST
        if (!(symbol->kind == 10 && original.flags == 0x20003 && type == c1_data.builtin_types.type_0063) &&
            !(symbol->kind == 10 && original.flags == 3 && CONST_INT_ARRAY_TYPE(type)) &&
            !(symbol->kind == 10 && (original.flags == 0x1001 || original.flags == 3 || original.flags == 0x20) &&
              (INT_ARRAY_TYPE(type) || INT_MATRIX_TYPE(type))) &&
            !(symbol->kind == 3 && original.flags == 0x20 && DECLARED_RECORD_MEMBER_TYPE(type)) &&
            !(symbol->kind == 10 && storage_class == 7 && FIXED_INT_ARRAY(type)) &&
            !(symbol->kind == 10 && (original.flags == 7 || original.flags == 3) && WIDE_INTEGER_TYPE(type)) &&
            !(symbol->kind == 13 && !original.flags &&
              (SCALAR_ARITHMETIC_TYPE(type) || type == c1_data.builtin_types.type_0063 || type == c1_data.builtin_types.type_4063_first || CDECL_INT_CALLBACK(type) || CDECL_CALLBACK_TABLE_POINTER(type) || CONST_INT_ROW_POINTER(type) || COMPLETE_RECORD_POINTER(type) ||
               (!type->shape && (type->flags == 0x402 || type->flags == 0x404) &&
                !(type->attributes & ~0x100U) && type->symbol_owner &&
                type->symbol_owner->kind == 4 && (type->symbol_owner->flags & 1)))) &&
            !(symbol->kind == 10 && original.flags == 3 && (type->flags == 0x61 || type->flags == 0x4062) &&
              type->attributes == 0x100 && type->shape && type->shape->flags == 2 &&
              !type->shape->base && !type->shape->value) &&
            !(symbol->kind == 10 && original.flags == 3 &&
              (type == c1_data.builtin_types.type_0063 || SCALAR_ARITHMETIC_TYPE(type) || VOLATILE_UNSIGNED_INT(type))) &&
            !(symbol->kind == 10 && original.flags == 3 &&
              (CONST_CHAR_POINTER(type) || VOLATILE_SHORT_ARRAY(type) || FLOATING_ARRAY(type) || CONST_INT_MATRIX(type) || COMPLETE_RECORD_VALUE(type) || CDECL_INT_CALLBACK_ARRAY(type) ||
               CONST_CHAR_POINTER_ARRAY(type))) &&
            !(symbol->kind == 10 && original.flags == 3 && type->shape &&
              (type->shape->flags & 7) == 2 && type->flags == 0x402 && !(type->attributes & ~0x100U) &&
              type->symbol_owner && type->symbol_owner->kind == 4 && (type->symbol_owner->flags & 1)) &&
            !(symbol->kind == 10 && original.flags == 0x20 &&
              (SCALAR_ARITHMETIC_TYPE(type) || type == c1_data.builtin_types.type_0063 || type == c1_data.builtin_types.type_4063_first)) &&
            !(symbol->kind == 10 && original.flags == 0x20 &&
              (FLOATING_ARRAY(type) || CHAR_ARRAY_TYPE(type) || CONST_CHAR_POINTER_ARRAY(type) || COMPLETE_RECORD_VALUE(type) || CDECL_INT_CALLBACK(type) || CDECL_CALLBACK_TABLE_POINTER(type) || CONST_INT_ROW_POINTER(type) || VOID_POINTER(type))) &&
            !(symbol->kind == 10 && original.flags == 0x20 && type->shape &&
              (type->shape->flags & 7) == 1 && type->flags == 0x402 &&
              !(type->attributes & ~0x100U) && type->symbol_owner &&
              type->symbol_owner->kind == 4 && (type->symbol_owner->flags & 1)) &&
            !(symbol->kind == 10 && storage_class == 7 && DECLARED_INT_CALLBACK(type)) &&
            !(symbol->kind == 3 && original.flags == 0x20 && type->flags == 0x402 &&
              !type->attributes && type->shape && type->shape->flags == 1 &&
              !type->shape->attributes && !type->shape->base && type->symbol_owner &&
              type->symbol_owner->kind == 4) &&
            symbol->kind != 7 && symbol->kind != 11 &&
            !(symbol->kind == 12 && (CDECL_INT_CALLBACK(type) || SCALAR_ARITHMETIC_TYPE(type) ||
              VOLATILE_SHORT_POINTER(type) || CONST_CHAR_POINTER(type) || CONST_INT_POINTER(type) || COMPLETE_RECORD_VALUE(type) || COMPLETE_RECORD_POINTER(type) ||
              type == c1_data.builtin_types.type_0080 ||
              type == c1_data.builtin_types.type_0063 ||
              type == c1_data.builtin_types.type_4063_first ||
              (type->shape && (type->shape->flags & 7) == 1 && type->flags == 0x402 &&
               type->attributes == 0x100 && type->symbol_owner && type->symbol_owner->kind == 4 &&
               (type->symbol_owner->flags & 1)))) && !((symbol->kind == 10 &&
            (storage_class == 7 || ((original.flags == 0x20 || original.flags == 3) && !type->shape &&
             !type->attributes && type->flags == 0x404)) ||
            (symbol->kind == 2 || symbol->kind == 3) && original.flags == 0x20) &&
            (type == c1_data.builtin_types.type_0063 ||
             ((symbol->kind == 2 || symbol->kind == 3) && type == c1_data.builtin_types.type_4063_first) ||
             (symbol->kind == 3 && !type->shape && !type->attributes && (type->flags == 0x4062 || type->flags == 0x64)) ||
             (symbol->kind == 3 && !type->attributes && type->flags == 0x4061 &&
              type->shape && type->shape->flags == 2 && !type->shape->base && type->shape->value) ||
             (symbol->kind == 10 && (storage_class == 7 || original.flags == 0x20 || original.flags == 3) && !type->shape &&
              (type->flags == 0x402 || type->flags == 0x404) && type->symbol_owner && (type->symbol_owner->flags & 1)))))
            recon_unsupported("nonfunction declaration after type validation is not yet modeled");
#endif
        if (!type->shape && storage_class != 7) {
            if (type->flags == 0x80) {
                if (symbol->kind != 13 && symbol->kind != 12) {
                    compiler_warning(182, symbol);
                    symbol->value_type = c1_data.builtin_types.type_0063;
                }
            } else if (storage_class != 1 && (type->flags & 0x400) &&
                       !(((BYTE *)type->auxiliary)[1] & 1) &&
                       symbol->kind != 13 && symbol->kind != 12) {
                compiler_warning(79, symbol, type, type->auxiliary);
                symbol->value_type = c1_data.builtin_types.type_0063;
            }
        }
    }
    type = symbol->value_type;
    if (symbol->scope_bits & 1) flags |= 8;
    if (symbol->kind == 7 || symbol->kind == 8) {
        if (!(declarator->flags & 2)) {
            if ((c1_data.options.fastcall_calls || c1_data.options.pascal_calls ||
                 c1_data.options.stdcall_calls) &&
                symbol->binding.name == c1_data.builtin_names.main_entry) {
                type = type->WithFunctionConvention(0x10);
                report_language_diagnostic(3, 7, symbol, 37);
            } else if ((c1_data.options.fastcall_calls || c1_data.options.pascal_calls ||
                        c1_data.options.cdecl_calls) &&
                (symbol->binding.name == c1_data.builtin_names.winmain_entry ||
                 symbol->binding.name == c1_data.builtin_names.dllmain_entry)) {
                type = type->WithFunctionConvention(0x60);
                report_language_diagnostic(3, 7, symbol, 130);
            }
        }
        if ((symbol->flags & 1) && type->shape->parameters->count == 0)
            type = type->WithShapeFlags(0x4000);
        switch (flags & 7) {
        case 4: case 5: case 6:
            report_language_diagnostic(1, 42, symbol);
            /* fall through */
        case 0: case 2:
            flags = (flags & ~6U) | 0x21;
            break;
        case 3:
            if (flags & 0xc0000) {
                compiler_warning(201, symbol);
                flags = (flags & ~6U) | 0x21;
            }
            if (c1_bss.parameter_nesting) report_extension_diagnostic(205, 267);
            break;
        case 7:
            if (symbol->flags & 1) {
                compiler_warning(71, symbol);
                flags = (flags & ~6U) | 0x21;
            }
            break;
        }
        goto finish;
    }
    modifier = 1;
    if (flags & 0x100) {
        flags &= ~0x100U;
        modifier = 14;
    }
    if (modifier != 1) {
        if (symbol->kind == 12) compiler_warning(735, modifier);
        else if (symbol->kind == 13) compiler_warning(736, modifier);
        else compiler_warning(433, symbol, modifier);
    }
    if (symbol->kind == 11 || symbol->kind == 12) {
        storage_class = flags & 7;
        if (storage_class != 5) {
            if (storage_class) report_language_diagnostic(1, 42, symbol);
            flags = (flags & ~3U) | 4;
        }
        if (flags & (0x700U | 0x4f0000U | 0x1000000U)) {
            report_language_diagnostic(1, 42, symbol);
            flags &= 7;
            context = NULL;
        }
    } else if (symbol->kind == 10) {
        storage_class = flags & 7;
        if (!(c1_data.lexer_flags & 0x20000)) {
            if (storage_class >= 4 && storage_class <= 6) {
                report_language_diagnostic(1, 42, symbol);
                flags = (flags & ~5U) | 2;
            } else if (!storage_class) {
                if (((symbol->qualifier_flags & 5) == 5 &&
                     !(symbol->declaration_flags & 4)) || (symbol->linkage_flags & 8))
                    flags = (flags & ~6U) | 1;
                else flags = (flags & ~5U) | 2;
            }
        } else if (!storage_class) {
            if ((c1_data.lexer_flags & 0x20) || (flags & 0x80000))
                flags = (flags & ~6U) | 1;
            else flags = (flags & ~3U) | 4;
        }
    }
    storage_class = flags & 7;
    if (storage_class == 3 && (flags & 0xc0000)) {
        compiler_warning(201, symbol);
        flags = (flags & ~0xc0000U) | 8;
    }
    if ((flags & 0xc0000) && (c1_data.lexer_flags & 0x20000) &&
        storage_class != 1) compiler_warning(71, symbol);
finish:
    symbol->intrinsic_flags = flags;
    if ((flags & 0x1000000) && context)
        attach_declaration_context(symbol, context);
    else symbol->function_flags &= ~1;
    symbol->value_type = type;
}

extern "C" CompilerIrSymbol *__cdecl declare_declarator(
    const CompilerParserDeclaration *declaration, CompilerIrSymbol *symbol,
    CompilerDeclarator *declarator)
{
    unsigned flags;
    void *context;
    unsigned storage_class;
    int local;
    CompilerIrSymbol **found, *existing;
    CompilerType *type;
    DeclarationTagNameView *tag;
    if (!symbol || !declaration->type) return NULL;
    flags = declaration->flags;
    context = (void *)declaration->value;
    if ((flags & 7) == 1) flags |= 0x1000;
    if (declarator->flags & 8)
        prepare_function_declarator(symbol, declarator);
    if ((symbol->kind != 1 && !((symbol->kind == 2 || symbol->kind == 3 || symbol->kind == 11) && flags == 0x20)) ||
        (flags != 0x20 && flags != 7 && flags != 3 &&
         !(flags == 0x40020 && symbol->kind == 1 && STDCALL_INT_FUNCTION(declarator->resolved_type)) &&
         !(flags == 0x20003 && symbol->kind == 1 && declarator->resolved_type == c1_data.builtin_types.type_0063) &&
         !(flags == 0x1001 && symbol->kind == 1 && (CDECL_INT_FUNCTION(declarator->resolved_type) || INT_ARRAY_TYPE(declarator->resolved_type)))) || context) {
#ifdef __linux__
        recon_unsupported("declaration publication flags or symbol alternative is not yet modeled");
#else
        __asm { int 3 }
#endif
    }
    local = symbol->kind == 2 || symbol->kind == 3;
    if (symbol->kind == 1) symbol->kind = 10;
    symbol->intrinsic_flags = flags;
    if (flags & 0x1000000) {
        if (context) attach_declaration_context(symbol, context);
        else symbol->function_flags &= ~1;
    } else symbol->SetDeclarationContext(NULL);
    declare_symbol(declarator, symbol);
#ifdef RECON_NATIVE_TEST
    if (!(symbol->kind == 10 && flags == 0x20003 && symbol->value_type == c1_data.builtin_types.type_0063) &&
        !(symbol->kind == 10 && flags == 3 && CONST_INT_ARRAY_TYPE(symbol->value_type)) &&
        !(symbol->kind == 10 && (flags == 0x1001 || flags == 3 || flags == 0x20) &&
          (INT_ARRAY_TYPE(symbol->value_type) || INT_MATRIX_TYPE(symbol->value_type))) &&
        !(symbol->kind == 3 && flags == 0x20 && DECLARED_RECORD_MEMBER_TYPE(symbol->value_type)) &&
        !(symbol->kind == 10 && flags == 7 && FIXED_INT_ARRAY(symbol->value_type)) &&
        !(symbol->kind == 10 && (flags == 7 || flags == 3) && WIDE_INTEGER_TYPE(symbol->value_type)) &&
        !(symbol->kind == 10 && flags == 3 && (symbol->value_type->flags == 0x61 || symbol->value_type->flags == 0x4062) &&
          symbol->value_type->attributes == 0x100 && symbol->value_type->shape &&
          symbol->value_type->shape->flags == 2 && !symbol->value_type->shape->base &&
          !symbol->value_type->shape->value) &&
        !(symbol->kind == 10 && flags == 3 &&
          (symbol->value_type == c1_data.builtin_types.type_0063 ||
           SCALAR_ARITHMETIC_TYPE(symbol->value_type) || VOLATILE_UNSIGNED_INT(symbol->value_type))) &&
        !(symbol->kind == 10 && flags == 3 &&
          (CONST_CHAR_POINTER(symbol->value_type) || VOLATILE_SHORT_ARRAY(symbol->value_type) || FLOATING_ARRAY(symbol->value_type) || CONST_INT_MATRIX(symbol->value_type) || COMPLETE_RECORD_VALUE(symbol->value_type) ||
           CDECL_INT_CALLBACK_ARRAY(symbol->value_type) || CONST_CHAR_POINTER_ARRAY(symbol->value_type))) &&
        !(symbol->kind == 10 && flags == 3 && symbol->value_type->shape &&
          (symbol->value_type->shape->flags & 7) == 2 && symbol->value_type->flags == 0x402 &&
          !(symbol->value_type->attributes & ~0x100U) && symbol->value_type->symbol_owner &&
          symbol->value_type->symbol_owner->kind == 4 && (symbol->value_type->symbol_owner->flags & 1)) &&
        !(symbol->kind == 10 && flags == 0x20 &&
          (SCALAR_ARITHMETIC_TYPE(symbol->value_type) || symbol->value_type == c1_data.builtin_types.type_0063 ||
           symbol->value_type == c1_data.builtin_types.type_4063_first)) &&
        !(symbol->kind == 10 && flags == 0x20 &&
          (FLOATING_ARRAY(symbol->value_type) || CHAR_ARRAY_TYPE(symbol->value_type) || CONST_CHAR_POINTER_ARRAY(symbol->value_type) || COMPLETE_RECORD_VALUE(symbol->value_type) || CDECL_INT_CALLBACK(symbol->value_type) || CDECL_CALLBACK_TABLE_POINTER(symbol->value_type) ||
           CONST_INT_ROW_POINTER(symbol->value_type) || VOID_POINTER(symbol->value_type))) &&
        !(symbol->kind == 10 && flags == 0x20 && symbol->value_type->shape &&
          (symbol->value_type->shape->flags & 7) == 1 && symbol->value_type->flags == 0x402 &&
          !(symbol->value_type->attributes & ~0x100U) && symbol->value_type->symbol_owner &&
          symbol->value_type->symbol_owner->kind == 4 &&
          (symbol->value_type->symbol_owner->flags & 1)) &&
        !(symbol->kind == 10 && flags == 7 && DECLARED_INT_CALLBACK(symbol->value_type)) &&
        !(symbol->kind == 3 && flags == 0x20 && symbol->value_type->flags == 0x402 &&
          !symbol->value_type->attributes && symbol->value_type->shape &&
          symbol->value_type->shape->flags == 1 && !symbol->value_type->shape->attributes &&
          !symbol->value_type->shape->base && symbol->value_type->symbol_owner &&
          symbol->value_type->symbol_owner->kind == 4) &&
        symbol->kind != 7 && symbol->kind != 11 && !((symbol->kind == 10 &&
        (flags == 7 || ((flags == 0x20 || flags == 3) && !symbol->value_type->shape &&
         !symbol->value_type->attributes && symbol->value_type->flags == 0x404)) ||
        (symbol->kind == 2 || symbol->kind == 3) && flags == 0x20) &&
        (symbol->value_type == c1_data.builtin_types.type_0063 ||
         ((symbol->kind == 2 || symbol->kind == 3) && symbol->value_type == c1_data.builtin_types.type_4063_first) ||
         (symbol->kind == 3 && !symbol->value_type->shape && !symbol->value_type->attributes &&
          (symbol->value_type->flags == 0x4062 || symbol->value_type->flags == 0x64)) ||
         (symbol->kind == 3 && !symbol->value_type->attributes && symbol->value_type->flags == 0x4061 &&
          symbol->value_type->shape && symbol->value_type->shape->flags == 2 &&
          !symbol->value_type->shape->base && symbol->value_type->shape->value) ||
         (symbol->kind == 10 && (flags == 7 || flags == 0x20 || flags == 3) && !symbol->value_type->shape &&
          (symbol->value_type->flags == 0x402 || symbol->value_type->flags == 0x404) && symbol->value_type->symbol_owner &&
          (symbol->value_type->symbol_owner->flags & 1)))))
        recon_unsupported("nonfunction declaration publication is not yet modeled");
#endif
    flags = declaration->flags;
    context = (void *)declaration->value;
    if ((flags & 0x10) && (flags & 0x20)) {
        if ((symbol->kind != 7 && symbol->kind != 8) ||
            symbol->function_type->FunctionResult()->shape)
            compiler_warning(501, 83, symbol);
    }
    if ((symbol->linkage_flags & 1) &&
        !((symbol->symbol_qualifiers & 1) && !(symbol->scope_bits & 1)) &&
        !((symbol->kind == 7 || symbol->kind == 8) && c1_constant_diagnostic_mode < 0) &&
        !(symbol->scope_bits & 1)) {
        compiler_warning(488, symbol);
        symbol->scope_bits |= 1;
    }
    if (c1_bss.parameter_nesting == 2 && c1_bss.current_scope_symbol &&
        (c1_bss.current_scope_symbol->linkage_flags & 1)) {
        storage_class = symbol->storage_class & 7;
        if (storage_class >= 4 && storage_class <= 6 &&
            (symbol->declaration_flags & 4)) compiler_warning(489, symbol);
    }
    storage_class = symbol->storage_class & 7;
    if (symbol->kind == 10 && storage_class >= 1 && storage_class <= 3) {
        if (c1_data.data_linkage_option) symbol->linkage_flags |= 2;
    }
    if (symbol->linkage_flags & 2) {
        if (symbol->kind != 10 || storage_class < 1 || storage_class > 3) {
            compiler_warning(480, symbol);
            symbol->linkage_flags &= ~2;
        }
        if (((CompilerDeclarationFlagView *)&symbol->intrinsic_flags)->QualifierCode())
            compiler_warning(492, symbol);
    }
    if (symbol->function_flags & 1) {
        if (symbol->kind != 10 || storage_class < 1 || storage_class > 3) {
            compiler_warning(479, symbol);
            symbol->SetDeclarationContext(NULL);
        }
    }
    if (symbol->linkage_flags & 0x40) {
        if (symbol->kind == 10 && (symbol->declaration_flags & 4) &&
            (storage_class == 1 || storage_class == 2 ||
             (storage_class == 3 && (symbol->storage_class & 0x20))))
            symbol->symbol_qualifiers |= 0x80;
        else {
            compiler_warning(496, symbol);
            symbol->linkage_flags &= ~0x40;
            symbol->storage_class &= ~0x40;
        }
    }
    if ((symbol->storage_class & 7) == 7) {
        symbol->flags |= 4;
        if (symbol->binding.name) symbol->binding.name->flags |= 0x20000;
        type = symbol->value_type;
        if (!c1_bss.options.prototypes_only && type &&
            (type->flags & 0x400) && !type->shape) {
            tag = (DeclarationTagNameView *)type->auxiliary;
            if (tag->name == c1_data.builtin_names.unnamed)
                tag->name = symbol->binding.name;
        }
    }
    if (local) return symbol;
    found = c1_data.translation_unit->current->MergeDeclaration(symbol);
    existing = found ? *found : NULL;
    if (!existing) {
        if (c1_bss.parameter_nesting >= 2) {
            found = c1_data.translation_unit->FindScopedDeclaration(symbol);
            existing = found ? *found : NULL;
        } else if (!c1_data.options.language_extensions &&
                   c1_data.translation_unit->current->level == 1) {
            found = c1_data.translation_unit->FindName(symbol->binding.name);
            existing = found ? *found : NULL;
            if (existing && (existing->flags & 4))
                report_language_diagnostic(1, 224, symbol);
            existing = NULL;
        }
    }
    if (existing) {
        if (symbol->declaration_flags & 0x80) existing->declaration_flags |= 0x80;
        if (existing != symbol) discard_declaration_symbol(symbol);
        return existing;
    }
    if (!(symbol->record_id & 0xffffff)) {
        if (c1_data.next_symbol_record_id >= 0x7fffffff) compiler_fatal(55);
        unsigned index = c1_data.next_symbol_record_id++;
        symbol->record_id = (symbol->record_id & 0xff000000) | (index & 0xffffff);
    }
    if (c1_bss.parameter_nesting >= 2) record_scoped_declaration(symbol);
    c1_data.translation_unit->current->Insert(symbol);
    return symbol;
}

extern "C" CompilerIrSymbol *__cdecl prepare_function_declarator(CompilerIrSymbol *symbol,
    CompilerDeclarator *declarator)
{
    CompilerParameterDescription *parameters = declarator->parameters;
    symbol->declaration_flags |= 0x80;
    c1_bss.parameter_scope_initial_value = parameters->value;
    c1_bss.parameter_storage_class = parameters->kind;
    if (declarator->resolved_type->shape->flags & 0x4000)
        c1_bss.parameter_storage_class = 5;
    if (c1_bss.parameter_storage_class == 2) {
        report_language_diagnostic(4, 131, symbol);
        symbol->declaration_flags |= 0x10;
    } else symbol->declaration_flags &= ~0x10;
    return symbol;
}
void RECON_THISCALL CompilerIrSymbol::SetDeclarationContext(void *context)
{
    if (context) attach_declaration_context(this, context);
    else function_flags &= ~1;
}
CompilerIrSymbol **RECON_THISCALL CompilerScopeTable::MergeDeclaration(CompilerIrSymbol *symbol)
{
    CompilerIrSymbol **found = (CompilerIrSymbol **)Find(symbol->binding.name);
    if (!found) return NULL;
    CompilerIrSymbol *selected = merge_scoped_declaration(*found, symbol, 0);
    if (!selected) return NULL;
    CompilerArena *arena = &c1_bss.arenas[3];
    CompilerScopeLookup *result = (CompilerScopeLookup *)arena->cursor;
    arena->cursor += 8;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (!result) return NULL;
    result->symbol = selected;
    result->scope = NULL;
    return (CompilerIrSymbol **)result;
}
extern "C" unsigned __cdecl assign_symbol_output_index(CompilerIrSymbol *);
CompilerIrSymbol **RECON_THISCALL CompilerTranslationUnit::FindScopedDeclaration(CompilerIrSymbol *added)
{
    CompilerIrSymbol *previous, *selected, *context;
    CompilerScopeLookup *found, *result;
    CompilerParameterBindings *parameters;
    CompilerSymbolBinding *binding;
    CompilerArena *arena;
    unsigned index;
    if (c1_bss.parameter_nesting == 2 && current->level != 1) {
        parameters = (CompilerParameterBindings *)c1_bss.parameter_scope_value;
        binding = parameters ? parameters->first : NULL;
        for (; binding; binding = binding->next) {
            if (binding->name == added->binding.name) {
                compiler_warning(82, added);
                selected = added;
                goto make_result;
            }
        }
    }
    if ((added->storage_class & 7) != 1) return NULL;
    found = (CompilerScopeLookup *)root->Find(added->binding.name);
    previous = found ? found->symbol : NULL;
    if (!previous || previous->kind == 4) return NULL;
    if (previous->kind >= 1 && previous->kind <= 14 && previous->kind != 5 &&
        (previous->storage_class & 7) == 7) return NULL;
    c1_data.parser_flags |= 0x2000;
    if (check_scoped_declaration(previous, added, 0)) {
        c1_data.parser_flags &= ~0x2000U;
        return NULL;
    }
    c1_data.parser_flags &= ~0x2000U;
    added->record_id = (added->record_id & 0xff000000) | (previous->record_id & 0xffffff);
    if (c1_data.c9_il) index = previous->record_id & 0xffffff;
    else if (previous->reference_flags & ~7U) index = previous->reference_flags >> 3;
    else index = assign_symbol_output_index(previous);
    added->reference_flags = (index << 3) | (added->reference_flags & 7);
    added->value_type = previous->value_type;
    added->qualifier_flags = (added->qualifier_flags & ~4U) | (previous->qualifier_flags & 4);
    if (previous->function_flags & 1) {
        context = lookup_ir_symbol(previous);
        if (context) attach_declaration_context(added, context);
        else added->function_flags &= ~1;
    }
    added->qualifier_flags = (added->qualifier_flags & ~1U) | (previous->qualifier_flags & 1);
    current->HasBuckets();
    if (previous->kind == 6) {
        added->storage_class = (added->storage_class & ~7) | 1;
        added->kind = 6;
        added->intrinsic_opcode = previous->intrinsic_opcode;
        added->intrinsic_traits = previous->intrinsic_traits;
    }
    if ((previous->reference_flags & 3) < 3)
        previous->reference_flags = (previous->reference_flags & ~3U) |
            ((previous->reference_flags + 1) & 3);
    previous->reference_flags = (previous->reference_flags & ~4U) |
        ((c1_data.ir_reference_epoch & 1) << 2);
    added->flags |= 2;
    previous->flags &= ~2;
    selected = previous;
make_result:
    arena = &c1_bss.arenas[3];
    result = (CompilerScopeLookup *)arena->cursor;
    arena->cursor += 8;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (!result) return NULL;
    result->symbol = selected;
    result->scope = NULL;
    return (CompilerIrSymbol **)result;
}
extern "C" void __cdecl discard_declaration_symbol(CompilerIrSymbol *symbol)
{
    unsigned index;
    CompilerSymbolBinding *binding;
    if (c1_bss.parameter_nesting) return;
    if (!((symbol->kind == 7 && c1_data.function_symbol_cache_index == 9) ||
          (symbol->kind == 10 && c1_data.static_symbol_cache_index == 9))) {
#ifdef __linux__
        recon_unsupported("discarded symbol kind/cache selector is not yet recovered");
#else
        __asm { int 3 }
#endif
    }
    index = symbol->kind == 7 ? c1_data.function_symbol_cache_index : c1_data.static_symbol_cache_index;
    binding = &symbol->binding;
    binding->next = c1_data.symbol_cache.indexed[index];
    c1_data.symbol_cache.indexed[index] = binding;
}
extern "C" int __cdecl uses_global_function_record(CompilerIrSymbol *);
extern "C" CompilerNameEntry *__cdecl make_static_declaration_name(CompilerIrSymbol *);
extern "C" void __cdecl link_static_declaration(CompilerIrSymbol *, CompilerIrSymbol *);
extern "C" void __cdecl record_scoped_declaration(CompilerIrSymbol *symbol)
{
    CompilerIrSymbol *copy, *context, *function;
    BYTE *memory;
    unsigned size, index;
    if (c1_data.options.language_extensions && (symbol->kind == 7 || symbol->kind == 8)) {
        report_language_diagnostic(4, 210);
        /* Function-record size rows have not been recovered. The original
         * indexed allocation/copy remains visible below, but neither build
         * may consume an unknown row. */
        if (symbol->kind != 10) {
#ifdef __linux__
            recon_unsupported("scoped function-copy record size is not yet recovered");
#else
            __asm { int 3 }
#endif
        }
        size = c1_data.record_sizes.indexed[symbol->kind];
        memory = c1_bss.arenas[0].AllocateBytes((size + 3) & ~3U);
        copy = memory ? ((CompilerIrSymbol *)memory)->CopyFrom(symbol, 0) : NULL;
        copy->qualifier_flags |= 0x20;
        c1_data.translation_unit->root->Insert(copy);
        if ((copy->reference_flags & 3) < 3)
            copy->reference_flags = (copy->reference_flags & ~3U) | ((copy->reference_flags + 1) & 3);
        copy->reference_flags = (copy->reference_flags & ~4U) | ((c1_data.ir_reference_epoch & 1) << 2);
        symbol->flags |= 2;
        return;
    }
    if (symbol->kind != 10 || (symbol->storage_class & 7) != 3) return;
    if (!c1_data.c9_il && !uses_global_function_record(c1_bss.current_scope_symbol)) return;
    size = c1_data.record_sizes.indexed[symbol->kind];
    if (size != 52) {
#ifdef __linux__
        recon_unsupported("scoped static-copy record extent is not yet supported");
#else
        __asm { int 3 }
#endif
    }
    memory = c1_bss.arenas[0].AllocateBytes((size + 3) & ~3U);
    copy = memory ? ((CompilerIrSymbol *)memory)->CopyFrom(symbol, 0) : NULL;
    copy->binding.name = make_static_declaration_name(symbol);
    if (c1_data.next_symbol_record_id >= 0x7fffffff) compiler_fatal(55);
    index = c1_data.next_symbol_record_id++;
    copy->record_id = (copy->record_id & 0xff000000) | (index & 0xffffff);
    if (symbol->function_flags & 1) {
        context = lookup_ir_symbol(symbol);
        if (context) attach_declaration_context(copy, context);
        else copy->function_flags &= ~1;
    }
    copy->flags |= 1;
    symbol->declaration_flags |= 0x40;
    link_static_declaration(symbol, copy);
    c1_data.translation_unit->root->Insert(copy);
    function = c1_bss.current_scope_symbol;
    if (!function || !(function->symbol_qualifiers & 1) ||
        (function->storage_class & 7) == 3) return;
    if (function->linkage_flags & 12) {
        copy->linkage_flags = (copy->linkage_flags & ~12) | (function->linkage_flags & 12);
        symbol->intrinsic_flags = copy->intrinsic_flags;
    }
    copy->storage_class = (copy->storage_class & ~7) | 0x42;
}
extern "C" char *__cdecl format_static_declaration_name(CompilerIrSymbol *, unsigned);
extern "C" CompilerNameEntry *__cdecl find_or_create_identifier(const char *);
extern "C" CompilerNameEntry *__cdecl make_static_declaration_name(CompilerIrSymbol *symbol)
{
    return find_or_create_identifier(format_static_declaration_name(symbol,
        c1_data.function_label_depth));
}

extern "C" void *__cdecl create_enum_context(unsigned, unsigned, unsigned, unsigned, int, int);
extern "C" CompilerSymbolMapEntry *__cdecl append_enum_symbol(CompilerSymbolMap *, void *, CompilerIrSymbol *);
extern "C" void __cdecl link_static_declaration(CompilerIrSymbol *source, CompilerIrSymbol *copy)
{
    if (!c1_data.function_scope_cache)
        c1_data.function_scope_cache = (CompilerSymbolMap *)create_enum_context(31, 2, 2, 1, 1, 1);
    append_enum_symbol(c1_data.function_scope_cache, source, copy);
}
extern "C" unsigned __cdecl hash_static_symbol(void *symbol)
{ return ((unsigned)symbol >> 2) & 0xfff; }
extern "C" int __cdecl static_symbols_equal(void *left, void *right)
{ return left == right; }
unsigned RECON_THISCALL CompilerDeclarationFlagView::QualifierCode(void)
{
    return (flags >> 18) & 3;
}
extern "C" CompilerType *__cdecl intern_type(const CompilerType *);
CompilerType *RECON_THISCALL CompilerType::FunctionResult(void)
{
    CompilerType description;
#ifdef RECON_NATIVE_TEST
    /* The interner consumes the established twelve-byte type key only. */
    description.flags = flags;
    description.attributes = attributes;
    description.shape = shape;
    description.auxiliary = auxiliary;
#else
    description = *this;
#endif
    description.attributes &= 0xff8f;
    description.shape = shape->base;
    return intern_type(&description);
}

void RECON_THISCALL CompilerDeclarationContext::SetContext(void *value)
{
    context = value;
    if (value) flags |= 0x1000000;
    else flags &= ~0x1000000U;
}
extern "C" void __cdecl validate_declaration_type(CompilerIrSymbol *symbol, int typedef_name)
{
    CompilerType *type = symbol->value_type;
    CompilerTypeShape *shape = type->shape;
#ifdef RECON_NATIVE_TEST
    if (symbol->kind == 10 && (typedef_name || symbol->storage_class == 3) && WIDE_INTEGER_TYPE(type)) {
        /* Explicit64-bit typedefs and static scalars retain their canonical type. */
    } else if (symbol->kind == 13 && !typedef_name &&
        (SCALAR_ARITHMETIC_TYPE(type) || type == c1_data.builtin_types.type_0063 || type == c1_data.builtin_types.type_4063_first || CDECL_CALLBACK_TABLE_POINTER(type) || CONST_INT_ROW_POINTER(type) || COMPLETE_RECORD_POINTER(type) ||
         (!shape && (type->flags == 0x402 || type->flags == 0x404) &&
          !(type->attributes & ~0x100U) && type->symbol_owner &&
          type->symbol_owner->kind == 4 && (type->symbol_owner->flags & 1)))) {
        /* Reached abstract int and complete record/union type names retain
         * their established type; no object storage or array rewrite. */
    } else if (symbol->kind == 12 && !typedef_name && type == c1_data.builtin_types.type_0080) {
        /* An unnamed void parameter retains the already interned scalar type. */
    } else if (symbol->kind == 11 || symbol->kind == 12) {
        if (typedef_name || !(DECLARED_INT_CALLBACK(type) || SCALAR_ARITHMETIC_TYPE(type) ||
            (symbol->kind == 11 && type->flags == 0x62 && !type->attributes && !shape && !type->auxiliary) ||
            VOLATILE_SHORT_POINTER(type) || CONST_CHAR_POINTER(type) || CONST_INT_POINTER(type) || COMPLETE_RECORD_VALUE(type) || COMPLETE_RECORD_POINTER(type) ||
            type == c1_data.builtin_types.type_0063 ||
            type == c1_data.builtin_types.type_4063_first ||
            (shape && (shape->flags & 7) == 1 && type->flags == 0x402 &&
             type->attributes == 0x100 && type->symbol_owner && type->symbol_owner->kind == 4 &&
             (type->symbol_owner->flags & 1))))
            recon_unsupported("parameter pointer type domain is not yet modeled");
    } else if (symbol->kind == 13 && !typedef_name && CDECL_INT_CALLBACK(type)) {
        /* Abstract callback type used by sizeof; its shape remains unchanged. */
    } else if (symbol->kind == 10 && !typedef_name &&
               (CDECL_CALLBACK_TABLE_POINTER(type) || CONST_CHAR_POINTER(type) || VOLATILE_SHORT_ARRAY(type) || FLOATING_ARRAY(type) || VOID_POINTER(type) || CONST_INT_ROW_POINTER(type) || CDECL_INT_CALLBACK(type) || CONST_INT_MATRIX(type) || COMPLETE_RECORD_VALUE(type) || CDECL_INT_CALLBACK_ARRAY(type) ||
               CONST_CHAR_POINTER_ARRAY(type))) {
        /* Complete record values and both array dimensions have ordinary
         * declarator producers; no inferred storage/table domain is added. */
    } else if (symbol->kind == 3 && !typedef_name && type->flags == 0x402 &&
        !type->attributes && shape && shape->flags == 1 && !shape->attributes &&
        !shape->base && type->symbol_owner && type->symbol_owner->kind == 4) {
        /* A self-referential record pointer needs no complete pointee read. */
    } else if (symbol->kind == 10 && typedef_name && DECLARED_INT_CALLBACK(type)) {
        /* The explicit cdecl callback typedef retains its pointer/function
         * shape chain. No parameter-list or pointee bytes are consumed. */
    } else if (symbol->kind == 10 && !typedef_name && type->flags == 0x63 && type->attributes == 0x100 &&
        shape && shape->flags == 2 && !shape->attributes && !shape->base && shape->value) {
        /* Complete const-int array: the real qualifier and bound writers
         * retain the same established primitive and array shape. */
    } else if (symbol->kind == 10 && !typedef_name && (INT_ARRAY_TYPE(type) || INT_MATRIX_TYPE(type) || CHAR_ARRAY_TYPE(type))) {
        /* An int array object has the existing bound/qualifier writers below,
         * including the zero bound of an incomplete extern declaration. */
    } else if (symbol->kind == 10 && typedef_name && FIXED_INT_ARRAY(type)) {
        /* A fixed-size int array typedef retains its existing shape. The
         * ordinary array branch below performs no qualifier update for it. */
    } else if (symbol->kind == 10 && !typedef_name && shape &&
        (shape->flags & 7) == 1 && type->flags == 0x402 &&
        !(type->attributes & ~0x100U) && type->symbol_owner &&
        type->symbol_owner->kind == 4 && (type->symbol_owner->flags & 1)) {
        /* The local complete-record pointer retains the same known type,
         * just as the parameter pointer above; no array/function rewrite. */
    } else if (symbol->kind == 10 && !typedef_name && shape && shape->flags == 2 &&
        !shape->base && !shape->value && (type->flags == 0x61 || type->flags == 0x4062) && type->attributes == 0x100) {
        /* The reached const-char/ushort arrays have omitted bounds; the real branch
         * below marks that bound for completion by the string initializer. */
    } else if (symbol->kind == 10 && !typedef_name &&
               (symbol->storage_class & 7) == 3 && VOLATILE_UNSIGNED_INT(type)) {
        /* The volatile unsigned static retains its existing scalar type. */
    } else if (symbol->kind == 10 && !typedef_name && !shape &&
        (SCALAR_ARITHMETIC_TYPE(type) || type == c1_data.builtin_types.type_0063 ||
         type == c1_data.builtin_types.type_4063_first)) {
        /* Reached scalar declarations retain their interned primitive type. */
    } else if (symbol->kind == 10 && !typedef_name && shape &&
        (shape->flags & 7) == 2 && type->flags == 0x402 && !(type->attributes & ~0x100U) &&
        type->symbol_owner && type->symbol_owner->kind == 4 && (type->symbol_owner->flags & 1)) {
        /* Current complete record-array domain. Its qualifier bit is updated
         * by the genuine array branch below. */
    } else if (symbol->kind == 10 && !typedef_name && !shape && !type->attributes && type->flags == 0x404 &&
               type->symbol_owner && type->symbol_owner->kind == 4 && (type->symbol_owner->flags & 1)) {
        /* An initialized complete union retains its established tag owner. */
    } else if (symbol->kind == 3 && !typedef_name && DECLARED_RECORD_MEMBER_TYPE(type)) {
        /* Ordinary arithmetic record members retain their canonical type. */
    } else if (symbol->kind == 3 && !typedef_name && !shape && !type->attributes && (type->flags == 0x4062 || type->flags == 0x64)) {
        /* The reached ushort/long record members retain their primitive types. */
    } else if (symbol->kind == 3 && !typedef_name && !type->attributes && type->flags == 0x4061 &&
               shape && shape->flags == 2 && !shape->base && shape->value) {
        /* A complete unsigned-char member array needs no type rewrite. */
    } else if ((symbol->kind != 10 && !((symbol->kind == 2 || symbol->kind == 3) && !typedef_name &&
          (type == c1_data.builtin_types.type_0063 ||
             ((symbol->kind == 2 || symbol->kind == 3) && type == c1_data.builtin_types.type_4063_first)))) ||
        ((!shape || (shape->flags & 7) != 4) &&
         !((typedef_name || symbol->kind == 2 || symbol->kind == 3) && (type == c1_data.builtin_types.type_0063 ||
             ((symbol->kind == 2 || symbol->kind == 3) && type == c1_data.builtin_types.type_4063_first) ||
             ((typedef_name || symbol->kind == 10) && !shape && (type->flags == 0x402 || type->flags == 0x404) && type->symbol_owner &&
              (type->symbol_owner->flags & 1))))))
        recon_unsupported("nonfunction declaration type domain is not yet modeled");
#endif
    if (shape) {
        if ((shape->flags & 7) == 4) {
            if (symbol->kind == 10) symbol->kind = 7;
            else {
                if (symbol->kind >= 2 && symbol->kind <= 3) {
                    if (c1_constant_diagnostic_mode != 0) {
#ifdef __linux__
                        recon_unsupported("diagnostic record-definition index is not yet modeled");
#else
                        __asm { int 3 }
#endif
                    }
                    CompilerIrSymbol *owner = c1_bss.record_definitions[c1_constant_diagnostic_mode].symbol;
                    compiler_warning(32, symbol, owner->value_type, owner);
                } else if (symbol->kind == 13) compiler_warning(66);
                type = type->DecayFunction();
            }
        } else if ((shape->flags & 7) == 2) {
            if (symbol->kind == 11 || symbol->kind == 12) {
                if (shape->flags & 0x1000) compiler_warning(540);
                type = type->DecayArray();
            } else if (!typedef_name) {
                symbol->qualifier_flags |= 4;
                if (!(shape->flags & 0x1000) && !shape->value)
                    symbol->qualifier_flags |= 1;
            }
        }
    }
    symbol->value_type = type;
}
extern "C" CompilerTypeShape *__cdecl intern_type_shape(CompilerTypeShape *);
extern "C" CompilerType *__cdecl intern_type(const CompilerType *);
CompilerType *RECON_THISCALL CompilerType::WithShapeFlags(unsigned bits)
{
    if (!shape) return this;
    CompilerTypeShape modified = *shape;
    modified.flags |= (WORD)bits;
    CompilerType description;
#ifdef RECON_NATIVE_TEST
    /* Only the interner's established12-byte semantic key is transferred.
     * The original also copies its separate, unused debug-index DWORD. */
    description.flags = flags;
    description.attributes = attributes;
    description.shape = shape;
    description.auxiliary = auxiliary;
#else
    description = *this;
#endif
    description.shape = intern_type_shape(&modified);
    return intern_type(&description);
}
CompilerType *RECON_THISCALL CompilerType::WithFunctionConvention(unsigned) { __asm { int 3 } return NULL; }
CompilerType *RECON_THISCALL CompilerType::DecayFunction(void) { __asm { int 3 } return NULL; }
CompilerType *RECON_THISCALL CompilerType::DecayArray(void) { __asm { int 3 } return NULL; }

extern "C" void __cdecl compiler_warning_v(unsigned, va_list);
extern "C" void __cdecl report_extension_diagnostic(unsigned extension, unsigned warning, ...)
{
    va_list arguments;
    if (c1_data.options.language_extensions)
        report_language_diagnostic(4, extension);
    else {
        va_start(arguments, warning);
        compiler_warning_v(warning, arguments);
        va_end(arguments);
    }
}

extern "C" int __cdecl check_scoped_declaration(CompilerIrSymbol *, CompilerIrSymbol *, int);
extern "C" void __cdecl copy_declaration_properties(CompilerIrSymbol *, CompilerIrSymbol *);
extern "C" CompilerIrSymbol *__cdecl merge_scoped_declaration(CompilerIrSymbol *previous,
    CompilerIrSymbol *added, int mode)
{
    unsigned previous_storage;
    int error;
    if (!previous) return NULL;
    previous_storage = previous->storage_class & 7;
    if (c1_data.options.language_extensions && previous_storage == 7 &&
        (added->storage_class & 7) == previous_storage && previous->value_type == added->value_type)
        return previous;
    if (c1_bss.parameter_nesting) {
        error = check_scoped_declaration(previous, added, mode);
        if (previous_storage == 1 && (added->storage_class & 7) == 1) {
            if (!error) return previous;
        } else if (!error) compiler_warning(86, added);
    } else {
        error = check_scoped_declaration(previous, added, mode);
        if (!error) {
            copy_declaration_properties(previous, added);
            return previous;
        }
    }
    added->scope_bits |= 1;
    return added;
}
extern "C" int __cdecl compatible_declaration_flags(WORD, WORD);
extern "C" int __cdecl compare_declaration_types(CompilerIrSymbol *, CompilerIrSymbol *, CompilerType *, int);
extern "C" int __cdecl compatible_declaration_qualifiers(WORD, WORD, CompilerIrSymbol *, int);
extern "C" int __cdecl compatible_declaration_storage(CompilerIrSymbol *, CompilerIrSymbol *);
extern "C" int __cdecl check_scoped_declaration(CompilerIrSymbol *previous,
    CompilerIrSymbol *added, int mode)
{
    int diagnostic;
    CompilerIrSymbol *context;
    if (added->scope_bits & 1) {
        previous->scope_bits |= 1;
        return 1;
    }
    if (!compatible_declaration_flags(previous->value_type->flags, added->value_type->flags) ||
        previous->value_type->auxiliary != added->value_type->auxiliary) {
        diagnostic = 371;
        goto error;
    }
    diagnostic = compare_declaration_types(previous, added, 0, mode);
    if (diagnostic) {
        if (diagnostic != 40) goto error;
        added->scope_bits |= 1;
        previous->scope_bits |= 1;
        compiler_warning(40, 83, previous, added->value_type, previous->value_type);
        return 1;
    }
    if (!compatible_declaration_storage(previous, added)) {
        diagnostic = previous->kind == 7 || previous->kind == 8 ? 375 : 370;
        goto error;
    }
    if (!compatible_declaration_qualifiers(previous->value_type->attributes,
            added->value_type->attributes, previous, mode)) {
        diagnostic = 373;
        goto error;
    }
    if ((previous->declaration_flags & 4) && (added->declaration_flags & 4)) {
        diagnostic = 374;
        goto error;
    }
    if (added->function_flags & 1) {
        if (!(previous->function_flags & 1)) {
            context = lookup_ir_symbol(added);
            if (context) attach_declaration_context(previous, context);
            else previous->function_flags &= ~1;
            return 0;
        }
        context = lookup_ir_symbol(added);
        if (lookup_ir_symbol(previous) != context) {
            diagnostic = 368;
            goto error;
        }
    }
    if ((added->linkage_flags & 2) && !(previous->linkage_flags & 2)) {
        diagnostic = 370;
        goto error;
    }
    return 0;
error:
    added->scope_bits |= 1;
    previous->scope_bits |= 1;
    compiler_warning(diagnostic, previous);
    return 1;
}
extern "C" const signed char c1_primitive_widths[16];
extern "C" int __cdecl compatible_declaration_flags(WORD previous, WORD added)
{
    int previous_width, added_width;
    if (!((previous ^ added) & 0xefcf)) return 1;
    previous_width = (previous & 0x340) || !(previous & 0xfff0) ?
        c1_primitive_widths[previous & 15] : previous & 15;
    added_width = (added & 0x340) || !(added & 0xfff0) ?
        c1_primitive_widths[added & 15] : added & 15;
    if (previous_width != added_width) return 0;
    report_language_diagnostic(1, 142);
    return 1;
}
extern "C" CompilerType *__cdecl compose_declaration_type(CompilerType *, CompilerType *, int, int, int);
extern "C" int __cdecl compare_declaration_types(CompilerIrSymbol *previous,
    CompilerIrSymbol *added, CompilerType *type, int mode)
{
    CompilerTypeShape *left = previous->value_type->shape;
    CompilerTypeShape *right = added ? added->value_type->shape : type->shape;
    CompilerIrSymbol *left_context, *right_context;
    unsigned depth = 0;
    if (added && (added->function_flags & 1)) {
        if (!(previous->function_flags & 1)) {
            right_context = lookup_ir_symbol(added);
            if (right_context) attach_declaration_context(previous, right_context);
            else previous->function_flags &= ~1;
        } else {
            right_context = lookup_ir_symbol(added);
            left_context = lookup_ir_symbol(previous);
            if (left_context != right_context &&
                (lookup_ir_symbol(previous)->record_id & 0xffffff) != 5 &&
                (lookup_ir_symbol(added)->record_id & 0xffffff) != 2)
                return 376;
        }
    } else if (previous->function_flags & 1) {
        left_context = lookup_ir_symbol(previous);
        if (left_context) attach_declaration_context(added, left_context);
        else added->function_flags &= ~1;
    }
    while (left != right) {
        if (!left || !right) return 40;
        ++depth;
        if ((left->flags ^ right->flags) & 7) return 372;
        if (!compatible_declaration_qualifiers(left->attributes, right->attributes, previous, mode))
            return 373;
        switch (left->flags & 7) {
        case 2:
            if (left->value != right->value &&
                (depth != 1 || (left->value && right->value))) return 369;
            break;
        case 4:
            if (added && !(added->flags & 1)) {
                if ((left->flags ^ right->flags) & 0x8000) {
                    if (c1_data.options.language_extensions) report_language_diagnostic(4, 52);
                    else compiler_warning(142);
                }
                if (left->value != right->value) {
                    CompilerType *composite = compose_declaration_type(previous->value_type,
                        added->value_type, 1, depth == 1, previous->kind == 6);
                    if (depth == 1) previous->value_type = composite;
                }
            }
            break;
        }
        left = left->base;
        right = right->base;
    }
    return 0;
}
extern "C" CompilerType *__cdecl compose_declaration_type(CompilerType *, CompilerType *, int, int, int) { __asm { int 3 } return NULL; }
extern "C" int __cdecl compatible_declaration_qualifiers(WORD previous, WORD added,
    CompilerIrSymbol *, int)
{
    if ((previous ^ added) & 0xfffc) return 0;
    if (previous == added || !(previous & 3) || !(added & 3)) return 1;
    return 0;
}
extern "C" int __cdecl compatible_declaration_storage(CompilerIrSymbol *previous,
    CompilerIrSymbol *added)
{
    unsigned old_storage = previous->storage_class & 7;
    unsigned new_storage = added->storage_class & 7;
    if (previous->kind == 7 || previous->kind == 8 || !(previous->declaration_flags & 4)) {
        if ((old_storage == new_storage || (old_storage == 1 && new_storage == 2)) &&
            !((previous->linkage_flags ^ added->linkage_flags) & 15)) return 1;
    }
    if (new_storage == 3) {
        if (!c1_data.options.language_extensions || old_storage != 1 || (previous->flags & 1))
            return 0;
        report_language_diagnostic(4, 211);
        return 1;
    }
    if ((old_storage == 7 || new_storage == 7) && !c1_bss.parameter_nesting) return 0;
    if (previous->linkage_flags & 12) return 1;
    return !(added->linkage_flags & 12);
}
extern "C" void __cdecl copy_declaration_properties(CompilerIrSymbol *previous,
    CompilerIrSymbol *added)
{
    CompilerIrSymbol *context;
    CompilerTypeShape *shape;
    int changed = 0;
    if (previous->function_flags & 1) {
        if (added->function_flags & 1) {
            CompilerIrSymbol *added_context = lookup_ir_symbol(added);
            context = lookup_ir_symbol(previous);
            if (context != added_context) {
                context = lookup_ir_symbol(added);
                if (context) attach_declaration_context(previous, context);
                else previous->function_flags &= ~1;
            }
        }
    } else if (added->function_flags & 1) {
        context = lookup_ir_symbol(added);
        if (context) attach_declaration_context(previous, context);
        else previous->function_flags &= ~1;
    }
    shape = added->value_type->shape;
    if (shape) {
        if ((shape->flags & 7) == 2) {
            if (shape->value && !previous->value_type->shape->value) {
                previous->value_type = added->value_type;
                previous->qualifier_flags &= ~1;
            }
        } else if ((shape->flags & 7) == 4 && (added->flags & 1)) {
            if (previous->flags & 1) {
                if (!(previous->scope_bits & 1)) {
                    compiler_warning(84, previous);
                    previous->scope_bits |= 1;
                }
            } else previous->flags |= 1;
        }
    }
    if (((previous->linkage_flags & 4) && (added->linkage_flags & 8)) ||
        ((previous->linkage_flags & 8) && !(added->linkage_flags & 8) &&
         (added->storage_class & 7) != 3 && !(added->symbol_qualifiers & 1) &&
         !(added->scope_bits & 1) && !(previous->scope_bits & 1))) {
        report_language_diagnostic(1, 273, previous);
        previous->storage_class = (previous->storage_class & ~7) | 1;
        previous->linkage_flags = (previous->linkage_flags & ~8) | 4;
    }
    if ((previous->storage_class & 7) == 1 && (added->storage_class & 7) != 1) {
        if (previous->symbol_qualifiers & 0x10) added->symbol_qualifiers |= 0x10;
        added->linkage_flags = (added->linkage_flags & ~12) | (previous->linkage_flags & 12);
        previous->intrinsic_flags = added->intrinsic_flags;
    }
    if (added->flags & 1) previous->flags |= 1;
    if (added->declaration_flags & 4) previous->declaration_flags |= 4;
    if ((added->symbol_qualifiers & 1) && !(previous->symbol_qualifiers & 1)) {
        previous->symbol_qualifiers |= 1;
        changed = 1;
    }
    if ((added->storage_class & 0x40) && !(previous->storage_class & 0x40)) {
        previous->storage_class |= 0x40;
        changed = 1;
    }
    if ((added->symbol_qualifiers & 0x80) && !(previous->symbol_qualifiers & 0x80)) {
        previous->symbol_qualifiers |= 0x80;
        changed = 1;
    }
    if ((added->linkage_flags & 0x40) && !(previous->linkage_flags & 0x40)) {
        previous->linkage_flags |= 0x40;
        changed = 1;
    }
    if ((added->symbol_qualifiers & 0x10) && !(previous->symbol_qualifiers & 0x10)) {
        previous->symbol_qualifiers |= 0x10;
        changed = 1;
    }
    if ((added->linkage_flags & 1) && !(previous->linkage_flags & 1)) {
        previous->linkage_flags |= 1;
        changed = 1;
    }
    if ((previous->linkage_flags ^ added->linkage_flags) & 12) {
        if (added->linkage_flags & 4) {
            previous->linkage_flags = (previous->linkage_flags & ~8) | 4;
            changed = 1;
        } else if ((added->linkage_flags & 8) && !(previous->linkage_flags & 4)) {
            previous->linkage_flags |= 8;
            changed = 1;
        }
    }
    if (changed) previous->flags &= ~2;
}

void *RECON_THISCALL CompilerSymbolTable::Find(CompilerNameEntry *name)
{
    if (!buckets) return NULL;
    if (bucket_mask != 1023 && bucket_mask != 15) {
#ifdef __linux__
        recon_unsupported("scope lookup bucket extent is not yet modeled");
#else
        __asm { int 3 }
#endif
    }
    unsigned index = (name->flags & bucket_mask) & 0xffff;
    CompilerIrSymbol *symbol = find_named_symbol(buckets[index], name);
    if (!symbol) return NULL;
    CompilerArena *arena = &c1_bss.arenas[3];
    CompilerScopeLookup *result = (CompilerScopeLookup *)arena->cursor;
    arena->cursor += 8;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (!result) return NULL;
    result->symbol = symbol;
    result->scope = this;
    return result;
}
extern "C" CompilerIrSymbol *__cdecl find_named_symbol(CompilerSymbolBinding *binding,
    CompilerNameEntry *name)
{
    for (; binding; binding = binding->next) {
        if (binding->name == name) {
            CompilerIrSymbol *symbol = (CompilerIrSymbol *)((BYTE *)binding - 8);
            if (symbol->kind != 4) return symbol;
        }
    }
    return NULL;
}

extern "C" unsigned __cdecl global_type_size(CompilerType *);
extern "C" void __cdecl assign_symbol_segment(CompilerIrSymbol *, CompilerIrSymbol *);
extern "C" void __cdecl finalize_declared_symbol(CompilerIrSymbol *symbol)
{
    unsigned storage = symbol->storage_class & 7;
    CompilerIrSymbol *tag;
    /* Automatic primitive and encountered complete-record pointer declarations
     * use no segment-default owner. */
    if (storage != 7 && storage != 3 &&
        !(storage == 1 && !(symbol->declaration_flags & 4)) &&
        !(storage == 2 && symbol->kind == 10 && (symbol->declaration_flags & 4) &&
          INT_ARRAY_TYPE(symbol->value_type)) &&
        !(storage == 4 && symbol->kind == 10 && !symbol->function_flags &&
          (SCALAR_ARITHMETIC_TYPE(symbol->value_type) || INT_ARRAY_TYPE(symbol->value_type) ||
           FLOATING_ARRAY(symbol->value_type) || CHAR_ARRAY_TYPE(symbol->value_type) || CONST_CHAR_POINTER_ARRAY(symbol->value_type) ||
           symbol->value_type == c1_data.builtin_types.type_0063 ||
           symbol->value_type == c1_data.builtin_types.type_4063_first ||
           CDECL_INT_CALLBACK(symbol->value_type) || CDECL_CALLBACK_TABLE_POINTER(symbol->value_type) || CONST_INT_ROW_POINTER(symbol->value_type) ||
           VOID_POINTER(symbol->value_type) ||
           (!symbol->value_type->shape && !symbol->value_type->attributes &&
            (symbol->value_type->flags == 0x402 || symbol->value_type->flags == 0x404) &&
            symbol->value_type->symbol_owner && symbol->value_type->symbol_owner->kind == 4 &&
            (symbol->value_type->symbol_owner->flags & 1)) ||
           (symbol->value_type->shape && (symbol->value_type->shape->flags & 7) == 1 &&
            symbol->value_type->flags == 0x402 && !(symbol->value_type->attributes & ~0x100U) &&
            symbol->value_type->symbol_owner && symbol->value_type->symbol_owner->kind == 4 &&
            (symbol->value_type->symbol_owner->flags & 1))))) {
#ifdef __linux__
        recon_unsupported("declaration segment/default domain is not yet recovered");
#else
        __asm { int 3 }
#endif
    }
    switch (storage) {
    case 1:
        if (!(symbol->declaration_flags & 4)) return;
        /* fall through */
    case 2:
    case 3:
        if (symbol->value_type->GetCallingConvention() & 0x100) {
            if (declaration_reference_defaults.type_context) {
                if (!(symbol->function_flags & 1) || (symbol->function_flags & 2)) {
                    symbol->SetDeclarationContext(declaration_reference_defaults.type_context);
                    symbol->function_flags |= 2;
                }
            } else if (symbol->function_flags & 2) symbol->SetDeclarationContext(NULL);
        } else {
            assign_symbol_segment(symbol, (symbol->declaration_flags & 4) ?
                declaration_reference_defaults.definition : declaration_reference_defaults.fallback);
        }
        break;
    case 4:
    case 5:
    case 6:
        if (symbol->function_flags & 1) compiler_warning(185, symbol);
        break;
    case 7:
        return;
    }
    if (symbol->declaration_flags & 4) return;
    if (global_type_size(symbol->value_type) || storage == 1) return;
    if (symbol->value_type->flags & 0x400) {
        tag = symbol->value_type->symbol_owner;
        if (tag->kind != 4) tag = tag->value_type->symbol_owner;
        if (!(tag->flags & 1)) {
            compiler_warning(79, symbol, tag->value_type, tag);
            symbol->scope_bits |= 1;
            return;
        }
    }
    compiler_warning(133, symbol);
}
extern "C" void __cdecl assign_symbol_segment(CompilerIrSymbol *symbol, CompilerIrSymbol *segment)
{
    if (!segment) {
        if (symbol->function_flags & 2) symbol->function_flags &= ~1;
        return;
    }
    if ((symbol->function_flags & 3) == 1) return;
    attach_declaration_context(symbol, segment);
    symbol->function_flags |= 2;
}

CompilerIrSymbol *RECON_THISCALL CompilerIrSymbol::CopyFrom(CompilerIrSymbol *source, unsigned arena)
{
    unsigned bytes;
    /* This copy consumes the complete storage, including uninterpreted bytes.
     * The encountered static-symbol owner and its size row are both52 bytes;
     * other original kinds remain valid but unsupported here. */
    if (source->kind != 10) {
#ifdef __linux__
        recon_unsupported("symbol copy kind has no encountered storage contract");
#else
        __asm { int 3 }
#endif
    }
    bytes = c1_data.record_sizes.indexed[source->kind];
    if (bytes != 52) {
#ifdef __linux__
        recon_unsupported("symbol copy size differs from the encountered owner");
#else
        __asm { int 3 }
#endif
    }
    memcpy(this, source, bytes);
    binding.name = source->binding.name;
    binding.next = NULL;
    if (source->kind == 4) kind4_context = source->kind4_context;
    return this;
}
