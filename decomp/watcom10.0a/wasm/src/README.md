Reconstructed application source belongs here. `main.c` is a trapping entry
stub; no function is accepted yet.
Use names and signatures established from the 1994 target, consulting the exact
2002 source checkout recorded in `../cases/help/plan.json`. Preserve applicable
upstream notices when importing source. Include `<recon.h>` last in C/header
include lists. The original Watcom and native i386 entry-stub builds pass;
fixtures remain gated on CF masking and CRT-boundary review. Native code may
use GCC i386 ABIs; the Watcom Win32 build retains and audits the original ABIs.
