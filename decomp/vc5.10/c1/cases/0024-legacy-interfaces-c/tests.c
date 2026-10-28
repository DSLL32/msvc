#include <recon-tool.h>
#include <recon.h>
extern int c1_test_symbol_copy_storage(void);
const ReconTest legacy_tests[1] = {
 {"symbol copies retain complete function/member/static storage and destination neighbors", 1, c1_test_symbol_copy_storage}
};
