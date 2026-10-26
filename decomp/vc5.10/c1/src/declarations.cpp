#include "frontend.h"
#include "keywords.h"
#include <recon.h>

/* The option at5ef9c is source-only until its actual consumer is reached.
 * The current native prefix stops at PrepareFunctionDeclarator first. */
static unsigned declaration_option_5ef9c;
extern int c1_constant_diagnostic_mode;
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
/* Only this first pointer is interpreted by an unvisited diagnostic path.
 * Original slots have24-byte stride; neither the count nor their defaults
 * are modeled. The native index guard precedes every access. */
struct CompilerDiagnosticSlot {
    CompilerIrSymbol *symbol;
    BYTE unmodeled_04[20];
};
static CompilerDiagnosticSlot diagnostic_slots[1]; /* RVA53be8, source-only */
extern "C" {
void __cdecl compiler_warning(int, ...);
CompilerIrSymbol *__cdecl prepare_function_declarator(CompilerIrSymbol *, CompilerDeclarator *);
void __cdecl declare_symbol(CompilerDeclarator *, CompilerIrSymbol *);
void __cdecl validate_declaration_type(CompilerIrSymbol *, int);
CompilerIrSymbol *__cdecl lookup_ir_symbol(CompilerIrSymbol *);
CompilerIrSymbol *__cdecl find_named_symbol(CompilerSymbolBinding *, CompilerNameEntry *);
void __cdecl report_extension_diagnostic(unsigned, unsigned);
CompilerIrSymbol *__cdecl merge_scoped_declaration(CompilerIrSymbol *, CompilerIrSymbol *, int);
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
#ifdef RECON_NATIVE_TEST
    if (original.flags != 0x20 || original.context || symbol->kind != 10 ||
        !declarator || (declarator->flags & 1) || !declarator->resolved_type)
        recon_unsupported("symbol declaration context or declarator domain is not yet modeled");
#endif
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
        if (symbol->kind != 7)
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
#ifdef RECON_NATIVE_TEST
    if (symbol->kind != 1 || flags != 0x20 || context)
        recon_unsupported("declaration publication flags or symbol alternative is not yet modeled");
#endif
    local = symbol->kind == 2 || symbol->kind == 3;
    if (symbol->kind == 1) symbol->kind = 10;
    symbol->intrinsic_flags = flags;
    if (flags & 0x1000000) {
        if (context) attach_declaration_context(symbol, context);
        else symbol->function_flags &= ~1;
    } else symbol->SetDeclarationContext(NULL);
    declare_symbol(declarator, symbol);
#ifdef RECON_NATIVE_TEST
    if (symbol->kind != 7)
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
    if (symbol->kind == 10) {
#ifdef RECON_NATIVE_TEST
        recon_unsupported("declaration option5ef9c is not yet modeled");
#endif
        if (declaration_option_5ef9c && storage_class >= 1 && storage_class <= 3)
            symbol->linkage_flags |= 2;
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
/* These lookup records have the authentic8-byte allocation. The first
 * pointer is the selected declaration; Find's record also names its scope. */
struct CompilerScopeLookup {
    CompilerIrSymbol *symbol;
    CompilerScopeTable *scope;
};
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
CompilerIrSymbol **RECON_THISCALL CompilerTranslationUnit::FindScopedDeclaration(CompilerIrSymbol *)
{ __asm { int 3 } return NULL; }
extern "C" void __cdecl discard_declaration_symbol(CompilerIrSymbol *) { __asm { int 3 } }
extern "C" void __cdecl record_scoped_declaration(CompilerIrSymbol *) { __asm { int 3 } }
unsigned RECON_THISCALL CompilerDeclarationFlagView::QualifierCode(void) { __asm { int 3 } return 0; }
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
    if (symbol->kind != 10 || !shape || (shape->flags & 7) != 4)
        recon_unsupported("nonfunction declaration type domain is not yet modeled");
#endif
    if (shape) {
        if ((shape->flags & 7) == 4) {
            if (symbol->kind == 10) symbol->kind = 7;
            else {
                if (symbol->kind >= 2 && symbol->kind <= 3) {
#ifdef RECON_NATIVE_TEST
                    recon_unsupported("diagnostic declaration slot is not yet modeled");
#endif
                    CompilerIrSymbol *owner = diagnostic_slots[c1_constant_diagnostic_mode].symbol;
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

extern "C" void __cdecl report_extension_diagnostic(unsigned, unsigned) { __asm { int 3 } }

extern "C" CompilerIrSymbol *__cdecl merge_scoped_declaration(CompilerIrSymbol *, CompilerIrSymbol *, int) { __asm { int 3 } return NULL; }

void *RECON_THISCALL CompilerSymbolTable::Find(CompilerNameEntry *name)
{
    if (!buckets) return NULL;
#ifdef RECON_NATIVE_TEST
    if (bucket_mask != 1023)
        recon_unsupported("scope lookup bucket extent is not yet modeled");
#endif
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
