#include <recon-tool.h>
#include <recon.h>
extern int c1_test_parameter_append_links(void);
extern int c1_test_suppressed_language_diagnostic(void);
extern int c1_test_symbol_copy_storage(void);
extern int c1_test_declaration_number_encoding(void);
extern int c1_test_static_symbol_map_lookup(void);
const ReconTest declarator_tests[5] = {
 { "parameter append preserves surrounding storage and joins empty, ordinary and sentinel lists", 1,
   c1_test_parameter_append_links },
 { "suppressed language diagnostics preserve the single error-count owner", 1,
   c1_test_suppressed_language_diagnostic },
 { "static symbol copy retains opaque bytes and clears only its binding link", 1,
   c1_test_symbol_copy_storage },
 { "declaration numbers preserve returned cursor and untouched tails at encoding boundaries", 1,
   c1_test_declaration_number_encoding },
 { "static symbol map lookup follows collisions and preserves map storage", 1,
   c1_test_static_symbol_map_lookup }
};
