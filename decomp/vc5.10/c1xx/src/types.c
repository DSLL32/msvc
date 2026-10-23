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
