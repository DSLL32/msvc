#include "function-records.h"
#include "code-records.h"
#include "optimizer-translate.h"
#include <recon.h>

C2RegisterBlock **function_register_blocks;
C2RegisterBlock **next_function_register_block, **end_function_register_blocks;
C2RegisterBlock *first_function_register_block, *last_function_register_block;
unsigned next_function_register_number;
C2RegisterBlock *tracked_register_block, *secondary_register_block;
C2RegisterBlock *plain_register_block, *default_register_block, *extra_register_block;
C2FunctionRegister *free_plain_register_nodes, *free_tracked_register_nodes;
C2RegisterSet *secondary_register_set, *tracked_register_set;
C2FunctionRegister *empty_function_register;

void initialize_function_records(void)
{
    function_register_blocks = c2_pool_allocate(1, 1000 * sizeof(C2RegisterBlock *));
    next_function_register_block = function_register_blocks;
    end_function_register_blocks = function_register_blocks + 1000;
    next_function_register_number = 0;
    first_function_register_block = NULL;
    last_function_register_block = NULL;
    tracked_register_block = new_register_block();
    secondary_register_block = new_register_block();
    plain_register_block = new_register_block();
    default_register_block = new_register_block();
    extra_register_block = NULL;
    free_plain_register_nodes = NULL;
    free_tracked_register_nodes = NULL;
    pending_debug_symbols = NULL;
    secondary_register_set = new_register_set(1);
    tracked_register_set = new_register_set(1);
    empty_function_register = new_function_register(4);
    empty_function_register->kind = 0x10;
    clear_register_set(tracked_register_set);
}

C2RegisterBlock *new_register_block(void)
{
    C2RegisterBlock *block;
    C2FunctionRegister *reg;
    C2RegisterBlock **old_directory;
    int capacity;

    block = c2_pool_allocate(1, sizeof(C2RegisterBlock));
    block->next = NULL;
    block->cursor = block->records;
    block->end = block->records + 32;
    for (reg = block->records; reg < block->end; ++reg)
        reg->number = next_function_register_number++;
    if (next_function_register_block == end_function_register_blocks) {
        capacity = end_function_register_blocks - function_register_blocks;
        old_directory = function_register_blocks;
        function_register_blocks = c2_pool_allocate(1, 2 * capacity * sizeof(C2RegisterBlock *));
        memcpy(function_register_blocks, old_directory,
               capacity * sizeof(C2RegisterBlock *));
        next_function_register_block = function_register_blocks + capacity;
        end_function_register_blocks = function_register_blocks + 2 * capacity;
    }
    if (first_function_register_block)
        last_function_register_block->next = block;
    else
        first_function_register_block = block;
    *next_function_register_block++ = block;
    last_function_register_block = block;
    return block;
}

C2FunctionRegister *__fastcall new_function_register(unsigned char kind)
{
    C2FunctionRegister *reg;
    C2RegisterBlock *block;
    unsigned number;

    switch (kind) {
    case 3:
    case 6:
        if (free_plain_register_nodes) {
            reg = free_plain_register_nodes;
            free_plain_register_nodes = reg->next_free;
            number = reg->number;
            memset(reg, 0, sizeof(C2FunctionRegister));
            reg->number = number;
            return reg;
        }
        block = plain_register_block;
        if (block->cursor >= block->end)
            plain_register_block = block = new_register_block();
        reg = block->cursor++;
        return reg;
    case 4:
    case 5:
        if (free_tracked_register_nodes) {
            reg = free_tracked_register_nodes;
            free_tracked_register_nodes = reg->next_free;
            number = reg->number;
            memset(reg, 0, sizeof(C2FunctionRegister));
            reg->number = number;
        } else {
            block = tracked_register_block;
            if (block->cursor >= block->end)
                tracked_register_block = block = new_register_block();
            reg = block->cursor++;
        }
        add_register_to_set(tracked_register_set, reg->number);
        return reg;
    case 7:
    case 8:
        block = secondary_register_block;
        if (block->cursor >= block->end)
            secondary_register_block = block = new_register_block();
        reg = block->cursor++;
        add_register_to_set(secondary_register_set, reg->number);
        return reg;
    case 15:
        block = extra_register_block;
        if (!block || block->cursor >= block->end)
            extra_register_block = block = new_register_block();
        reg = block->cursor++;
        return reg;
    default:
        block = default_register_block;
        if (block->cursor >= block->end)
            default_register_block = block = new_register_block();
        reg = block->cursor++;
        return reg;
    }
}

C2FunctionRegister *__fastcall new_address_register(C2FunctionRegister *source,
                                      unsigned short type, unsigned bytes, long displacement)
{
    C2FunctionRegister *root = source->canonical, *view, *widest, *reg, *next;
    C2Expression *value = source->value;
    C2InputSymbol *owner;
    unsigned largest, extent;
    long offset = (long)((unsigned)source->displacement + (unsigned)displacement);
    long end;
    unsigned char kind;

    if (type & 0xfff) bytes = type & 0xfff;
    if (root->kind == 1)
        return new_bitfield_register(root, bytes << 3, (long)((unsigned)offset << 3));
    if (bytes) {
        for (view = root; view; view = view->variant_next) {
            if (register_types_match(view->type, type) &&
                view->displacement == offset && view->value_bytes == bytes)
                return view;
        }
        owner = (C2InputSymbol *)root->canonical->symbol;
        if (owner && owner->symbol.kind == 1 && (owner->attributes & 0x70) == 0x10 &&
            !owner->symbol.name && (owner->attributes & 0x80000) &&
            root->displacement == offset && type == 0x3004)
            return root;
    } else {
        largest = 0;
        widest = NULL;
        for (view = root; view; view = view->variant_next) {
            if (view->displacement == offset && largest <= view->value_bytes &&
                (view->type & 0xf000) == 0x5000) {
                widest = view;
                largest = view->value_bytes;
            }
        }
        if (widest) return widest;
        end = offset;
        for (view = root; view; view = view->variant_next) {
            long view_end = (long)((unsigned)view->displacement + view->value_bytes);
            if (end < view_end) end = view_end;
        }
        bytes = (unsigned)end - (unsigned)offset;
        type = (unsigned short)(bytes < 0x1000 ? bytes | 0x5000 : 0x5000);
    }
    kind = root->kind;
    reg = new_function_register(kind);
    reg->type = type;
    reg->displacement = offset;
    reg->value_bytes = bytes;
    reg->kind = kind;
    reg->canonical = root;
    reg->view_flags = (reg->view_flags & ~7) | (root->view_flags & 7);
    reg->value = value;
    if ((offset < root->displacement ||
         (long)((unsigned)root->displacement + root->value_bytes) <
         (long)((unsigned)offset + bytes)) && root->canonical->symbol) {
        extent = bytes + (unsigned)offset - (unsigned)root->displacement;
        if ((kind != 3 && kind != 4 && kind != 5 && kind != 6) ||
            (long)(extent - (unsigned)root->symbol->value) <= 4) {
            if (root->type & 0xfff) {
                if (offset) {
                    type = (unsigned short)((long)extent < 0x1000 ? extent | 0x5000 : 0x5000);
                    view = new_address_register(root, type, extent, 0);
                    reg->canonical = view;
                    reg->variant_next = view->variant_next;
                    view->variant_next = reg;
                    return reg;
                }
                replace_register_uses(root, reg);
                reg->canonical = reg;
                reg->symbol = root->symbol;
                reg->variant_next = NULL;
                reg->symbol->input_register = reg;
                root->symbol = NULL;
                for (view = root; view; view = next) {
                    next = view->variant_next;
                    view->variant_next = reg->variant_next;
                    view->canonical = reg;
                    reg->variant_next = view;
                }
                return reg;
            }
            root->value_bytes = extent;
        }
    }
    if (kind == 3 && (long)root->value_bytes < (long)reg->value_bytes && !offset) {
        reg->canonical = reg;
        reg->variant_next = NULL;
        for (view = root; view; view = next) {
            next = view->variant_next;
            view->variant_next = reg->variant_next;
            view->canonical = reg;
            reg->variant_next = view;
        }
    } else {
        reg->canonical = root;
        reg->variant_next = root->variant_next;
        root->variant_next = reg;
    }
    return reg;
}

int __fastcall register_types_match(unsigned short first, unsigned short second)
{
    unsigned first_kind = first >> 12, second_kind = second >> 12;
    if (first == second) return 1;
    if (first_kind != 1 && first_kind != 2 && first_kind != 3 && first_kind != 5)
        return 0;
    if (second_kind != 1 && second_kind != 2 && second_kind != 3 && second_kind != 5)
        return 0;
    return (first & 0xfff) == (second & 0xfff);
}

C2FunctionRegister *__fastcall new_temporary_register(unsigned short type)
{
    C2FunctionRegister *reg = new_function_register(3);
    reg->type = type;
    reg->kind = 3;
    reg->value_bytes = type & 0xfff;
    reg->view_flags |= 1;
    reg->canonical = reg;
    return reg;
}

void __fastcall release_function_register(C2FunctionRegister *reg)
{
    C2FunctionRegister *last;
    if (reg->kind == 3) {
        last = reg;
        while (reg) {
            /* Every variant receives the old head. The original publishes only
             * the last variant; do not silently turn this into a new chain. */
            reg->next_free = free_plain_register_nodes;
            reg->kind = 0x10;
            last = reg;
            reg = reg->variant_next;
        }
        free_plain_register_nodes = last;
    } else {
        while (reg) {
            remove_optimizer_symbol(tracked_register_set, reg->number);
            reg->kind = 0x10;
            reg->next_free = free_tracked_register_nodes;
            free_tracked_register_nodes = reg;
            reg = reg->variant_next;
        }
    }
}

void __fastcall mark_register_references(C2Expression *records);

unsigned __fastcall release_unreferenced_temporaries(C2Function *function)
{
    C2RegisterBlock *block;
    C2FunctionRegister *reg, *variant;
    unsigned released = 0;
    int used;
    mark_register_references((C2Expression *)function->flow->last->records);
    for (block = first_function_register_block; block; block = block->next) {
        for (reg = block->records; reg < block->cursor; ++reg) {
            if (reg->kind == 0x10 || reg->kind != 3 || reg->canonical != reg) continue;
            used = 0;
            for (variant = reg; variant; variant = variant->variant_next) {
                if (variant->flags & 2) {
                    variant->flags &= ~2;
                    used = 1;
                }
            }
            if (!used) {
                ++released;
                release_function_register(reg);
            }
        }
    }
    return released;
}

void __fastcall mark_register_references(C2Expression *records)
{
    C2FunctionRegister *reg, *original;
    C2OptimizerHandler *handler;
    C2OptimizerScope *scope;
    for (; records; records = records->next) {
        switch (records->kind) {
        case 1:
            reg = records->value.reg;
            reg->flags |= 2;
            original = records->original_register;
            if (original != reg && original->kind != 2) original->flags |= 2;
            break;
        case 2:
        case 3:
            records->value.reg->flags |= 2;
            break;
        case 19:
            if (records->opcode == 0x16d) {
                handler = records->detail.data;
                scope = handler->scope;
                if (scope && scope->handler_function)
                    mark_register_references((C2Expression *)scope->handler_function->flow->last->records);
            }
            goto children;
        case 20:
            mark_register_references(records->detail.data);
            /* The embedded list is additional to the ordinary operands. */
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
children:
            if (records->left) mark_register_references(records->left);
            if (records->tail.right) mark_register_references(records->tail.right);
            break;
        }
    }
}

int __fastcall compare_function_registers(C2FunctionRegister *left,
    C2FunctionRegister *right)
{
    unsigned first, second;

    if (left == right) return 0;
    if (!left) return -1;
    if (!right) return 1;
    if (left->kind == 3) {
        if (right->kind != 3) return -1;
        return left->number < right->number ? -1 : 1;
    }
    if (right->kind == 3) return 1;
    if (left->kind == 6 || right->kind == 6) return -1;
    if (left->displacement != right->displacement)
        return left->displacement < right->displacement ? -1 : 1;
    if (left->value_bytes != right->value_bytes)
        return (long)left->value_bytes < (long)right->value_bytes ? -1 : 1;
    if (!register_types_match(left->type, right->type))
        return left->type < right->type ? -1 : 1;
    first = left->canonical->symbol->serial;
    second = right->canonical->symbol->serial;
    if (first == second) return 0;
    return first < second ? -1 : 1;
}
