No application functions have been reconstructed yet. Source belongs here after
the original compiler and initial capture/scaffold plan are verified. Native
tests use GCC's i386 ABI by explicit user direction; the Watcom Win32 build
retains and audits the original ABI. Native observers compare semantic formal
values and ownership against the original's actual register/stack formals.
Do not add per-function ABI veneers or claim raw register equivalence. Each
new C/header file must include `<recon.h>` last in its include list. Preserve
binary-proven application names, signatures and edges; the newer Open Watcom
source is a reference, not proof that its graph matches this 1994 executable.
