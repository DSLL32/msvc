/* Public generated registry boundary; only defined slots are consumed. */
#include "bulk-registry.h"
unsigned registry_slot_00000 = 3;
unsigned registry_slot_32768 = 5;
unsigned registry_slot_65450 = 7;
unsigned registry_slot_65999 = 11;
static unsigned selected_checksum(void)
{
    unsigned sum = registry_slot_00000;
    sum = sum * 3 + registry_slot_32768;
    sum = sum * 3 + registry_slot_65450;
    return sum * 3 + registry_slot_65999;
}
int main(void)
{
    return selected_checksum() != 158;
}
