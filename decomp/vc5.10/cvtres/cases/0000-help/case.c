#include <recon-tool.h>
extern int main(int, char **, char **);
extern int cvtres_case_setup(void);
static int invoke(int argc, char **argv) { return main(argc, argv, (char **)0); }
static const uint32_t order[] = {
    0x14a5, /* main */
    0x1460, /* usage */
    0x310a, /* load_message */
};
RECON_EXPORT const ReconCase case_descriptor = {
    .abi_version = RECON_ABI_VERSION, .struct_size = sizeof(ReconCase),
    .name = "help",
    .description = "Saved original invocation; exact streams, status, complete artifacts and unseeded App order. Migration only.",
    .first_entry_order = order, .first_entry_count = RECON_COUNT(order),
    .execution = RECON_IO, .terminal = RECON_EXIT,
    .setup = cvtres_case_setup, .io = invoke,
};
