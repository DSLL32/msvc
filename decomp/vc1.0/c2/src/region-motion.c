/* Region invariant and induction-value motion. */
#include "region-motion.h"
#include "expression-fields.h"
#include "tree-input.h"
#include "function-input.h"
#include "work-prepare.h"
#include "algebra.h"
#include "work-expressions.h"
#include <recon.h>
FlowRegion *hoist_current_region; /* 0007bea8 */
WorkItem *hoist_entry_work;  /* 0007bea0 */
WorkItem *hoist_header_work; /* 0007be9c */
WorkItem *hoist_first_work;  /* 0007be98 */
WorkItem *hoist_last_work;   /* 0007be94 */
WorkItem *hoist_after_work;  /* 0007be90 */
/* Original000830d0: one cursor shared by expression and machine phases. */
TransformationCursor transformation_cursor;

/* Original [0005c020,0005c120). Process nested regions before their
   parent, establish expression anchors, and run induction/value-motion
   stages with the original early-exit and cleanup order. */
void __fastcall hoist_region_values(FlowRegion *regions)
{
    WorkItem *header;
    for (; regions; regions = regions->next) {
        if (regions->children) hoist_region_values(regions->children);
        if (regions->flags & 1) continue;
        hoist_current_region = regions;
        hoist_entry_work = regions->entry;
        if (!hoist_entry_work->body.expressions)
            hoist_entry_work->body.expressions = insert_tree_expression(0, 0, new_node(1));
        header = hoist_entry_work->next;
        if (!header->body.expressions)
            header->body.expressions = insert_tree_expression(0, 0, new_node(1));
        hoist_header_work = hoist_entry_work->next;
        transformation_cursor.expression = hoist_header_work->body.expressions;
        hoist_first_work = regions->entry->next->next;
        hoist_last_work = regions->last;
        hoist_after_work = hoist_last_work->next;
        hoist_invariant_region_stores();
        collect_basic_region_inductions();
        if (!prepare_region_exit_motion()) {
            collect_derived_region_inductions();
            combine_region_inductions();
            reduce_region_induction_strength();
            rewrite_region_inductions();
            simplify_region_exit();
            finish_region_motion();
            rebuild_region_header_references();
            release_region_motion();
        }
    }
}

static BitSet *hoist_seen_memory_values; /* 0007be6c: reads seen during store scan. */
static BitSet *hoist_selected_values;    /* 0007be70: values selected for later rewriting. */
static BitSet *hoist_invariant_values;   /* 0007be74: invariant expression identifiers. */
int __fastcall work_dominates_region_exits(WorkItem *work);
int __fastcall work_dominates_region_backedges(WorkItem *work);
int __fastcall classify_region_invariants(Node *node);
int __fastcall can_hoist_region_store(Node *node);
void __fastcall remove_region_candidate(Node *node, int ancestors);
/* Original [0005f1c0,0005f2db). Mark invariant stores, then move a safe
   store to the region's insertion anchor only when every relevant exit/backedge
   passes through its block and no prior read or conflicting store prevents it. */
void hoist_invariant_region_stores(void)
{
    WorkItem *work;
    Expression *expression;
    Node *node;
    int blocked, identifier;
    hoist_seen_memory_values = 0;
    hoist_selected_values = 0;
    hoist_invariant_values = 0;
    for (work = hoist_first_work; work != hoist_after_work; work = work->next) {
        blocked = !(work_dominates_region_exits(work) && work_dominates_region_backedges(work));
        for (expression = work->body.expressions; expression; expression = expression->next) {
            node = expression->tree;
            node->attributes.words.flags &= ~0x400;
            if (classify_region_invariants(node) &&
                work->exception_scope == hoist_first_work->exception_scope && node->opcode == 0x32) {
                node->attributes.words.flags |= 0x400;
                if (!blocked && can_hoist_region_store(node) &&
                    !bitset_contains(hoist_seen_memory_values,
                        (short)node->detail.node->info.words.identifier)) {
                    insert_tree_expression(transformation_cursor.expression, 0, node);
                    expression->tree = new_node(1);
                    remove_region_candidate(node, 0);
                    identifier = (short)node->detail.node->info.words.identifier;
                    bitset_remove(work->flow_sets[WORK_MEMORY_STORES], identifier);
                    bitset_remove(work->flow_sets[WORK_UNREAD_STORES], identifier);
                }
            }
        }
    }
    release_bitset(hoist_seen_memory_values);
}

int __fastcall work_dominates_block(WorkItem *candidate, WorkItem *block);
/* Original [0005f3a0,0005f3fe). A movable store must dominate each
   block which can leave the region through a later-numbered successor. */
int __fastcall work_dominates_region_exits(WorkItem *work)
{
    WorkItem *block;
    FlowLink *successor;
    for (block = hoist_first_work; block != hoist_after_work; block = block->next)
        for (successor = block->successors; successor; successor = successor->next)
            if (successor->work->number > hoist_last_work->number &&
                !work_dominates_block(work, block)) return 0;
    return 1;
}

/* Original [0005f320,0005f32e). Distinct dominance-tail adapter. */
int __fastcall work_dominates_block(WorkItem *candidate, WorkItem *block)
{
    return bitset_contains(block->analysis.dominators, candidate->number);
}

int __fastcall work_dominates_target_predecessors(WorkItem *work, WorkItem *target);
/* Original [0005f330,0005f33b). Distinct adapter for region backedges. */
int __fastcall work_dominates_region_backedges(WorkItem *work)
{
    return work_dominates_target_predecessors(work, hoist_first_work);
}

/* Original [0005f340,0005f398). Check every in-region source of an
   incoming edge to the chosen target, including each loop backedge. */
int __fastcall work_dominates_target_predecessors(WorkItem *work, WorkItem *target)
{
    WorkItem *block;
    FlowLink *successor;
    for (block = hoist_first_work; block != hoist_after_work; block = block->next) {
        for (successor = block->successors; successor; successor = successor->next)
            if (successor->work == target) break;
        if (successor && !work_dominates_block(work, block)) return 0;
    }
    return 1;
}

/* Original [0005f800,0005f910). Classify invariant subtrees after their
   children, rejecting volatile or loop-clobbered loads and remembering all
   reads. Original shape0100 maps exactly to semantic NODE_VALUE_EXPRESSION. */
int __fastcall classify_region_invariants(Node *node)
{
    unsigned shape = node_shapes[node->opcode];
    int left, right = 1, eligible;
    RegionValue *candidate;
    if ((shape & NODE_ARITY) == NODE_LEAF) return 1;
    left = classify_region_invariants(node->left.node);
    if (node_shapes[node->opcode] & NODE_HAS_SECOND)
        right = classify_region_invariants(node->second.node);
    shape = node_shapes[node->opcode];
    eligible = (shape & NODE_VALUE_EXPRESSION) != 0 || node->opcode == 0x32 || node->opcode == 0x55;
    if (node->opcode == 0x30) {
        if (node->attributes.words.flags & 0x800) eligible = 0;
        else {
            for (candidate = hoist_current_region->candidates; candidate; candidate = candidate->next)
                if (work_effect_invalidates_value(candidate->tree, node)) break;
            if (candidate) eligible = 0;
        }
        hoist_seen_memory_values = bitset_include(hoist_seen_memory_values,
            (short)node->detail.node->info.words.identifier);
    }
    if (eligible & left & right) {
        if (node_shapes[node->opcode] & NODE_VALUE_EXPRESSION) {
            hoist_invariant_values = bitset_include(hoist_invariant_values,
                (short)node->info.words.identifier);
            node->attributes.words.flags |= 0x10;
        }
        return 1;
    }
    node->attributes.words.flags &= ~0x10;
    return 0;
}

/* Original0007bc2c: head of arena-4 basic/derived induction records. */
static struct BasicInduction *basic_region_inductions;
/* Original [0005f070,0005f1be). Recognize canonical store updates by an
   invariant increment, excluding nested regions and restricted storage. */
void collect_basic_region_inductions(void)
{
    WorkItem *work;
    Expression *expression;
    Node *node, *address, *value;
    TemporarySymbol *symbol;
    unsigned attributes;
    basic_region_inductions = 0;
    for (work = hoist_first_work; work != hoist_after_work; work = work->next) {
        for (expression = work->body.expressions; expression; expression = expression->next) {
            node = expression->tree;
            if (node->opcode != 0x32) continue;
            symbol = (TemporarySymbol *)node->right.symbol;
            if (symbol && symbol->symbol.kind != 0x13) {
                attributes = symbol->attributes;
                if ((attributes & 0x2000) || (attributes & 15) == 6) continue;
                if (!(node->attributes.words.flags & 1) && (attributes & 0x4000)) continue;
            }
            if ((node->attributes.words.flags & 0x800) ||
                !work_is_outside_child_regions(work) || !can_hoist_region_store(node) ||
                !is_region_invariant_value(node->left.node) ||
                work->exception_scope != hoist_first_work->exception_scope) continue;
            address = node->left.node;
            value = node->second.node;
            if ((value->opcode == 2 || value->opcode == 3) &&
                matches_assignment_load(address, value->left.node) &&
                is_region_invariant_value(value->second.node))
                new_basic_induction(node, value->second.node, work, 0, 0, 0);
            if (value->opcode == 2 && matches_assignment_load(address, value->second.node) &&
                is_region_invariant_value(value->left.node))
                new_basic_induction(node, value->left.node, work, 0, 0, 0);
        }
    }
}

/* Original [0005f2e0,0005f313). Signed block ranges of immediate nested
   regions exclude their bodies from the enclosing basic-induction scan. */
int __fastcall work_is_outside_child_regions(WorkItem *work)
{
    FlowRegion *child;
    for (child = hoist_current_region->children; child; child = child->next)
        if (child->header->number <= work->number && child->last->number >= work->number)
            return 0;
    return 1;
}

/* Original [0005f7c0,0005f7fc). The store's own canonical candidate is
   permitted; a second candidate invalidating the same value prevents motion. */
int __fastcall can_hoist_region_store(Node *node)
{
    RegionValue *candidate;
    unsigned conflicts = 0;
    for (candidate = hoist_current_region->candidates; candidate; candidate = candidate->next)
        if (work_effect_invalidates_value(candidate->tree, node) && ++conflicts > 1)
            return 0;
    return 1;
}

/* Original [0005f580,0005f5b7). A numbered value is invariant when it
   is a leaf, or its expression identifier was classified invariant. */
int __fastcall is_region_invariant_value(Node *node)
{
    unsigned shape = node_shapes[node->opcode];
    short identifier = node->info.words.identifier;
    if (!(shape & NODE_VALUE_EXPRESSION) || !identifier) return 0;
    if ((shape & NODE_ARITY) == NODE_LEAF) return 1;
    return bitset_contains(hoist_invariant_values, identifier);
}


/* Original0007be78..0007be8c: live loop-exit and count trees, each owned
   by region value motion; cleanup releases the three independently. */
static Expression *region_entry_test; /*7be78*/
static Node *region_exit_branch;      /*7be7c*/
static Node *region_induction_counter;/*7be80*/
static Node *multiple_exit_trip_count;/*7be84*/
static Node *region_trip_count;       /*7be88*/
int __cdecl addition_overflows(unsigned left, unsigned right, int is_unsigned, unsigned bytes);

/* Original [0005eab0,0005f064). Identify a supported terminal comparison,
   its basic induction and invariant bound, then build a canonical trip count.
   Keep the original distinct dead-loop and multi-exit cleanup paths. */
int prepare_region_exit_motion(void)
{
    Expression *entry_tail, *tail;
    WorkItem *work;
    FlowLink *link;
    BasicInduction *induction;
    Node *entry_branch, *comparison, *entry_comparison;
    Node *assignment, *lower, *upper, *swap, *stride_node, *count;
    int exits = 0, incoming = -1, stride, inclusive = 1;
    unsigned difference;
    multiple_exit_trip_count = 0;
    region_trip_count = 0;
    region_induction_counter = 0;
    region_exit_branch = 0;
    entry_tail = hoist_entry_work->body.expressions;
    for (tail = entry_tail; tail->next; tail = tail->next) ;
    entry_branch = tail->tree;
    if (entry_branch->opcode == 0x3a) {
        for (induction = basic_region_inductions; induction; induction = induction->next)
            if (induction_is_dead_on_exit(induction->assignment))
                bitset_remove(hoist_entry_work->next->flow_sets[WORK_MEMORY_LIVE_OUT],
                    (short)induction->assignment->detail.node->info.words.identifier);
        remove_region_blocks();
        count = hoist_after_work->body.expressions->tree;
        if (count->opcode == 0x29 && count->right.symbol == entry_branch->right.symbol) {
            entry_branch->opcode = 1;
            remove_region_block(hoist_entry_work->next);
        }
        return 1;
    }
    for (work = hoist_first_work; work != hoist_after_work; work = work->next) {
        for (link = work->successors; link; link = link->next) {
            if (link->work->number > hoist_last_work->number ||
                link->work->number < hoist_first_work->number)
                if (++exits > 1) break;
        }
    }
    if (!exits) return 1;
    for (link = hoist_first_work->predecessors; link; link = link->next) ++incoming;
    if (incoming > 1) return 0;
    tail = hoist_last_work->body.expressions;
    if (!tail) return 1;
    while (tail->next) tail = tail->next;
    if (tail->tree->opcode != 0x38) return 0;
    region_exit_branch = tail->tree;
    region_entry_test = 0;
    while (entry_tail->next) entry_tail = entry_tail->next;
    entry_branch = entry_tail->tree;
    if (entry_branch->opcode == 0x38) {
        entry_comparison = entry_branch->left.node;
        comparison = region_exit_branch->left.node;
        if (map_comparison_opcode(entry_comparison->opcode, 1) == comparison->opcode &&
            trees_equal(comparison->second.node, entry_comparison->second.node, 1) &&
            trees_equal(comparison->left.node, entry_comparison->left.node, 1)) {
            link = hoist_entry_work->successors;
            if (link->work == hoist_last_work->next || link->next->work == hoist_last_work->next)
                region_entry_test = entry_tail;
        }
    }
    comparison = region_exit_branch->left.node;
    induction = find_basic_induction_load(comparison->second.node);
    if (induction && is_region_invariant_value(comparison->left.node)) {
        comparison->opcode = map_comparison_opcode(comparison->opcode, 0);
        swap = comparison->second.node;
        comparison->second.node = comparison->left.node;
        comparison->left.node = swap;
    } else {
        induction = find_basic_induction_load(comparison->left.node);
        if (!induction || !is_region_invariant_value(comparison->second.node)) return 0;
    }
    assignment = induction->assignment;
    if ((assignment->attributes.words.type & 0x1000) || induction->step->opcode != 0x33 ||
        !induction->dominates_region) return 0;
    lower = find_induction_initial_value(assignment);
    if (!lower || (node_shapes[lower->opcode] & NODE_ARITY) != NODE_LEAF)
        lower = assignment->second.node->left.node;
    upper = comparison->second.node;
    stride = basic_induction_increment(induction);
    switch (comparison->opcode) {
    case 0x1f:
        if (stride < 0) {
            stride = (int)(0U - (unsigned)stride);
            swap = lower; lower = upper; upper = swap;
        }
        break;
    case 0x22: inclusive = 0; /* Fall through: decreasing comparison. */
    case 0x21:
        if (stride >= 0) return 0;
        stride = (int)(0U - (unsigned)stride);
        swap = lower; lower = upper; upper = swap;
        break;
    case 0x24: inclusive = 0; /* Fall through: increasing comparison. */
    case 0x23:
        if (stride <= 0) return 0;
        break;
    default: return 0;
    }
    stride_node = typed_integer_node(induction->step, (unsigned)stride);
    if (lower->opcode == 0x33 && upper->opcode == 0x33 &&
        addition_overflows(upper->left.value - lower->left.value,
            stride_node->left.value, 1, stride_node->attributes.words.type & 255)) return 0;
    lower = copy_tree(lower);
    if (upper->attributes.words.type & 0x100)
        lower = build_tree("PNt", lower, 0x34, (lower->attributes.words.type & 255) | 0x200);
    lower = optimize_tree_global(lower);
    upper = copy_tree(upper);
    if (upper->attributes.words.type & 0x100)
        upper = build_tree("PNt", upper, 0x34, (upper->attributes.words.type & 255) | 0x200);
    upper = optimize_tree_global(upper);
    region_trip_count = build_tree("PPNYDNtNY", upper, lower, 3, stride_node,
        0x34, (stride_node->attributes.words.type & 255) | 0x200, 2);
    if (inclusive)
        region_trip_count = build_tree("PPNY", region_trip_count,
            typed_integer_node(region_trip_count, 1), 3);
    region_trip_count = build_tree("PPNY", region_trip_count, stride_node, 5);
    count = optimize_tree_global(region_trip_count);
    if (count->opcode != 0x33 && !hoist_entry_work->successors->next) {
        region_trip_count = count;
        count = build_tree("PPNY", count, typed_integer_node(count, 1), 0xae);
    }
    if (count->opcode == 0x33 || stride == 1) {
        if (count->opcode != 0x33) goto usable_count;
        region_trip_count = count;
        difference = normalize_integer_width(lower, upper->left.value - lower->left.value);
        if (difference == normalize_integer_width(lower, count->left.value * (unsigned)stride))
            goto usable_count;
    }
    if (comparison->opcode == 0x1f) {
        region_trip_count = count;
        release_tree(count);
        region_trip_count = 0;
        return 0;
    }
    goto save_count;
usable_count:
    if ((region_entry_test || count->opcode == 0x33) &&
        (comparison->opcode == 0x23 || comparison->opcode == 0x21)) {
        region_trip_count = count;
        if (is_constant_value(comparison->second.node, 0)) comparison->opcode = 0x1f;
    }
save_count:
    region_trip_count = count;
    if (exits > 1) {
        multiple_exit_trip_count = count;
        region_trip_count = 0;
    }
    return 0;
}


static DerivedInduction *derived_region_inductions; /*0007bc18*/
static DerivedInductionGroup *derived_induction_groups; /*0007bc14*/
static DerivedInductionRewrite *derived_induction_rewrites; /*0007bb90*/
static int derived_motion_blocked; /*0007bc1c*/
static int relaxed_induction_motion; /*000806b0: default-zero motion policy*/

/* Original [0005e790,0005eaae). Collect affine uses, group compatible
   derived values of one basic induction, choose a representative, then rewrite
   or discard each group and release the temporary expression trees. */
void collect_derived_region_inductions(void)
{
    WorkItem *work;
    Expression *expression;
    DerivedInduction *head, *candidate, *next, *remaining, **position, *best;
    DerivedInductionGroup *group, *last_group = 0;
    DerivedInductionRewrite *rewrite;
    Node *factor, *difference, *representative;
    int blocked, weight;
    derived_region_inductions = 0;
    for (work = hoist_first_work; work != hoist_after_work; work = work->next) {
        current_expression_work = work;
        derived_motion_blocked = !relaxed_induction_motion &&
            !(work_dominates_region_exits(work) && work_dominates_region_backedges(work));
        for (expression = work->body.expressions; expression; expression = expression->next) {
            work_expression = expression;
            collect_derived_induction_tree(expression->tree, 0);
        }
    }
    head = derived_region_inductions;
    if (head && !head->next) {
        factor = head->factor;
        if ((is_constant_value(factor, 2) || is_constant_value(factor, 4) ||
             is_constant_value(factor, 8)) && head->uses <= 3 &&
            head->basic->derived_uses + 2 < head->basic->total_uses) {
            release_tree(head->factor);
            release_tree(head->tree);
            return;
        }
    }
    derived_induction_groups = 0;
    while (derived_region_inductions) {
        head = derived_region_inductions;
        remaining = head->next;
        derived_region_inductions = remaining;
        head->next = 0;
        blocked = head->blocked;
        representative = 0;
        position = &remaining;
        for (candidate = remaining; candidate; candidate = next) {
            next = candidate->next;
            if (head->basic == candidate->basic && trees_equal(candidate->factor, head->factor, 1)) {
                difference = optimize_tree_global(build_tree("DDNY", head->tree, candidate->tree, 3));
                if (difference->opcode == 0x33 ||
                    choose_derived_induction_representative(difference, head->tree,
                        candidate->tree, &representative)) {
                    *position = candidate->next;
                    candidate->next = head;
                    head = candidate;
                    blocked &= candidate->blocked;
                    release_tree(difference);
                    continue;
                }
                release_tree(difference);
            }
            position = &candidate->next;
        }
        derived_region_inductions = remaining;
        if (!representative) {
            best = head;
            weight = head->uses;
            for (candidate = head->next; candidate; candidate = candidate->next)
                if (candidate->uses > weight) {
                    weight = candidate->uses;
                    best = candidate;
                }
            representative = copy_tree(best->tree);
        }
        group = arena_alloc(4, sizeof(DerivedInductionGroup));
        group->next = 0;
        group->candidates = head;
        group->representative = representative;
        group->blocked = blocked;
        if (!last_group) derived_induction_groups = group;
        else last_group->next = group;
        last_group = group;
    }
    for (group = derived_induction_groups; group; group = group->next)
        if (group->blocked && derived_induction_is_profitable(group->representative))
            group->blocked = 0;
    derived_induction_rewrites = 0;
    for (group = derived_induction_groups; group; group = group->next) {
        if (group->blocked) {
            for (candidate = group->candidates; candidate; candidate = candidate->next) {
                release_tree(candidate->factor);
                release_tree(candidate->tree);
            }
        } else rewrite_derived_induction_group(group->candidates, group->representative);
        release_tree(group->representative);
    }
    for (rewrite = derived_induction_rewrites; rewrite; rewrite = rewrite->next) {
        release_tree(rewrite->field04);
        release_tree(rewrite->field08);
    }
}


/* Original [00060550,000607d4). Follow an affine operand through invariant
   additions/multiplications, constant shifts and permitted conversions. Count
   its basic induction uses and record only non-unit scale expressions. */
void __fastcall collect_derived_induction_tree(Node *node, Node *parent)
{
    Node *factor, *walk, *next, *left, *right;
    BasicInduction *basic;
    unsigned opcode, type, child_type;
    int negative, can_narrow;
    while ((node_shapes[node->opcode] & NODE_ARITY) != NODE_LEAF) {
        negative = 0;
        can_narrow = 1;
        factor = integer_node(1, 0x104);
        walk = node;
        while (walk) {
            next = 0;
            opcode = walk->opcode;
            switch (opcode) {
            case 2:
            case 3:
                right = walk->second.node;
                if ((right->attributes.words.flags & 0x10) ||
                    (node_shapes[right->opcode] & NODE_ARITY) == NODE_LEAF)
                    next = walk->left.node;
                else {
                    left = walk->left.node;
                    if ((left->attributes.words.flags & 0x10) ||
                        (node_shapes[left->opcode] & NODE_ARITY) == NODE_LEAF) {
                        next = right;
                        if (opcode == 3) negative = !negative;
                    }
                }
                break;
            case 4:
                right = walk->second.node;
                if ((right->attributes.words.flags & 0x10) ||
                    (node_shapes[right->opcode] & NODE_ARITY) == NODE_LEAF) {
                    factor = build_tree("PDNY", factor, right, 4);
                    next = walk->left.node;
                } else {
                    left = walk->left.node;
                    if ((left->attributes.words.flags & 0x10) ||
                        (node_shapes[left->opcode] & NODE_ARITY) == NODE_LEAF) {
                        factor = build_tree("PDNY", factor, left, 4);
                        next = walk->second.node;
                    }
                }
                break;
            case 9:
                right = walk->second.node;
                if (right->opcode != 0x33) goto affine_end;
                factor = build_tree("PPNY", factor,
                    integer_node(1U << (right->left.value & 31), walk->attributes.words.type), 4);
                next = walk->left.node;
                break;
            case 0x34:
                left = walk->left.node;
                type = walk->attributes.words.type;
                child_type = left->attributes.words.type;
                if (!((type ^ child_type) & 0xff00)) {
                    if ((child_type & 255) >= (type & 255)) {
                        if (can_narrow) next = left;
                    } else if (relaxed_induction_motion && (type & 255) <= 4) {
                        next = left;
                        can_narrow = 0;
                    }
                }
                break;
            default:
                goto affine_end;
            }
            walk = next;
        }
affine_end:
        if (walk && walk->opcode == 0x30 &&
            (basic = find_basic_induction(walk->left.node)) != 0) {
            ++basic->total_uses;
            factor = optimize_tree_global(factor);
            if (!is_constant_value(factor, 1)) {
                add_derived_induction(node, parent, basic, factor, negative, derived_motion_blocked);
                ++basic->derived_uses;
            }
            release_tree(factor);
            return;
        }
        release_tree(factor);
        collect_derived_induction_tree(node->left.node, node);
        if (!(node_shapes[node->opcode] & NODE_HAS_SECOND)) return;
        parent = node;
        node = node->second.node;
    }
}

/* Original [0005fe70,0005fea5). Keep lookup distinct from the load
   wrapper; compare the basic assignment's address with the requested tree. */
BasicInduction *__fastcall find_basic_induction(Node *address)
{
    BasicInduction *induction;
    for (induction = basic_region_inductions; induction; induction = induction->next)
        if (trees_equal(induction->assignment->left.node, address, 1)) return induction;
    return 0;
}


Node *__fastcall numbered_expression_temporary(Node *node, Node **canonical, int intern);
void __fastcall record_hoisted_work_effect(Node *node);
static BitSet *hoist_materialized_values; /*0007be68: insertion deduplication*/
/* Original [0005e680,0005e786). Select invariant numbered values, rewrite
   dominated expressions and materialize selected assignment operands, then
   discard both numbered-expression indices and the per-region insertion set. */
void combine_region_inductions(void)
{
    WorkItem *work;
    Expression *expression;
    NumberedExpression *numbered;
    Node *node;
    unsigned i;
    for (work = hoist_first_work; work != hoist_after_work; work = work->next)
        for (expression = work->body.expressions; expression; expression = expression->next)
            collect_selected_region_values(expression->tree);
    hoist_materialized_values = 0;
    for (work = hoist_first_work; work != hoist_after_work; work = work->next) {
        if (!relaxed_induction_motion &&
            !(work_dominates_region_exits(work) && work_dominates_region_backedges(work))) continue;
        current_expression_work = work;
        for (expression = work->body.expressions; expression; expression = expression->next) {
            node = expression->tree;
            rewrite_selected_region_values(node);
            if (node->opcode == 0x32 && (node->attributes.words.flags & 0x400)) {
                node->left.node = materialize_region_value(node->left.node);
                node->second.node = materialize_region_value(node->second.node);
            }
        }
    }
    for (i = 0; i < NUMBERED_EXPRESSION_BUCKETS; ++i) {
        for (numbered = numbered_expressions_by_id[i]; numbered; numbered = numbered->identifier_next)
            release_node(numbered->canonical);
        numbered_expressions[i] = 0;
        numbered_expressions_by_id[i] = 0;
    }
    release_bitset(hoist_materialized_values);
}

/* Original [0005fb00,0005fb78). Select the outermost numbered invariant
   computation in each subtree; descend only when the current value is not
   selected. Keep the right-child tail traversal. */
void __fastcall collect_selected_region_values(Node *node)
{
    unsigned shape;
    for (;;) {
        shape = node_shapes[node->opcode];
        if ((shape & NODE_VALUE_EXPRESSION) && (shape & NODE_ARITY) != NODE_LEAF &&
            bitset_contains(hoist_invariant_values, (short)node->info.words.identifier)) {
            hoist_selected_values = bitset_include(hoist_selected_values,
                (short)node->info.words.identifier);
            return;
        }
        if ((shape & NODE_ARITY) == NODE_LEAF) return;
        collect_selected_region_values(node->left.node);
        if (!(node_shapes[node->opcode] & NODE_HAS_SECOND)) return;
        node = node->second.node;
    }
}

/* Original [0005fa90,0005fafd). Rewrite children before replacing a
   selected numbered expression with its materialized region value. */
Node *__fastcall rewrite_selected_region_values(Node *node)
{
    if ((node_shapes[node->opcode] & NODE_ARITY) == NODE_LEAF) return node;
    node->left.node = rewrite_selected_region_values(node->left.node);
    if (node_shapes[node->opcode] & NODE_HAS_SECOND)
        node->second.node = rewrite_selected_region_values(node->second.node);
    if ((node_shapes[node->opcode] & NODE_VALUE_EXPRESSION) && node->info.words.identifier &&
        bitset_contains(hoist_selected_values, (short)node->info.words.identifier))
        node = materialize_region_value(node);
    return node;
}


/* Original0007bb84: candidate bulk memory operations, arena4/40bytes. */
static RegionMemoryOperation *region_memory_operations;
/* Original [0005e430,0005e67d). Recognize repeated stores/copies with a
   usable loop count, exclude intervening effects and replace each candidate by
   the target bulk-memory operation while retaining original use accounting. */
void reduce_region_induction_strength(void)
{
    WorkItem *work;
    Expression *expression;
    RegionMemoryOperation *operation;
    BasicInduction *destination, *source;
    FunctionSymbol *symbol;
    Node *node, *value;
    unsigned bytes, opcode, reverse_destination, reverse_source, type;
    if (!region_trip_count || !supports_region_memory_operations()) return;
    region_memory_operations = 0;
    for (work = hoist_first_work; work != hoist_after_work; work = work->next) {
        if (!work_dominates_region_paths(work)) continue;
        for (expression = work->body.expressions; expression; expression = expression->next) {
            node = expression->tree;
            if (node->opcode != 0x32) continue;
            symbol = (FunctionSymbol *)node->right.symbol;
            if ((node->attributes.words.flags & 1) && symbol && symbol->symbol.kind != 0x13 &&
                (symbol->field24 & 15) == 9)
                bytes = (symbol->field24 & 0x10000) ? symbol->field2c : symbol->symbol.value;
            else bytes = node->attributes.words.type & 255;
            destination = find_region_memory_induction(node->left.node, node, bytes, &reverse_destination);
            if (!destination) continue;
            source = 0; /* This slot is unused for fills; original stack contents are indeterminate. */
            if (is_region_invariant_value(node->second.node)) opcode = 0x97;
            else {
                value = node->second.node;
                if (value->opcode != 0x30) continue;
                source = find_region_memory_induction(value->left.node, node, bytes, &reverse_source);
                if (!source || reverse_source != reverse_destination) continue;
                opcode = 0x96;
            }
            operation = arena_alloc(4, sizeof(RegionMemoryOperation));
            operation->assignment = node;
            operation->reverse = reverse_destination;
            operation->opcode = opcode;
            operation->bytes = bytes;
            operation->destination = destination;
            operation->source = source;
            operation->original_assignment = node;
            operation->value = node->second.node;
            operation->expression = expression;
            operation->next = region_memory_operations;
            region_memory_operations = operation;
        }
    }
    for (work = hoist_first_work; work != hoist_after_work; work = work->next)
        for (expression = work->body.expressions; expression; expression = expression->next)
            exclude_region_memory_operations(expression->tree);
    if (!region_memory_operations) return;
    create_region_induction_counter();
    for (operation = region_memory_operations; operation; operation = operation->next) {
        node = operation->assignment;
        if (operation->reverse) node->attributes.words.flags |= 0x80;
        --operation->destination->total_uses;
        value = node->second.node;
        if (operation->opcode == 0x96) {
            value = value->left.node;
            --operation->source->total_uses;
        } else {
            type = (value->attributes.words.type & 0xff00) | operation->bytes;
            if (type != value->attributes.words.type) value = integer_node(value->left.value, type);
        }
        insert_region_memory_operation(transformation_cursor.expression, value, node->left.node,
            operation->opcode, region_induction_counter, operation->bytes, node);
        release_node_children(node);
        node->opcode = 1;
    }
}

/* Original [0005e350,0005e42d). Rewrite induction uses, eliminate singly
   used basic updates, and replace a remaining terminal induction by the
   prepared trip counter when its type and liveness permit it. */
void rewrite_region_inductions(void)
{
    WorkItem *work;
    Expression *expression;
    BasicInduction *induction, *next;
    Node *comparison, *count;
    for (work = hoist_first_work; work != hoist_after_work; work = work->next)
        for (expression = work->body.expressions; expression; expression = expression->next)
            rewrite_region_induction_tree(expression->tree);
    for (induction = basic_region_inductions; induction; induction = next) {
        next = induction->next;
        if (induction->total_uses == 1) remove_basic_induction(induction);
    }
    if (!region_exit_branch || (!region_trip_count && !multiple_exit_trip_count)) return;
    comparison = region_exit_branch->left.node;
    induction = find_basic_induction_load(comparison->left.node);
    if (!induction) return;
    if (basic_induction_increment(induction) == -1 && is_constant_value(comparison->second.node, 0)) return;
    count = region_trip_count ? region_trip_count : multiple_exit_trip_count;
    if ((count->attributes.words.type & 0x3000) || induction->total_uses != 2) return;
    if (region_trip_count || induction_is_dead_on_exit(induction->assignment))
        rewrite_region_exit_counter(count);
    if (induction->total_uses == 2) remove_basic_induction(induction);
}

/* Original [00061080,000611ea). Redirect a constant-bound comparison to
   a compatible derived induction, adjusting its bound and comparison direction
   only when the separate representability/termination check succeeds. */
void __fastcall rewrite_region_induction_tree(Node *node)
{
    Node *comparison, *bound, *old_value, *adjustment;
    BasicInduction *basic, *candidate;
    unsigned candidate_type;
    if (node->opcode != 0x38) return;
    comparison = node->left.node;
    bound = comparison->second.node;
    old_value = comparison->left.node;
    if (bound->opcode != 0x33) return;
    basic = find_basic_induction_load(old_value);
    if (!basic || basic->parent || basic->step->opcode != 0x33) return;
    for (candidate = basic_region_inductions; candidate; candidate = candidate->next) {
        if (candidate->parent != basic) continue;
        candidate_type = candidate->assignment->attributes.words.type;
        if ((basic->assignment->attributes.words.type & 255U) < (candidate_type & 255U) ||
            (candidate_type & 0x3000) || candidate->total_uses < 2 || candidate->step->opcode != 0x33)
            continue;
        adjustment = optimize_tree_global(build_tree("DDNSTDNYNY", candidate->left,
            basic->assignment->left.node, 0x30, basic->assignment, candidate->right, 4, 3));
        if (can_rewrite_induction_comparison(candidate, bound, adjustment)) break;
        release_tree(adjustment);
    }
    if (!candidate) return;
    --basic->total_uses;
    ++candidate->total_uses;
    release_tree(old_value);
    comparison->left.node = build_tree("DNST", candidate->assignment->left.node,
        0x30, candidate->assignment);
    comparison->second.node = optimize_tree_global(build_tree("PDNYPNY", bound,
        candidate->right, 4, adjustment, 2));
    comparison->attributes.words.type = candidate->assignment->attributes.words.type;
    if (basic->assignment->second.node->opcode != candidate->assignment->second.node->opcode ||
        (int)(candidate->step->left.value ^ basic->step->left.value) < 0)
        comparison->opcode = map_comparison_opcode(comparison->opcode, 0);
}

/* Original [0005f5c0,0005f5f3). Transparent conversions may surround a
   basic induction load; reject a narrowing conversion except pointer type404. */
BasicInduction *__fastcall find_basic_induction_load(Node *node)
{
    Node *child;
    while (node->opcode == 0x34) {
        child = node->left.node;
        if ((child->attributes.words.type & 255) > (node->attributes.words.type & 255) &&
            node->attributes.words.type != 0x404) return 0;
        node = child;
    }
    if (node->opcode == 0x30) return find_basic_induction(node->left.node);
    return 0;
}

/* Original [0005e1d0,0005e34c). A region containing only control flow and
   at most one basic update may be removed or simplified, provided every exit
   reaches the same later block and no directive or effect must be retained. */
int simplify_region_exit(void)
{
    BasicInduction *basic = basic_region_inductions;
    WorkItem *work, *exit = 0;
    Expression *expression;
    FlowLink *link;
    Node *node;
    if (basic && basic->next) return 0;
    for (work = hoist_first_work; work != hoist_after_work; work = work->next) {
        for (expression = work->body.expressions; expression; expression = expression->next) {
            node = expression->tree;
            switch (node->opcode) {
            case 1:
            case 0x29:
                break;
            case 0x38:
                if (!basic || !find_basic_induction_load(node->left.node->left.node) ||
                    !can_repeat_tree(node)) return 0;
                /* Fall through: conditional and unconditional exits share checks. */
            case 0x3a:
                for (link = work->successors; link; link = link->next) {
                    if (link->work->number < hoist_first_work->number) return 0;
                    if (link->work->number > hoist_last_work->number) {
                        if (exit && exit != link->work) return 0;
                        exit = link->work;
                    }
                }
                break;
            case 0x4f:
                if (node->detail.control.code == 0x16) return 0;
                break;
            default:
                if (!basic || basic->assignment != node) return 0;
                break;
            }
        }
    }
    if (basic && (basic->parent || region_trip_count)) remove_basic_induction(basic);
    if (!basic_region_inductions) remove_region_blocks();
    return 1;
}

/* Original [0005e1c0,0005e1c1): intentionally empty phase hook. */
void finish_region_motion(void)
{
}

/* Original [0005e170,0005e1be). Rebuild the insertion preheader's memory
   sets using the enclosing region's saved reference table. */
void rebuild_region_header_references(void)
{
    WorkItem *work = hoist_entry_work->next;
    unsigned i;
    for (i = 0; i < 6; ++i) {
        release_bitset(work->flow_sets[i]);
        work->flow_sets[i] = 0;
    }
    summarize_work_memory(work);
    for (i = 0; i < WORK_REFERENCE_BUCKETS; ++i)
        work_references.buckets[i] = hoist_current_region->references->buckets[i];
    analyze_block_references(work);
}

/* Original [0005e0b0,0005e16d). Release region-owned induction records
   and loop trees, then free the three count trees and reset reference caches. */
void release_region_motion(void)
{
    BasicInduction *basic, *next_basic;
    LoopTreeRecord *tree, *next_tree;
    for (basic = basic_region_inductions; basic; basic = next_basic) {
        next_basic = basic->next;
        release_basic_induction(basic);
    }
    basic_region_inductions = 0;
    for (tree = hoist_current_region->loop_trees; tree; tree = next_tree) {
        next_tree = tree->next;
        release_loop_tree_record(tree);
    }
    hoist_current_region->loop_trees = 0;
    if (region_trip_count) release_tree(region_trip_count);
    if (region_induction_counter) release_tree(region_induction_counter);
    if (multiple_exit_trip_count) release_tree(multiple_exit_trip_count);
    multiple_exit_trip_count = 0;
    region_trip_count = 0;
    region_induction_counter = 0;
    free_pending_work_effects = 0;
    clear_work_expression_cache();
    release_bitset(hoist_invariant_values);
    hoist_invariant_values = 0;
}

/* Original [0005fc70,0005fd02). Own one arena-4 basic or derived
   induction record, deep-copying its optional affine left/right terms. */
BasicInduction *__fastcall new_basic_induction(Node *assignment, Node *step,
    WorkItem *work, BasicInduction *parent, Node *left, Node *right)
{
    BasicInduction *induction = arena_alloc(4, sizeof(BasicInduction));
    induction->assignment = assignment;
    induction->step = step;
    induction->dominates_region = work ? work_dominates_region_paths(work) : parent->dominates_region;
    induction->total_uses = induction->derived_uses = 0;
    induction->left = induction->right = 0;
    if (right) induction->right = copy_tree(right);
    if (left) induction->left = copy_tree(left);
    induction->parent = parent;
    induction->next = basic_region_inductions;
    basic_region_inductions = induction;
    return induction;
}

/* Original [0005f400,0005f422). Evaluate all three region constraints
   in order, even when an earlier dominance condition fails. */
int __fastcall work_dominates_region_paths(WorkItem *work)
{
    int result = work_dominates_region_exits(work);
    result &= work_dominates_region_backedges(work);
    result &= work_is_outside_child_regions(work);
    return result;
}

/* Original [0005f510,0005f572). Find an available final store of the
   induction destination in its entry block and return that store's value. */
Node *__fastcall find_induction_initial_value(Node *assignment)
{
    Expression *expression;
    Node *candidate;
    int identifier = assignment->detail.node->info.words.identifier;
    for (expression = hoist_entry_work->body.expressions; expression;
         expression = expression->next) {
        candidate = expression->tree;
        if (candidate->opcode == 0x32 &&
            candidate->detail.node->info.words.identifier == identifier &&
            bitset_contains(hoist_entry_work->flow_sets[WORK_UNREAD_STORES], identifier) &&
            has_no_later_work_store(expression)) return candidate->second.node;
    }
    return 0;
}

/* Original [0005f4d0,0005f503). A subtracting update reverses its
   constant increment, retaining the original32-bit negation behavior. */
int __fastcall basic_induction_increment(BasicInduction *induction)
{
    Node *step = induction->step;
    unsigned value;
    if (step->opcode != 0x33)
        compiler_internal_error("@(#)nglobal.c:1.48", 0xf02);
    value = step->left.value;
    if (induction->assignment->second.node->opcode == 3) value = 0U - value;
    return (int)value;
}

/* Original [0005fd60,0005fe64). Intern an optimized derived tree and
   retain each actual use with its expression/work context. The group owns
   deep tree/factor copies; duplicate uses release their temporary tree. */
void __fastcall add_derived_induction(Node *tree, Node *parent, BasicInduction *basic,
    Node *factor, int negative, int blocked)
{
    Node *value = optimize_tree_global(copy_tree(tree));
    DerivedInduction *candidate;
    DerivedInductionUse *use;
    for (candidate = derived_region_inductions; candidate; candidate = candidate->next)
        if (trees_equal(value, candidate->tree, 1)) break;
    if (!candidate) {
        candidate = arena_alloc(4, sizeof(DerivedInduction));
        use = arena_alloc(4, sizeof(DerivedInductionUse));
        candidate->references = use;
        use->next = 0;
        use->tree = tree;
        use->work = current_expression_work;
        use->expression = work_expression;
        candidate->tree = value;
        candidate->factor = copy_tree(factor);
        candidate->basic = basic;
        candidate->negative = negative;
        candidate->parent = parent;
        candidate->uses = 1;
        candidate->blocked = blocked;
        candidate->next = derived_region_inductions;
        derived_region_inductions = candidate;
    } else {
        use = arena_alloc(4, sizeof(DerivedInductionUse));
        use->next = candidate->references;
        use->tree = tree;
        use->work = current_expression_work;
        use->expression = work_expression;
        candidate->references = use;
        release_tree(value);
        ++candidate->uses;
        candidate->blocked &= blocked;
    }
}

/* Original [0005f910,0005fa86). Materialize a numbered invariant once
   in the region preheader and maintain its load/store liveness. Comparisons
   (original shape bit0010, exactly opcodes1f..24) retain their two operands. */
Node *__fastcall materialize_region_value(Node *node)
{
    unsigned opcode = node->opcode;
    Node *load, *store, *reference;
    int identifier;
    if ((node_shapes[opcode] & NODE_ARITY) == NODE_LEAF ||
        (opcode == 0x30 && node->left.node->opcode == 0x26)) return node;
    if ((node->attributes.words.type & 0x1000) && opcode == 0x30) {
        node->left.node = materialize_region_value(node->left.node);
        return node;
    }
    if ((opcode >= 0x1f && opcode <= 0x24) || opcode == 0x43) {
        node->second.node = materialize_region_value(node->second.node);
        node->left.node = materialize_region_value(node->left.node);
        return node;
    }
    if (opcode == 0x59) {
        node->left.node = materialize_region_value(node->left.node);
        return node;
    }
    load = copy_tree(numbered_expression_temporary(node, 0, 1));
    load->attributes.words.flags |= 0x20;
    load->info.words.identifier = node->info.words.identifier;
    if (!bitset_contains(hoist_materialized_values, node->info.words.identifier)) {
        store = build_tree("PPNTS", load->left.node, node, 0x32, node);
        store->attributes.words.flags |= 0x20;
        insert_tree_expression(transformation_cursor.expression, 0, store);
        hoist_materialized_values = bitset_include(hoist_materialized_values, node->info.words.identifier);
        reference = load->detail.node;
        store->detail.node = reference;
        record_hoisted_work_effect(store);
        identifier = reference->info.words.identifier;
        current_expression_work->flow_sets[WORK_MEMORY_READS] =
            bitset_include(current_expression_work->flow_sets[WORK_MEMORY_READS], identifier);
        hoist_header_work->flow_sets[WORK_MEMORY_STORES] =
            bitset_include(hoist_header_work->flow_sets[WORK_MEMORY_STORES], identifier);
        hoist_header_work->flow_sets[WORK_UNREAD_STORES] =
            bitset_include(hoist_header_work->flow_sets[WORK_UNREAD_STORES], identifier);
        hoist_header_work->flow_sets[WORK_MEMORY_LIVE_OUT] =
            bitset_include(hoist_header_work->flow_sets[WORK_MEMORY_LIVE_OUT], identifier);
    }
    return copy_tree(load);
}

void __fastcall record_work_effect_in_regions(Node *node, FlowRegion *region);
/* Original [0005fb80,0005fb92). A preheader insertion changes the
   enclosing region's effects, leaving its own loop's candidate set separate. */
void __fastcall record_hoisted_work_effect(Node *node)
{
    FlowRegion *region = hoist_entry_work->region;
    if (region) record_work_effect_in_regions(node, region);
}

void __fastcall add_region_candidate(Node *node, FlowRegion *region);
/* Original [0005fc40,0005fc5d). Record the effect in every enclosing
   region, preserving its independent candidate chain. */
void __fastcall record_work_effect_in_regions(Node *node, FlowRegion *region)
{
    for (; region; region = region->parent) add_region_candidate(node, region);
}
