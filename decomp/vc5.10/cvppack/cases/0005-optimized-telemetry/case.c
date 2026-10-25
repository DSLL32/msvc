#include <recon-tool.h>
extern int cvpack_native_setup(int,const char *);
extern int cvpack_main(int,char **,char **);
static int setup(void) { return cvpack_native_setup(0,"R:\\toolchains\\5.10\\bin\\CVPACK.EXE"); }
static int invoke(int argc,char **argv) { return cvpack_main(argc,argv,(char **)0); }
static const uint32_t order[] = {
    0x4620, /* _main */
    0x7680, /* _initialize_messages */
    0x48c0, /* _parse_options */
    0x55a0, /* _warning */
};
RECON_EXPORT const ReconCase case_descriptor = {
 .abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),
 .name="optimized-telemetry",.description="Optimized CRT-free telemetry readers and exported global data, with an ignored unknown option",
 .first_entry_order=order,.first_entry_count=RECON_COUNT(order),
 .execution=RECON_IO,.terminal=RECON_FRONTIER,.frontier_rva=0x55a0,.setup=setup,.io=invoke
};
