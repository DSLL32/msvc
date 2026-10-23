#include "binding.h"
char *binding_base;
struct BindingTable binding_table;
struct BindingCompact binding_compact;
enum BindingCode binding_request = binding_ready;

int binding_inspect(const BindingNode *node, enum BindingCode code)
{
    return (int)node->sample + node->coordinate[0] +
           node->coordinate[1] + node->rank + code + BINDING_BIAS;
}
int binding_initialize(void)
{
    BindingLink first = (BindingLink)16;
    BindingLink second = first + 1;
    binding_base = binding_table.arena.bytes;
    first->next = second;
    first->code = binding_negative;
    first->active = 1;
    first->rank = 3;
    first->coordinate[0] = 4;
    first->coordinate[1] = -2;
    first->sample = 2.5;
    second->next = 0;
    second->code = binding_wide;
    second->active = 1;
    second->rank = 5;
    second->coordinate[0] = 6;
    second->coordinate[1] = 1;
    second->sample = 3.5;
    binding_table.first = first;
    binding_table.dispatch = binding_inspect;
    binding_table.last_code = binding_high;
    binding_table.caption = "PDB binding";
    binding_compact.marker = 'B';
    binding_compact.count = 2;
    binding_compact.owner = second;
    return BINDING_BIAS;
}
