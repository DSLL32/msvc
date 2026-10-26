#include <string.h>
#include "frontend.h"
#include "keywords.h"
#pragma intrinsic(memcpy)
#include <recon.h>

/* Output-only pair populated by the pending PCH module-data query. Its stack
 * storage has no initial value; the current IL domain never consumes it. */
struct CompilerModuleData { const BYTE *bytes; int length; };
extern "C" {
void __cdecl write_global_reference(CompilerSymbolBinding *);
void __cdecl finalize_function_directory(void);
CompilerParserNode *__cdecl make_label_expression(CompilerIrSymbol *, int);
CompilerIrSymbol *__cdecl begin_initializer_symbol(CompilerIrSymbol *);
void __cdecl apply_initializer(CompilerIrSymbol *, CompilerParserNode *);
int __cdecl get_pch_module_data(unsigned, CompilerModuleData *);
void __cdecl write_global_record(unsigned, CompilerIrSymbol *);
}

void RECON_THISCALL CompilerTranslationUnit::Finish(void)
{
    CompilerSymbolBinding *binding;
    CompilerIrSymbol *symbol;
    CompilerParserNode *expression;
    CompilerModuleData module;
    CompilerOutputBuffer *buffer;
    unsigned requested;
    int good;
    if (!c1_data.c9_il) {
#ifdef __linux__
        recon_unsupported("translation finish: PCH module output is not yet modeled");
#else
        __asm { int 3 }
#endif
    }
    c1_bss.declaration_scope_active = 0;
    if (c1_bss.parser_early_result) return;
    for (binding = symbols; binding; binding = binding->next)
        write_global_reference(binding);
    finalize_function_directory();
    root->Check();
    for (binding = symbols; binding; binding = binding->next) {
        symbol = (CompilerIrSymbol *)((BYTE *)binding - 8);
        if (symbol->initializer_symbol) {
            write_global_reference(&symbol->initializer_symbol->binding);
            expression = make_label_expression(symbol, 0);
            apply_initializer(begin_initializer_symbol(symbol->initializer_symbol), expression);
        }
    }
    if (!c1_data.c9_il) {
        get_pch_module_data(1, &module);
        buffer = &c1_bss.output_buffers[2];
        requested = (unsigned)module.length + 1;
        if ((int)requested < 0) good = 0;
        else if (buffer->cursor + requested > buffer->data + buffer->capacity)
            good = buffer->Grow(requested) != 0;
        else good = 1;
        buffer->good &= good;
        if (buffer->good) {
            memcpy(buffer->cursor, module.bytes, module.length);
            buffer->cursor += module.length;
        }
    }
    write_global_record(10, NULL);
}

/* Genuine pending children, with the caller's actual stack/pointer roles. */
extern "C" CompilerIrSymbol *__cdecl begin_initializer_symbol(CompilerIrSymbol *)
{ __asm { int 3 } return NULL; }
extern "C" int __cdecl get_pch_module_data(unsigned, CompilerModuleData *)
{ __asm { int 3 } return 0; }

extern int c1_inline_record_flag;
extern int c1_include_pch_reference_counts;
/* Source-only replay suppression; supported paths guard before this read. */
static int suppress_replayed_function; /* RVA5f3b4 */
extern "C" {
void __cdecl select_function_debug_handle(unsigned);
void __cdecl write_global_binding_record(unsigned, CompilerSymbolBinding *, int);
void __cdecl prepare_debug_type(CompilerType *);
unsigned __cdecl finish_debug_type(CompilerIrSymbol *);
void __cdecl report_language_diagnostic(unsigned, unsigned, ...);
}

extern "C" void __cdecl finalize_function_directory(void)
{
    CompilerFunctionDirectoryBlock *block;
    CompilerFunctionDirectoryRecord *record;
    CompilerSymbolBinding *binding;
    CompilerIrSymbol *symbol;
    CompilerType *type;
    unsigned count, i;
    int recognized_entry;
    if (!c1_data.c9_il || c1_data.options.debug_info) {
#ifdef __linux__
        recon_unsupported("function directory: non-IL or debug records are not yet modeled");
#else
        __asm { int 3 }
#endif
    }
    while ((block = c1_data.function_directory_head) != NULL) {
        count = (block == c1_data.function_directory_current ? c1_data.function_directory_index : 19) + 1;
        if (count > 20) {
#ifdef __linux__
            recon_unsupported("function directory: unsupported record count");
#else
            __asm { int 3 }
#endif
        }
        for (i = 0; i < count; ++i) {
            block = c1_data.function_directory_head;
            select_function_debug_handle(block->records[i].debug_handle);
            block = c1_data.function_directory_head;
            record = &block->records[i];
            binding = record->binding;
            symbol = (CompilerIrSymbol *)((BYTE *)binding - 8);
            if (symbol->flags & 2) continue;
            c1_inline_record_flag = 0;
            c1_include_pch_reference_counts = (record->flags >> 4) & 1;
            recognized_entry = binding->name == c1_data.builtin_names.main_entry ||
                binding->name == c1_data.builtin_names.winmain_entry ||
                binding->name == c1_data.builtin_names.dllmain_entry;
            if (recognized_entry) {
                if ((symbol->storage_class & 7) == 3) {
                    report_language_diagnostic(3, 8, symbol, 9);
                    symbol->storage_class = (symbol->storage_class & ~7) | 1;
                }
                if (symbol->symbol_qualifiers & 1) {
                    report_language_diagnostic(3, 8, symbol, 14);
                    symbol->symbol_qualifiers &= ~1;
                }
                ((BYTE *)&symbol->qualifier_flags)[0] |= 0x80;
                c1_inline_record_flag = 1;
            } else if ((symbol->storage_class & 7) != 3) {
                if (!(symbol->symbol_qualifiers & 1)) c1_inline_record_flag = 1;
                else {
                    symbol->storage_class |= 0x40;
                    if (!(symbol->linkage_flags & 8)) {
                        if (symbol->symbol_qualifiers & 0x10) c1_inline_record_flag = 1;
                        else if (symbol->linkage_flags & 4) {
#ifdef __linux__
                            recon_unsupported("function directory: PCH replay suppression is not yet modeled");
#else
                            __asm { int 3 }
#endif
                            if (!suppress_replayed_function &&
                                (!c1_include_pch_reference_counts || c1_data.options.pch_auto ||
                                 c1_data.saved_pch_mode || c1_bss.pch_replayed))
                                c1_inline_record_flag = 1;
                        }
                    }
                }
            }
            if (symbol->symbol_qualifiers & 0x80) c1_inline_record_flag = 1;
            if (!c1_inline_record_flag && !(symbol->reference_flags & 3)) {
                if (!c1_data.c9_il)
                    c1_bss.debug_module->Slot78((unsigned)binding->name->name);
                report_language_diagnostic(4, (symbol->symbol_qualifiers & 1) ? 514 : 505, symbol);
            } else {
                c1_bss.function_expression_offset = record->expression_position;
                c1_bss.function_symbol_offset = record->symbol_position;
                c1_bss.ir_record_count = record->ir_records;
                c1_bss.nonil_type_index = record->context_index;
                c1_bss.parameter_symbol_count = record->parameter_symbols;
                c1_data.ir_symbol_state_changed = 1;
                c1_data.ir_symbol_state_ready = record->flags & 1;
                c1_data.disable_inline_records = (record->flags >> 2) & 1;
                c1_data.emit_global_locations = (record->flags >> 5) & 1;
                if (c1_data.options.debug_info && !c1_bss.function_output_mode) {
                    type = symbol->value_type;
                    prepare_debug_type(type);
                    if (type->symbol_owner && type->symbol_owner->kind == 4 &&
                        !type->symbol_owner->alternate_size)
                        finish_debug_type(type->symbol_owner);
                }
                write_global_binding_record(14, binding, c1_inline_record_flag);
            }
            symbol->flags |= 2;
        }
        c1_data.function_directory_head = c1_data.function_directory_head->next;
    }
}
/* The non-IL cache is source-only until its actual producer/domain is
 * reached; IL does not touch the cache or the provider-owned name map. */
static unsigned current_function_debug_handle; /* RVA5f4d4 */
extern const char *c1_global_string_record; /* Same owner as the global writer */
extern "C" void __cdecl select_function_debug_handle(unsigned handle)
{
    if (!c1_data.c9_il) {
#ifdef __linux__
        recon_unsupported("function debug handle: non-IL NameMap state is not yet modeled");
#else
        __asm { int 3 }
#endif
    }
    if (c1_data.c9_il) return;
    if (handle == current_function_debug_handle) return;
    c1_current_pdb_context->names->QueryName(handle, &c1_global_string_record);
    write_global_record(22, NULL);
    current_function_debug_handle = handle;
}
extern "C" void __cdecl write_global_binding_record(unsigned opcode,
    CompilerSymbolBinding *binding, int omit_names)
{
    int previous = c1_data.omit_external_names;
    c1_data.omit_external_names = omit_names;
    write_global_record(opcode, (CompilerIrSymbol *)((BYTE *)binding - 8));
    c1_data.omit_external_names = previous;
}
extern "C" void __cdecl prepare_debug_type(CompilerType *) { __asm { int 3 } }
extern "C" unsigned __cdecl finish_debug_type(CompilerIrSymbol *) { __asm { int 3 } return 0; }
