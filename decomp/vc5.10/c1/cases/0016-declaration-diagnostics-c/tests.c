#include <string.h>
#include <recon-tool.h>
#include <recon.h>
extern int __cdecl format_buffer(char *, const char *, unsigned, ...);
static int diagnostic_format_extent(void)
{
    struct { unsigned before; char text[64]; unsigned after; } owned;
    unsigned i;
    int count;
    memset(&owned, 0xa5, sizeof(owned));
    count = format_buffer(owned.text, "%$*/%$L/%d", sizeof(owned.text), 76, 127, 116, 17);
    if (count != 17 || strcmp(owned.text, ";/initializing/17") ||
        owned.before != 0xa5a5a5a5U || owned.after != 0xa5a5a5a5U) return 0;
    for (i = 18; i != sizeof(owned.text); ++i) if ((unsigned char)owned.text[i] != 0xa5) return 0;
    memset(&owned, 0xa5, sizeof(owned));
    count = format_buffer(owned.text, "%$L", 8, 116);
    if (count != 8 || memcmp(owned.text, "init...", 8) ||
        owned.before != 0xa5a5a5a5U || owned.after != 0xa5a5a5a5U) return 0;
    for (i = 8; i != sizeof(owned.text); ++i) if ((unsigned char)owned.text[i] != 0xa5) return 0;
    return 1;
}
const ReconTest diagnostic_tests[1] = {
    { "diagnostic selectors consume their actual varargs and respect output bounds", 1, diagnostic_format_extent }
};
