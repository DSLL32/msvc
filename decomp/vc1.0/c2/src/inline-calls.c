#include "inline-calls.h"
#include "records.h"
#include "tree-rewrite.h"
#include "function-input.h"
#include "expression-fields.h"
InlineSite *free_inline_sites; /* 0007b9ac */
InlineBinding *free_inline_bindings; /* 0007b9b0 */
int inline_expansion_cost; /* 0007b9a8 */

/* Original [00063f60,00063fc0). Bound growth in the recursive inliner and
   discard its arena after processing this function. */
void __fastcall expand_inline_calls(Function *function)
{
    int budget;
    free_inline_bindings = 0;
    free_inline_sites = 0;
    inline_expansion_cost = (short)((FunctionSymbol *)function->symbol)->field50;
    budget = 2 * inline_expansion_cost;
    if (budget < 1000) budget = 1000;
    else if (budget > 35000) budget = 35000;
    inline_function_calls(function, 1, budget);
    reset_arena(4);
}

/* Original [00063fc0,00064570). Expand candidates in their recorded order,
   sharing a growth budget across recursive expansion of callee bodies. */
int __fastcall inline_function_calls(Function *function, int depth, int budget)
{
    InlineSite *sites, *site, *next_site;
    InlineBinding *bindings, *binding, *next_binding;
    FunctionSymbol *symbol, *body_symbol;
    Function *body;
    Node *call, *argument, *result, *prefix, *tree, *replacement;
    Node **entry_tree;
    Expression *entry, *discard, *last, *previous, *inserted;
    int original_budget = budget, remaining, count, size, used, returns, hidden, other_call;
    ((FunctionSymbol *)function->symbol)->flags52 |= 0x10;
    sites = collect_inline_sites(function, &remaining);
    for (site = sites; site; site = site->next, --remaining) {
        call = site->call;
        symbol = call->left.node->right.function;
        count = (short)symbol->field48 + needs_return_temp(symbol->symbol.field0e & 0x1c,
                                                         (unsigned short)(symbol->field24 & 15), symbol->field28);
        argument = call->second.node;
        while (count && argument->opcode != 0x4c) {
            --count;
            argument = argument->second.node;
        }
        if (count || argument->opcode != 0x4c || (int)(site->flags & 255) < depth)
            goto rejected;
        size = (short)symbol->field50;
        if ((budget < size && size > 40) || inline_expansion_cost > 35000) goto rejected;
        if (size > 40) budget -= size;
        inline_expansion_cost += size;
        if (!(symbol->flags52 & 0x80)) {
            if (site->expression->source_location) current_source_location = site->expression->source_location;
            compiler_warning(4, 0x2c7, format_function_name(&symbol->symbol));
        }
        body = read_function(symbol);
        if (!optimize_global && (body->field28 & 1)) {
            if (symbol->flags52 & 0x80) goto rejected;
            goto next;
        }
        canonicalize_function(body);
        optimize_function_flow(body);
        used = inline_function_calls(body, depth + 1, budget / remaining);
        budget -= used;
        inline_expansion_cost += used;
        merge_inline_scope(function->scope, body->scope);
        entry = body->expressions;
        while (entry->tree->opcode != 0x46) {
            discard = entry;
            entry = entry->next;
            release_tree(discard->tree);
            release_expression(discard);
        }
        body->expressions = entry;
        entry_tree = &entry->tree;
        (*entry_tree)->second.node = reverse_node_list((*entry_tree)->second.node);
        bindings = bind_inline_arguments(body, *entry_tree, site->call);
        check_inline_argument_aliases(bindings);
        returns = analyze_inline_returns(entry, bindings);
        body_symbol = (FunctionSymbol *)body->symbol;
        hidden = needs_return_temp(body_symbol->symbol.field0e & 0x1c, (unsigned short)(body_symbol->field24 & 15), body_symbol->field28);
        site->call->opcode = 1;
        other_call = tree_contains_opcode(site->expression->tree, 0x3e);
        site->call->opcode = 0x3e;
        result = 0;
        if (site->parent && !hidden) {
            if (site->parent->opcode == 0x32 && site->parent->second.node == site->call &&
                can_repeat_tree(site->parent->left.node)) result = site->parent->left.node;
            else if (returns > 2 || other_call || (body->field28 & 1) ||
                     tree_contains_opcode(site->expression->tree, 0x3b))
                result = new_inline_storage((signed char)(symbol->field24 & 15), symbol->field28);
        }
        if (hidden && !result) {
            binding = find_inline_binding(bindings, body->return_temp);
            if (!binding) compiler_internal_error("@(#)inline.c:1.31", 0x10c);
            binding->flags |= 1;
            tree = site->expression->tree;
            if (tree->opcode == 0x40 && tree->left.node->left.value == 0xac &&
                tree->second.node->second.node == site->parent && can_repeat_tree(tree->second.node->left.node)) {
                binding->argument = tree->second.node;
                site->expression->tree = site->call;
                site->parent = 0;
            }
        }
        prefix = inline_argument_prefix(bindings);
        materialize_inline_arguments(function, site, bindings);
        last = substitute_inline_body(body, bindings, result, site->expression->source_location, site, hidden);
        function->field28 |= body->field28 & 1;
        function->field28 |= body->field28 & 8;
        function->field28 |= body->field28 & 0x40;
        previous = function->expressions;
        while (previous && previous->next != site->expression) previous = previous->next;
        if (!previous) compiler_internal_error("@(#)inline.c:1.31", 0x12c);
        last->next = previous->next;
        previous->next = entry;
        if (prefix) {
            refresh_tree_references(prefix);
            inserted = new_expression();
            inserted->tree = prefix;
            inserted->next = entry;
            previous->next = inserted;
        }
        if (hidden && !result) {
            if (site->parent) {
                binding = find_inline_binding(bindings, body->return_temp);
                replacement = copy_tree(binding->argument->left.node);
                if (site->parent->left.node == site->call) site->parent->left.node = replacement;
                else site->parent->second.node = replacement;
                release_tree(site->call);
                site->call = 0;
            } else {
                release_node_children(site->call);
                site->call->opcode = 1;
            }
        } else if (result) {
            replacement = build_tree("DNt", result, 0x30, value_node_type((short)(symbol->field24 & 15), symbol->field28));
            if (site->parent->left.node == site->call) site->parent->left.node = replacement;
            else site->parent->second.node = replacement;
            release_tree(site->call);
            site->call = 0;
        } else {
            release_node_children(site->expression->tree);
            site->expression->tree->opcode = 1;
        }
        refresh_tree_references(site->expression->tree);
        last->tree->opcode = 1;
        release_node_children(*entry_tree);
        (*entry_tree)->opcode = 1;
        site->expression = 0;
        site->parent = 0;
        site->call = 0;
        while (bindings) {
            next_binding = bindings->next;
            recycle_inline_binding(bindings);
            bindings = next_binding;
        }
        goto next;
rejected:
        if (symbol->flags52 & 0x80) {
            if (site->expression->source_location) current_source_location = site->expression->source_location;
            compiler_warning(4, 0x2c6, format_function_name(&symbol->symbol));
        }
next:
        ;
    }
    ((FunctionSymbol *)function->symbol)->flags52 &= ~0x10;
    while (sites) {
        next_site = sites->next;
        recycle_inline_site(sites);
        sites = next_site;
    }
    return original_budget - budget;
}
