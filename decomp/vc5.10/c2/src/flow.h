#ifndef C2_FLOW_H
#define C2_FLOW_H
#include "function-reader.h"
#include <recon.h>

extern int flow_block_count;
C2FlowGraph *__fastcall new_flow(int pool);
void __fastcall initialize_flow_pool(C2FlowGraph *flow, int pool,
                                     unsigned bytes, unsigned count);
C2BasicBlock *__fastcall prepend_flow_block(C2FlowGraph *flow);
C2BasicBlock *__fastcall allocate_flow_block(C2FlowGraph *flow);
C2BasicBlock *__fastcall insert_flow_block(C2FlowGraph *flow, C2BasicBlock *after);
C2BasicBlock *__fastcall create_function_block(C2Function *function,
    C2BasicBlock *after, C2Record *first, C2Record *last);
#endif
