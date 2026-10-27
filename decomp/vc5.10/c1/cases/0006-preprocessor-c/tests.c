#include <recon-tool.h>
#include <recon.h>
extern int c1_test_diagnostic_text_lifetime(void);
extern int c1_test_include_trailing_space(void);
const ReconTest preprocessor_tests[2] = {
    { "diagnostic text contexts retain nested handles and reuse cleared owned buffers", 1,
      c1_test_diagnostic_text_lifetime },
    { "include trimming respects the explicit end and preserves input storage", 1,
      c1_test_include_trailing_space }
};
