#include "functions.h"
#include "function-records.h"
#include "code-records.h"
#include "output-callbacks.h"
#include "code-input.h"
#include "flow.h"
#include "inlining.h"
#include "symbol-storage.h"
#include "function-reader.h"
#include <string.h>
#include <recon.h>

extern int c2_basic_mode, c2_codeview_types;
extern unsigned prologue_unwind_state;
/* Exact initialized file DWORDs; only their reached truth-value use is known.
 * The pending producer owns any later nonzero semantics. */
unsigned function_output_state14, function_output_state18, function_output_state1c;
struct C2FunctionOutputFixup;
struct C2FunctionOutputFixup *function_output_fixups2c, *function_output_fixups30;
/* The controller clears this source-location cache before the layout pass.
 * The actual resolver supplies a filename and its mapped source line. */
unsigned function_source_line; /*497f0c*/
char *function_source_filename; /*497f10*/
char *default_function_source_filename; /*497dc4, initializedNULL*/

extern int function_input_flags;
C2Symbol *__fastcall find_runtime_function(char *name);
void initialize_function_output(void)
{
    C2Section *section;
    if (current_symbol && !output_enabled &&
        (!strcmp(current_symbol->name, "_main") || !strcmp(current_symbol->name, "_WinMain")))
        find_runtime_function("__acrtused");
    if (*(unsigned char *)&function_input_flags) find_runtime_function("__fltused");
    if (c2_il_eight_byte_target) {
        debug_types_section = NULL;
        debug_symbols_section = NULL;
        code_section = (C2Section *)lookup_global_symbol(5);
        data_section = (C2Section *)lookup_global_symbol(2);
        const_section = (C2Section *)lookup_global_symbol(3);
        bss_section = (C2Section *)lookup_global_symbol(4);
        if (c2_codeview_types) {
            debug_types_section = (C2Section *)lookup_global_symbol(8);
            debug_symbols_section = (C2Section *)lookup_global_symbol(7);
        }
        if (code_section) code_section->link.order = 1;
        if (data_section) data_section->link.order = 2;
        if (bss_section) bss_section->link.order = 4;
        if (const_section) const_section->link.order = 3;
        section = debug_symbols_section;
        if (section) {
            section->link.order = 5;
            section->alignment = 1;
            section->combine = 0;
            section->attribute = 1;
        }
        section = debug_types_section;
        if (section) {
            section->link.order = 6;
            section->alignment = 1;
            section->combine = 0;
            section->attribute = 1;
        }
    }
}
void __fastcall normalize_output_conditions(C2Function *function)
{
    C2Expression *record;
    unsigned condition;
    for (record = (C2Expression *)function->flow->last->records; record; record = record->next) {
        if (record->kind != 16 || (!record->detail.data && record->opcode != 0x15c)) continue;
        /* The actual DWORD-stride column48d208 converts the internal
         * condition code to its output encoding. Only this consumed byte
         * and the known0..16 condition domain are reconstructed. */
        condition = record->condition_code;
        switch (condition) {
        case 0: condition = 16; break;
        case 1: condition = 4; break;
        case 2: condition = 5; break;
        case 3: condition = 12; break;
        case 4: condition = 15; break;
        case 5: condition = 14; break;
        case 6: condition = 13; break;
        case 7: condition = 2; break;
        case 8: condition = 7; break;
        case 9: condition = 6; break;
        case 10: condition = 3; break;
        case 11: condition = 0; break;
        case 12: condition = 1; break;
        case 13: condition = 8; break;
        case 14: condition = 9; break;
        case 15: condition = 10; break;
        case 16: condition = 11; break;
        default: { __asm { int 3 } }
        }
        record->condition_code = (unsigned char)condition;
    }
}
void finalize_register_displacements(void)
{
    C2RegisterBlock *block;
    C2FunctionRegister *reg, *variant;
    long displacement;
    for (block = first_function_register_block; block; block = block->next) {
        for (reg = block->records; reg < block->cursor; ++reg) {
            switch (reg->kind) {
            case 3:
                if (reg->canonical != reg || !(reg->view_flags & 0x20)) continue;
                displacement = reg->frame_displacement;
                break;
            case 4: case 5: case 7: case 8:
                if (reg->canonical != reg) continue;
                displacement = reg->symbol->parameter_offset;
                break;
            default:
                continue;
            }
            for (variant = reg; variant; variant = variant->variant_next)
                variant->displacement = (long)((unsigned)variant->displacement + displacement);
        }
    }
}
extern int generate_frame, generate_prologue;
int __fastcall final_record_code_size(C2Expression *record);
void __fastcall initialize_instruction_sizes(C2Function *function)
{
    C2Expression *record, *input;
    int saved_prologue = generate_prologue, saved_frame = generate_frame;
    for (record = (C2Expression *)function->flow->last->records; record; record = record->next) {
        if (record->flags & 1) {
            if (record->kind == 16) {
                input = record->left;
                if (input->kind == 4 && input->value.symbol->kind == 3) {
                    if (record->opcode == 0xf) record->opcode = 0x102;
                    else if (record->opcode == 0x10) record->opcode = 0x104;
                }
            } else if (record->opcode == 0xba) record->opcode = 0xd;
            else if (record->opcode == 0xb9) record->opcode = 0xc;
            record->control_flags = (unsigned char)final_record_code_size(record);
        } else if (record->kind == 25) {
            record->control_flags = 0;
            if (record->declaration->symbol.source_kind == 0x54 ||
                record->declaration->symbol.source_kind == 0x55)
                generate_prologue = generate_frame = 0;
        }
    }
    generate_prologue = saved_prologue;
    generate_frame = saved_frame;
}
int __fastcall relax_instruction_branch(C2Expression **record, unsigned offset,
    unsigned char iteration, int delta) { __asm { int 3 } return 0; }
void __fastcall optimize_instruction_offsets(C2Function *function)
{
    C2Expression *record, *next;
    unsigned offset;
    unsigned char iteration = 0;
    int changed, delta, adjustment;
    do {
        ++iteration;
        changed = 0;
        delta = 0;
        offset = function->symbol->symbol.parameter_offset;
        for (record = (C2Expression *)function->flow->last->records; record; record = next) {
            next = record->next;
            if (record->flags & 1)
                offset += record->control_flags;
            else if (record->kind == 25) {
                record->declaration->symbol.parameter_offset = offset;
                record->declaration->branch_record->control_flags = iteration;
            }
            if (record->kind == 16) {
                adjustment = relax_instruction_branch(&record, offset, iteration, delta);
                delta += adjustment;
                offset += adjustment;
                if (adjustment) changed = 1;
            }
        }
    } while (changed);
}
void __fastcall layout_function_code(C2Function *function, int mode);
void __fastcall prepare_function_code_offsets(C2Function *function)
{
    initialize_instruction_sizes(function);
    layout_function_code(function, 0);
    optimize_instruction_offsets(function);
}
/* The debug section remains a distinct initialized NULL pointer until its
 * actual producer establishes an object. It is only selected on this body's
 * already identified cold debug-source path. */
C2Section *function_debug_section; /*497f04*/
C2Expression *__fastcall new_linked_instruction(unsigned opcode, unsigned short type,
    C2InputLink link, C2Record *prior) { __asm { int 3 } return NULL; }
extern unsigned diagnostic_position;
extern int c2_global_optimize, c2_zero_alignment;
void __fastcall layout_function_code(C2Function *function, int mode)
{
    C2FunctionSymbol *symbol = function->symbol;
    C2Symbol *section;
    C2Expression *record, *next, *padding;
    C2InputSymbol *declaration;
    C2DebugScope *scope;
    unsigned offset = (unsigned)-1, saved_offset, alignment;
    int saved_prologue = generate_prologue, saved_frame = generate_frame;
    int alternate_section = 0, entry_active;
    /* Like the original, entry_active is written by the real kind22 entry.
     * A malformed list reaching its end marker first has an unwritten local. */
    section = lookup_global_symbol(symbol->symbol.association.section_serial);
    for (record = (C2Expression *)function->flow->last->records; record; record = next) {
        next = record->next;
        if (record->location.line) {
            diagnostic_position = function->source_line_base + record->location.line;
            if (!function_source_filename)
                function_source_line = resolve_diagnostic_location(diagnostic_position, &function_source_filename);
        }
        switch (record->kind) {
        case 11: case 12: case 13: case 14: case 15:
        case 16: case 17: case 18: case 19: case 20:
            offset += record->control_flags;
            break;
        case 21:
            switch (record->opcode) {
            case 0x17e:
                if (!record->control_flags) {
                    if (entry_active) {
                        entry_active = 0;
                        symbol->prologue_bytes = offset == (unsigned)-1 ? 0 :
                            offset - symbol->symbol.parameter_offset;
                    }
                    if (!c2_global_optimize && c2_codeview_types) {
                        scope = (C2DebugScope *)record->left;
                        scope->start = offset;
                    }
                }
                break;
            case 0x17f:
                if (!record->control_flags && !c2_global_optimize && c2_codeview_types) {
                    scope = (C2DebugScope *)record->left;
                    scope->end = offset;
                }
                break;
            case 0x181:
                symbol->epilogue_offset = offset == (unsigned)-1 ? 0 :
                    offset - symbol->symbol.parameter_offset;
                break;
            case 0x185:
                alignment = (record->value.control - offset) & (record->value.control - 1);
                if (alignment && mode) {
                    padding = new_linked_instruction(0xa9, 0x6000, prepend_input_record, (C2Record *)record);
                    padding->control_flags = (unsigned char)alignment;
                    padding->flags |= 2;
                }
                offset += alignment;
                break;
            }
            break;
        case 22:
            entry_active = 1;
            offset = section->parameter_offset;
            symbol->symbol.parameter_offset = offset;
            break;
        case 23:
            if (alternate_section) offset = saved_offset;
            symbol->code_bytes = offset == (unsigned)-1 ? 0 : offset - symbol->symbol.parameter_offset;
            break;
        case 25:
            declaration = record->declaration;
            if (declaration->symbol.source_kind == 2) {
                if (mode && next->opcode == 0xbb && (next->type & 0xfff) == 4 &&
                    c2_zero_alignment && c2_global_optimize) {
                    alignment = -offset & 3;
                    if (alignment) {
                        padding = new_linked_instruction(0xa9, 0x6000, prepend_input_record, (C2Record *)record);
                        padding->control_flags = (unsigned char)alignment;
                    }
                    offset += alignment;
                }
            } else if ((declaration->symbol.source_kind == 0x54 ||
                        declaration->symbol.source_kind == 0x55) && !alternate_section) {
                generate_prologue = generate_frame = 0;
                alternate_section = 1;
                saved_offset = offset;
                if ((symbol->symbol.flags & 0x20) || c2_internal_listing)
                    section = &select_symbol_section(symbol, &function_debug_section->symbol)->symbol;
                else section = &function_debug_section->symbol;
                offset = section->parameter_offset;
            }
            declaration->symbol.parameter_offset = offset;
            declaration->symbol.serial = section->serial;
            break;
        }
    }
    generate_prologue = saved_prologue;
    generate_frame = saved_frame;
}
extern int c2_constant_strings;
int __fastcall encode_final_record(C2Expression *record, unsigned char *encoding,
    unsigned char *marks, void *relocations);
void __fastcall finish_function_code_offsets(C2Function *function)
{
    C2FunctionSymbol *symbol = function->symbol;
    C2Section *section, *active_section = NULL;
    C2Expression *record;
    C2InputSymbol *alternate_declaration = NULL;
    unsigned char bytes[20];
    unsigned offset = (unsigned)-1, saved_offset = 0, source_position = 0;
    unsigned position, mapped_line, count;
    char *file, *previous_file = NULL;
    int size, padding;
    int saved_prologue = generate_prologue, saved_frame = generate_frame;

    section = (C2Section *)lookup_global_symbol(symbol->symbol.association.section_serial);
    diagnostic_position = 0;
    for (record = (C2Expression *)function->flow->last->records; record; record = record->next) {
        if ((record->flags & 1) && record->location.line) {
            position = function->source_line_base + record->location.line;
            diagnostic_position = position;
            if (source_position != position) {
                mapped_line = resolve_diagnostic_location(position, &file);
                if (file != previous_file) {
                    previous_file = file;
                    output_slot_497ddc.target(file);
                }
                if (active_section != section) {
                    output_slot_497ce4.target(section);
                    active_section = section;
                }
                output_slot_497dcc.target(mapped_line, offset);
                source_position = position;
            }
        }
        switch (record->kind) {
        case 11: case 12: case 13: case 14: case 15:
        case 16: case 17: case 18: case 19: case 20:
            size = encode_final_record(record, bytes, (unsigned char *)offset, (void *)1);
            output_slot_497dd4.target(bytes, size);
            offset += size;
            active_section->symbol.parameter_offset += size;
            break;
        case 22:
            offset = symbol->symbol.parameter_offset;
            output_slot_497ce4.target(section);
            if (c2_constant_strings && offset != section->symbol.parameter_offset) {
                count = offset - section->symbol.parameter_offset;
                /* The original also uses this20-byte local without checking
                 * a larger section gap. Preserve that genuine defect. */
                memset(bytes, 0, count);
                output_slot_497dd4.target(bytes, count);
                section->symbol.parameter_offset += count;
            }
            output_slot_497dbc.target(&symbol->symbol);
            active_section = section;
            break;
        case 25:
            if ((record->declaration->symbol.source_kind == 0x54 ||
                 record->declaration->symbol.source_kind == 0x55) && !alternate_declaration) {
                generate_prologue = generate_frame = 0;
                alternate_declaration = record->declaration;
                saved_offset = offset;
                if ((symbol->symbol.flags & 0x20) || c2_internal_listing)
                    active_section = select_symbol_section(symbol, (C2Symbol *)function_debug_section);
                else active_section = function_debug_section;
                output_slot_497ce4.target(active_section);
            }
            offset = record->declaration->symbol.parameter_offset;
            break;
        }
    }
    generate_prologue = saved_prologue;
    generate_frame = saved_frame;
    output_slot_497ce4.target(section);
    output_slot_497de0.target(&symbol->symbol);
    if (c2_zero_alignment && c2_global_optimize) {
        if (alternate_declaration) offset = saved_offset;
        padding = (16 - ((int)(offset - symbol->symbol.parameter_offset) % 16)) % 16;
        if (padding > 0) memset(bytes, 0x90, padding);
        output_slot_497dd4.target(bytes, padding);
        section->symbol.parameter_offset += padding;
    }
    if ((function->flags & 0x20000) && !(function->flags & 0x40000))
        compiler_warning(1, 799, set_diagnostic_function(symbol, 1));
}
void __fastcall finish_function_output_state(C2Function *function);
void __fastcall write_fpo_record(C2Symbol *symbol, unsigned offset, unsigned code_bytes,
    unsigned local_words, unsigned parameter_words, unsigned char prologue,
    unsigned char saved_registers, int seh, int frame, int use_bp)
{
    FPO_DATA data;
    unsigned char flags;
    extern char *c2_output_name;
    extern C2Section *function_fpo_section;
    if (!c2_output_name || !output_enabled) return;
    output_slot_497ce4.target(function_fpo_section);
    data.ulOffStart = offset;
    data.cbProcSize = code_bytes;
    data.cdwLocals = local_words;
    data.cdwParams = (unsigned short)parameter_words;
    data.cbProlog = prologue;
    /* Preserve the original BYTE flag composition, including input bits
     * that can carry into adjacent bitfields, before naming each SDK field. */
    flags = (unsigned char)(((((((frame != 0) ? 3 : 0) << 2) |
        (unsigned char)use_bp) << 1) | (unsigned char)seh) << 3) | saved_registers;
    data.cbRegs = flags & 7;
    data.fHasSEH = (flags >> 3) & 1;
    data.fUseBP = (flags >> 4) & 1;
    data.reserved = (flags >> 5) & 1;
    data.cbFrame = flags >> 6;
    output_slot_497db0.target(&data, sizeof(data));
    output_slot_497db4.target((unsigned)-16, symbol);
    function_fpo_section->symbol.parameter_offset += sizeof(data);
}
void __fastcall write_function_unwind(C2Symbol *header, int plain, int flag5)
{
    C2FunctionSymbol *symbol = (C2FunctionSymbol *)header;
    if (symbol->prologue_bytes >= 256)
        c2_report_internal("E:\\utc\\src\\\\P2\\emit.c", 577);
    write_fpo_record(header, 0, symbol->code_bytes,
        (symbol->frame_stack_bytes >> 2) + (plain != 0), symbol->argument_stack_bytes >> 2,
        (unsigned char)symbol->prologue_bytes, (unsigned char)current_function->saved_register_count,
        0, plain, plain || flag5);
}
extern unsigned frame_allocated_offset;
extern C2Symbol *frame_reserved_symbol;
extern C2FunctionRegister *frame_extra_slot;
extern int prologue_has_reserved_storage;
/*497e6c: only this exact DWORD clear is encountered; no later role/default
 * or neighboring object extent is inferred. */
unsigned function_output_reset6c;
void finish_function_output(void)
{
    argument_stack_offset = 8;
    frame_allocated_offset = 0;
    frame_reserved_symbol = NULL;
    frame_extra_slot = NULL;
    prologue_has_reserved_storage = 0;
    function_output_reset6c = 0;
}
void __fastcall function_output_pass_487382(C2Function *function) { __asm { int 3 } }
void __fastcall function_output_pass_465509(C2Function *function) { __asm { int 3 } }
void __fastcall function_output_pass_4835e2(C2Function *function) { __asm { int 3 } }
void __fastcall function_output_pass_45880d(C2Function *function) { __asm { int 3 } }
void __fastcall function_output_pass_458bf2(C2Function *function) { __asm { int 3 } }
void __fastcall function_output_pass_4840ce(C2Function *function) { __asm { int 3 } }
void __fastcall function_output_pass_4872d4(C2Function *function) { __asm { int 3 } }
void __fastcall function_output_pass_483329(C2Function *function) { __asm { int 3 } }
void __fastcall function_output_pass_4846b8(C2Function *function) { __asm { int 3 } }
void __fastcall function_output_pass_484a8d(C2Function *function) { __asm { int 3 } }

void __fastcall emit_final_function(C2Function *function)
{
    C2FunctionSymbol *symbol = function->symbol;
    C2DebugLink *records;
    int unwind = prologue_unwind_state || (function->flags & 0x100) ||
        (symbol->symbol.flags & 0x1000) ? 1 : 0;

    initialize_function_output();
    if (function->flags & 0x10000) function_output_pass_487382(function);
    if (function->symbol->symbol.flags & 0x1000) {
        if (c2_basic_mode) function_output_pass_4835e2(function);
        function_output_pass_465509(function);
    } else if ((function->flags & 0x8000) || (function->symbol->additional_flags & 0x200)) {
        function_output_pass_45880d(function);
    }
    if (function_output_state14) function_output_pass_458bf2(function);
    normalize_output_conditions(function);
    if (function_output_state1c) function_output_pass_4840ce(function);
    if (function->flags & 1) function_output_pass_4872d4(function);
    if (function_output_fixups30 || function_output_fixups2c) function_output_pass_483329(function);
    records = prepare_function_records(function);
    function_source_filename = NULL;
    function_source_line = 0;
    finalize_register_displacements();
    prepare_function_code_offsets(function);
    layout_function_code(function, 1);
    if (function_output_state18) function_output_pass_4846b8(function);
    if (!function_source_filename) function_source_filename = default_function_source_filename;
    output_slot_497dec.target(&symbol->symbol, function_source_filename, unwind, function_source_line);
    process_records(records);
    finish_function_code_offsets(function);
    finish_function_output_state(function);
    if ((function->flags & 0x8000) && c2_codeview_types) function_output_pass_484a8d(function);
    output_slot_497de4.target(&symbol->symbol);
    if ((function->flags & 0x100) || prologue_unwind_state)
        write_function_unwind(&symbol->symbol, (~function->flags >> 8) & 1, (function->flags >> 5) & 1);
    finish_function_output();
}

void prune_unknown_output_symbols(void) { __asm { int 3 } }
/* Same global initializer chain used by the input producer and output walker. */
extern C2InitRecord *global_initializers;
C2InitRecord *last_global_initializer; /*497df4, initializedNULL*/
void discard_emitted_initializers(void)
{
    C2InitRecord *record = global_initializers, *previous = NULL, *discard;
    while (record) {
        if (record->emitted) {
            discard = record;
            record = record->list_next;
            if (previous) previous->list_next = record;
            else global_initializers = record;
            if (last_global_initializer == discard) last_global_initializer = previous;
            free_initializer(discard);
        } else {
            previous = record;
            record->symbol = NULL;
            record = record->list_next;
        }
    }
}
void clear_symbol_input_registers(void)
{
    C2RegisterBlock *block;
    C2FunctionRegister *reg;
    for (block = first_function_register_block; block; block = block->next)
        for (reg = block->records; reg < block->cursor; ++reg)
            if (reg->kind != 0x10 && reg->symbol) reg->symbol->input_register = NULL;
}
/*497e70: exact reset-only DWORD, with no inferred pointee or wider object. */
unsigned function_symbol_reset70;
void clear_function_symbol_scope(void)
{
    c2_node_bytes[1] = 0;
    reset_input_pool(1);
    function_symbol_reset70 = 0;
    current_symbol = NULL;
    local_symbol_table = NULL;
}
/*48fb24: this leaf establishes only a zero DWORD. Its unvisited producers
 * and consumers retain their pending identities and grant no payload model. */
unsigned function_finish_state;
void clear_function_finish_state(void)
{
    function_finish_state = 0;
}

void finish_function(void)
{
    if (c2_il_extended_header) prune_unknown_output_symbols();
    discard_emitted_initializers();
    clear_symbol_input_registers();
    clear_function_symbol_scope();
    reset_input_pool(5);
    reset_input_pool(12);
    reset_input_pool(8);
    reset_input_pool(1);
    reset_input_pool(6);
    reset_input_pool(4);
    clear_function_finish_state();
}
