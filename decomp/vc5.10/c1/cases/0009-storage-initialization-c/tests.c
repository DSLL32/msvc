#include <recon-tool.h>
#include <recon.h>
extern int c1_test_segment_symbol_storage(void);
const ReconTest storage_tests[1] = {
 { "segment symbols preserve fresh gaps and clear only their recycled owned span", 1,
   c1_test_segment_symbol_storage }
};
