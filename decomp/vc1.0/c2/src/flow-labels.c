/* Label aliases used while global work blocks are partitioned. */
#include "global-work.h"
#include "machine.h"
#include "tree-rewrite.h"
#include "instruction-selection.h"
#include "expression-fields.h"
#include <recon.h>
static FlowLabel *flow_label_buckets[32]; /* 0007bed0: label hash heads */
static FlowLabel *pending_flow_labels; /* 00083774: current alias group */

/* Original [0005abc0,0005abd0). End the current run of adjacent labels. */
void reset_flow_label_state(void)
{
    pending_flow_labels = 0;
}

/* Original [0005abb0,0005abc0): a distinct tail adapter in the original. */
void initialize_flow_labels(void)
{
    reset_flow_labels();
}

/* Original [000595f0,00059610). Label descriptors belong to arena four;
   clear their hash heads before returning all of that arena's allocations. */
void reset_flow_labels(void)
{
    unsigned i;
    for (i = 0; i < 32; ++i) flow_label_buckets[i] = 0;
    reset_arena(4);
}

/* Original [0005ac20,0005ac50). Record each referencing block once on
   the descriptor selected by the label's current alias representative. */
void __fastcall reference_flow_label(SymbolHeader *label, WorkItem *work)
{
    FlowLabel *canonical = lookup_flow_label(label)->canonical;
    FlowLink *reference;
    if (!has_flow_reference(canonical->references, work)) {
        reference = new_flow_link(work);
        reference->next = canonical->references;
        canonical->references = reference;
    }
}

/* Original [0005abd0,0005ac10). Descriptors are keyed by symbol identity;
   initially each descriptor represents its own adjacent-label alias group. */
FlowLabel *__fastcall lookup_flow_label(SymbolHeader *label)
{
    FlowLabel **bucket = &flow_label_buckets[(unsigned)label & 31];
    FlowLabel *record;
    for (record = *bucket; record && record->label != label;
         record = record->hash_next) ;
    if (!record) {
        record = new_flow_label();
        record->label = label;
        record->canonical = record;
        record->hash_next = *bucket;
        *bucket = record;
    }
    return record;
}

/* Original [0005ac10,0005ac20). A distinct allocator tail adapter; the
   arena allocator supplies zeroed storage before descriptors are linked. */
FlowLabel *new_flow_label(void)
{
    return arena_alloc(4, sizeof(FlowLabel));
}

/* Original [0005ac90,0005acb0). Test block identity in a reference chain. */
int __fastcall has_flow_reference(FlowLink *references, WorkItem *work)
{
    while (references && references->work != work) references = references->next;
    return references != 0;
}

/* Original [0005ac50,0005ac90). A recycled link retains its flag byte,
   matching the original free-list allocator; fresh arena storage is zeroed. */
FlowLink *__fastcall new_flow_link(WorkItem *work)
{
    FlowLink *link = free_flow_links;
    if (link) {
        free_flow_links = link->next;
        link->next = 0;
        link->work = work;
        return link;
    }
    link = arena_alloc(2, sizeof(FlowLink));
    link->work = work;
    return link;
}

/* Original [0005ad70,0005aea0). Fold adjacent ordinary labels, including
   intervening non-boundary control markers, while retaining address-taken
   labels and protected label pairs as separate expression records. */
Expression *__fastcall fold_adjacent_flow_labels(Expression *expression)
{
    Node *node = expression->tree, *following_node;
    Expression *following;
    FlowLabel *canonical;
    SymbolHeader *label;
    short code;
    if (((LabelSymbol *)node->right.symbol)->label_kind & 4) return expression;
    following = next_flow_expression();
    following_node = following->tree;
    switch (following_node->opcode) {
    case 0x29:
        if (((LabelSymbol *)following_node->right.symbol)->label_kind & 4) break;
        if (is_boundary_flow_label(node) && is_boundary_flow_label(following_node)) break;
        following = fold_adjacent_flow_labels(following);
        goto coalesce;
    case 0x3a:
        define_flow_label(following_node->right.symbol, 0);
coalesce:
        canonical = pending_flow_labels;
        while (canonical->canonical != canonical) canonical = canonical->canonical;
        label = expression->tree->right.symbol;
        if (canonical->label == label) {
            putback_flow_expression(following);
            pending_flow_labels = 0;
            define_flow_label(label, 0);
            return expression;
        }
        define_flow_label(label, 0);
        release_tree(expression->tree);
        release_expression(expression);
        return following;
    case 0x4f:
        code = following_node->detail.control.code;
        if (code == 0x16 || (code >= 0x19 && code <= 0x1a) ||
            (code >= 0x26 && code <= 0x2d)) break;
        expression = fold_adjacent_flow_labels(expression);
        putback_flow_expression(following);
        return expression;
    }
    putback_flow_expression(following);
    define_flow_label(expression->tree->right.symbol, 0);
    return expression;
}

/* Original [0005acb0,0005ad10). Coalesce adjacent label descriptors and
   transfer their pending block references to the first descriptor's list. */
void __fastcall define_flow_label(SymbolHeader *label, WorkItem *work)
{
    FlowLabel *record = lookup_flow_label(label);
    FlowLink *tail;
    ((LabelSymbol *)label)->references = 0;
    if (!pending_flow_labels) pending_flow_labels = record;
    else if (record != pending_flow_labels) {
        if (record->references) {
            tail = pending_flow_labels->references;
            if (tail) {
                while (tail->next) tail = tail->next;
                tail->next = record->references;
            } else pending_flow_labels->references = record->references;
            record->references = 0;
        }
        record->canonical = pending_flow_labels;
    }
    if (work) record->work = work;
}

/* Original [0005a8e0,0005a910). Normalize every descriptor after all
   expressions have been partitioned into work blocks. */
void resolve_flow_labels(void)
{
    unsigned i;
    FlowLabel *record;
    for (i = 0; i < 32; ++i)
        for (record = flow_label_buckets[i]; record; record = record->hash_next)
            canonical_flow_label(record);
}

/* Original [0005ad10,0005ad60). Compress alias chains and move outstanding
   references to the representative without changing their traversal order. */
FlowLabel *__fastcall canonical_flow_label(FlowLabel *record)
{
    FlowLabel *canonical;
    FlowLink *tail;
    if (record == record->canonical) return record;
    canonical = canonical_flow_label(record->canonical);
    record->canonical = canonical;
    tail = canonical->references;
    if (!tail) canonical->references = record->references;
    else {
        while (tail->next) tail = tail->next;
        tail->next = record->references;
    }
    record->references = 0;
    return canonical;
}

/* Original [00058b60,00058b80). A newly inserted flow label starts
   with one reference for the branch which caused its creation. */
Node *new_flow_label_node(void)
{
    Node *node = new_node(0x29);
    LabelSymbol *label = new_internal_label();
    node->right.symbol = &label->symbol;
    label->references = 1;
    return node;
}

unsigned next_symbol_identifier(void);
/* Original [0004e3d0,0004e3fe). Allocate an internal local label and give
   it the same unique identifier/scope registration as other local symbols. */
LabelSymbol *new_internal_label(void)
{
    LabelSymbol *label = allocate_descriptor(3, 1);
    label->symbol.field0d = 0x20;
    label->symbol.identifier = next_symbol_identifier();
    insert_symbol(&label->symbol, 1);
    return label;
}
