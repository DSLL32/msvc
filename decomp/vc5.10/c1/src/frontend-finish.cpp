#include <string.h>
#include "frontend.h"
#include "keywords.h"
#pragma intrinsic(memcpy)
#include <recon.h>

/* Output-only pair populated by the pending PCH module-data query. Its stack
 * storage has no initial value; the current IL domain never consumes it. */
struct CompilerModuleData { const BYTE *bytes; int length; };
extern "C" {
void __cdecl compiler_warning(int, ...);
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
extern "C" CompilerIrSymbol *__cdecl begin_initializer_symbol(CompilerIrSymbol *symbol)
{
    unsigned storage;
    c1_data.lexer_flags &= ~0x40000U;
    c1_bss.initializer_value_pending = 0;
    if (c1_bss.parameter_nesting) {
        storage = symbol->storage_class & 7;
        if (storage >= 4 && storage <= 6) c1_data.lexer_flags |= 0x100000;
        else c1_data.lexer_flags &= ~0x100000U;
    } else c1_data.lexer_flags &= ~0x100000U;
    c1_bss.initializer_symbol = symbol;
    c1_bss.initializer_bytes = 0;
    c1_data.initializer_reset_first = 0;
    c1_data.initializer_reset_second = 0;
    c1_data.initializer_reset_third = 0;
    return symbol;
}

extern "C" void __cdecl prepare_initializer_frame(CompilerIrSymbol *);
extern "C" int __cdecl initializer_requires_storage(CompilerIrSymbol *);

/* RVA207c2..20889 /38b9a..38be2. The second incoming argument is not read. */
extern "C" void __cdecl finish_initializer(CompilerIrSymbol *symbol, int)
{
    unsigned storage;
    int aggregate;
    if (symbol) {
        c1_bss.initializer_error_reported = 0;
        c1_data.initializer_depth = -1;
        c1_bss.initializer_saved_type = 0;
        aggregate = (symbol->qualifier_flags & 5) == 5;
        c1_bss.initializer_symbol = symbol;
        c1_bss.initializer_bytes = 0;
        prepare_initializer_frame(symbol);
        c1_bss.initializer_frame->flags = 1;
        if (initializer_requires_storage(c1_bss.initializer_frame->symbol) || aggregate)
            c1_bss.initializer_frame->flags |= 4;
        storage = c1_bss.parameter_nesting ?
            c1_bss.initializer_frame->symbol->storage_class & 7 : 0;
        if (!c1_bss.parameter_nesting || storage < 4 || storage > 6) {
            c1_data.last_initializer_symbol = symbol;
            if (c1_bss.browser_filename) {
                c1_data.backend->Slot0c();
                c1_data.backend->Slot30(c1_data.last_initializer_symbol);
                c1_data.function_record_flags |= 0x4000;
            }
        }
    }
    c1_data.lexer_flags &= ~0x40000U;
}
extern "C" void __cdecl prepare_initializer_frame(CompilerIrSymbol *symbol)
{
    CompilerInitializerFrame *frame;
    CompilerIrSymbol *created;
    CompilerType *type;
    unsigned bytes;
    if (++c1_data.initializer_depth >= 32) compiler_fatal(54);
    if (c1_data.initializer_depth < 0 || c1_data.initializer_depth > C1_INITIALIZER_LAST) {
#ifdef __linux__
        recon_unsupported("initializer frame beyond encountered records0..5");
#else
        __asm { int 3 }
#endif
    }
    frame = &c1_bss.initializer_frames[c1_data.initializer_depth];
    c1_bss.initializer_frame = frame;
    if (!frame->symbol) {
        bytes = (c1_data.record_sizes.known.declaration_symbol_bytes + 3) & ~3U;
        created = (CompilerIrSymbol *)c1_bss.arenas[1].AllocateBytes(bytes);
        frame->symbol = created ? created->Initialize(1, NULL, 1) : NULL;
    }
    frame->symbol->CopyInitializerFrom(symbol, 1);
    if (!frame->type) {
        type = (CompilerType *)c1_bss.arenas[1].cursor;
        c1_bss.arenas[1].cursor += 16;
        if (c1_bss.arenas[1].cursor > c1_bss.arenas[1].committed_limit)
            c1_bss.arenas[1].CommitExtent();
        frame->type = type;
    }
    /* The genuine sixteen-byte type transfer includes its opaque cache. */
    memcpy(frame->type, symbol->value_type, 16);
    frame->symbol->value_type = frame->type;
    frame->reset_byte = 0;
    frame->offset = 0;
    frame->flags = 0;
}
/* Kind4 uses the same authentic twelve-byte member-list owner as record
 * construction. Copying the list head does not copy or initialize members. */
void RECON_THISCALL CompilerIrSymbol::CopyInitializerFrom(CompilerIrSymbol *source, unsigned arena)
{
    CompilerRecordMembers *link;
    CompilerPool *pool;
    unsigned kind = source->kind;
    if (kind != 10 && kind != 4 && kind != 3 && kind != 2) {
#ifdef __linux__
        recon_unsupported("initializer symbol copy kind is not yet supported");
#else
        __asm { int 3 }
#endif
    }
    memcpy(this, source, c1_data.record_sizes.indexed[kind]);
    if (source->kind == 4) {
        if (arena) link = (CompilerRecordMembers *)c1_bss.arenas[arena].AllocateBytes(12);
        else {
            pool = &c1_bss.pools[0];
            if (pool->remaining < 12) pool->Refill(12);
            link = (CompilerRecordMembers *)pool->cursor;
            pool->cursor += 12;
            pool->remaining -= 12;
        }
        if (link) {
            link->arena_kind = arena;
            link->first = NULL;
            link->last = NULL;
        }
        kind4_context = link;
        link->last = ((CompilerRecordMembers *)source->kind4_context)->first;
        link->first = link->last;
    }
}
extern "C" int __cdecl initializer_requires_storage(CompilerIrSymbol *symbol)
{
    CompilerType *type = symbol->value_type;
    CompilerTypeShape *shape = type->shape;
    if (shape) {
        if (shape->flags & 1) return 1;
        if ((shape->flags & 7) == 2) return 0;
    }
    return (type->flags & 0xb40) != 0;
}
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

extern "C" int __cdecl advance_initializer_frame(CompilerParserNode *);
extern "C" void __cdecl begin_initializer_element(void)
{
    if (c1_data.initializer_depth < 0) return;
    if (initializer_requires_storage(c1_bss.initializer_frame->symbol))
        prepare_initializer_frame(c1_bss.initializer_frame->symbol);
    else if (!advance_initializer_frame(0)) {
        if (!c1_bss.initializer_error_reported) {
            c1_bss.initializer_error_reported = 1;
            compiler_warning(78);
        }
        return;
    }
    c1_bss.initializer_frame->flags = 1;
    if (initializer_requires_storage(c1_bss.initializer_frame->symbol))
        c1_bss.initializer_frame->flags |= 4;
}
extern "C" int __cdecl initializer_accepts_string(CompilerIrSymbol *, CompilerParserNode *);
extern "C" CompilerSymbolBinding *__cdecl next_initializer_member(CompilerIrSymbol *, CompilerSymbolBinding *);
extern "C" CompilerType *__cdecl get_function_return_type(CompilerType *);
extern "C" CompilerType *__cdecl intern_type(CompilerType *);

extern "C" int __cdecl advance_initializer_frame(CompilerParserNode *value)
{
    CompilerInitializerFrame *frame;
    CompilerIrSymbol *symbol, *member;
    CompilerRecordMembers *members;
    CompilerSymbolBinding *binding;
    CompilerType *type, *element;
    int unsized_array;
    unsigned flags;
    if (c1_data.initializer_depth < 0) return 0;
    frame = c1_bss.initializer_frame;
    if (frame->flags & 0x20) return 0;
    symbol = frame->symbol;
    if (initializer_requires_storage(symbol)) return !(frame->flags & 8);
    if (initializer_accepts_string(symbol, value)) return 1;
    frame = c1_bss.initializer_frame;
    if (frame->flags & 2) return 0;
    if (symbol->qualifier_flags & 4) {
        unsized_array = (symbol->qualifier_flags & 1) &&
            (frame->type->shape->flags & 7) == 2 && !frame->type->shape->value;
        prepare_initializer_frame(symbol);
        frame = c1_bss.initializer_frame;
        symbol = frame->symbol;
        element = get_function_return_type(symbol->value_type);
        memcpy(frame->type, element, 16);
        symbol->value_type = frame->type;
        c1_bss.initializer_saved_type = unsized_array ? frame->type : NULL;
        if (symbol->value_type->shape && (symbol->value_type->shape->flags & 7) == 2)
            symbol->qualifier_flags |= 4;
        else symbol->qualifier_flags &= ~4U;
        return 1;
    }
    type = symbol->value_type;
    flags = type->flags & 0x2fcf;
    if (flags != 0x402 && flags != 0x404 && flags != 0x408) {
        frame->flags |= 0x20;
        return 0;
    }
    if (!(frame->flags & 0x80)) {
        member = type->symbol_owner;
        if (!(member->flags & 1)) {
            /* A pop may refer back to record0; wider previous records remain
             * outside the encountered view. The negative cursor is retained
             * as an address only and is not dereferenced on this return. */
            if (c1_data.initializer_depth > C1_INITIALIZER_LAST) {
#ifdef __linux__
                recon_unsupported("incomplete aggregate initializer frame pop");
#else
                __asm { int 3 }
#endif
            }
            if (c1_data.initializer_depth >= 0) {
                --c1_data.initializer_depth;
                c1_bss.initializer_frame = (CompilerInitializerFrame *)
                    ((BYTE *)c1_bss.initializer_frames + 20 * c1_data.initializer_depth);
                if (c1_data.initializer_depth >= 0)
                    compiler_warning(79, c1_bss.initializer_frame->symbol, symbol->value_type, symbol);
            }
            c1_data.initializer_depth = -1;
            return 0;
        }
        prepare_initializer_frame(member);
        c1_bss.initializer_frame->flags |= 0x80;
        if (!(symbol->kind >= 1 && symbol->kind <= 14 && symbol->kind != 5 &&
              symbol->binding.name == c1_data.builtin_names.alignment_member && (symbol->qualifier_flags & 8)))
            symbol = c1_bss.initializer_frame->symbol;
    }
    members = (CompilerRecordMembers *)symbol->kind4_context;
    if (symbol->kind >= 1 && symbol->kind <= 14 && symbol->kind != 5 &&
        symbol->binding.name == c1_data.builtin_names.alignment_member && (symbol->qualifier_flags & 8))
        binding = symbol->binding.next;
    else binding = members->first;
    binding = next_initializer_member(NULL, binding);
    member = binding ? (CompilerIrSymbol *)((BYTE *)binding - 8) : NULL;
    if (!member) {
        --c1_data.initializer_depth;
        c1_bss.initializer_frame->flags |= 0x50;
        return 1;
    }
    frame = c1_bss.initializer_frame;
    members = (CompilerRecordMembers *)frame->symbol->kind4_context;
    if (!members) {
        members = (CompilerRecordMembers *)c1_bss.arenas[1].cursor;
        c1_bss.arenas[1].cursor += 12;
        if (c1_bss.arenas[1].cursor > c1_bss.arenas[1].committed_limit)
            c1_bss.arenas[1].CommitExtent();
        if (members) {
            /* First DWORD is an arena selector on this allocation path. */
            members->arena_kind = 1;
            members->first = members->last = NULL;
        }
        frame->symbol->kind4_context = members;
    }
    members->first = members->last = binding;
    prepare_initializer_frame(member);
    symbol = c1_bss.initializer_frame->symbol;
    if (initializer_accepts_string(symbol, value) && (symbol->qualifier_flags & 1))
        c1_bss.initializer_saved_type = intern_type(get_function_return_type(symbol->value_type));
    return 1;
}
extern "C" int __cdecl initializer_accepts_string(CompilerIrSymbol *symbol, CompilerParserNode *value)
{
    CompilerType *type;
    unsigned character_type;
    if (!value || value->kind != 7) return 0;
    if (!(symbol->qualifier_flags & 4)) return 0;
    /* The literal constructor now establishes the WORD length/flags view;
     * only the width bit is consumed here, without reading its text bytes. */
    character_type = 0x41 + ((value->string_flags & 1) != 0);
    type = symbol->value_type;
    return (type->flags & 0x2fcf) == character_type && !type->shape->base;
}
extern "C" void __cdecl flush_initializer_bitfield(CompilerIrSymbol *);
extern "C" void __cdecl write_initializer_member_padding(CompilerIrSymbol *);
extern "C" CompilerSymbolBinding *__cdecl next_initializer_member(CompilerIrSymbol *context,
    CompilerSymbolBinding *binding)
{
    CompilerIrSymbol *symbol;
    for (; binding; binding = binding->next) {
        symbol = (CompilerIrSymbol *)((BYTE *)binding - 8);
        if (symbol->kind < 1 || symbol->kind > 14 || symbol->kind == 5 ||
            symbol->binding.name != c1_data.builtin_names.alignment_member) return binding;
        if (!(symbol->qualifier_flags & 0x18)) {
            if (c1_bss.initializer_bitfield_active) {
                c1_bss.integer_work_value = (__int64)c1_bss.initializer_bitfield_value;
                flush_initializer_bitfield(context);
                c1_bss.initializer_bitfield_active = 0;
            }
            write_initializer_member_padding(symbol);
        } else if (symbol->qualifier_flags & 0x10) return NULL;
        else if (symbol->qualifier_flags & 8) return binding;
    }
    return binding;
}
extern "C" void __cdecl write_initializer_padding(CompilerIrSymbol *);
extern "C" const signed char c1_primitive_widths[16];
extern "C" void __cdecl flush_initializer_bitfield(CompilerIrSymbol *symbol)
{
    unsigned flags, bytes;
    write_initializer_padding(symbol);
    if (!(c1_data.lexer_flags & 0x100000)) {
        flags = symbol->value_type->flags;
        bytes = ((flags & 0x340) || !(flags & ~15U)) ?
            (unsigned)(int)c1_primitive_widths[flags & 15] : flags & 15;
        c1_bss.initializer_bytes += bytes;
    }
}
extern "C" void __cdecl write_initializer_member_padding(CompilerIrSymbol *symbol)
{
    c1_bss.integer_work_value = (__int64)(int)symbol->symbol_value;
    if (c1_data.lexer_flags & 0x100000)
        c1_bss.initializer_bytes += symbol->symbol_value;
    else
        write_initializer_padding(NULL);
}
extern "C" {
CompilerParserNode *__cdecl strip_value_wrappers(CompilerParserNode *);
int __cdecl prepare_initializer_value(CompilerParserNode *);
int __cdecl check_initializer_expression(CompilerIrSymbol *, CompilerParserNode **);
void __cdecl convert_initializer_expression(CompilerIrSymbol *, CompilerParserNode *);
void __cdecl emit_runtime_initializer(CompilerIrSymbol *, CompilerParserNode *);
void __cdecl write_initializer_bytes(CompilerIrSymbol *, CompilerParserNode *, CompilerOutputBuffer *);
int __cdecl finish_initializer_value(void);
WORD __cdecl get_initializer_string_length(CompilerIrSymbol *, CompilerParserNode *);
void __cdecl write_runtime_initializer_value(CompilerIrSymbol *, CompilerParserNode *);
void __cdecl finish_runtime_bitfield(CompilerParserNode *);
void __cdecl write_initializer_padding(CompilerIrSymbol *);
CompilerType *__cdecl decay_initializer_array(CompilerParserNode *);
int __cdecl convert_initializer_float_to_long(const BYTE *, int *);
unsigned __cdecl global_type_size(CompilerType *);
int __cdecl convert_expression_to_type(unsigned, CompilerType *, CompilerParserNode *);
void __cdecl write_ir_record(unsigned, const void *);
CompilerIntegerConstant *__cdecl make_integer_constant(int);
void __cdecl report_language_diagnostic(unsigned, unsigned, ...);
extern const signed char c1_primitive_widths[16];
}

extern "C" void __cdecl finish_aggregate_initializer(CompilerParserNode *input)
{
    CompilerParserNode *value, *original;
    CompilerIrSymbol *symbol;
    CompilerType *type;
    CompilerInitializerFrame *frame;
    unsigned flags, width, position, count, unit;
    unsigned __int64 bits, mask;
    int string, converted;
    if (!input || c1_data.initializer_depth < 0 || (c1_bss.initializer_frame->flags & 0x20)) return;
    original = strip_value_wrappers(input);
    value = original;
    if (value->kind != 101 && value->kind != 6 && value->kind != 7) {
#ifdef __linux__
        recon_unsupported("initializer expression classification cell is not yet modeled");
#else
        __asm { int 3 }
#endif
    }
    if (c1_expression_classes.indexed[value->kind] == 0x83 && value->left &&
        (value->left->flags & 7) == 3)
        value = (CompilerParserNode *)make_integer_constant(value->left->low + value->right->low);
    if (!prepare_initializer_value(original)) {
        if (!c1_bss.initializer_error_reported) {
            c1_bss.initializer_error_reported = 1;
            compiler_warning(78);
        }
        return;
    }
    if (c1_data.lexer_flags & 0x100000)
        emit_runtime_initializer(c1_bss.initializer_frame->symbol, value);
    else {
        if (!check_initializer_expression(c1_bss.initializer_frame->symbol, &value)) {
            compiler_warning(99);
            return;
        }
        convert_initializer_expression(c1_bss.initializer_frame->symbol, value);
    }
    frame = c1_bss.initializer_frame;
    symbol = frame->symbol;
    if (symbol->kind != 2) {
        string = initializer_accepts_string(symbol, original);
        if (string) value->string_length = (WORD)get_initializer_string_length(symbol, original);
        if (c1_data.lexer_flags & 0x100000) write_runtime_initializer_value(symbol, input);
        else write_initializer_bytes(symbol, original, &c1_bss.output_buffers[4]);
        if (!string) {
            if (!(c1_data.lexer_flags & 0x100000)) {
                type = symbol->value_type;
                if (input->type->shape && (input->type->shape->flags & 7) == 2)
                    input->type = decay_initializer_array(input);
                convert_expression_to_type(116, type, input);
                c1_bss.initializer_bytes += global_type_size(type);
            }
            finish_initializer_value();
            return;
        }
        frame = c1_bss.initializer_frame;
        if (frame->offset && (frame->reset_byte || !c1_data.options.language_extensions)) {
            if (!c1_bss.initializer_error_reported) {
                c1_bss.initializer_error_reported = 1;
                compiler_warning(78);
            }
        } else if (frame->offset) report_language_diagnostic(4, 207);
        unit = (original->string_flags & 1) ? 2 : (unsigned)(int)c1_primitive_widths[1];
        frame->offset += original->string_length / unit;
        frame->reset_byte = 1;
        count = frame->symbol->value_type->shape->value;
        if (count) {
            if (frame->offset < count) {
                c1_bss.integer_work_value = (int)((count - frame->offset) * unit);
                write_initializer_padding(0);
                c1_bss.initializer_frame->offset = count;
            } else {
                frame->flags |= 2;
                if (frame->offset > count && !c1_bss.initializer_error_reported) {
                    c1_bss.initializer_error_reported = 1;
                    compiler_warning(78);
                }
            }
        }
        if (!(c1_data.lexer_flags & 0x100000)) c1_bss.initializer_bytes += original->string_length;
        finish_initializer_value();
        return;
    }
    if (!c1_bss.initializer_bitfield_active) {
        c1_bss.initializer_bitfield_active = 1;
        c1_bss.initializer_bitfield_value = 0;
        c1_bss.initializer_bitfield_position = symbol->symbol_value;
    }
    original = value;
    flags = original->type->flags;
    if (!(flags & 0x140)) {
        compiler_warning(77, symbol);
        finish_initializer_value();
        return;
    }
    if ((original->flags & 7) != 3) {
        if ((c1_data.lexer_flags & 0x100000) &&
            (c1_bss.initializer_value_pending || c1_data.options.language_extensions)) {
            finish_runtime_bitfield(input);
            finish_initializer_value();
        } else finish_initializer_value();
        return;
    }
    if (flags & 0x100) {
        if (convert_initializer_float_to_long((const BYTE *)original->low, &converted))
            report_language_diagnostic(2, 56);
        value->low = converted;
        value->high = converted < 0 ? ~0U : 0;
        original = value;
    }
    width = symbol->bit_width;
    mask = width >= 64 ? ~(unsigned __int64)0 : (((unsigned __int64)1 << width) - 1);
    bits = (((unsigned __int64)original->high << 32) | original->low) & mask;
    position = symbol->parameter_kind;
    c1_bss.initializer_bitfield_value |= position >= 64 ? 0 : bits << position;
    finish_initializer_value();
}

extern "C" int __cdecl prepare_initializer_value(CompilerParserNode *value)
{
    if (!advance_initializer_frame(value)) return 0;
    for (;;) {
        if (initializer_requires_storage(c1_bss.initializer_frame->symbol) ||
            initializer_accepts_string(c1_bss.initializer_frame->symbol, value)) return 1;
        if (!advance_initializer_frame(value)) return 0;
    }
}
extern "C" int __cdecl get_static_initializer_reference(CompilerParserNode *, CompilerIrSymbol **, int *);
extern "C" CompilerIrSymbol *__cdecl lookup_ir_symbol(CompilerIrSymbol *);
extern "C" int __cdecl check_initializer_expression(CompilerIrSymbol *symbol, CompilerParserNode **value)
{
    CompilerIrSymbol *referenced = NULL;
    int displacement = 0;
    CompilerParserNode *node = *value;
    CompilerType *source, *target;
    CompilerTypeShape *shape;
    unsigned flags, storage;
    if (!node) return 0;
    source = node->type;
    target = symbol->value_type;
    shape = target->shape;
    if (source != target && shape && (shape->flags & 1) && (shape->storage & 3) == 3) {
#ifdef __linux__
        recon_unsupported("symbol-bearing initializer target shape is not yet qualified");
#else
        __asm { int 3 }
#endif
        if (shape->symbol_owner->value_type->shape && !(node->flags & 0x80)) {
            if ((node->flags & 7) != 3 || (source->flags & 0x2fcf) != 0x63) return 0;
        }
    }
    flags = node->flags & 7;
    if (flags == 5 || flags == 3) return 1;
    if (!get_static_initializer_reference(node, &referenced, &displacement)) return 0;
    if (!referenced) return 1;
    if (referenced->kind == 14 && (referenced->record_id & 0xffffff) == 6) return 0;
    if ((referenced->function_flags & 1) && lookup_ir_symbol(referenced)->kind != 14) return 0;
    storage = referenced->storage_class & 7;
    if (storage == 4 && (symbol->kind == 2 || symbol->kind == 3 || (symbol->storage_class & 7) == 3))
        return 0;
    if ((referenced->linkage_flags & 2) && referenced->kind != 7 && referenced->kind != 8) return 0;
    if ((referenced->linkage_flags & 8) && referenced->kind != 7 && referenced->kind != 8) return 0;
    return 1;
}
extern "C" CompilerParserNode *__cdecl lower_ir_node(CompilerParserNode *);
extern "C" int __cdecl get_static_initializer_reference(CompilerParserNode *value,
    CompilerIrSymbol **symbol, int *displacement)
{
    CompilerParserNode *offset, *left;
    unsigned amount;
    value = strip_value_wrappers(value);
    for (;;) {
        switch (value->flags & 7) {
        case 2:
            switch (value->kind) {
            case 81:
                left = value->left;
                if ((left->flags & 7) == 3) {
                    value->left = value->right;
                    value->right = left;
                }
                /* Addition accepts the constant on either side. */
            case 65: case 70: case 80: case 83:
                offset = value->right;
                if ((offset->flags & 7) != 3 || !(offset->type->flags & 0x40)) return 0;
                amount = offset->low;
                if (value->kind == 70) amount = 0U - amount;
                *displacement = (int)((unsigned)*displacement + amount);
                value = strip_value_wrappers(value->left);
                break;
            default:
                return 0;
            }
            break;
        case 3:
            *displacement = (int)((unsigned)*displacement + value->low);
            return 1;
        case 5:
            value = lower_ir_node(value);
            /* Lowering yields the same symbol alternative as class4. */
        case 4:
            *symbol = value->symbol;
            return 1;
        default:
            return 0;
        }
    }
}
extern "C" void __cdecl convert_initializer_expression(CompilerIrSymbol *symbol, CompilerParserNode *value)
{
    CompilerIrSymbol *referenced = NULL;
    int displacement = 0;
    unsigned flags = value->flags & 7;
    if (flags == 5 || flags == 3) return;
    if (get_static_initializer_reference(value, &referenced, &displacement) && referenced &&
        (referenced->kind == 7 || referenced->kind == 8) && (referenced->linkage_flags & 8)) {
        if (!c1_data.options.language_extensions) compiler_warning(322, symbol, referenced);
        else report_language_diagnostic(4, 232, symbol, referenced);
    }
}
extern "C" void __cdecl emit_runtime_initializer(CompilerIrSymbol *, CompilerParserNode *) { __asm { int 3 } }
extern "C" int __cdecl finish_initializer_value(void)
{
    CompilerInitializerFrame *frame;
    CompilerIrSymbol *symbol, *member, *next;
    CompilerRecordMembers *members;
    CompilerSymbolBinding *binding;
    CompilerType *type;
    unsigned kind, count;
    int nesting;
examine_frame:
    if (c1_data.initializer_depth < 0) return 0;
    if (c1_data.initializer_depth > C1_INITIALIZER_LAST) {
#ifdef __linux__
        recon_unsupported("initializer completion beyond encountered frame records");
#else
        __asm { int 3 }
#endif
    }
    frame = c1_bss.initializer_frame;
    if (frame->flags & 0x20) {
        if (c1_data.initializer_depth >= 0) {
            --c1_data.initializer_depth;
            c1_bss.initializer_frame = (CompilerInitializerFrame *)
                ((BYTE *)c1_bss.initializer_frames + 20 * c1_data.initializer_depth);
        }
        return 1;
    }
    if (frame->flags & 1) { frame->flags |= 8; return 0; }
    if (c1_data.initializer_depth < 0) return 0;
    --c1_data.initializer_depth;
    frame = (CompilerInitializerFrame *)((BYTE *)c1_bss.initializer_frames + 20 * c1_data.initializer_depth);
    c1_bss.initializer_frame = frame;
    if (c1_data.initializer_depth < 0) return 0;
    symbol = frame->symbol;
    if (symbol->qualifier_flags & 4) {
        if (frame->reset_byte && !c1_bss.initializer_error_reported) {
            c1_bss.initializer_error_reported = 1;
            compiler_warning(78);
            frame = c1_bss.initializer_frame;
        }
        count = ++frame->offset;
        if (count != frame->symbol->value_type->shape->value) return 1;
        frame->flags |= 2;
        if (frame->flags & 1) return 1;
        goto examine_frame;
    }
    type = symbol->value_type;
    kind = type->flags & 0x2fcf;
    if (kind == 0x404) {
        if (c1_bss.initializer_bitfield_active) {
            members = (CompilerRecordMembers *)symbol->kind4_context;
            member = (CompilerIrSymbol *)((BYTE *)members->first - 8);
            c1_bss.integer_work_value = (__int64)c1_bss.initializer_bitfield_value;
            flush_initializer_bitfield(member);
            c1_bss.initializer_bitfield_active = 0;
            frame = c1_bss.initializer_frame;
            symbol = frame->symbol;
            type = symbol->value_type;
        }
        members = (CompilerRecordMembers *)type->symbol_owner->kind4_context;
        member = (CompilerIrSymbol *)((BYTE *)members->first - 8);
        count = global_type_size(member->value_type);
        c1_bss.integer_work_value = (unsigned)(global_type_size(type) - count);
        if (c1_bss.integer_work_value) write_initializer_padding(0);
        if (c1_data.initializer_depth >= 0) {
            --c1_data.initializer_depth;
            c1_bss.initializer_frame = (CompilerInitializerFrame *)
                ((BYTE *)c1_bss.initializer_frames + 20 * c1_data.initializer_depth);
        }
        if (c1_data.initializer_depth < 0) {
#ifdef __linux__
            recon_unsupported("union completion reads an unmodeled preceding frame");
#else
            __asm { int 3 }
#endif
        }
        frame = c1_bss.initializer_frame;
        if (frame->flags & 1) { frame->flags |= 2; return 1; }
        goto examine_frame;
    }
    if (kind != 0x402 && kind != 0x408) return 1;
    if (!(frame->flags & 0x80)) {
        if (frame->flags & 1) { frame->flags |= 2; return 1; }
        goto examine_frame;
    }
    members = (CompilerRecordMembers *)symbol->kind4_context;
    binding = members->first;
    member = (CompilerIrSymbol *)((BYTE *)binding - 8);
    if (member->kind >= 1 && member->kind <= 14 && member->kind != 5 && (member->qualifier_flags & 8)) {
        nesting = 1;
        do {
            binding = binding->next;
            member = (CompilerIrSymbol *)((BYTE *)binding - 8);
            if (member->kind >= 1 && member->kind <= 14 && member->kind != 5 &&
                member->binding.name == c1_data.builtin_names.alignment_member) {
                if (member->qualifier_flags & 8) ++nesting;
                else if (member->qualifier_flags & 0x10) --nesting;
            }
        } while (nesting);
    }
    binding = next_initializer_member(member, binding->next);
    next = binding ? (CompilerIrSymbol *)((BYTE *)binding - 8) : NULL;
    if (!next) {
        if (c1_bss.initializer_bitfield_active) {
            c1_bss.integer_work_value = (__int64)c1_bss.initializer_bitfield_value;
            flush_initializer_bitfield(member);
            c1_bss.initializer_bitfield_active = 0;
        }
        goto examine_frame;
    }
    if (c1_bss.initializer_bitfield_active && c1_bss.initializer_bitfield_position != next->symbol_value) {
        c1_bss.integer_work_value = (__int64)c1_bss.initializer_bitfield_value;
        flush_initializer_bitfield(member);
        c1_bss.initializer_bitfield_active = 0;
    }
    frame = c1_bss.initializer_frame;
    members = (CompilerRecordMembers *)frame->symbol->kind4_context;
    if (!members) {
        members = (CompilerRecordMembers *)c1_bss.arenas[1].cursor;
        c1_bss.arenas[1].cursor += 12;
        if (c1_bss.arenas[1].cursor > c1_bss.arenas[1].committed_limit)
            c1_bss.arenas[1].CommitExtent();
        if (members) { members->arena_kind = 1; members->first = members->last = NULL; }
        frame = c1_bss.initializer_frame;
        frame->symbol->kind4_context = members;
    }
    members->first = members->last = binding;
    return 1;
}
extern "C" WORD __cdecl get_initializer_string_length(CompilerIrSymbol *symbol, CompilerParserNode *value)
{
    unsigned unit = value->string_flags & 1 ? 2 : (unsigned)(int)c1_primitive_widths[1];
    WORD bytes = (WORD)((WORD)symbol->value_type->shape->value * unit);
    if (!bytes || bytes >= value->string_length) return value->string_length;
    if (bytes < (WORD)(value->string_length - 1))
        report_language_diagnostic(1, 45, value->text);
    return bytes;
}
extern "C" void __cdecl write_runtime_initializer_value(CompilerIrSymbol *, CompilerParserNode *) { __asm { int 3 } }
extern "C" void __cdecl finish_runtime_bitfield(CompilerParserNode *) { __asm { int 3 } }
extern "C" CompilerParserNode *__cdecl make_typed_constant(CompilerType *, const void *, unsigned);
extern "C" CompilerParserNode *__cdecl take_expression_address(CompilerParserNode *);
extern "C" CompilerParserNode *__cdecl make_binary_expression(BYTE, CompilerParserNode *, CompilerParserNode *);
extern "C" void __cdecl write_anonymous_runtime_initializer(void);
/* RVA5756c is source-only until the runtime-initializer producer is reached. */
static int runtime_initializer_pending;
extern "C" void __cdecl write_initializer_padding(CompilerIrSymbol *symbol)
{
    CompilerParserNode *value;
    if (!(c1_data.lexer_flags & 0x100000)) {
        value = make_typed_constant(c1_data.builtin_types.type_0063,
            &c1_bss.integer_work_value, c1_data.expression_arena_kind);
        write_initializer_bytes(symbol, value, &c1_bss.output_buffers[4]);
        if (!symbol) c1_bss.initializer_bytes += (unsigned)c1_bss.integer_work_value;
    } else {
        if (!symbol) { write_anonymous_runtime_initializer(); return; }
        value = make_typed_constant(c1_data.builtin_types.type_0063,
            &c1_bss.integer_work_value, c1_data.expression_arena_kind);
        if (c1_bss.function_cleanup_context) {
            value = make_binary_expression(77, take_expression_address(
                make_label_expression((CompilerIrSymbol *)c1_bss.function_cleanup_context, 0)), value);
            write_runtime_initializer_value(symbol, value);
            /* The genuine pending writer is reached before this later,
             * still-unmodeled runtime completion flag. */
#ifdef __linux__
            recon_unsupported("runtime initializer completion flag is not yet modeled");
#else
            __asm { int 3 }
#endif
            runtime_initializer_pending = 1;
        } else write_runtime_initializer_value(symbol, value);
    }
}
extern "C" void __cdecl write_anonymous_runtime_initializer(void)
{ __asm { int 3 } }
extern "C" CompilerType *__cdecl replace_function_parameters(CompilerType *, unsigned, WORD, unsigned *);
extern "C" CompilerType *__cdecl decay_initializer_array(CompilerParserNode *value)
{
    if (value->kind == 80 && value->left->kind == 112 && !value->left->type->shape)
        report_language_diagnostic(c1_data.options.language_extensions ? 4 : 1, 223);
    if ((value->flags & 7) == 5) return value->type;
    return replace_function_parameters(value->type, 1, 0, NULL);
}
extern "C" int __cdecl convert_initializer_float_to_long(const BYTE *, int *) { __asm { int 3 } return 0; }

extern "C" unsigned __cdecl collect_initializer_remainder(void);
extern "C" unsigned __cdecl initializer_remaining_size(void);
extern "C" void __cdecl finish_initializer_element(void)
{
    unsigned padding;
    if (c1_data.initializer_depth < 0) return;
    if (c1_data.initializer_depth > C1_INITIALIZER_LAST) {
#ifdef __linux__
        recon_unsupported("element completion beyond encountered initializer frames");
#else
        __asm { int 3 }
#endif
    }
    if (c1_bss.initializer_frame->flags & 0x20) {
        --c1_data.initializer_depth;
        c1_bss.initializer_frame = (CompilerInitializerFrame *)
            ((BYTE *)c1_bss.initializer_frames + 20 * c1_data.initializer_depth);
        return;
    }
    padding = collect_initializer_remainder();
    if (!(c1_bss.initializer_frame->flags & 6))
        padding += initializer_remaining_size();
    c1_bss.initializer_frame->flags = 0;
    finish_initializer_value();
    if (padding) {
        c1_bss.integer_work_value = padding;
        write_initializer_padding(0);
    }
}
extern "C" unsigned __cdecl collect_initializer_remainder(void)
{
    unsigned count = 0;
    if (c1_data.initializer_depth < 0) return 0;
    if (c1_data.initializer_depth > C1_INITIALIZER_LAST) {
#ifdef __linux__
        recon_unsupported("initializer remainder beyond encountered frame records");
#else
        __asm { int 3 }
#endif
    }
    if (c1_bss.initializer_frame->flags & 1) return 0;
    do {
        count += initializer_remaining_size();
        finish_initializer_value();
        /* The original rereads the cursor without checking its depth. The
         * preceding raw slot has no semantic contract in this projection. */
        if (c1_data.initializer_depth < 0 || c1_data.initializer_depth > C1_INITIALIZER_LAST) {
#ifdef __linux__
            recon_unsupported("initializer remainder reads an unmodeled preceding frame");
#else
            __asm { int 3 }
#endif
        }
    } while (!(c1_bss.initializer_frame->flags & 1));
    return count;
}
extern "C" unsigned __cdecl initializer_remaining_size(void) { __asm { int 3 } return 0; }

extern "C" {
void __cdecl write_initializer_symbol(CompilerIrSymbol *, CompilerParserNode *, CompilerOutputBuffer *);
void __cdecl finalize_initializer_symbol(CompilerIrSymbol *);
int __cdecl emit_runtime_initializer_assignment(CompilerParserNode *, CompilerParserNode *);
unsigned __cdecl initializer_string_padding(CompilerIrSymbol *, CompilerParserNode *);
void __cdecl initialize_symbol_value(CompilerIrSymbol *symbol, CompilerParserNode *value)
{
    CompilerParserNode *constant;
    CompilerType *type;
    CompilerTypeShape *shape;
    unsigned padding;
    int string;
    if (!symbol || (symbol->scope_bits & 1) || !value) return;
    string = initializer_accepts_string(symbol, value);
    if (string) value->string_length = (WORD)get_initializer_string_length(symbol, value);
    if (!(c1_data.lexer_flags & 0x100000)) {
        if (!check_initializer_expression(symbol, &value)) { compiler_warning(99); return; }
        convert_initializer_expression(symbol, value);
    }
    if ((c1_data.lexer_flags & 0x100000) && initializer_requires_storage(symbol)) {
        emit_runtime_initializer_assignment(make_label_expression(symbol, 0), value);
        finalize_initializer_symbol(symbol);
        return;
    }
    type = symbol->value_type;
    shape = type->shape;
    if (!shape && (type->flags & 0x400)) {
        if (c1_data.lexer_flags & 0x100000) {
            symbol->declaration_flags &= ~4;
            emit_runtime_initializer_assignment(make_label_expression(symbol, 0), value);
            finalize_initializer_symbol(symbol);
        } else compiler_warning(74, symbol, type);
        return;
    }
    if (shape && (symbol->qualifier_flags & 4) && !string) {
        if (value->kind == 7 && !shape->base &&
            ((value->string_flags & 1) ? (type->flags & 0x2fcf) == 0x41 :
             (type->flags & 0xefcf) == 0x4042)) compiler_warning(53, symbol);
        else compiler_warning(75, symbol);
        return;
    }
    constant = strip_value_wrappers(value);
    if (c1_data.lexer_flags & 0x100000) write_runtime_initializer_value(symbol, constant);
    else write_initializer_symbol(symbol, constant, &c1_bss.output_buffers[4]);
    if ((c1_data.lexer_flags & 0x100000) || initializer_accepts_string(symbol, constant)) {
        padding = initializer_string_padding(symbol, constant);
        c1_bss.integer_work_value = padding;
        if (padding) write_initializer_padding(0);
        finalize_initializer_symbol(symbol);
        return;
    }
    if (value->type->shape && (value->type->shape->flags & 7) == 2)
        value->type = decay_initializer_array(value);
    convert_expression_to_type(116, symbol->value_type, value);
    finalize_initializer_symbol(symbol);
}
CompilerParserNode *__cdecl take_expression_address(CompilerParserNode *);
CompilerParserNode *__cdecl make_binary_expression(BYTE, CompilerParserNode *, CompilerParserNode *);
void __cdecl emit_expression(CompilerParserNode *);
int __cdecl emit_runtime_initializer_assignment(CompilerParserNode *target, CompilerParserNode *value)
{
    CompilerParserNode *address = take_expression_address(target);
    emit_expression(make_binary_expression(116, address, value));
    return 1;
}
CompilerType *__cdecl replace_function_parameters(CompilerType *, unsigned, WORD, unsigned *);
unsigned __cdecl initializer_string_padding(CompilerIrSymbol *symbol, CompilerParserNode *value)
{
    CompilerType *type;
    CompilerTypeShape *shape;
    unsigned count, unit;
    if (c1_data.initializer_depth >= 0) return 0;
    type = symbol->value_type;
    shape = type->shape;
    count = shape->value;
    unit = value->string_flags & 1 ? 2 : (unsigned)(int)c1_primitive_widths[1];
    if (count) return count * unit - value->string_length;
    count = value->string_length / unit;
    symbol->value_type = replace_function_parameters(type, shape->flags, shape->attributes, &count);
    return 0;
}
}
