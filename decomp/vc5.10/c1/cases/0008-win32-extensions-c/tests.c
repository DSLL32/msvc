#include <recon-tool.h>
#include <recon.h>
extern int c1_test_assembly_operand_storage(void);
extern int c1_test_assembly_casefold(void);
extern int c1_test_assembly_register_storage(void);
extern int c1_test_assembly_reference_flags(void);
extern int c1_test_assembly_address_storage(void);
extern int c1_test_assembly_mov_record(void);
const ReconTest assembly_tests[6] = {
 { "assembler operand initialization preserves gaps and adjacent records", 1,
   c1_test_assembly_operand_storage },
 { "spelling comparator preserves ASCII fold and signed raw difference", 1,
   c1_test_assembly_casefold },
 { "register builder writes only the established expression and operand bytes", 1,
   c1_test_assembly_register_storage },
 { "reference marking saturates the count and replaces only the epoch bit", 1,
   c1_test_assembly_reference_flags },
 { "base and index addressing preserve gaps and accumulate displacement", 1,
   c1_test_assembly_address_storage },
 { "MOV register/address encoding builds the exact counted record prefix", 1,
   c1_test_assembly_mov_record }
};
