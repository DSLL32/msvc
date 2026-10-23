/* Native-only source-line ownership example, not additional original coverage. */
#define RECON_SHIM_IMPL
#include "pack.h"
#include <recon.h>
#undef main
int main(void)
{
    unsigned char input[7] = { 1, 3, 5, 7, 9, 11, 13 };
    CodeViewDirectoryEntry entry;
    ModuleInfo info;
    memset(&entry, 0, sizeof(entry));
    memset(&info, 0, sizeof(info));
    entry.subsection = 0x127;
    entry.module = info.module = 2;
    entry.size = sizeof(input);
    rewrite_module_global_types(input, &entry, &info);
    if (entry.data == input || entry.data != info.source_lines ||
        entry.size != sizeof(input) || info.source_lines_size != sizeof(input) ||
        memcmp(entry.data, input, sizeof(input)) || info.symbols || info.symbols_size ||
        info.fields_after[0] || info.fields_after[1] || info.next) return 1;
    puts("source-lines copied-all-bytes directory-module-alias");
    return 0;
}
