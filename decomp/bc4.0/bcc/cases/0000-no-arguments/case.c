#include <recon-tool.h>
#include <windows.h>
#include <recon.h>
extern int main(int, char **, char **);
extern void initialize_exception_handler(void);
static int invoke(int argc, char **argv)
{
    initialize_exception_handler();
    recon_borland_400_initialize_gp_handler();
    return main(argc, argv, (char **)0);
}
static const uint32_t order[] = { 0x1013b, 0x10176, 0xa2bd0, 0x103ec, 0x10404 };
RECON_EXPORT const ReconCase case_descriptor = {
    .abi_version = RECON_ABI_VERSION, .struct_size = sizeof(ReconCase),
    .name = "no-arguments",
    .description = "No-argument usage and Borland quick-exit output",
    .first_entry_order = order, .first_entry_count = RECON_COUNT(order),
    .execution = RECON_IO, .terminal = RECON_EXIT, .io = invoke
};
