#include "recon-tool.h"
#include <stdlib.h>
extern int fixture_main(int, char **);
extern int fixture_add(int, int);
static int check_add(void)
{
    return fixture_add(3, 7) == 10 && !getenv("RECON_DIRECT_FAIL");
}
static const uint32_t order[] = {0x1100};
RECON_EXPORT const ReconCase case_descriptor = {RECON_ABI_VERSION,
                                                sizeof(ReconCase),
                                                "0005-direct",
                                                "focused harness fixture",
                                                order,
                                                RECON_COUNT(order),
                                                0,
                                                0,
                                                RECON_DIRECT,
                                                RECON_EXIT,
                                                0,
                                                0,
                                                0,
                                                check_add,
                                                0,
                                                0};
