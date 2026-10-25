# WCC386, Watcom 10.0a

The target is the CF payload in `binb/WCC386.EXE`. The shared capture backend
uses the separate `binnt` loader. Canonical original addresses are CF offsets.

`make -j4 cases check` builds and runs the native i386 DSO through shared
`recon-tool`; fixtures never start Wine. `make -j4 original` checks application
sources with Watcom 10.0a and the verified patched WLINK through the shared
launcher. `harness/original-host.c` only supplies the executable link host.

The single initial case is `cases/0000-help`, identity
`wcc-first-help-frontier`, with argument `?`. It is **in progress**: `MemAlloc`
(93 original bytes) is reconstructed and fresh original/native evidence agrees
at pending `MemInit`. Five justified startup/support owners receive no App
credit. Total App coverage is unknown until its code extents are mapped.

`harness/startup.c` performs genuine native startup state changes: reverse
stack environment pointers, initializer order, saved selector, FILE flags and
allocation into FILE list nodes. The first private heap allocation precedes
`main`. The compiler's memory manager remains App code. The baseline native
state check and original receipts are recorded with the case; historical raw
receipt compression preserves its bytes. `src/memory.c` adapts the initial
Open Watcom source, retaining its license and checking actual 1994 behavior.

Current captures admit only the five startup owners and `MemAlloc`; unsupported
owners, instructions and services stay stopped. The case has not completed help
output, reached `main`, or exercised the gateway. See `progress.json` for split
timings, input/report hashes and the current frontier.
