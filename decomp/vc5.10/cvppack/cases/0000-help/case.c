#include <recon-tool.h>
extern int cvpack_native_setup(int,const char *);
extern int cvpack_main(int,char **,char **);
static int setup(void) { return cvpack_native_setup(0,"S:\\vc5.10\\cvppack\\captures\\f0a0df0d3064-1792755221726656475\\CVPACK.EXE"); }
static int invoke(int argc,char **argv) { return cvpack_main(argc,argv,(char **)0); }
static const uint32_t order[] = {
    0x4620, /* cvpack_main */
    0x7680, /* initialize_messages */
    0x48c0, /* parse_options */
    0x54d0, /* fatal_error */
    0x49e0, /* print_banner */
    0x7610, /* message_text */
};
RECON_EXPORT const ReconCase case_descriptor = {
 .abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),
 .name="help",.description="Usage output",
 .first_entry_order=order,.first_entry_count=RECON_COUNT(order),
 .execution=RECON_IO,.terminal=RECON_EXIT,.frontier_rva=0,
 .setup=setup,.io=invoke
};
