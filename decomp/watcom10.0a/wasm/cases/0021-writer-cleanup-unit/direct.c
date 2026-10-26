#include <string.h>
#include <recon-tool.h>
#include "direct.h"
#include <recon.h>
void write_fini(void);
#define CHECK(x) do { if (!(x)) return 0; } while (0)
static int cleanup(unsigned count)
{
    fixup anchor,before,*items[65];
    global_options options;
    module_info module;
    unsigned i,j,line;
    memset(&anchor,0,sizeof(anchor));anchor.loc_offset=0x1234;before=anchor;
    options=Options;module=ModuleInfo;line=LineNumber;
    FixInit();
    for(i=0;i<count;++i) {
        items[i]=FixNew();CHECK(items[i] != NULL);
        for(j=0;j<i;++j) CHECK(items[i] != items[j]);
        memset(items[i],0,sizeof(*items[i]));items[i]->next=&anchor;
        items[i]->loc_method=FIX_OFFSET;items[i]->loc_offset=i+1;
    }
    for(i=0;i<count;++i)
        CHECK(items[i]->next == &anchor && items[i]->loc_offset == i+1);
    /* Live records are owned by the carve blocks, which cleanup releases.
     * Never inspect those pointers afterward. Borrowed links remain borrowed. */
    write_fini();
    CHECK(memcmp(&anchor,&before,sizeof(anchor)) == 0);
    CHECK(memcmp(&Options,&options,sizeof(options)) == 0);
    CHECK(memcmp(&ModuleInfo,&module,sizeof(module)) == 0 && LineNumber == line);
    /* FixFini leaves its private handle dangling, as in the original. A new
     * real FixInit replaces that owner before any subsequent allocation. */
    FixInit();items[0]=FixNew();CHECK(items[0] != NULL);
    memset(items[0],0,sizeof(*items[0]));items[0]->loc_offset=77;
    CHECK(items[0]->loc_offset == 77);FixKill(items[0]);write_fini();
    CHECK(memcmp(&anchor,&before,sizeof(anchor)) == 0);
    return 1;
}
int writer_cleanup_unit(void) { return cleanup(65); }
static int empty_owner(void) { return cleanup(0); }
const ReconTest cleanup_tests[]={
    {"empty-fixup-owner-cleanup-and-reinitialize",1,empty_owner},
    {"two-carve-blocks-borrowed-links-and-new-owner",1,writer_cleanup_unit}
};
