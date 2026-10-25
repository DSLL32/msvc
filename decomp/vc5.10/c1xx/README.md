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

The single registry is binary.c. The current prefix has 97 accepted application
functions / 7726 code bytes; historical CRT support is separate
(2 functions / 451 bytes), with 12 bytes of generated import support. These
8189 bytes are compared with a 751216-byte .text extent; that denominator also
contains runtime, data and padding and is not a non-CRT coverage measure.
The audit checks 99 source bodies. The two six-byte import endpoints retain
identity/catalog checks and zero application credit; they are not RET-body targets.

0000 checks typed DLL attach, export presence without invoking exports, and
detach against the completed original lifecycle case. 0001 invokes the actual
CL-derived 26-argument C++ pass with BOOL1 on minimal.cpp. Its current oracle is
the genuine push_packing INT3 at RVA2b89d, not completed compilation.
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
Frame acquisition now implements its full 88-byte hot/cold member graph,
including the named pending recycle helper. The latest fresh pair reaches
push_packing(0,NULL): original 1.328 s/native 10.705 ms (setup 5.320,
execution 4.351, checks 1.034). Build took 2.718 s/audit 0.173 s; complete
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
