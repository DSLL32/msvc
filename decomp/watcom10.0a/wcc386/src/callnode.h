#ifndef WCC_CALLNODE_H
#define WCC_CALLNODE_H
#include "address.h"
#include <recon.h>
/* Old parameter nodes have no later alignment/offset fields. AssgnParms
 * consumes all five fields at the exact0,4,8,12,16 byte offsets. */
struct parm_node {
    struct parm_node *next;
    struct address_name *name;
    instruction *ins;
    hw_reg_set regs;
    int num;
};
/* AssgnParms establishes the final DWORD as the pushed byte count, distinct
 * from the register/stack assignment cursor in state->parm.offset. */
struct call_node {
    call_state *state;
    struct address_name *name;
    instruction *ins;
    type_def *tipe;
    struct parm_node *parms;
    unsigned stack_bytes;
};
typedef char old_call_node_size[(sizeof(struct call_node)==24)?1:-1];
typedef char old_parm_node_size[(sizeof(struct parm_node)==20)?1:-1];
typedef char old_call_state_request[(sizeof(call_state)==36)?1:-1];
#endif
