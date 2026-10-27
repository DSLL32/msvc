#include "case.h"
#include <recon.h>
extern int c1_test_real_to_float_boundaries(void);
extern int c1_test_float_to_real_overlap(void);
extern int c1_test_extended_subtraction_precision(void);
extern int c1_test_extended_division_order(void);
const ReconTest scalar_tests[4] = {
    { "finite float conversion preserves inputs, range failures, output extent and x87 depth", 1, c1_test_real_to_float_boundaries },
    { "float expansion loads overlapping input before the precision store and preserves neighbors", 1, c1_test_float_to_real_overlap },
    { "extended subtraction selects actual precision and rounds only under the precise option", 1, c1_test_extended_subtraction_precision },
    { "extended division preserves operand order, input bytes, output extent and x87 depth", 1, c1_test_extended_division_order }
};
