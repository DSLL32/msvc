/* help: cached untouched-original invocation; migration only.
 * Independent frozen input contract replaces historical live stage handoff.
 * Source capture 1a484599d00716ab9673c588f925c537ed109cab1c1d995c3977299a1f14bde9. */
#define RECON_SHIM_IMPL
#include <sys/stat.h>
#include <errno.h>
#include <recon-tool.h>
#include <recon-output.h>
extern int main(int,char **);
static int invoke(int argc,char **argv){return main(argc,argv);}
extern int link_native_host_setup(void);
static int setup(void){
    if(!link_native_host_setup())return 0;
    if(mkdir("tmp",0700)&&errno!=EEXIST)return 0;
    return 1;
}
static const uint32_t order[] = {
    0x16b40, /* main */
    0x2a750, /* debug_initialize */
    0x2a980, /* debug_enabled */
    0x2c020, /* files_initialize */
    0x2d100, /* new_page */
    0x2c1b0, /* new_file_slot */
    0x7ea0, /* linker_main */
    0xfc60, /* initialize_log */
    0x7000, /* create_image */
    0x16190, /* initialize_symbols */
    0x165d0, /* initialize_hash */
    0x103f0, /* collect_arguments */
    0x10330, /* collect_search_path */
    0x109d0, /* append_argument */
    0x10970, /* append_argument_node */
    0x106e0, /* classify_argument */
    0x83d0, /* process_options */
    0x2a580, /* parse_switch */
    0x10160, /* link_usage */
    0x154f0, /* print_banner */
};
RECON_EXPORT const ReconCase case_descriptor = {
.abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),
.name="help",
.description="Public LINK invocation; cached full status/streams/artifacts and explicitly seeded first-entry suffix",
.first_entry_order=order,.first_entry_count=RECON_COUNT(order),
.first_entry_seen=NULL,.first_entry_seen_count=0,
.execution=RECON_IO,.terminal=RECON_EXIT,.setup=setup,.io=invoke
};

extern int link_fixture_output(const ReconOutputContext *);
RECON_EXPORT const ReconOutputPolicy case_output_policy={
    RECON_OUTPUT_POLICY_VERSION,sizeof(ReconOutputPolicy),link_fixture_output
};
