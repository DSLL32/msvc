# Pool initialization

The first wrapper126ef is accepted separately (13 bytes). Its genuine target
member receives the object in ECX, unsigned minimum-pages=1 on the stack and
returns with RET4. The actual member remains INT3 for this acceptance. A small
C++ TU expresses the member convention in both VC5 and native i386; its real
caller is in that TU, with C linkage for the callback. No synthetic ABI adapter,
constructor or C++ runtime is introduced.

Four encountered fields are cursor, remaining byte count, minimum page count
and an opaque arena pointer. Consumers238c, aa9c and12b30 establish these roles.
The first object is55078; native storage is16 bytes. The original complete
object extent and any tail remain unknown. All four loaded defaults are zero;
nonzero snapshot-interior sentinels plus91/7 prove that BSS reset clears them.
The initialized6548-byte snapshot and persistent PDB contexts keep their own
previously established lifetimes.

Fresh normal, clear and retain auxiliaries match member126fc, with entry ECX,
stack, exact return-call instruction, argument, RET4, default and reset guards.
The native entry observation occurs before its prologue, independently of DWARF
argument recovery. Each check resolves the receipt-bound immutable ELF. Fresh
pool controls:22/31/31. Generic controller checkpoint:49 normal and102 clear;
retain generic controls were not rerun. That checkpoint ran before a final
pool-only tightening of return-site validation; current positive pairs and all
pool negatives pass afterward. No original reruns were needed for validator
changes. See evidence/pool-wrapper-* for exact sources, checks and timings.

Original captures took1.217/1.368/0.968s; native GDB captures0.213/0.796/0.763s.
Rust graph audit took0.020s. Cached native lifecycle fixture took4.30ms including
setup/execution/checks; none of these native checks invokes Wine.

The genuine member126fc is now accepted separately (22 bytes), reaching12712.
It clears cursor/remaining/arena, stores its unsigned argument and returns this.
Fresh raw entry and actual return observations prove ECX, stack argument,
EAX=receiver, ESP+8 and RET4. Nonzero auxiliaries use owned snapshot interiors
and91/7, with exactly these four original field writes; full original BSS,
initialized snapshot and persistent data comparisons guard adjacent state.
Native before-prologue observation is independent of debug argument decoding.
Fresh pool controls30/42/42 pass, including missing/wrong return, original
neighbor writes, seed identity and the separate post-PDB point. Four generic
controls per pair reran;43/96/96 reuse the identified unchanged checkpoint,
with exact checker/contract/input hashes. These are not counted as fresh.
Original capture times0.917/0.967/1.068s; native0.198/0.831/0.842s. Graph audit
0.016s, cached native lifecycle fixture4.32ms. Both native and original builds
passed; build time was not separately measured. Shared source drift after
capture does not replace the receipt-bound ELF or its archived source inputs.
Durable39 source functions:37 application /1535B,2 CRT /451B,+6B import=1992B.
