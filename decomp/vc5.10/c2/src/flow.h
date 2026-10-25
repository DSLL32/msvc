#ifndef C2_FLOW_H
#define C2_FLOW_H
#include "function-reader.h"
#include <recon.h>

/* Only the encountered target pointer is modeled here; preceding fields remain
 * unnamed until their actual producer or consumer is reconstructed. */
struct C2BranchTarget {
    unsigned char pending00[20];
    C2BasicBlock *block;
};
struct C2FlowEdge {
    C2FlowEdge *next_outgoing, *next_incoming;
    C2BasicBlock *source, *destination;
    unsigned flags;
};
typedef char C2FlowEdgeSizeCheck[sizeof(C2FlowEdge) == 20 ? 1 : -1];
typedef char C2FlowEdgeFlagsCheck[offsetof(C2FlowEdge, flags) == 16 ? 1 : -1];
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
#endif
