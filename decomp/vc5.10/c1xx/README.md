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

The single registry is binary.c. Migration preserves 92 accepted application
functions / 7016 code bytes (f6b743ba); historical CRT support is separate
(2 functions / 451 bytes), with 12 bytes of generated import support. These
7479 bytes are compared with a 751216-byte .text extent; that denominator also
contains runtime, data and padding and is not a non-CRT coverage measure.
The audit checks 94 source bodies. The two six-byte import endpoints retain
identity/catalog checks and zero application credit; they are not RET-body targets.

0000 checks typed DLL attach, export presence without invoking exports, and
detach against the completed original lifecycle case. 0001 invokes the actual
CL-derived 26-argument C++ pass with BOOL1 on minimal.cpp. Its current oracle is
the genuine resolve_option_path INT3 at RVA397db, not completed compilation.
Both cases compare the original-backed first application encounters.

The minimal case uses a small original Win32 host in
harness/original-host.c: LoadLibrary, resolve the stdcall export, forward its
26 arguments with mode1, then FreeLibrary. Build it with `make -j4 original-host`.
Explicit references use shared `original capture --host build/host/c1xx-host.exe
--progress --native-msvcrt`; native fixtures use cached data and never Wine.
The fresh reference uses T:\vc5.10\c1xx\0001-minimal-cpp and only declares the
consumed MSC_CMD_FLAGS absence. The native host binds the forwarded argument
vector and an empty environment qualified for that query only; other Windows
names are outside the current scope. It does not seed LastError. The existing
system-info profile bytes are harness-only capture assets. Historical private
HOST3/environment inputs remain historical evidence, not current dependencies.
The new host baseline captured the same 93 application encounters in 1.585 s;
its matching native fixture took 8.46 ms (setup 5.13, execution 2.32, checks 1.01).

The unaccepted 422-byte resolve_option_path body (RVA397db..398cd plus
72696..7274a) was compiled but never accepted. Its exact source, patch, metadata
and inputs are preserved under:
`~/.ai-tmp/msvc2/vc5.10/c1xx/standardization-20261025/resolver-wip/`.
The compiled WIP native host/objects and historical Win32 build receipt are also
retained there; the unaccepted Win32 DLL itself was replaced by the baseline
build before archival and is not claimed as preserved. No accepted evidence
depended on that DLL. The migrated source deliberately retains the accepted
named INT3 at that entry.
No function or case was reconstructed or accepted during this migration.

Historical source/proofs remain in Git. Raw acceptance observations remain in
owned scratch, including minimal-option-append-original-3; full historical
revalidation depends on those external artifacts. The separate ecfe2c3a observer
maintenance reduced offline remote reads79→7 but was never timed in a new
original execution; it is not a measured runtime speedup. Legacy capture,
Python replay, identification and graph frameworks have been retired in favor
of shared tooling. Wine remains outside fixtures.
