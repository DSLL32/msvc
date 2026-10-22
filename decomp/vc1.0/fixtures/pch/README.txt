Original-backed paired precompiled-header case for MSVC 1.0 NT C1.

Producer: CL /nologo /c /Od /W3 /Ycpch.h /Fpstate.pch producer.c
Consumer: CL /nologo /Od /W3 /Yupch.h /Fpstate.pch /Fepch.exe consumer.c producer.obj

The untouched driver's help documents /Yc[file], /Yu[file] and /Fp<file>.
The shared header defines macros, an enum, a struct, a function-pointer typedef
and an external declaration. The producer implements that declaration; the
consumer checks macro expansion, struct fields and a call through the typedef.
Both complete original driver runs built a program that exits 0 silently.

The capture helper stops at CL's CreateProcessA import thunk before each child
launch and copies the entire working file set. capture/*/before-1 contains the
C1 inputs; before-2 contains C2 inputs including all C1 outputs; before-3 and
after retain the original LINK inputs and reference final artifacts. These
C2/LINK records are references only, not reconstruction assignments. Object and
executable timestamps are not assigned C1 output oracles.

manifest.json phases.c1-create and phases.c1-use contain explicit single-entry
MSC_CMD_FLAGS environments, empty argv, canonical N:\pch cwd and complete file
inventories/hashes. Only the temporary IR prefix changes to relative `ir` in
portable phase environments. File/path bytes in outputs are never normalized.
The 38164-byte state.pch matches raw across two full driver runs and two
independent original C1 producer/consumer replays. All four IR outputs of each
phase also match raw. There are no ignored fields. The generated producer PCH
is the consumer dependency, and capture-original.py checks that dependency before use.
Native PCH serialization/loading provenance remains part of reconstruction.

Run python3 capture-original.py for an independent original-toolchain phase replay. It
builds run-phase.c with the original toolchain and uses a private Wine prefix
under ~/.ai-tmp/vc1.0-c1/pch/independent. This helper passes exactly the captured
single environment entry, with an explicit canonical child working directory.
All fixture contents are immutable after this original-backed capture commit.

Explicit original evidence tooling is capture-original.py with run-phase.c.
It is separate from native regression execution and is never called by check/replay.
Native C1 regressions use c1/scripts/check-native.py without Wine or originals.
Captured input/output/reference bytes remain unchanged.
