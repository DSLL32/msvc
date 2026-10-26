#include "optimizer-sets.h"
#include <recon.h>

extern unsigned optimizer_index_limit, optimizer_state_c, optimizer_initial_register_count;
extern C2FunctionRegister *optimizer_memory_register, *optimizer_parameter_register;
extern C2FunctionRegister *optimizer_address_registers;
extern C2OptimizerAlias *optimizer_aliases;
extern C2OptimizerScopeState *optimizer_scope_states;
extern C2OptimizerIndexMap *optimizer_index_maps;
extern unsigned optimizer_exception_index;

C2OptimizerUse **optimizer_index_uses;     /* 48e2f0 */
C2RegisterSet **optimizer_index_sets;      /* 48e2f8 */
C2RegisterSet *optimizer_alias_registers;  /* 490b90 */

C2RegisterSet *__fastcall new_optimizer_dense_set(unsigned pool, unsigned count)
{
    C2RegisterSet *set;
    unsigned slot = C2_REGISTER_SET_HEADER_POOL(pool);
    set = free_register_sets[slot];
    if (set) free_register_sets[slot] = set->links.next_free;
    else set = c2_pool_allocate(pool, sizeof(C2RegisterSet));
    set->links.words = NULL;
    return initialize_optimizer_dense_set(set, pool, count);
}

C2RegisterSet *__fastcall initialize_optimizer_dense_set(C2RegisterSet *set,
    unsigned pool, unsigned count)
{
    set->links.words = NULL;
    set->state = (set->state & 0xff000000U) | (count & 0x00ffffffU);
    set->links.words = c2_pool_allocate(pool, ((count + 31) >> 5) * sizeof(unsigned));
    set->pool = (unsigned char)pool;
    return set;
}

void __fastcall add_optimizer_dense_member(C2RegisterSet *set, unsigned number)
{
    set->links.words[number >> 5] |= 1U << (number & 31);
}

void __fastcall merge_optimizer_dense_sets(C2RegisterSet *destination, C2RegisterSet *source)
{
    unsigned *target = destination->links.words, *values = source->links.words;
    unsigned words = ((source->state & 0x00ffffffU) + 31) >> 5;
    unsigned index;
    for (index = 0; index < words; ++index) target[index] |= values[index];
}

void __fastcall build_optimizer_register_sets(unsigned expression_limit,
    unsigned count, unsigned alias_count, unsigned register_count, int aliases)
{
    C2RegisterBlock *block;
    C2FunctionRegister *reg, *variant, *parameter;
    C2OptimizerUse *use;
    C2OptimizerCallState *call_state;
    C2OptimizerScopeState *scope_state;
    C2OptimizerIndexMap *mapping;
    C2RegisterSet *set, *part, *common = NULL;
    unsigned index, number, scope_number = 1;

    optimizer_index_uses = c2_pool_allocate(1, optimizer_index_limit * sizeof(C2OptimizerUse *));
    optimizer_index_sets = c2_pool_allocate(1, optimizer_index_limit * sizeof(C2RegisterSet *));
    optimizer_index_sets[1] = new_optimizer_dense_set(8, register_count);
    if (optimizer_state_c)
        optimizer_index_sets[optimizer_state_c] = new_optimizer_dense_set(8, register_count);
    set = new_optimizer_dense_set(8, register_count);
    for (use = optimizer_memory_register->optimizer_uses; use; use = use->next)
        add_optimizer_dense_member(set, use->reg->optimizer_index);
    add_optimizer_dense_member(set, 0);
    optimizer_memory_register->optimizer_set = set;
    if (aliases) optimizer_alias_registers = new_optimizer_dense_set(8, register_count);

    parameter = optimizer_parameter_register;
    for (index = expression_limit - 1; index > 1; --index) {
        optimizer_index_sets[index] = new_optimizer_dense_set(8, register_count);
        if (parameter->optimizer_uses) {
            optimizer_index_uses[index] = add_optimizer_use(NULL, parameter, 1);
            set = new_optimizer_dense_set(8, register_count);
            for (use = parameter->optimizer_uses; use; use = use->next)
                add_optimizer_dense_member(set, use->reg->optimizer_index);
            parameter->optimizer_set = set;
            merge_optimizer_dense_sets(optimizer_index_sets[index], set);
            parameter->optimizer_scope_number = scope_number++;
        }
        if (!aliases) {
            optimizer_index_uses[index] = add_optimizer_use(optimizer_index_uses[index], optimizer_memory_register, 1);
            merge_optimizer_dense_sets(optimizer_index_sets[index], optimizer_memory_register->optimizer_set);
        }
        parameter = parameter->previous_parameter;
    }

    for (block = first_function_register_block; block; block = block->next) {
        for (reg = block->records; reg < block->cursor; ++reg) {
            if (reg->kind == 16) continue;
            if (reg->canonical == reg &&
                (reg->kind == 4 || reg->kind == 5 || reg->kind == 7 || reg->kind == 8)) {
                index = ((C2InputSymbol *)reg->symbol)->detail.optimizer_number;
                optimizer_index_uses[index] = add_optimizer_use(NULL, reg, 1);
                if (!optimizer_index_sets[index])
                    optimizer_index_sets[index] = new_optimizer_dense_set(8, register_count);
                add_optimizer_dense_member(optimizer_index_sets[index], reg->optimizer_index);
            }
            if (reg->view_flags & 8) {
                build_optimizer_register_state(reg, alias_count, register_count, aliases);
                for (variant = reg->optimizer->owner; variant; variant = variant->variant_next)
                    if (variant->optimizer) prepare_optimizer_register_aliases(variant, register_count);
            } else if (reg->canonical == reg && aliases && reg->optimizer_index)
                prepare_optimizer_register_aliases(reg, register_count);
        }
    }
    for (reg = optimizer_address_registers; reg; reg = reg->previous_parameter) {
        build_optimizer_register_state(reg, alias_count, register_count, aliases);
        if (reg->optimizer_index) {
            index = reg->optimizer->values->number;
            for (use = optimizer_index_uses[reg->optimizer_index]; use; use = use->next)
                optimizer_index_uses[index] = add_optimizer_use(optimizer_index_uses[index], use->reg, 1);
            merge_optimizer_dense_sets(optimizer_index_sets[index], optimizer_index_sets[reg->optimizer_index]);
        }
    }

    if (aliases) {
        common = new_optimizer_dense_set(8, register_count);
        add_optimizer_dense_member(common, 0);
        for (number = optimizer_initial_register_count; number < register_count; ++number)
            add_optimizer_dense_member(common, number);
        subtract_optimizer_dense_sets(common, optimizer_alias_registers);
        release_optimizer_set_header(optimizer_alias_registers);
        for (index = expression_limit - 1; index > 1; --index)
            merge_optimizer_dense_sets(optimizer_index_sets[index], common);
        optimizer_index_uses[2] = add_optimizer_use(optimizer_index_uses[2], optimizer_memory_register, 1);
        merge_optimizer_dense_sets(optimizer_index_sets[2], optimizer_memory_register->optimizer_set);
    }
    for (call_state = optimizer_call_states; call_state; call_state = call_state->next) {
        index = call_state->number;
        optimizer_index_uses[index] = add_optimizer_use(NULL, optimizer_memory_register, 1);
        set = new_optimizer_dense_set(8, register_count);
        optimizer_index_sets[index] = set;
        parameter = call_state->parameter;
        if (parameter->optimizer_uses) {
            optimizer_index_uses[index] = add_optimizer_use(optimizer_index_uses[index], parameter, 1);
            merge_optimizer_dense_sets(set, parameter->optimizer_set);
        }
        add_optimizer_dense_member(set, 0);
        for (use = call_state->additional_uses; use; use = use->next)
            add_optimizer_dense_member(set, use->reg->optimizer_index);
        for (number = next_optimizer_symbol(call_state->registers); number != ~0U;
             number = next_optimizer_symbol(NULL)) {
            if (!optimizer_aliases[number].effects) {
                part = new_optimizer_dense_set(6, register_count);
                optimizer_aliases[number].effects = part;
                fill_optimizer_dense_uses(optimizer_aliases[number].uses, part, register_count);
            }
            merge_optimizer_dense_sets(set, optimizer_aliases[number].effects);
        }
        merge_optimizer_dense_sets(set, optimizer_memory_register->optimizer_set);
        release_optimizer_symbol_set(call_state->registers);
    }

    for (scope_state = optimizer_scope_states; scope_state; scope_state = scope_state->next) {
        index = scope_state->input_index;
        if (index) {
            set = new_optimizer_dense_set(8, register_count);
            optimizer_index_sets[index] = set;
            for (use = scope_state->inputs; use; use = use->next) {
                reg = use->reg;
                add_optimizer_dense_member(set, reg->optimizer_index);
                if (reg->optimizer)
                    for (variant = reg->optimizer->owner; variant; variant = variant->variant_next)
                        add_optimizer_dense_member(set, variant->optimizer_index);
            }
            parameter = scope_state->parameter;
            if (parameter->optimizer_uses) {
                scope_state->inputs = add_optimizer_use(scope_state->inputs, parameter, 1);
                if (parameter->optimizer_set) merge_optimizer_dense_sets(set, parameter->optimizer_set);
            }
            scope_state->inputs = add_optimizer_use(scope_state->inputs, optimizer_memory_register, 1);
            part = optimizer_memory_register->optimizer_set;
            if (!part) part = optimizer_index_sets[2];
            merge_optimizer_dense_sets(set, part);
            if (common) merge_optimizer_dense_sets(set, common);
            optimizer_index_uses[index] = scope_state->inputs;
        }
        index = scope_state->output_index;
        if (index) {
            set = new_optimizer_dense_set(8, register_count);
            optimizer_index_sets[index] = set;
            for (use = scope_state->outputs; use; use = use->next) {
                reg = use->reg;
                if (!(reg->view_flags & 2))
                    for (variant = reg; variant; variant = variant->variant_next)
                        variant->view_flags |= 2;
                add_optimizer_dense_member(set, reg->optimizer_index);
                if (reg->optimizer)
                    for (variant = reg->optimizer->owner; variant; variant = variant->variant_next)
                        add_optimizer_dense_member(set, variant->optimizer_index);
            }
            parameter = scope_state->parameter;
            if (parameter->optimizer_uses) {
                scope_state->outputs = add_optimizer_use(scope_state->outputs, parameter, 1);
                if (parameter->optimizer_set) merge_optimizer_dense_sets(set, parameter->optimizer_set);
            }
            scope_state->outputs = add_optimizer_use(scope_state->outputs, optimizer_memory_register, 1);
            part = optimizer_memory_register->optimizer_set;
            if (!part) part = optimizer_index_sets[2];
            merge_optimizer_dense_sets(set, part);
            if (common) merge_optimizer_dense_sets(set, common);
            optimizer_index_uses[index] = scope_state->outputs;
        }
    }
    optimizer_scope_states = NULL;

    while ((mapping = optimizer_index_maps) != NULL) {
        index = mapping->replacement;
        optimizer_index_sets[index] = new_optimizer_dense_set(8, register_count);
        for (use = optimizer_index_uses[mapping->original]; use; use = use->next) {
            reg = use->reg;
            if (reg->kind != 14 && reg->kind != 12) {
                optimizer_index_uses[index] = add_optimizer_use(optimizer_index_uses[index], reg, 1);
                add_optimizer_dense_member(optimizer_index_sets[index], reg->optimizer_index);
            }
        }
        optimizer_index_maps = mapping->next;
    }
    if (optimizer_exception_index) {
        index = optimizer_exception_index;
        set = new_optimizer_dense_set(8, register_count);
        optimizer_index_sets[index] = set;
        for (block = first_function_register_block; block; block = block->next) {
            for (reg = block->records; reg < block->cursor; ++reg) {
                if (reg->kind != 16 && reg->canonical == reg && (reg->kind == 4 || reg->kind == 5)) {
                    optimizer_index_uses[index] = add_optimizer_use(optimizer_index_uses[index], reg, 1);
                    add_optimizer_dense_member(set, reg->optimizer_index);
                    if (!(reg->view_flags & 2))
                        for (variant = reg; variant; variant = variant->variant_next)
                            variant->view_flags |= 2;
                }
            }
        }
        optimizer_index_uses[index] = add_optimizer_use(optimizer_index_uses[index], optimizer_memory_register, 1);
        merge_optimizer_dense_sets(set, optimizer_memory_register->optimizer_set);
        optimizer_exception_index = 0;
    }
    if (common) release_optimizer_set_header(common);
}

void __fastcall build_optimizer_register_state(C2FunctionRegister *reg,
    unsigned alias_count, unsigned register_count, int aliases)
{
    C2OptimizerRegisterInfo *info = reg->optimizer;
    C2OptimizerValue *value;
    C2OptimizerUse *uses, *use, *parameter_use;
    C2FunctionRegister *used, *variant;
    unsigned index, number, symbol_index, value_bytes;
    long displacement;
    int keep_aliases = aliases;

    release_optimizer_symbol_set(info->aliases);
    info->aliases = NULL;
    for (value = info->values; value; value = value->next) {
        index = value->number;
        if (!optimizer_index_uses[index]) {
            /* Original defect: these fresh locals are assigned only with
             * alias mode enabled, but its kind14 branch can read them without
             * that assignment. Do not invent zero values or hidden arguments.
             * The native unsupported-domain check precedes either local read. */
            if (aliases) {
                value_bytes = value->value_bytes;
                displacement = value->reg->displacement;
            }
            optimizer_index_sets[index] = new_optimizer_dense_set(8, register_count);
            uses = NULL;
            for (number = next_optimizer_symbol(value->effects); number != ~0U;
                 number = next_optimizer_symbol(NULL)) {
                if ((int)number < (int)alias_count && !aliases)
                    uses = add_optimizer_use(uses, optimizer_memory_register, 1);
                else {
                    for (use = optimizer_aliases[number].uses; use; use = use->next) {
                        used = use->reg;
                        if (used->kind == 12 && used != optimizer_memory_register && !used->optimizer_uses)
                            keep_aliases = 0;
                        else if (used->kind == 14) {
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
                            if (!aliases)
                                recon_unsupported("optimizer kind14 range without initialized alias locals");
#endif
                            if (displacement > 0)
                                used = find_optimizer_register_variant(used,
                                    (long)((unsigned)used->displacement + (unsigned)displacement));
                            add_optimizer_register_range(used, optimizer_index_sets[index],
                                displacement, value_bytes);
                            if (used != value->reg) keep_aliases = 0;
                        } else
                            uses = add_optimizer_use(uses, used, 1);
                    }
                }
                if ((int)number < (int)alias_count || optimizer_aliases[number].pending10)
                    keep_aliases = 0;
            }
            optimizer_index_uses[index] = uses;
            for (use = uses; use; use = use->next) {
                used = use->reg;
                if (used->kind == 12) {
                    if (!aliases)
                        merge_optimizer_dense_sets(optimizer_index_sets[index], used->optimizer_set);
                    else {
                        for (parameter_use = used->optimizer_uses; parameter_use;
                             parameter_use = parameter_use->next) {
                            symbol_index = ((C2InputSymbol *)parameter_use->reg->symbol)->detail.optimizer_number;
                            if (!optimizer_index_sets[symbol_index])
                                optimizer_index_sets[symbol_index] = new_optimizer_dense_set(8, register_count);
                            for (variant = info->owner; variant; variant = variant->variant_next)
                                add_optimizer_dense_member(optimizer_index_sets[symbol_index], variant->optimizer_index);
                        }
                    }
                    keep_aliases = 0;
                } else {
                    add_optimizer_dense_member(optimizer_index_sets[index], used->optimizer_index);
                    if (used->view_flags & 4) keep_aliases = 0;
                }
            }
        }
        release_optimizer_symbol_set(value->effects);
        value->effects = NULL;
    }
    if (keep_aliases)
        for (variant = info->owner; variant; variant = variant->variant_next)
            add_optimizer_dense_member(optimizer_alias_registers, variant->optimizer_index);
}

int __fastcall optimizer_dense_sets_equal(C2RegisterSet *first, C2RegisterSet *second)
{
    unsigned count = first->state & 0x00ffffffU;
    unsigned words, *left, *right;
    if ((first->state ^ second->state) & 0x00ffffffU) return 0;
    left = first->links.words;
    right = second->links.words;
    words = (count + 31) >> 5;
    while (words--) {
        if (*left++ != *right++) return 0;
    }
    return 1;
}

void __fastcall fill_optimizer_dense_set(C2RegisterSet *set, unsigned count)
{
    unsigned full = ((count + 31) >> 5) - 1;
    unsigned *words = set->links.words, i, remaining;
    for (i = 0; i < full; ++i) *words++ = ~0U;
    remaining = count - (full << 5);
    if (remaining == 32) *words = ~0U;
    else for (i = 0; i < remaining; ++i) *words |= 1U << (i & 31);
}

void __fastcall intersect_optimizer_dense_sets(C2RegisterSet *destination, C2RegisterSet *source)
{
    unsigned *target = destination->links.words, *values = source->links.words;
    unsigned count = ((source->state & 0x00ffffffU) + 31) >> 5;
    while (count--) *target++ &= *values++;
}

void __fastcall copy_optimizer_dense_set(C2RegisterSet *destination, C2RegisterSet *source)
{
    unsigned *target = destination->links.words, *values = source->links.words;
    unsigned count = ((source->state & 0x00ffffffU) + 31) >> 5;
    if (count) memcpy(target, values, count * sizeof(*target));
}

int __fastcall optimizer_dense_contains(C2RegisterSet *set, unsigned number)
{
    return (set->links.words[number >> 5] & (1U << (number & 31))) != 0;
}
