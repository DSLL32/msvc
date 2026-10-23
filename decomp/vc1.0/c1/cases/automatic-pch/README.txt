Automatic PCH producer/reuser and macro/conditional invalidation
==============================================================

Original CL /nologo /YXcase.h /Fpstate.pch /Fetest.exe test.c accepts all three
phases. First it produces38026-byte state.pch; the next run reuses that exact
file and prints Using precompiled header. Changing FLAVOR from1 to2 selects a
different main-file conditional branch and regenerates38027-byte state.pch.
All three complete original CL/C2/LINK builds produce a program that exits0
silently. Backend captures/program results are references only, not assignments.

The manifests preserve canonical N:\auto-pch paths, argv/environment and exact
source/header/PCH dependencies. Complete CL working-tree inventories retain
hashes for incidental test.obj/test.exe files; independent C1 replays exclude
those backend files and still reproduce every raw frontend artifact.
Generated PCH input dependencies retain producer-to-reuser-to-changed ordering.

Automatic PCH records the included header's modification time. The initial
independent replay recreated files with new mtimes and changed only that PCH
value. Explicit input mtime_ns metadata now preserves the captured timestamps;
all outputs match raw without normalization. The native writer/API provenance
will be established before accepting the automatic-PCH reconstruction.

C1's status writer targets stderr. Original CL prepare_execution calls dup2(1,2),
so the real CL transport carries the message on stdout. Phase stdio metadata
preserves this alias before launch on original and native sides. The explicit
original-only debugger supports an optional final stderr-to-stdout argument;
evidence/original-debugger-source.c records its complete readable API source
(derived from6aec6e5's preceding C1 debug-run helper). It was built with the
original toolchain privately; receipt hashes identify the helper. Normal native
fixture runs never invoke that debugger, Wine or any original target.

Each of the three frontend phases passes two independent untouched original
runs with pinned mtimes and stdio alias. This family follows prototypes in the
approved queue; source bodies are reconstructed only as their frontiers are met.
