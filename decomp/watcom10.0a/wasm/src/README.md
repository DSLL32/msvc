Reconstructed application source belongs here. No function is reconstructed yet.
Use names and signatures established from the 1994 target, consulting the exact
2002 source checkout recorded in `../cases/help/plan.json`. Preserve applicable
upstream notices when importing source. Include `<recon.h>` last in C/header
include lists. The original Watcom and native i386 builds remain gated pending
the CF-image and CRT-boundary review. Native code may use GCC i386 ABIs;
the Watcom Win32 build retains and audits the original Watcom ABIs.
