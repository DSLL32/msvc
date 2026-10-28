#include <string.h>
#include <recon-tool.h>
#include <recon.h>
extern const char *__cdecl format_initializer_integer(const unsigned __int64 *, char *, unsigned, int);
static int integer_formatter_edges(void)
{
    static const struct {
        unsigned __int64 value;
        unsigned radix;
        int unsigned_value;
        const char *text;
    } rows[] = {
        {0,10,0,"0"}, {1,10,0,"1"},
        {~(unsigned __int64)0,10,0,"-1"},
        {~(unsigned __int64)0,10,1,"18446744073709551615"},
        {((unsigned __int64)1 << 63),10,0,"-9223372036854775808"},
        {~(unsigned __int64)0,16,0,"ffffffffffffffff"},
        {~(unsigned __int64)0,8,1,"1777777777777777777777"},
        {12345,36,1,"9ix"}, {5,2,1,"101"}
    };
    unsigned i, j;
    for (i = 0; i < RECON_COUNT(rows); ++i) {
        char storage[82];
        unsigned __int64 original = rows[i].value;
        unsigned length = strlen(rows[i].text);
        memset(storage,0xa5,sizeof(storage));
        if (format_initializer_integer(&rows[i].value,storage+1,rows[i].radix,
                rows[i].unsigned_value) != storage+1 ||
            strcmp(storage+1,rows[i].text) || rows[i].value != original ||
            (unsigned char)storage[0] != 0xa5) return 0;
        for (j = length+2; j < sizeof(storage); ++j)
            if ((unsigned char)storage[j] != 0xa5) return 0;
    }
    return 1;
}
const ReconTest conversion_tests[1] = {
    {"integer text covers signed limits, radix digits and untouched output neighbors",1,
      integer_formatter_edges}
};
