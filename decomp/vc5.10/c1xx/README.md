C1XX.DLL (Visual C++ 5.10)
=========================

`make -j4 check` runs the two numbered native cases through the shared recon-tool;
`make -j4 unit` runs their initialized test checkpoints. A narrow forked unit checks nonzero restore/reset of the exception counters. `make -j4 win32` builds the real DLL with
5.10 CL/LINK; `make -j4 audit` checks the accepted graph and individual ABIs.

Application reconstruction is in src/; the two existing startup support bodies
and their declarations are in crt/, with their retained-variant rationale in
crt/startup.c. Native case adapters and the import-library build definition are
in harness/. Numbered case files contain descriptors and entry orders only.

The single registry is binary.c. The current prefix has 101 accepted application
functions / 11613 code bytes; historical CRT support is separate
(2 functions / 451 bytes), with 12 bytes of generated import support. These
12076 bytes are compared with a 751216-byte .text extent; that denominator also
contains runtime, data and padding and is not a non-CRT coverage measure.
The audit checks 103 source bodies. The two six-byte import endpoints retain
identity/catalog checks and zero application credit; they are not RET-body targets.

0000 checks typed DLL attach, export presence without invoking exports, and
detach against the completed original lifecycle case. 0001 invokes the actual
CL-derived 26-argument C++ pass with BOOL1 on minimal.cpp. Its current oracle is
the genuine create_input INT3 at RVA3a434, not completed compilation.
Both cases compare the original-backed first application encounters.

The minimal case uses a small original Win32 host in
harness/original-host.c: LoadLibrary, resolve the stdcall export, forward its
26 arguments with mode1, then FreeLibrary. Build it with `make -j4 original-host`.
Explicit references use shared `original capture --host build/host/c1xx-host.exe
--progress --native-msvcrt`; native fixtures use cached data and never Wine.
The fresh reference uses `T:\vc5.10\c1xx\0001-minimal-cpp` and only declares the
consumed MSC_CMD_FLAGS absence. The native host binds the forwarded argument
vector and an empty environment qualified for that query only; other Windows
names are outside the current scope. It does not seed LastError. The existing
system-info profile bytes are harness-only capture assets. Historical private
HOST3/environment inputs remain historical evidence, not current dependencies.
The new host baseline captured the same 93 application encounters in 1.585 s;
its matching native fixture took 8.46 ms (setup 5.13, execution 2.32, checks 1.01).

Path resolution and normalization are reconstructed in src/path.c. Packing
initialization in src/packing.cpp uses the observed 8-byte stack and 12-byte
frames, with a genuine pending thiscall member and cdecl allocation helpers.
Packing initialization, allocation, frame acquisition and push are reconstructed.
The recycle clear helper remains a named trap. The compilation wrapper preserves its full 193-byte hot/cold SEH graph; native
execution currently exercises its normal try body only. Its encountered signed
recursion counter (datafe cd514) and diagnostic count (bssfe c0abc) join the
typed snapshot/reset projection, with original zero defaults and one nonzero
restore unit. The latest fresh pair reaches the real no-argument run_compiler:
original 1.243 s/native 10.572 ms (setup 6.080, execution 3.453, checks 1.039).
Build took 6.308 s; the strict 101-body audit and unit checks pass;
complete
phase logs remain in `~/.ai-tmp/msvc2/vc5.10/c1xx/minimal-resume-20261025/`.
The prior unaccepted resolver archive remains historical under
`~/.ai-tmp/msvc2/vc5.10/c1xx/standardization-20261025/resolver-wip/`;
its missing old DLL is not claimed as preserved. Current fixtures have no
private HOST-vector dependencies.

Historical source/proofs remain in Git. Raw acceptance observations remain in
owned scratch, including minimal-option-append-original-3; full historical
revalidation depends on those external artifacts. The separate ecfe2c3a observer
maintenance reduced offline remote reads79→7 but was never timed in a new
original execution; it is not a measured runtime speedup. Legacy capture,
Python replay, identification and graph frameworks have been retired in favor
of shared tooling. Wine remains outside fixtures.

The 1,321-byte compilation driver is reconstructed in src/driver.cpp, including
its full hot/cold graph and actual virtual/member call ABIs. The 1,052-byte
original PCH settings transfer has a typed projection of known fields only;
unknown bytes have no native placeholders and remain behind pending semantic
consumers. All newly encountered initialized fields and BSS join snapshot/reset,
with a focused nonzero unit. Current execution stops at initialize_frontend,
before the settings transfer and virtual calls. Fresh original/native timings
were 1.472 s/10.766 ms (setup 5.617, execution 4.001, checks 1.147).

The 2,336-byte frontend initializer is in src/frontend-init.cpp. Its fresh
reference reaches create_input(0,&input,&text_input), with actual preprologue
arguments recorded: original 1.467 s/native 10.796 ms. Later allocations,
builtin types, lexer construction and C++ bool declaration are static/source
coverage only. Original allocations remain 16/572/88/12 bytes where encountered;
compact semantic projections do not claim their unknown tails or full layouts.
The six newly grounded normalized options also join the PCH projection (756
known bytes/296 unmodeled). The focused reset unit checks the actual nonzero
stack-check default. Only this initializer TU uses /Ob1 to inline proven helper
constructors/placement new; real application functions remain outlined.
