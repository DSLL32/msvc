#include <recon-tool.h>
#include <recon.h>
extern int c1_test_assembly_member_lookup(void);
extern int c1_test_assembly_offset_write(void);
extern int c1_test_assembly_imul_duplicate(void);
const ReconTest assembly_tests[3] = {
 {"assembly member search retains order and skips the alignment pseudo-member", 1,
  c1_test_assembly_member_lookup},
 {"assembly OFFSET updates the operand form and retains the separate displacement", 1,
  c1_test_assembly_offset_write},
 {"two operand IMUL preserves copied record padding and duplicates the destination", 1,
  c1_test_assembly_imul_duplicate}
};
