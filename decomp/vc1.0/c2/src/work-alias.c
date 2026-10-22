/* Alias/effect rules for available global-work values. */
#include "work-optimizer.h"
#include "tree-input.h"
#include "tree-rewrite.h"
extern int optimize_alias, optimize_weak_inverse;
extern void *output_state; /* existing unknown-base descriptor, original 000831b0 */

typedef enum WorkAliasRule {
    ALIAS_ESCAPING_ADDRESS,
    ALIAS_SYMBOL_IDENTITY,
    ALIAS_ADDRESS_IDENTITY,
    ALIAS_NONPRIVATE,
    ALIAS_SYMBOL_OR_UNKNOWN,
    ALIAS_ALWAYS
} WorkAliasRule;

/* Original [000837d0,000837e0) and [000837e0,000837f0), exactly
   sixteen one-byte policy cells each. Rows describe the effect's canonical
   address flags, columns the candidate's. Bit 0 records a direct symbol;
   bit 1 records an uncertain/variant address (see analyze_tree_references).
   Each cell names the predicate applied below, rather than an opaque byte. */
const signed char alias_value_rules[4][4] = {
    { ALIAS_ADDRESS_IDENTITY, ALIAS_ESCAPING_ADDRESS, ALIAS_SYMBOL_OR_UNKNOWN, ALIAS_ESCAPING_ADDRESS },
    { ALIAS_ESCAPING_ADDRESS, ALIAS_ADDRESS_IDENTITY, ALIAS_ESCAPING_ADDRESS, ALIAS_SYMBOL_IDENTITY },
    { ALIAS_SYMBOL_OR_UNKNOWN, ALIAS_ESCAPING_ADDRESS, ALIAS_SYMBOL_OR_UNKNOWN, ALIAS_ESCAPING_ADDRESS },
    { ALIAS_ESCAPING_ADDRESS, ALIAS_SYMBOL_IDENTITY, ALIAS_ESCAPING_ADDRESS, ALIAS_SYMBOL_IDENTITY }
};
const signed char conservative_alias_value_rules[4][4] = {
    { ALIAS_ALWAYS, ALIAS_NONPRIVATE, ALIAS_ALWAYS, ALIAS_NONPRIVATE },
    { ALIAS_NONPRIVATE, ALIAS_ADDRESS_IDENTITY, ALIAS_NONPRIVATE, ALIAS_SYMBOL_IDENTITY },
    { ALIAS_ALWAYS, ALIAS_NONPRIVATE, ALIAS_ALWAYS, ALIAS_NONPRIVATE },
    { ALIAS_NONPRIVATE, ALIAS_SYMBOL_IDENTITY, ALIAS_NONPRIVATE, ALIAS_SYMBOL_IDENTITY }
};

/* Original [0005d0e0,0005d2e0). Match a memory effect against a load or
   address-sensitive value. Calls/intrinsics first examine annotated operand
   effects, then apply the original global/private-symbol policy. */
int __fastcall work_effect_invalidates_value(Node *effect, Node *value)
{
    Node *effect_value, *candidate, *operand;
    unsigned rule;
    if (!(node_shapes[value->opcode] & NODE_MEMORY_ACCESS) && value->opcode != 0x55) return 0;
    switch (effect->opcode) {
    case 0x30: case 0x32: case 0x55:
        effect_value = effect->detail.node;
        candidate = value->detail.node;
        if (optimize_alias)
            rule = alias_value_rules[effect_value->attributes.words.flags & 3][candidate->attributes.words.flags & 3];
        else rule = conservative_alias_value_rules[effect_value->attributes.words.flags & 3][candidate->attributes.words.flags & 3];
        switch (rule) {
        case ALIAS_ESCAPING_ADDRESS:
            return is_address_exposed_value(effect_value) || is_address_exposed_value(candidate);
        case ALIAS_SYMBOL_IDENTITY:
            return effect_value->right.symbol == candidate->right.symbol;
        case ALIAS_ADDRESS_IDENTITY:
            return effect_value->right.symbol == candidate->right.symbol &&
                trees_equal(effect_value->left.node, candidate->left.node, 0);
        case ALIAS_NONPRIVATE:
            return !is_private_work_value(effect_value) && !is_private_work_value(candidate);
        case ALIAS_SYMBOL_OR_UNKNOWN:
            return effect_value->right.pointer == output_state || candidate->right.pointer == output_state ||
                effect_value->right.symbol == candidate->right.symbol;
        default:
            return 1;
        }
    case 0x3e: case 0x40: case 0x7e:
        if (effect->opcode == 0x40 && effect->left.node->left.value == 0xf0)
            operand = effect->right.node;
        else operand = effect->second.node;
        for (; operand->opcode != 0x4c; operand = operand->second.node)
            if (operand->detail.node && work_effect_invalidates_value(operand->detail.node, value) == 1) return 1;
        if (effect->opcode == 0x40) return effect->left.node->left.value == 0xdb;
        if (optimize_alias && optimize_weak_inverse) return is_global_work_value(value, 0);
        return !is_private_work_value(value);
    default:
        return 0;
    }
}

/* Original [0005d360,0005d3c0). A direct value is private when its known
   address has not escaped and the function's conservative mode permits it. */
int __fastcall is_private_work_value(Node *node)
{
    TemporarySymbol *symbol;
    if (!(node->attributes.words.flags & 1)) return 0;
    symbol = (TemporarySymbol *)node->right.symbol;
    if (!symbol || symbol->symbol.kind == 19) return 1;
    if (current_function->field28 & 0x40) return 0;
    return is_nonnull_address(node) && !(symbol->attributes & 0x2000);
}
