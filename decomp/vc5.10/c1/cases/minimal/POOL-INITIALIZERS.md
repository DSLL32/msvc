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
