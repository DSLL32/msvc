/* Injected native controls; parser mocks are not reconstruction credit. */
#include "preprocessor-expressions.h"
#include <recon.h>
#undef write /* This native harness records mock-parser side effects to host stdout. */
extern int write(int descriptor, const void *bytes, unsigned count);
static PreprocessorValue values[3] = { {0, 0}, {0, 11}, {0, 22} };
static int mode, parsed, matched;
PreprocessorValue *parse_preprocessor_logical_or(void)
{
    write(1, "P", 1);
    return &values[parsed++];
}
int __stdcall accept_preprocessor_token(unsigned char token)
{
    ++matched;
    if (matched == 1) return mode != 0;
    return mode >= 3;
}
int main(int argc, char **argv)
{
    PreprocessorValue *result;
    mode = argc > 1 ? argv[1][0] - '0' : 0;
    values[0].value = mode == 2 || mode == 4;
    result = parse_preprocessor_conditional();
    if (mode == 0) return result != &values[0] || parsed != 1 || matched != 1;
    if (mode == 2) return result != &values[1] || parsed != 2 || matched != 2;
    if (mode == 3) return result != &values[2] || parsed != 3 || matched != 2;
    if (mode == 4) return result != &values[1] || parsed != 3 || matched != 2;
    return 3; /* false/missing-colon must trap before returning */
}
