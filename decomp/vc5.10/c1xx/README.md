C1XX.DLL (Visual C++ 5.10)
=========================

`make -j4 check` runs the two numbered native cases through the shared recon-tool;
`make -j4 unit` runs their initialized test checkpoints. No additional unit
callbacks are currently registered. `make -j4 win32` builds the real DLL with
5.10 CL/LINK; `make -j4 audit` checks the accepted graph and individual ABIs.

Application reconstruction is in src/; the two existing startup support bodies
and their declarations are in crt/, with their retained-variant rationale in
crt/startup.c. Native case adapters and the import-library build definition are
in harness/. Numbered case files contain descriptors and entry orders only.

The single registry is binary.c. The current prefix has 94 accepted application
functions / 7510 code bytes; historical CRT support is separate
(2 functions / 451 bytes), with 12 bytes of generated import support. These
7973 bytes are compared with a 751216-byte .text extent; that denominator also
contains runtime, data and padding and is not a non-CRT coverage measure.
The audit checks 96 source bodies. The two six-byte import endpoints retain
identity/catalog checks and zero application credit; they are not RET-body targets.

0000 checks typed DLL attach, export presence without invoking exports, and
detach against the completed original lifecycle case. 0001 invokes the actual
CL-derived 26-argument C++ pass with BOOL1 on minimal.cpp. Its current oracle is
the genuine initialize_packing INT3 at RVA3990d, not completed compilation.
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

The 422-byte resolve_option_path body is reconstructed in src/path.c, with its
full hot/cold graph and finite cached-import preservation witness. The resolver
reference reached the real normalizer stub in 1.430 s; native replay took
10.625 ms and its ESI-contract removal control rejects. The 72-byte normalizer
then advanced the real case to packing initialization, with fresh original
1.318 s/native 8.291 ms and separate build 1.777 s/audit 0.168 s. It uses the
existing filename table; original dead incoming-slot reuse is documented in
source without requiring caller-stack mutation.
The prior compiled-but-unaccepted resolver WIP remains historical under
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
