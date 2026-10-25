#include "flow.h"
#include <recon.h>

int flow_block_count;

void __fastcall create_function_flow(C2Function *function, C2Record *first,
                                     C2Record *last, int pool)
{
    flow_block_count = 0;
    function->flow = new_flow(pool);
    create_function_block(function, NULL, first, last);
}
