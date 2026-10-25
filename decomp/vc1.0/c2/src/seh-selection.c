/* Instruction selection for i386 structured-exception records. */
#include "seh-selection.h"
#include "intrinsic-selection.h"
#include "records.h"
#include "bitsets.h"
#include <recon.h>
extern unsigned code_pointer_bytes;
BackendSymbol *__fastcall new_generated_storage(unsigned kind, unsigned bytes,
    unsigned short storage, int local, unsigned char alignment, unsigned short flags);
SymbolHeader *__fastcall require_external_symbol(char *name);

BackendSymbol *seh_finally_stack; /* 0007e1b0: optional saved finally stack */
BackendSymbol *seh_registration; /* 0007e1b4: 24-byte registration record */
BackendSymbol *seh_saved_stack; /* 0007e1b8: initial stack pointer */
BackendSymbol *seh_scope_table; /* 0007cb00: three DWORDs per scope */

/* Original [0001bc00,0001c424), switch data [0001c424,0001c49c).
   The ordinary result26 denotes no value register. Runtime intrinsics and
   scope annotations update the registration record using distinct owners. */
unsigned short __fastcall select_assembly_record(Node *node)
{
    MachineOperand *operand, *temporary, *address;
    ExceptionScope *scope;
    unsigned count, needs_finally_stack, intrinsic, bytes;
    short code;
    switch (node->opcode) {
    case 0x40: case 0x7a:
        intrinsic = node->left.node->left.value;
        switch (intrinsic) {
        case 0xd7:
            operand = new_operand(OPERAND_SYMBOL, 0, 0,
                require_external_symbol("__abnormal_termination"));
            emit_call(code_pointer_bytes, operand, 0, 0, 0);
            return REG_EAX;
        case 0xd8: case 0xdb:
            if (!seh_saved_stack)
                compiler_internal_error("@(#)cgintrin.c:1.57", 0x409);
            operand = frame_temporary_operand(&seh_saved_stack->symbol, 0);
            emit_machine_operation(MC_MOV, 0x104, operand, &operand_esp);
            break;
        case 0xd9:
            if (!seh_finally_stack)
                compiler_internal_error("@(#)cgintrin.c:1.57", 0x3fb);
            operand = frame_temporary_operand(&seh_finally_stack->symbol, 0);
            temporary = operand_for_register(new_value_register(0x104));
            emit_machine_operation(MC_MOV, 0x104, temporary, operand);
            return temporary->value.reg.words.number;
        case 0xda:
            operand = frame_temporary_operand(&seh_registration->symbol, 20);
            temporary = operand_for_register(new_value_register(0x104));
            emit_machine_operation(MC_MOV, 0x104, temporary, operand);
            return temporary->value.reg.words.number;
        case 0xf1:
            emit_machine_operation(MC_EFWAIT, 0);
            temporary = operand_for_register(select_value_register(node->second.node->left.node));
            emit_machine_operation(MC_PUSH, 0x104, temporary);
            if (!seh_registration)
                compiler_internal_error("@(#)cgintrin.c:1.57", 0x3e3);
            operand = frame_temporary_operand(&seh_registration->symbol, 0);
            temporary = operand_for_register(new_value_register(0x104));
            emit_machine_operation(MC_LEA, 0x104, temporary, operand);
            emit_machine_operation(MC_PUSH, 0x104, temporary);
            operand = new_operand(OPERAND_SYMBOL, 0, 0,
                require_external_symbol("__local_unwind2"));
            emit_call(code_pointer_bytes, operand, 0, 0, 0);
            emit_machine_operation(MC_ADD, 0x104, &operand_esp, immediate_operand(8));
            break;
        case 0xf2:
            bytes = node->second.node->left.node->left.value;
            if (!bytes) {
                operand = frame_temporary_operand(&seh_saved_stack->symbol, 0);
                emit_machine_operation(MC_MOV, 0x104, &operand_esp, operand);
            } else {
                temporary = operand_for_register(new_value_register(0x104));
                operand = frame_temporary_operand(&seh_saved_stack->symbol, 0);
                emit_machine_operation(MC_MOV, 0x104, temporary, operand);
                emit_machine_operation(MC_SUB, 0x104, temporary, immediate_operand(bytes));
                emit_machine_operation(MC_MOV, 0x104, &operand_esp, temporary);
            }
            break;
        case 0xf4:
            emit_machine_operation(MC_EFWAIT, 0);
            temporary = immediate_operand(node->second.node->left.node->left.value);
            operand = frame_temporary_operand(&seh_registration->symbol, 12);
            emit_machine_operation(MC_MOV, 0x104, operand, temporary);
            break;
        }
        break;
    case 0x46:
        seh_saved_stack = 0;
        seh_registration = 0;
        seh_finally_stack = 0;
        count = needs_finally_stack = 0;
        for (scope = current_function->exception_scopes; scope; scope = scope->next) {
            ++count;
            if (scope->flags & 1) needs_finally_stack = 1;
        }
        seh_scope_table = new_generated_storage(6, count * 12, 4, 1, 4, 0x80);
        queue_symbol_debug(&seh_scope_table->symbol);
        initialize_exception_scope_table();
        seh_saved_stack = new_generated_storage(3, 4, 1, 1, 4, 0);
        operand = frame_temporary_operand(&seh_saved_stack->symbol, 0);
        emit_machine_operation(MC_MOV, 0x104, operand, &operand_esp);
        seh_registration = new_generated_storage(3, 24, 1, 1, 4, 0);
        if (needs_finally_stack == 1)
            seh_finally_stack = new_generated_storage(3, 4, 1, 1, 4, 0);
        operand = frame_temporary_operand(&seh_registration->symbol, 16);
        emit_machine_operation(MC_MOV, 0x104, operand, &operand_ebp);
        operand = frame_temporary_operand(&seh_registration->symbol, 12);
        emit_machine_operation(MC_MOV, 0x104, operand, immediate_operand(~0U));
        operand = frame_temporary_operand(&seh_registration->symbol, 8);
        address = new_operand(OPERAND_LABEL_ADDRESS, 0, 0, &seh_scope_table->symbol);
        emit_machine_operation(MC_MOV, 0x104, operand, address);
        operand = frame_temporary_operand(&seh_registration->symbol, 4);
        address = new_operand(OPERAND_LABEL_ADDRESS, 0, 0,
            require_external_symbol("__except_handler2"));
        emit_machine_operation(MC_MOV, 0x104, operand, address);
        address = new_operand(OPERAND_ABSOLUTE, 0, 0, lookup_external_name("__except_list"));
        address->flags = (address->flags & ~0x30U) | 0xc0;
        temporary = operand_for_register(new_value_register(0x104));
        emit_machine_operation(MC_MOV, 0x104, temporary, address);
        operand = frame_temporary_operand(&seh_registration->symbol, 0);
        emit_machine_operation(MC_MOV, 0x104, operand, temporary);
        operand = frame_temporary_operand(&seh_registration->symbol, 0);
        emit_machine_operation(MC_LEA, 0x104, temporary, operand);
        emit_machine_operation(MC_MOV, 0x104, address, temporary);
        break;
    case 0x47:
        if (!seh_registration)
            compiler_internal_error("@(#)cgintrin.c:1.57", 0x412);
        temporary = operand_for_register(new_value_register(0x104));
        operand = frame_temporary_operand(&seh_registration->symbol, 0);
        emit_machine_operation(MC_MOV, 0x104, temporary, operand);
        address = new_operand(OPERAND_ABSOLUTE, 0, 0, lookup_external_name("__except_list"));
        address->flags = (address->flags & ~0x30U) | 0xc0;
        emit_machine_operation(MC_MOV, 0x104, address, temporary);
        seh_finally_stack = 0;
        seh_saved_stack = 0;
        seh_registration = 0;
        break;
    case 0x4f:
        code = node->detail.control.code;
        scope = node->left.pointer;
        switch (code) {
        case 0x26:
            emit_machine_operation(MC_EFWAIT, 0);
            temporary = immediate_operand(scope->identifier);
            operand = frame_temporary_operand(&seh_registration->symbol, 12);
            emit_machine_operation(MC_MOV, 0x104, operand, temporary);
            break;
        case 0x27: case 0x2b:
            if (code == 0x27) emit_machine_operation(MC_EFWAIT, 0);
            temporary = immediate_operand((unsigned)scope->parent_identifier);
            operand = frame_temporary_operand(&seh_registration->symbol, 12);
            emit_machine_operation(MC_MOV, 0x104, operand, temporary);
            if (code == 0x27 && scope->kind == 2) {
                if (scope->alternate_label)
                    operand = new_operand(OPERAND_LABEL_ADDRESS, 0, 0, scope->alternate_label);
                else operand = immediate_operand(0);
                emit_machine_operation(MC_PUSH, 0x104, operand);
            }
            break;
        case 0x28:
            if (!(scope->flags & 1)) break;
            if (!seh_finally_stack)
                compiler_internal_error("@(#)cgintrin.c:1.57", 0x3b7);
            temporary = operand_for_register(new_value_register(0x104));
            operand = frame_temporary_operand(&seh_registration->symbol, 20);
            emit_machine_operation(MC_MOV, 0x104, temporary, operand);
            address = new_operand(OPERAND_BASED, temporary->value.reg.words.number, 0, 0);
            emit_machine_operation(MC_MOV, 0x104, temporary, address);
            emit_machine_operation(MC_MOV, 0x104, temporary, address);
            operand = frame_temporary_operand(&seh_finally_stack->symbol, 0);
            emit_machine_operation(MC_MOV, 0x104, operand, temporary);
            break;
        case 0x2d:
            emit_machine_operation(MC_RET, 4, immediate_operand(0));
            break;
        }
        break;
    case 0x8d:
        temporary = operand_for_register(select_value_register(node->left.node));
        emit_machine_operation(MC_MOV, 0x104, &operand_eax, temporary);
        emit_machine_operation(MC_RET, 4, immediate_operand(0));
        selected_block->definitions = bitset_include(selected_block->definitions, REG_EAX);
        break;
    }
    return 0x1a;
}
