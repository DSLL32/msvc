#include "compiler-state.h"
#include <recon.h>

/* Encountered datafe fields. SYSTEM_INFO starts with the original page and
 * allocation-granularity defaults; GetSystemInfo refreshes it later in main. */
CompilerInitializedState c1_data = {
    1, 0, 0, 0,
    { { 0 }, 4096, NULL, NULL, 0, 0, 0, 65536, 0, 0 }
};
CompilerBss c1_bss;
int c1_reuse_state;

/* All34 actual .initfe slots are represented; unrecovered consumers trap. */
C1Initializer const c1_initialization_callbacks[34] = {
    NULL,
    initialize_state_12365,
    initialize_state_12377,
    initialize_state_12389,
    initialize_state_123a4,
    initialize_state_123a5,
    initialize_state_123a6,
    initialize_state_123a7,
    initialize_state_1241c,
    initialize_state_1246b,
    initialize_state_124ba,
    initialize_state_12509,
    initialize_state_12558,
    initialize_state_125a7,
    initialize_state_125f6,
    initialize_state_12645,
    initialize_state_12646,
    initialize_state_12663,
    initialize_state_12664,
    initialize_state_1266f,
    initialize_state_126af,
    initialize_state_126ef,
    initialize_state_12712,
    initialize_state_1271f,
    initialize_state_1272c,
    initialize_state_12739,
    initialize_state_12746,
    initialize_state_12753,
    initialize_state_12760,
    initialize_state_1276d,
    initialize_state_1277a,
    initialize_state_1277b,
    initialize_state_1279c,
    initialize_state_127a6
};
