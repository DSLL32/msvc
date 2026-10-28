#include <recon-tool.h>
#include <recon.h>
extern int c1_test_nested_record_definition_write_extent(void);
extern int c1_test_symbol_copy_storage(void);
extern int c1_test_simple_function_type_composition(void);
const ReconTest callback_tests[3] = {
 { "nested record definitions retain parent, next frame and member-allocation neighbors", 1,
   c1_test_nested_record_definition_write_extent },
 { "static and member symbol copies preserve opaque storage and owned neighbors", 1, c1_test_symbol_copy_storage },
 { "simple function composition selects the prototype and preserves all input storage", 1, c1_test_simple_function_type_composition }
};
