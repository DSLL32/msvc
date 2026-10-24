#include "recon-tool.h"
extern int fixture_main(int, char **);
extern int fixture_add(int, int);
static int check_add(void) { return fixture_add(9, 2) == 11; }
static const ReconTest tests[] = {{"add", 1, check_add}, {"pure-add", 0, check_add}};
static const uint32_t order[] = {0x1000, 0x1100};
RECON_EXPORT const ReconCase case_descriptor = {RECON_ABI_VERSION,
                                                sizeof(ReconCase),
                                                "fixture-io",
                                                "standard IO",
                                                order,
                                                RECON_COUNT(order),
                                                0,
                                                0,
                                                RECON_IO,
                                                RECON_EXIT,
                                                0,
                                                0,
                                                fixture_main,
                                                0,
                                                tests,
                                                RECON_COUNT(tests)};
