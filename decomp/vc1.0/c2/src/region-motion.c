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
