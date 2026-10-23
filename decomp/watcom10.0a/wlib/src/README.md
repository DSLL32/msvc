`main.c` reconstructs the actual 1994 application main; `memory.c` retains its
empty, one-byte original CheckMemoryHook. Both are accepted through the matching
InitMsg frontier. Later named INT3 frontiers remain in `stubs.c`; reached CRT
behavior uses the reviewed shared first-call and startup profiles. CRT bodies stay in the library/shared compatibility layer.

Native tests use GCC's i386 ABI by explicit user direction; the original Watcom
Win32 build retains and audits historical ABIs. Native observers compare
semantic formal values and owned data, without per-function veneers or raw
register equivalence. Include `<recon.h>` last in every reconstructed C/header
include list. Preserve binary-proven names, signatures and function edges;
the newer Open Watcom source is a reference, not proof of the 1994 graph.
