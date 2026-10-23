#include "binding.h"
int main(void)
{
    BindingLink cursor;
    BindingNode *ordinary;
    int count = 0, sum = 0;
    if (binding_initialize() != BINDING_BIAS) return 1;
    cursor = binding_table.first;
    while (cursor) {
        ordinary = (BindingNode *)cursor;
        sum += binding_table.dispatch(ordinary, binding_request);
        ++count;
        cursor = cursor->next;
    }
    if (count != 2 || sum != 70) return 2;
    if (binding_table.last_code != binding_high ||
        (unsigned)binding_table.last_code != 0x80000010U) return 3;
    if (binding_compact.marker != 'B' || binding_compact.count != 2 ||
        binding_compact.owner != binding_table.first + 1) return 4;
    if (binding_table.first->code != binding_negative ||
        binding_compact.owner->code != binding_wide) return 5;
    return binding_table.caption[0] != 'P';
}
