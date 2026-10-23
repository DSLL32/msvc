/* Machine selection of serialized intrinsic operators. */
#include "intrinsic-selection.h"
#include "frame.h"
#include <recon.h>
extern int default_segment_alignment, code_pointer_bytes;
SymbolHeader *__fastcall require_external_symbol(char *name);

/* Original [0001b270,0001bbf4), including95 dispatch entries at1ba78.
   Emission is variadic: no-operand and single-operand instructions pass only
   their original arguments, avoiding the decompiler's spurious spare GPRs. */
unsigned short __fastcall select_intrinsic(Node *node)
{
    unsigned short result = 0xffff, type = node->attributes.words.type, address_register;
    unsigned intrinsic = node->left.node->left.value, bytes, count, part, width;
    int floats;
    Node *arguments, *value;
    MachineOperand *left, *right, *memory, *temporary;
    FunctionSymbol *symbol;
    switch (intrinsic) {
    case 0x96:
        result = select_intrinsic_block(node, (unsigned short)node->detail.control.code, 0);
        break;
    case 0x97:
        result = select_intrinsic_block(node, (unsigned short)node->detail.control.code, 1);
        break;
    case 0xa3: result = select_intrinsic_a3_a4(node, 1); break;
    case 0xa4: result = select_intrinsic_a3_a4(node, 0); break;
    case 0xa5: result = select_intrinsic_a5(node); break;
    case 0xa6: result = select_intrinsic_a6(node); break;
    case 0xa7:
        result = select_intrinsic_string_length(node->second.node->left.node, 0, type);
        break;
    case 0xa8: case 0xaf: case 0xe0:
        left = operand_for_register(select_value_register(node->second.node->left.node));
        if (intrinsic == 0xa8) type = 0x101;
        else if (intrinsic == 0xaf) type = 0x102;
        else type = 0x104;
        emit_machine_operation(MC_MOV, 0x102, &operand_edx, left);
        emit_machine_operation(MC_IN, type, &operand_eax, &operand_edx);
        left = operand_for_register(new_value_register(type));
        emit_machine_operation(MC_MOV, type, left, &operand_eax);
        result = left->value.reg.words.number;
        break;
    case 0xa9: case 0xb0: case 0xe1:
        left = operand_for_register(select_value_register(node->second.node->left.node));
        right = operand_for_register(select_value_register(node->second.node->second.node->left.node));
        if (intrinsic == 0xa9) type = 0x101;
        else if (intrinsic == 0xb0) type = 0x102;
        else type = 0x104;
        emit_machine_operation(MC_MOV, 0x102, &operand_edx, left);
        emit_machine_operation(MC_MOV, type, &operand_eax, right);
        emit_machine_operation(MC_OUT, type, &operand_edx, &operand_eax);
        result = right->value.reg.words.number;
        break;
    case 0xaa: result = select_intrinsic_aa_ab(node, 0); break;
    case 0xab: result = select_intrinsic_aa_ab(node, 1); break;
    case 0xac: result = select_intrinsic_block(node, 0x101, 0); break;
    case 0xad: result = select_intrinsic_block(node, 0x101, 1); break;
    case 0xb1:
        emit_machine_operation(MC_STI, 0);
        result = 0;
        break;
    case 0xb2:
        emit_machine_operation(MC_CLI, 0);
        result = 0;
        break;
    case 0xb3:
        arguments = node->second.node;
        bytes = aligned_value_size(node);
        if ((int)bytes <= 20) {
            address_register = 0;
            if (arguments->left.node->opcode != 0x26)
                address_register = select_value_register(arguments->left.node);
            count = arguments->second.node->left.node->left.value;
            while ((int)count > 0) {
                left = operand_for_register(new_value_register(0x100));
                if (address_register) memory = new_operand(OPERAND_BASED, address_register, 0, 0);
                else memory = symbol_operand(arguments->left.node, 1);
                part = count & 3;
                if (!part) part = 4;
                if (part == 3) {
                    memory->value.address->first.displacement += count - 1;
                    emit_machine_operation(MC_MOV, 0x101, left, memory);
                    emit_machine_operation(MC_SHL, 0x104, left,
                        new_operand(OPERAND_IMMEDIATE, 0, 16, 0));
                    memory = duplicate_operand(memory);
                    memory->value.address->first.displacement -= 2;
                    width = 0x102;
                } else {
                    memory->value.address->first.displacement += count - part;
                    width = part | 0x100;
                }
                emit_machine_operation(MC_MOV, width, left, memory);
                emit_machine_operation(MC_PUSH, 0x104, left);
                count -= part;
            }
        } else {
            emit_machine_operation(MC_SUB, 0x104, &operand_esp,
                new_operand(OPERAND_IMMEDIATE, 0, bytes, 0));
            left = operand_for_register(select_value_register(arguments->left.node));
            emit_machine_operation(MC_MOV, 0x104, &operand_esi, left);
            emit_machine_operation(MC_MOV, 0x104, &operand_edi, &operand_esp);
            count = arguments->second.node->left.node->left.value;
            emit_machine_operation(MC_MOV, 0x104, &operand_ecx,
                new_operand(OPERAND_IMMEDIATE, 0, (unsigned)((int)count / 4), 0));
            emit_machine_operation(MC_REP, 0);
            emit_machine_operation(MC_MOVS, 0x104);
            if (count & 2) emit_machine_operation(MC_MOVS, 0x102);
            if (count & 1) emit_machine_operation(MC_MOVS, 0x101);
        }
        result = 0;
        break;
    case 0xd7: case 0xd9: case 0xda: case 0xf1: case 0xf2: case 0xf4:
        result = select_assembly_record(node);
        break;
    case 0xd8:
        stack_frame_extent = 1;
        dynamic_stack_allocation = 1;
        value = node->second.node->left.node;
        if (value->opcode == 0x33)
            emit_machine_operation(MC_MOV, 0x104, &operand_eax,
                immediate_operand((value->left.value + default_segment_alignment - 1) &
                    ~(default_segment_alignment - 1U)));
        else {
            left = operand_for_register(select_value_register(value));
            emit_machine_operation(MC_MOV, 0x104, &operand_eax, left);
            emit_machine_operation(MC_ADD, 0x104, &operand_eax,
                immediate_operand(default_segment_alignment - 1));
            emit_machine_operation(MC_AND, 0x104, &operand_eax,
                immediate_operand(~(default_segment_alignment - 1U)));
        }
        emit_register_barrier(&operand_eax, 0x104);
        left = new_operand(OPERAND_SYMBOL, 0, 0, require_external_symbol("__alloca_probe"));
        emit_call(code_pointer_bytes, left, 0, 0, 0);
        emit_register_barrier(&operand_esp, 0x104);
        if (current_function->symbol->field0e & 0x1000) select_assembly_record(node);
        result = REG_ESP;
        break;
    case 0xdb:
        stack_frame_extent = 1;
        if (current_function->symbol->field0e & 0x1000) select_assembly_record(node);
        left = operand_for_register(select_value_register(node->second.node->left.node));
        emit_machine_operation(MC_PUSH, 0x104, left);
        symbol = (FunctionSymbol *)require_external_symbol("__setjmp");
        symbol->field24 &= ~0x700U;
        left = new_operand(OPERAND_SYMBOL, 0, 0, &symbol->symbol);
        emit_call(code_pointer_bytes, left, 0, 0, 0);
        emit_machine_operation(MC_ADD, 4, &operand_esp, immediate_operand(4));
        result = REG_EAX;
        break;
    case 0xde:
        emit_machine_operation(MC_SUB, 0x104, &operand_esp, immediate_operand(aligned_value_size(node)));
        left = select_lvalue_operand(node->second.node->left.node);
        emit_machine_operation(MC_MOV, 0x104, left, &operand_esp);
        result = 0x1a;
        break;
    case 0xdf:
        select_value_register(node->second.node->second.node->left.node);
        result = select_value_register(node->second.node->second.node->second.node->left.node);
        break;
    case 0xf3:
        left = operand_for_register(new_value_register(0x104));
        right = operand_for_register(new_value_register(0x104));
        memory = new_operand(OPERAND_ABSOLUTE, 0, 0, lookup_external_name("__tls_array"));
        memory->flags = (memory->flags & ~0x30U) | 0xc0;
        temporary = new_operand(OPERAND_ABSOLUTE, 0, 0, lookup_external_name("__tls_index"));
        emit_machine_operation(MC_MOV, 0x104, left, memory);
        emit_machine_operation(MC_MOV, 0x104, right, temporary);
        emit_machine_operation(MC_SHL, 0x104, right, immediate_operand(2));
        emit_machine_operation(MC_ADD, 0x104, left, right);
        memory = new_operand(OPERAND_BASED, left->value.reg.words.number, 0, 0);
        emit_machine_operation(MC_MOV, 0x104, left, memory);
        result = left->value.reg.words.number;
        break;
    default:
        if (node->opcode == 0x7a) result = select_runtime_intrinsic(node);
        else compiler_internal_error("@(#)cgintrin.c:1.57", 0x244);
        break;
    }
    if (node->left.node->opcode == 0x26 && node->left.node->right.symbol->kind == 4) {
        floats = intrinsic_float_cleanup_count((FunctionSymbol *)node->left.node->right.symbol);
        while (floats-- > 0) emit_float_return();
    }
    return result;
}

/* Original [0001c4a0,0001c676). Choose count/fill operands and dword
   transfer eligibility, preserving signed constant shifts and the original
   multiplication/shift proofs for a remainder-free byte or word count. */
unsigned short __fastcall select_intrinsic_block(Node *node, unsigned short type, int fill)
{
    Node *destination_tree, *source_tree, *count_tree, *arguments;
    MachineOperand *count_operand, *source, *destination;
    unsigned constant_count = 0, bytes = type & 255;
    int wide, remainder, direction;
    arguments = node->second.node;
    destination_tree = arguments->left.node;
    arguments = arguments->second.node;
    source_tree = arguments->left.node;
    count_tree = arguments->second.node->left.node;
    if (count_tree->opcode == 0x33) constant_count = count_tree->left.value;
    if (!constant_count && count_tree->opcode == 0x33)
        return operand_for_register(select_value_register(destination_tree))->value.reg.words.number;
    if (bytes != 4 && !(node->attributes.words.flags & 0x80)) {
        if (count_tree->opcode == 0x33)
            count_operand = new_operand(OPERAND_IMMEDIATE, 0,
                (unsigned)((int)count_tree->left.value >> (bytes == 1 ? 2 : 1)), 0);
        else count_operand = operand_for_register(select_value_register(count_tree));
        wide = 1;
    } else {
        count_operand = operand_for_register(select_value_register(count_tree));
        wide = 0;
    }
    if (fill && wide) source = select_replicated_fill_operand(source_tree, type, constant_count);
    else source = operand_for_register(select_value_register(source_tree));
    destination = operand_for_register(select_value_register(destination_tree));
    remainder = 0;
    if (wide) {
        remainder = 1;
        if (bytes == 1 &&
            ((count_tree->opcode == 4 && count_tree->second.node->opcode == 0x33 &&
              !(count_tree->second.node->left.value & 3)) ||
             (count_tree->opcode == 9 && count_tree->second.node->opcode == 0x33 &&
              (int)count_tree->second.node->left.value >= 2))) remainder = 0;
        else if (bytes == 2 &&
            ((count_tree->opcode == 4 && count_tree->second.node->opcode == 0x33 &&
              !(count_tree->second.node->left.value & 1)) ||
             (count_tree->opcode == 9 && count_tree->second.node->opcode == 0x33 &&
              (int)count_tree->second.node->left.value >= 1))) remainder = 0;
    }
    direction = (node->attributes.words.flags & 0x80) ? -1 : 1;
    select_block_operation(constant_count, count_operand, source, destination,
        wide, type, fill, remainder, direction);
    return destination->value.reg.words.number;
}

/* Original [0001d4e0,0001d66a). Form a dword whose byte/word lanes carry
   the fill value, skipping unused upper lanes for a known short transfer. */
MachineOperand *__fastcall select_replicated_fill_operand(Node *value, unsigned short type, unsigned count)
{
    MachineOperand *result, *source, *pair;
    unsigned bits;
    result = operand_for_register(new_value_register(0x100));
    if (value->opcode == 0x33) {
        bits = value->left.value;
        if ((type & 255) == 1) bits = (bits & 255) | (bits << 8);
        bits = (bits & 0xffff) | (bits << 16);
        emit_machine_operation(MC_MOV, 0x204, result,
            new_operand(OPERAND_IMMEDIATE, 0, bits, 0));
        return result;
    }
    source = operand_for_register(select_value_register(value));
    if ((type & 255) == 1) {
        pair = operand_for_register(new_value_register(0x100));
        emit_machine_operation(MC_MOV, 0x201, pair, source);
        emit_machine_operation(MC_SHL, 0x202, pair, new_operand(OPERAND_IMMEDIATE, 0, 8, 0));
        emit_machine_operation(MC_MOV, 0x201, pair, source);
        emit_machine_operation(MC_MOV, 0x204, result, pair);
        if (!count || count > 3) {
            emit_machine_operation(MC_SHL, 0x204, result, new_operand(OPERAND_IMMEDIATE, 0, 16, 0));
            emit_machine_operation(MC_MOV, 0x202, result, pair);
        }
    } else {
        emit_machine_operation(MC_MOV, 0x204, result, source);
        if (!count || count > 1) {
            emit_machine_operation(MC_SHL, 0x204, result, new_operand(OPERAND_IMMEDIATE, 0, 16, 0));
            emit_machine_operation(MC_MOV, 0x202, result, source);
        }
    }
    return result;
}

extern int optimize_speed, cpu_model;
/* Original [0001d670,0001da89). Select short unrolled transfers or REP
   string operations, then emit byte/word remainders and restore direction.
   Nine fixed fastcall arguments: seven stack words, original RET1c. */
void __fastcall select_block_operation(unsigned constant_count, MachineOperand *count,
    MachineOperand *source, MachineOperand *destination, int wide, unsigned short type,
    int fill, int remainder, int direction)
{
    unsigned units = constant_count, opcode = fill ? MC_STOS : MC_MOVS;
    unsigned short unit_type = wide ? 0x104 : type;
    unsigned offset = 0, step, i;
    int direct = optimize_speed && cpu_model > 3;
    int unrolled;
    MachineOperand *physical_source;
    if (wide) units >>= ((type & 255) == 1 ? 2 : 1);
    if (constant_count && units <= (fill ? 5U : 4U)) {
        if (direction == -1 && !direct) emit_machine_operation(MC_STD, 0);
        unrolled = 1;
        if (!direct) {
            emit_machine_operation(MC_MOV, 0x104, &operand_edi, destination);
            physical_source = fill ? &operand_eax : &operand_esi;
            emit_machine_operation(MC_MOV, 0x104, physical_source, source);
        }
        step = (unit_type & 255U) * (unsigned)direction;
        for (i = 0; i < units; ++i) {
            if (direct) emit_explicit_block_transfer(fill, source, destination, unit_type, offset);
            else emit_machine_operation(opcode, unit_type);
            offset += step;
        }
        offset = direct ? (unit_type & 255U) * units : 0;
    } else {
        if (direction == -1) emit_machine_operation(MC_STD, 0);
        unrolled = 0;
        emit_machine_operation(MC_MOV, 0x104, &operand_edi, destination);
        destination = &operand_edi;
        physical_source = fill ? &operand_eax : &operand_esi;
        emit_machine_operation(MC_MOV, 0x104, physical_source, source);
        source = physical_source;
        emit_machine_operation(MC_MOV, 0x104, &operand_ecx, count);
        if (wide && !constant_count)
            emit_machine_operation(MC_SHR, 0x104, &operand_ecx,
                new_operand(OPERAND_IMMEDIATE, 0, (type & 255) == 1 ? 2 : 1, 0));
        emit_machine_operation(MC_REP, 0);
        emit_machine_operation(opcode, unit_type);
    }
    if (!wide) goto done;
    if ((type & 255) == 1) {
        if (!constant_count) {
            if (!remainder) goto done;
            emit_machine_operation(MC_MOV, 0x104, &operand_ecx, count);
            emit_machine_operation(MC_AND, 0x104, &operand_ecx,
                new_operand(OPERAND_IMMEDIATE, 0, 3, 0));
            emit_machine_operation(MC_REP, 0);
            emit_machine_operation(opcode, type);
        } else {
            if (constant_count & 2) {
                if (direct && unrolled) {
                    emit_explicit_block_transfer(fill, source, destination, 0x102, offset);
                    offset += 2;
                } else emit_machine_operation(opcode, 0x102);
            }
            if (constant_count & 1) {
                if (direct && unrolled)
                    emit_explicit_block_transfer(fill, source, destination, 0x101, offset);
                else emit_machine_operation(opcode, 0x101);
            }
        }
    } else if (!constant_count) {
        if (!remainder) goto done;
        /* The carry from count's earlier SHR identifies the residual word. */
        emit_machine_operation(MC_ADC, 0x104, &operand_ecx, &operand_ecx);
        emit_machine_operation(MC_REP, 0);
        emit_machine_operation(opcode, type);
    } else if (constant_count & 1) {
        if (direct && unrolled)
            emit_explicit_block_transfer(fill, source, destination, 0x102, offset);
        else emit_machine_operation(opcode, 0x102);
    }
done:
    if (direction == -1 && (!unrolled || !direct)) emit_machine_operation(MC_CLD, 0);
}

/* Original80238..80278: the complete intrinsic/ISA operation map, with
   the original unsigned -1 terminator. Other runtime operations use calls. */
const OpcodeMap runtime_intrinsic_opcodes[] = {
    { 0x6b, MC_FSIN }, { 0x6a, MC_FSIN },
    { 0x6f, MC_FCOS }, { 0x6e, MC_FCOS },
    { 0x5c, MC_FSQRT }, { 0x5d, MC_FSQRT },
    { 0x47, MC_FMULP }, { ~0U, 0 }
};
extern int optimize_global, optimize_precision, optimize_speed;
unsigned __fastcall lookup_runtime_intrinsic_opcode(const OpcodeMap *map, unsigned intrinsic);

/* Original [0001d060,0001d3ac), including the42-entry dispatch table at
   1d304. Inline x87 sequences retain their original operand counts and
   stack order. Unsupported operations evaluate all arguments then call
   the generated runtime symbol established during tree lowering. */
unsigned short __fastcall select_runtime_intrinsic(Node *node)
{
    unsigned intrinsic = node->left.node->left.value;
    unsigned short type = node->attributes.words.type;
    Node *argument, *arguments;
    MachineOperand *target;
    if (optimize_global && !optimize_precision) {
        switch (intrinsic) {
        case 0x5c: case 0x5d: case 0x6a: case 0x6b:
        case 0x6e: case 0x6f:
            select_value_register(node->second.node->left.node);
            emit_machine_operation(lookup_runtime_intrinsic_opcode(
                runtime_intrinsic_opcodes, intrinsic), type, &operand_st0, &operand_st0);
            return REG_ST0;
        case 0x60: case 0x61:
            if (!optimize_speed) break;
            select_value_register(node->second.node->left.node);
            emit_machine_operation(MC_FLDL2E, type);
            emit_machine_operation(MC_FMULP, type, &operand_st1, &operand_st0);
            emit_machine_operation(MC_FLD, type, &operand_st0, &operand_st0);
            emit_machine_operation(MC_FRNDINT, type);
            emit_machine_operation(MC_FXCH, type, &operand_st1);
            emit_machine_operation(MC_FSUB, type, &operand_st0, &operand_st1);
            emit_machine_operation(MC_F2XM1, type);
            emit_machine_operation(MC_FLD1, type);
            emit_machine_operation(MC_FADDP, type, &operand_st1, &operand_st0);
            emit_machine_operation(MC_FSCALE, type);
            emit_machine_operation(MC_FSTP, type, &operand_st1, &operand_st0);
            return REG_ST0;
        case 0x64: case 0x65: case 0x68: case 0x69:
            select_value_register(node->second.node->left.node);
            emit_machine_operation(intrinsic == 0x64 || intrinsic == 0x65 ?
                MC_FLDLN2 : MC_FLDLG2, type);
            emit_machine_operation(MC_FXCH, type, &operand_st1);
            emit_machine_operation(MC_FYL2X, type);
            return REG_ST0;
        case 0x72: case 0x73:
            select_value_register(node->second.node->left.node);
            emit_machine_operation(MC_FPTAN, type);
            emit_machine_operation(MC_FSTP, type, &operand_st0, &operand_st0);
            return REG_ST0;
        case 0x82: case 0x83:
            select_value_register(node->second.node->left.node);
            emit_machine_operation(MC_FLD1, type);
            emit_machine_operation(MC_FPATAN, type);
            return REG_ST0;
        case 0x84: case 0x85:
            select_value_register(node->second.node->left.node);
            select_value_register(node->second.node->second.node->left.node);
            arrange_x87_left(node->second.node->left.node);
            emit_machine_operation(MC_FPATAN, type);
            return REG_ST0;
        }
    }
    arguments = node->second.node;
    for (argument = arguments; argument->opcode != 0x4c;
         argument = argument->second.node)
        select_value_register(argument->left.node);
    arrange_x87_left(arguments->left.node);
    target = new_operand(OPERAND_SYMBOL, 0, 0, node->left.node->right.symbol);
    emit_call(node->left.node->attributes.words.type & 255, target, 0, 0, 0x4000);
    return REG_ST0;
}

/* Original [0001b240,0001b26b). This map uses -1 as its terminator;
   unlike the arithmetic opcode table there is no wildcard/default row. */
unsigned __fastcall lookup_runtime_intrinsic_opcode(const OpcodeMap *map,
    unsigned intrinsic)
{
    while (map->tree_opcode != ~0U) {
        if (map->tree_opcode == intrinsic) return map->selection;
        ++map;
    }
    compiler_internal_error("@(#)cgintrin.c:1.57", 0x57);
}

/* Original [0001ca50,0001cb51). Scan a zero-terminated byte string using
   EDI/ECX/EAX. Mode0 materializes its length in a new register; other modes
   retain the original address register and optionally leave the scan count
   complemented for a subsequent string operation. RET4 preserves type's
   third fastcall argument slot. */
unsigned short __fastcall select_intrinsic_string_length(Node *value, int mode,
    unsigned type)
{
    MachineOperand *source = operand_for_register(select_value_register(value));
    unsigned short result;
    emit_machine_operation(MC_MOV, 0x104, &operand_edi, source);
    emit_machine_operation(MC_MOV, 0x104, &operand_ecx,
        new_operand(OPERAND_IMMEDIATE, 0, ~0U, 0));
    emit_machine_operation(MC_SUB, 0x104, &operand_eax, &operand_eax);
    emit_machine_operation(MC_REPNZ, 0);
    emit_machine_operation(MC_SCAS, 0x101);
    if (mode != 2) emit_machine_operation(MC_NOT, 0x104, &operand_ecx);
    if (!mode) {
        emit_machine_operation(MC_ADD, 0x104, &operand_ecx, immediate_operand(~0U));
        result = new_value_register(0x100);
        emit_machine_operation(MC_MOV, (unsigned short)type,
            operand_for_register(result), &operand_ecx);
        return result;
    }
    return source->value.reg.words.number;
}

/* Original [0001cf00,0001d051). Shared zero-terminated string copy/fill
   setup. The scan count includes NUL for copying; filling subtracts that
   byte. Save the destination before REP changes its physical register. */
unsigned short __fastcall select_intrinsic_a3_a4(Node *node, int fill)
{
    Node *arguments = node->second.node;
    Node *scanned = fill ? arguments : arguments->second.node;
    MachineOperand *count, *source, *destination, *result;
    select_intrinsic_string_length(scanned->left.node, 1, 0);
    emit_machine_operation(MC_SUB, 0x104, &operand_edi, &operand_ecx);
    count = operand_for_register(new_value_register(0x100));
    if (!fill) {
        emit_machine_operation(MC_MOV, 0x104, count, &operand_ecx);
        source = operand_for_register(new_value_register(0x100));
        emit_machine_operation(MC_MOV, 0x104, source, &operand_edi);
        destination = operand_for_register(select_value_register(arguments->left.node));
    } else {
        emit_machine_operation(MC_ADD, 0x104, &operand_ecx, immediate_operand(~0U));
        emit_machine_operation(MC_MOV, 0x104, count, &operand_ecx);
        source = select_replicated_fill_operand(arguments->second.node->left.node, 0x101, 0);
        destination = &operand_edi;
    }
    result = operand_for_register(new_value_register(0x100));
    emit_machine_operation(MC_MOV, 0x104, result, destination);
    select_block_operation(0, count, source, destination, 1, 0x101, fill, 1, 1);
    return result->value.reg.words.number;
}

/* Original [0001ccb0,0001cef4). Compare two zero-terminated byte strings
   two characters per loop. Mismatches retain CMP's carry for the -1/+1
   result; equal NUL bytes take the zero-result path. Virtual addresses are
   retained under global optimization, otherwise EAX/ECX carry them. */
unsigned short __fastcall select_intrinsic_a5(Node *node)
{
    enum { STRING_CC_EQUAL = 4, STRING_CC_NOT_EQUAL = 5 };
    Node *arguments = node->second.node;
    MachineOperand *left, *right, *loop, *different, *end, *join;
    MachineOperand *part, *step, *result;
    unsigned short type;
    left = operand_for_register(select_value_register(arguments->left.node));
    right = operand_for_register(select_value_register(arguments->second.node->left.node));
    if (!optimize_global) {
        emit_machine_operation(MC_MOV, 0x104, &operand_eax, left);
        emit_machine_operation(MC_MOV, 0x104, &operand_ecx, right);
        left = &operand_eax;
        right = &operand_ecx;
    }
    loop = new_operand(OPERAND_SYMBOL, 0, 0, (SymbolHeader *)new_internal_label());
    emit_label(loop->value.symbol);
    different = new_operand(OPERAND_SYMBOL, 0, 0, (SymbolHeader *)new_internal_label());
    part = compare_intrinsic_string_part(left, right, 0, 0x101, different);
    emit_machine_operation(MC_OR, 0x101, part, part);
    end = new_operand(OPERAND_SYMBOL, 0, 0, (SymbolHeader *)new_internal_label());
    emit_machine_operation(MC_JCC, 0x104, end, STRING_CC_EQUAL);
    part = compare_intrinsic_string_part(left, right, 1, 0x101, different);
    step = new_operand(OPERAND_IMMEDIATE, 0, 2, 0);
    emit_machine_operation(MC_ADD, 0x104, left, step);
    emit_machine_operation(MC_ADD, 0x104, right, step);
    emit_machine_operation(MC_OR, 0x101, part, part);
    emit_machine_operation(MC_JCC, 0x104, loop, STRING_CC_NOT_EQUAL);
    emit_register_barrier(left, 0x104);
    emit_register_barrier(right, 0x104);
    emit_label(end->value.symbol);
    type = node->attributes.words.type;
    result = operand_for_register(new_value_register(type));
    emit_machine_operation(MC_MOV, type, result, immediate_operand(0));
    emit_register_barrier(result, 0x104);
    join = new_operand(OPERAND_SYMBOL, 0, 0, (SymbolHeader *)new_internal_label());
    emit_machine_operation(MC_JMP, 0x104, join);
    emit_label(different->value.symbol);
    emit_machine_operation(MC_SBB, node->attributes.words.type, result, result);
    emit_machine_operation(MC_SBB, node->attributes.words.type, result,
        new_operand(OPERAND_IMMEDIATE, 0, ~0U, 0));
    emit_label(join->value.symbol);
    return result->value.reg.words.number;
}

/* Original [0001db40,0001dbc2). Load the left string part, compare the
   right part at the same displacement, and optionally branch on mismatch.
   The caller receives the loaded register while CMP's flags stay live. */
MachineOperand *__fastcall compare_intrinsic_string_part(MachineOperand *left,
    MachineOperand *right, unsigned offset, unsigned type, MachineOperand *different)
{
    MachineOperand *value = operand_for_register(new_value_register(0x100));
    emit_machine_operation(MC_MOV, type, value,
        new_operand(OPERAND_BASED, left->value.reg.words.number, offset, 0));
    emit_machine_operation(MC_CMP, type, value,
        new_operand(OPERAND_BASED, right->value.reg.words.number, offset, 0));
    if (different) emit_machine_operation(MC_JCC, 0x104, different, 5);
    return value;
}
