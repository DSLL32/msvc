#ifndef C2_FLOW_H
#define C2_FLOW_H
#include "function-reader.h"
#include <recon.h>

/* Only the encountered target pointer is modeled here; preceding fields remain
 * unnamed until their actual producer or consumer is reconstructed. */
struct C2BranchTarget {
    union {
        unsigned char pending00[20];
        struct {
            unsigned char pending00_before_kind[8];
            unsigned char kind, pending09[3];
            C2BranchTarget *previous;
            unsigned char pending10[4];
        };
    };
    C2BasicBlock *block;
};
typedef char C2BranchTargetKindCheck[offsetof(C2BranchTarget, kind) == 8 ? 1 : -1];
typedef char C2BranchTargetPreviousCheck[offsetof(C2BranchTarget, previous) == 12 ? 1 : -1];
struct C2FlowEdge {
    C2FlowEdge *next_outgoing, *next_incoming;
    C2BasicBlock *source, *destination;
    unsigned flags;
};
typedef char C2FlowEdgeSizeCheck[sizeof(C2FlowEdge) == 20 ? 1 : -1];
typedef char C2FlowEdgeFlagsCheck[offsetof(C2FlowEdge, flags) == 16 ? 1 : -1];
/* Encountered region prefix. Its allocator requests 64 bytes; fields beyond
 * this prefix remain unmodeled until their own consumers are reached. */
typedef struct C2FlowBlockLink {
    C2BasicBlock *block;
    struct C2FlowBlockLink *next;
    unsigned pending08; /* Actual constructor allocates12B; this tail is unmodeled. */
} C2FlowBlockLink;
typedef char C2FlowBlockLinkSizeCheck[sizeof(C2FlowBlockLink) == 12 ? 1 : -1];
struct C2FlowRegion {
    C2FlowRegion *next, *children, *parent;
    C2BasicBlock *entry;
    union { unsigned pending10; C2BasicBlock *rewrite_entry; };
    C2BasicBlock *header, *last;
    unsigned pending1c;
    union { unsigned pending20; C2FlowBlockLink *exits; };
    unsigned flags;
    union { unsigned char pending28[12];
        struct { unsigned pending28_word, pending2c_word; C2FlowRegion *allocation_next; };
        /* Counted-region analysis reuses the same first two DWORDs. */
        struct { C2Expression *iteration_count, *iteration_comparison;
            union { unsigned pending30; struct C2OptimizerRecordLink *initial_updates; }; }; };
    C2RegisterSet *global_registers; /* Actual consumer at region+52. */
    C2RegisterSet *allocation_eligible; /* Actual allocator writes region+56. */
    struct C2OptimizerRecordLink *assignment_copies; /* Region motion's copied entry assignments, +60. */
};
typedef char C2FlowRegionInitialUpdatesOffset[
    offsetof(C2FlowRegion, initial_updates) == 48 ? 1 : -1];
typedef char C2FlowRegionCopiesOffset[offsetof(C2FlowRegion, assignment_copies) == 60 ? 1 : -1];
typedef char C2FlowRegionAllocationSize[sizeof(C2FlowRegion) == 64 ? 1 : -1];
extern int flow_block_count;
C2FlowGraph *__fastcall new_flow(int pool);
void __fastcall initialize_flow_pool(C2FlowGraph *flow, int pool,
                                     unsigned bytes, unsigned count);
C2BasicBlock *__fastcall prepend_flow_block(C2FlowGraph *flow);
C2BasicBlock *__fastcall allocate_flow_block(C2FlowGraph *flow);
C2BasicBlock *__fastcall insert_flow_block(C2FlowGraph *flow, C2BasicBlock *after);
C2BasicBlock *__fastcall create_function_block(C2Function *function,
    C2BasicBlock *after, C2Record *first, C2Record *last);
C2BasicBlock *__fastcall split_function_block(C2Function *function,
    C2BasicBlock *block, C2Record *first);
void __fastcall prepend_input_record(C2Record *before, C2Record *record);
void __fastcall normalize_line_record(C2Record *record);
void __fastcall move_input_record_before(C2Record *before, C2Record *record);
int __fastcall record_closes_scope(C2Record *record);
C2FlowEdge *__fastcall new_function_edge(C2Function *function,
    C2BasicBlock *source, C2BasicBlock *destination);
C2FlowEdge *__fastcall new_flow_edge(C2FlowGraph *flow,
    C2BasicBlock *source, C2BasicBlock *destination);
void __fastcall remove_function_edge(C2BasicBlock *source,
    C2BasicBlock *destination, C2Function *function);
void __fastcall rebuild_branch_target_edges(C2Function *function,
    C2BasicBlock *source, C2Expression *targets);
void __fastcall release_flow_edge(C2FlowGraph *flow, C2FlowEdge *edge);
void __fastcall recycle_flow_block(C2FlowGraph *flow, C2BasicBlock *block);
#endif
