#include "inline-calls.h"
#include "tree-input.h"
#include "records.h"
#include "tree-rewrite.h"
#include "function-input.h"
#include "expression-fields.h"
#include "work-optimizer.h"
#include "flow-edges.h"
#include "instruction-selection.h"
#include <recon.h>
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
                result = new_inline_storage((unsigned short)(symbol->field24 & 15), symbol->field28);
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

/* Original [00064fc0,0006502b). Move inline locals into the caller's
  64-bucket scope, giving them fresh identities. Parameters retain their
   descriptors for binding, and automatic copies advance their generation. */
void __fastcall merge_inline_scope(ScopeHash *outer, ScopeHash *inner)
{
    unsigned next_symbol_identifier(void);
    SymbolHeader *symbol, *next;
    TemporarySymbol *temporary;
    unsigned bucket;
    for (bucket = 0; bucket < 64; ++bucket) {
        for (symbol = inner->buckets[bucket]; symbol; symbol = next) {
            next = symbol->hash_next;
            temporary = (TemporarySymbol *)symbol;
            if (symbol->kind == 1 && (temporary->attributes & 0x70) == 0x30)
                continue;
            symbol->identifier = next_symbol_identifier();
            if (symbol->kind == 1 && (temporary->attributes & 0x70) == 0x10)
                ++temporary->anonymous;
            put_symbol(outer, symbol);
        }
        inner->buckets[bucket] = 0;
    }
}

/* Original [00064f10,00064fb6). Pair parameter records with call arguments
   in list order. Hidden-result bindings and repeatable unescaped arguments
   may be substituted directly; other arguments require storage later. */
InlineBinding *__fastcall bind_inline_arguments(Function *function, Node *entry, Node *call)
{
    InlineBinding *__fastcall new_inline_binding(Node *parameter, Node *argument);
    InlineBinding *head = 0, **tail = &head, *binding;
    Node *parameter = entry->second.node, *argument = call->second.node;
    while (parameter->opcode != 0x4c && argument->opcode != 0x4c) {
        binding = new_inline_binding(parameter, argument);
        if (function->return_temp && parameter->right.symbol == function->return_temp)
            binding->flags |= 1;
        else if (!(parameter->right.symbol->field0e & 0x2000) &&
                 can_repeat_tree(argument->left.node)) binding->flags |= 1;
        else binding->flags &= ~1U;
        binding->next = *tail;
        *tail = binding;
        tail = &binding->next;
        parameter = parameter->second.node;
        argument = argument->second.node;
    }
    return head;
}

/* Original [00065430,00065479). Arena4 owns20-byte binding records.
   Only the direct-substitution flag is cleared: preserve other flag bits
   on reuse, as the original constructor does. */
InlineBinding *__fastcall new_inline_binding(Node *parameter, Node *argument)
{
    InlineBinding *binding = free_inline_bindings;
    if (binding) free_inline_bindings = binding->next;
    else binding = arena_alloc(4, sizeof(*binding));
    binding->next = 0;
    binding->parameter = parameter;
    binding->argument = argument;
    binding->storage = 0;
    binding->flags &= ~1U;
    return binding;
}

/* Original [00064ea0,00064f08). Pairwise argument effects can revoke
   direct substitution. Constants and an explicitly independent later
   argument do not conflict; stop after the first invalidating pair. */
void __fastcall check_inline_argument_aliases(InlineBinding *bindings)
{
    InlineBinding *first, *second;
    Node *argument, *other;
    unsigned direct;
    for (first = bindings; first; first = first->next) {
        argument = first->argument;
        direct = first->flags & 1;
        for (second = first->next; second; second = second->next) {
            if (!direct && !(second->flags & 1)) continue;
            other = second->argument;
            if (argument->left.node->opcode == 0x33 || other->left.node->opcode == 0x33 ||
                (other->attributes.words.flags & 0x40)) continue;
            if (work_effect_invalidates_value(argument, other)) {
                second->flags &= ~1U;
                first->flags &= ~1U;
                break;
            }
        }
    }
}

/* Original [00064e10,00064e97). Count value-return statements and revoke
   unsafe direct bindings while walking the body after its entry record.
   Embedded assembly uses the original symbol visitor/callback boundary. */
int __fastcall analyze_inline_returns(Expression *entry, InlineBinding *bindings)
{
    void __fastcall scan_inline_argument_uses(InlineBinding *bindings, Node *node);
    unsigned __fastcall mark_inline_assembly_parameter(SymbolHeader *symbol, void *context);
    InlineBinding *binding;
    Expression *expression;
    Node *node;
    int direct = 0, returns = 0;
    for (binding = bindings; binding; binding = binding->next)
        if (binding->flags & 1) { direct = 1; break; }
    for (expression = entry->next; expression; expression = expression->next) {
        node = expression->tree;
        switch (node->opcode) {
        case 0x41:
            ++returns;
            break;
        case 1: case 0x29: case 0x3a: case 0x56:
            continue;
        case 0x4f:
            if (node->detail.control.code == 0x16)
                visit_machine_symbols(node->left.pointer, mark_inline_assembly_parameter, bindings);
            continue;
        }
        if (direct) scan_inline_argument_uses(bindings, node);
    }
    return returns;
}

/* Original [00064de0,00064e02). Create a generated automatic object for
   an inline result and intern its pointer reference. Callers supply the
   low-four-bit scalar kind; preserve the outer leaf's five cdecl arguments. */
Node *__fastcall new_inline_storage(unsigned short kind, unsigned bytes)
{
    BackendSymbol *__fastcall new_generated_storage(unsigned kind, unsigned bytes,
        unsigned short storage, int local, unsigned char alignment, unsigned short flags);
    BackendSymbol *storage = new_generated_storage(kind, bytes, 1, 1, 4, 0);
    return intern_leaf(0x26, 0, (SymbolHeader *)storage, 0, 0x404);
}

/* Original [00064d40,00064dd3). Retain nonconstant argument evaluation
   in a reversed copied chain. The internal F0 node carries that chain in
   its right field; constants need no evaluation prefix. */
Node *__fastcall inline_argument_prefix(InlineBinding *bindings)
{
    InlineBinding *binding;
    Node *head = 0, *node, *tail;
    for (binding = bindings; binding; binding = binding->next) {
        node = binding->argument;
        if (node->left.node->opcode != 0x33) {
            node = build_tree("DNt", node->left.node, 0x55,
                (unsigned)node->attributes.words.type);
            node->second.node = head;
            head = node;
        }
    }
    if (!head) return 0;
    for (tail = head; tail->second.node; tail = tail->second.node) ;
    tail->second.node = intern_leaf(0x4c, 0);
    node = build_tree("NPNt", 0x4c, integer_node(0xf0, 0x102), 0x40, 0x102);
    node->right.node = head;
    return node;
}

/* Original [00064ba0,00064d36). Evaluate bindings that cannot be directly
   substituted into generated locals before the call's expression. Aggregate
   values add the destination to their intrinsic argument list; scalar forms
   become assignments. Move remaining nested-call sites with the new tree. */
void __fastcall materialize_inline_arguments(Function *function, InlineSite *site,
    InlineBinding *bindings)
{
    void __fastcall relocate_inline_sites(InlineSite *sites, Expression *expression);
    extern int floating_frontend;
    InlineBinding *binding;
    TemporarySymbol *parameter;
    Expression *previous = function->expressions, *inserted;
    Node *value, *argument, *reference;
    unsigned bytes;
    unsigned short kind, type;
    while (previous->next != site->expression) previous = previous->next;
    for (binding = bindings; binding; binding = binding->next) {
        if (binding->flags & 1) continue;
        argument = binding->argument;
        if (argument->attributes.words.flags & 0x40)
            compiler_internal_error("@(#)inline.c:1.31", 0x2e7);
        parameter = (TemporarySymbol *)binding->parameter->right.symbol;
        bytes = parameter->symbol.value;
        kind = (unsigned short)(parameter->attributes & 15);
        type = value_node_type((short)kind, bytes);
        binding->storage = new_inline_storage(kind, bytes);
        if (parameter->attributes & 0x2000)
            ((TemporarySymbol *)binding->storage->right.symbol)->attributes |= 0x2000;
        if (parameter->symbol.field0e & 0x2000)
            binding->storage->right.symbol->field0e |= 0x2000;
        value = argument->left.node;
        if ((type & 0xff00) == 0x8000 &&
            (floating_frontend || !is_register_return_size(bytes))) {
            reference = build_tree("DNt", binding->storage, 0x55, 0x404);
            reference->second.node = value->second.node;
            value->second.node = reference;
            release_node(value->left.node);
            value->left.node = integer_node(0xac, 0x104);
        } else {
            reference = intern_leaf(0x26, 0, binding->storage->right.symbol, 0, 0x404);
            value = build_tree("PPNtY", reference, value, 0x32, (unsigned)type);
        }
        refresh_tree_references(value);
        argument->left.node = new_node(1);
        inserted = new_expression();
        inserted->tree = value;
        inserted->next = previous->next;
        previous->next = inserted;
        previous = inserted;
        relocate_inline_sites(site->next, inserted);
    }
}

/* Original [000645c0,00064b9c), including12 directive targets64b6c..64b9c.
   Substitute formal values throughout the copied body, rewrite returns into
   caller expressions/assignments, and remove inline-only scope markers.
   The leading entry record46 establishes the predecessor before any valid
   removable marker; a null predecessor retains the original diagnostic. */
Expression *__fastcall substitute_inline_body(Function *function, InlineBinding *bindings,
    Node *result, unsigned source_location, InlineSite *site, int hidden_result)
{
    Node *__fastcall substitute_inline_node(InlineBinding *bindings, Node *node, int *changed);
    Node *__fastcall find_matching_parent(Node *tree, Node *target);
    unsigned __fastcall substitute_inline_assembly_symbol(SymbolHeader *symbol, void *context);
    unsigned __fastcall second_return_value_register(FunctionSymbol *symbol);
    int __fastcall is_tail_call_position(Expression *expressions, Expression *position);
    extern int floating_frontend, data_pointer_bytes;
    Expression *expression = function->expressions, *previous = 0, *next, *inserted, *discard;
    FunctionSymbol *symbol;
    TemporarySymbol *storage_symbol;
    Node *node, *copy, *parent, *old, *replacement, *assignment, *prefix, *argument;
    Node *storage, *register_leaf, *offset;
    unsigned depth = 0, kind, primary, secondary = 0, pointer_type;
    unsigned short type;
    int changed, return_changed, small_aggregate;
    while (expression->tree->opcode != 0x47) {
        next = expression->next;
        node = expression->tree;
        expression->source_location = source_location;
        switch (node->opcode) {
        case 1: case 0x29: case 0x3a: case 0x46: case 0x56:
            break;
        case 0x41:
            return_changed = 0;
            node->left.node = substitute_inline_node(bindings, node->left.node, &return_changed);
            if (!result && site->parent && !hidden_result) {
                copy = copy_tree(site->expression->tree);
                parent = find_matching_parent(copy, site->call);
                if (trees_equal(parent->left.node, site->call, 0)) {
                    old = parent->left.node;
                    parent->left.node = node->left.node;
                } else {
                    old = parent->second.node;
                    parent->second.node = node->left.node;
                }
                release_tree(old);
                release_node(node);
                expression->tree = copy;
            } else if (!result) {
                expression->tree = node->left.node;
                release_node(node);
            } else {
                copy = copy_tree(result);
                assignment = build_tree("PPNT", copy, node->left.node, 0x32, node);
                release_node(node);
                expression->tree = assignment;
                prefix = build_tree("NPNt", 0x4c, integer_node(0xf0, 0x102), 0x40, 0x102);
                argument = build_tree("DNt", assignment->second.node, 0x55,
                    (unsigned)assignment->attributes.words.type);
                prefix->right.node = argument;
                argument->second.node = intern_leaf(0x4c, 0);
                expression->tree = build_tree("PPNt", prefix, assignment, 0x44,
                    (unsigned)assignment->attributes.words.type);
            }
            refresh_tree_references(expression->tree);
            break;
        case 0x4f:
            switch (node->detail.control.code) {
            case 15:
                ++depth;
                goto remove_marker;
            case 16:
                --depth;
                if (!depth && !result && site->parent && !hidden_result) {
                    copy = copy_tree(site->expression->tree);
                    symbol = (FunctionSymbol *)function->symbol;
                    storage = new_inline_storage((unsigned short)(symbol->field24 & 15), symbol->field28);
                    replacement = build_tree("DNt", storage, 0x30,
                        (unsigned)site->call->attributes.words.type);
                    refresh_tree_references(replacement);
                    parent = find_matching_parent(copy, site->call);
                    if (trees_equal(parent->left.node, site->call, 0)) {
                        old = parent->left.node;
                        parent->left.node = replacement;
                    } else {
                        old = parent->second.node;
                        parent->second.node = replacement;
                    }
                    release_tree(old);
                    release_node(node);
                    expression->tree = copy;
                    break;
                }
                goto remove_marker;
            case 17: case 18:
                goto remove_marker;
            case 22:
                visit_machine_symbols(node->left.pointer, substitute_inline_assembly_symbol, bindings);
                break;
            case 26:
                if (!result || function->return_temp) break;
                symbol = (FunctionSymbol *)function->symbol;
                if ((symbol->field24 & 15) == 7 ||
                    !is_tail_call_position(function->expressions, expression)) break;
                small_aggregate = 0;
                kind = symbol->field24 & 15;
                if (!kind) compiler_internal_error("@(#)inline.c:1.31", 0x214);
                if (kind == 6 && !floating_frontend &&
                    is_register_return_size(symbol->symbol.value)) {
                    type = 0x204;
                    small_aggregate = 1;
                } else if (result->opcode == 0x26 && result->right.symbol->kind != 19) {
                    storage_symbol = (TemporarySymbol *)result->right.symbol;
                    type = value_node_type((short)(storage_symbol->attributes & 15),
                        storage_symbol->symbol.value);
                } else type = result->attributes.words.type;
                primary = return_value_register(symbol);
                if (!(type & 0x1000)) {
                    replacement = build_tree("Nt", 0x4f, (unsigned)type);
                    replacement->left.value = primary;
                    replacement->detail.control.code = 0x24;
                    refresh_tree_references(replacement);
                    inserted = new_expression();
                    inserted->tree = replacement;
                    expression->next = inserted;
                    expression = inserted;
                }
                if (small_aggregate) {
                    secondary = second_return_value_register((FunctionSymbol *)function->symbol);
                    replacement = build_tree("Nt", 0x4f, (unsigned)type);
                    replacement->left.value = secondary;
                    replacement->detail.control.code = 0x24;
                    refresh_tree_references(replacement);
                    inserted = new_expression();
                    inserted->tree = replacement;
                    expression->next = inserted;
                    expression = inserted;
                }
                pointer_type = data_pointer_bytes | 0x400;
                register_leaf = intern_leaf(0xa2, 0, pointer_type, primary);
                copy = copy_tree(result);
                assignment = build_tree("PPNtNt", copy, register_leaf, 0x30,
                    (unsigned)type, 0x32, (unsigned)type);
                refresh_tree_references(assignment);
                inserted = new_expression();
                inserted->tree = assignment;
                inserted->next = next;
                expression->next = inserted;
                expression = inserted;
                if (small_aggregate) {
                    register_leaf = intern_leaf(0xa2, 0, pointer_type, secondary);
                    copy = copy_tree(result);
                    offset = intern_leaf(0x33, 0, 0x104, (unsigned)(type & 255));
                    assignment = build_tree("PPNtPNtNt", offset, copy, 2, pointer_type,
                        register_leaf, 0x30, (unsigned)type, 0x32, (unsigned)type);
                    refresh_tree_references(assignment);
                    inserted = new_expression();
                    inserted->tree = assignment;
                    inserted->next = next;
                    expression->next = inserted;
                    expression = inserted;
                }
                break;
            }
            break;
        default:
            changed = 0;
            expression->tree = substitute_inline_node(bindings, expression->tree, &changed);
            if (changed) refresh_tree_references(expression->tree);
            break;
        }
        goto next_expression;
remove_marker:
        if (!previous) compiler_internal_error("@(#)inline.c:1.31", 0x1f8);
        previous->next = next;
        release_tree(node);
        release_expression(expression);
        expression = previous;
next_expression:
        previous = expression;
        expression = next;
    }
    discard = expression->next;
    expression->next = 0;
    while (discard) {
        next = discard->next;
        release_tree(discard->tree);
        release_expression(discard);
        discard = next;
    }
    return expression;
}

/* Original [000650f0,0006525f). Replace formal parameter addresses by
   materialized storage, or direct loads by copied caller arguments. F0
   evaluation lists live in the right field; other intrinsic argument lists
   retain their root and are rewritten for their child effects. */
Node *__fastcall substitute_inline_node(InlineBinding *bindings, Node *node, int *changed)
{
    InlineBinding *binding;
    SymbolHeader *symbol;
    Node *left, *replacement;
    switch (node->opcode) {
    case 0x26:
        symbol = node->right.symbol;
        if (symbol && symbol->kind == 1 &&
            (((TemporarySymbol *)symbol)->attributes & 0x70) == 0x30) {
            binding = find_inline_binding(bindings, symbol);
            replacement = intern_leaf(0x26, 0, binding->storage->right.symbol,
                node->left.value, 0x404);
            release_node(node);
            *changed = 1;
            return replacement;
        }
        return node;
    case 0x30:
        left = node->left.node;
        if (left->opcode == 0x26 && (symbol = left->right.symbol) != 0 &&
            symbol->kind == 1 && (((TemporarySymbol *)symbol)->attributes & 0x70) == 0x30) {
            binding = find_inline_binding(bindings, symbol);
            if (binding->flags & 1) {
                release_tree(node);
                *changed = 1;
                return copy_tree(binding->argument->left.node);
            }
            node->attributes.words.flags &= ~0x40;
        }
        break;
    case 0x40:
        if (node->left.node->left.value == 0xf0)
            node->right.node = substitute_inline_node(bindings, node->right.node, changed);
        else substitute_inline_node(bindings, node->second.node, changed);
        return node;
    }
    if ((node_shapes[node->opcode] & NODE_ARITY) != NODE_LEAF) {
        node->left.node = substitute_inline_node(bindings, node->left.node, changed);
        if (node_shapes[node->opcode] & NODE_HAS_SECOND)
            node->second.node = substitute_inline_node(bindings, node->second.node, changed);
    }
    return node;
}

/* Original [00064590,000645b7). Formal identity must have a binding;
   retain the diagnostic and null fallback if the invariant is violated. */
InlineBinding *__fastcall find_inline_binding(InlineBinding *bindings, SymbolHeader *symbol)
{
    for (; bindings; bindings = bindings->next)
        if (bindings->parameter->right.symbol == symbol) return bindings;
    compiler_internal_error("@(#)inline.c:1.31", 0x320);
    return 0;
}

/* Original [00064580,0006458e). Arena ownership remains unchanged;
   recycling only links the record into the binding free list. */
void __fastcall recycle_inline_binding(InlineBinding *binding)
{
    binding->next = free_inline_bindings;
    free_inline_bindings = binding;
}

/* Original [000652a0,000653bd). Reject direct substitutions invalidated
   by body effects. Alias assumptions are temporarily relaxed for arguments
   containing global loads, then restored after every binding. Left recursion
   and right-tail traversal retain the original walk/call structure. */
void __fastcall scan_inline_argument_uses(InlineBinding *bindings, Node *node)
{
    int __fastcall inline_argument_has_global_load(Node *node);
    extern int optimize_alias;
    InlineBinding *binding;
    Node *argument;
    int saved_alias;
    unsigned flags;
    while ((node_shapes[node->opcode] & NODE_ARITY) != NODE_LEAF) {
        saved_alias = optimize_alias;
        if (node_shapes[node->opcode] & NODE_SIDE_EFFECT) {
            for (binding = bindings; binding; binding = binding->next) {
                flags = binding->flags;
                if (flags & 1) {
                    if ((node_shapes[node->opcode] & NODE_WRITES_LEFT) &&
                        (node->attributes.words.flags & 1) &&
                        binding->parameter->right.symbol == node->right.symbol) {
                        binding->flags = flags & ~1U;
                    } else {
                        optimize_alias = saved_alias;
                        argument = binding->argument;
                        if (saved_alias && inline_argument_has_global_load(argument)) optimize_alias = 0;
                        if (argument->left.node->opcode != 0x33 &&
                            !(argument->attributes.words.flags & 0x40) &&
                            work_effect_invalidates_value(node, argument))
                            binding->flags = flags & ~1U;
                    }
                }
                optimize_alias = saved_alias;
            }
        }
        scan_inline_argument_uses(bindings, node->left.node);
        if (!(node_shapes[node->opcode] & NODE_HAS_SECOND)) return;
        node = node->second.node;
    }
}

/* Original [00064570,0006457e). Return a site record to its arena's
   free chain without clearing its caller-owned fields. */
void __fastcall recycle_inline_site(InlineSite *site)
{
    site->next = free_inline_sites;
    free_inline_sites = site;
}

/* Original [00065270,00065293). Machine operand visitation pins an
   automatic parameter's binding instead of substituting its argument tree. */
unsigned __fastcall mark_inline_assembly_parameter(SymbolHeader *symbol, void *context)
{
    InlineBinding *binding;
    if (symbol->kind == 1 && (((FunctionSymbol *)symbol)->field24 & 0x70) == 0x30) {
        binding = find_inline_binding(context, symbol);
        binding->flags &= ~1U;
    }
    return 0;
}

/* Original [000650c0,000650e9). Assembly operands substitute the
   materialized parameter symbol's identifier, never its argument tree. */
unsigned __fastcall substitute_inline_assembly_symbol(SymbolHeader *symbol, void *context)
{
    InlineBinding *binding;
    if (symbol->kind == 1 && (((FunctionSymbol *)symbol)->field24 & 0x70) == 0x30) {
        binding = find_inline_binding(context, symbol);
        return binding->storage->right.symbol->identifier;
    }
    return 0;
}

/* Original [000653c0,00065425). A repeatable inline argument may still
   load globally affected storage. Recurse over original opcode arity and
   second-child property before testing this node's memory reference. */
int __fastcall inline_argument_has_global_load(Node *node)
{
    if ((node_shapes[node->opcode] & NODE_ARITY) != NODE_LEAF) {
        if (inline_argument_has_global_load(node->left.node)) return 1;
        if ((node_shapes[node->opcode] & NODE_HAS_SECOND) &&
            inline_argument_has_global_load(node->second.node)) return 1;
    }
    if (node->opcode == 0x30 && is_global_work_value(node, 1)) return 1;
    return 0;
}
