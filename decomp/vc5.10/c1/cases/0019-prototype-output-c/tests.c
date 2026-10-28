#include <string.h>
#include "c1.h"
#include <recon-tool.h>
#include <recon.h>
extern const char *__cdecl tag_type_name(WORD);
extern int c1_test_diagnostic_text_growth(void);
static int scalar_type_spellings(void)
{
    static const struct { WORD flags; const char *text; } rows[] = {
        {0x41,"char"}, {0x42,"short"}, {0x43,"int"}, {0x44,"long"},
        {0x4c,"__int64"}, {0x4d,"__int128"}, {0x63,"int"}, {0x4063,"int"},
        {0x106,"float"}, {0x107,"double"}, {0x108,"long double"},
        {0x402,"struct"}, {0x404,"union"}, {0x408,"class"},
        {0x200,"enum"}, {0x80,"void"}, {0x800,"__segment"},
        {0,""}, {0x45,""}, {0x146,""}, {0x4000,""}
    };
    unsigned i;
    for (i = 0; i < RECON_COUNT(rows); ++i)
        if (strcmp(tag_type_name(rows[i].flags), rows[i].text)) return 0;
    return 1;
}
const ReconTest prototype_tests[2] = {
    { "scalar type spellings preserve encoding precedence and unsupported-code fallback", 1,
      scalar_type_spellings },
    { "diagnostic text preserves prefix and append bytes across growth and handle reuse", 1,
      c1_test_diagnostic_text_growth }
};
