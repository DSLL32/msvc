# Main and independent initialized state

The C1XX main owns137 hot bytes38bfd..38c86 and27 cold bytes7206f..7208a.
Its real signature is cdecl(int argc,char **argv). The first separate callee is
initialize_work_buffers38c86; all later callees remain named traps, preserving
the original graph. Real GetSystemInfo and DebugBreak imports stay in the source.
Neither is executed before this genuine first helper frontier.

The actual '-C9IL' string at da468 is referenced by option record d65b0,
whose next DWORD is the flag addresscd488. Main reads that flag to set/clear
compiler_flags bit0x2000 atccf7c. Phasecd5c0 is written1 by main and3/5/8 by later
compilation stages. Main stores argc/argv atc1708/c170c and tests c1704 before
DebugBreak. The canonical36-byte SDK SYSTEM_INFO resides atcd7b8. Actual PE and
loaded entry snapshots prove its initial page size4096 and granularity65536,
with the other fields zero. This is a typed SDK object, not an opaque blob.

The new48 initialized semantic bytes comprise12 scalar bytes and36 SDK bytes.
All join the actual8520-byte snapshot allocation's typed copy/restore alongside
the already modeled89 initialized bytes:137 semantic bytes plus51 natural native
record padding, native sizeof188. Other8383 original initialized bytes remain
UNMODELED. The command-line BSS adds12 bytes and gains nonzero reset proof.
Native BSS is1100 bytes:1069 semantic and31 natural padding.

An independent actual main-entry trap and native preprologue breakpoint verify
argc26, argv allocation/vector identity, both actual caller returns and mode1
set by Invoke after initialization. This is not inferred from a post-main helper
snapshot. Canonical defaults or explicitly different live/saved auxiliary values
are checked at pre-initializer, restore phase, main entry, and first helper.
SYSTEM_INFO pointer values use owned allocation/interior identities; auxiliary
pointers never reach an API or dereference before the named helper trap.

Main changes only compiler_phase to1 before the first helper. All other original
8520 initialized bytes and the full63132 BSS bytes remain unchanged. Native
modeled neighbors and the saved snapshot remain unchanged. Actual first-helper
cdecl(void), original return38c0e and native caller identity are independently
observed. Native GDB captures use the compile-once collector with all current
field observations and frozen body/path hashes; no mutable state is cached.
