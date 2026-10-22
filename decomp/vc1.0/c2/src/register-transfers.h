#ifndef REGISTER_TRANSFERS_H
#define REGISTER_TRANSFERS_H
#include "global-registers.h"
#include <recon.h>
/* Arena-one indices created only for this transfer-insertion pass. */
typedef struct AllocationBinding {
    AllocationValue *value;
    AllocationRange *range;
} AllocationBinding;
typedef char AllocationBindingSizeCheck[sizeof(AllocationBinding) == 8 ? 1 : -1];
extern AllocationBinding *allocation_register_bindings;
extern WorkItem **allocation_definition_blocks;
void __fastcall emit_global_register_transfer(unsigned operation, unsigned number,
    Node *value, WorkItem *work, int placement);
void __fastcall split_register_transfer_edge(AllocationRange *range, Node *value,
    WorkItem *work, FlowLink *edge);
void __fastcall mark_register_definition_ends(WorkItem *work, AllocationValue *value,
    AllocationRange *range);
int __fastcall tree_uses_virtual_register(unsigned number, Node *tree);
int __fastcall get_stack_address(Node *address, unsigned *offset, SymbolHeader **symbol);
int __fastcall is_parameter_memory_value(Node *node);
Node *__fastcall register_memory_address(unsigned short number);
/* want_range selects the range pointer; zero selects its owning value. */
void *__fastcall find_register_allocation(unsigned short number, int want_range);
#endif
