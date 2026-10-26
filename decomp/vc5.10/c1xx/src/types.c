#include "frontend.h"
#include <recon.h>

int initialize_type_ranks(void)
{
    TypeRankMapping *mapping;
    for (mapping = frontend_data.primitive_type_ranks; mapping->code; ++mapping)
        frontend_bss.primitive_type_rank[mapping->code >> 5] = mapping->rank;
    for (mapping = frontend_data.complex_type_ranks; mapping->code; ++mapping)
        frontend_bss.complex_type_rank[mapping->code] = mapping->rank;
    return 1;
}

/* CF000: exact four-bit primitive-kind domain, shared by size and alignment
 * consumers. Values outside this semantic table are not copied. */
const signed char compiler_primitive_type_bytes[16] = {
    1, 1, 2, 4, 4, 2, 4, 8, 8, 1, 2, 4, 8, 16, 0, 0
};
