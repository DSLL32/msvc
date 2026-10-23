# VC5.10 CVPACK

Target: `toolchains/5.10/bin/CVPACK.EXE`; the requested folder spelling is
`cvppack`. Reconstructed source is readable Win32 C, compiled with the provided
5.10 toolchain and also as native i386 Linux code through `lib/recon`.

`/help` is complete: status 1, 193 stdout bytes, no stderr, six application
entries. The genuine embedded-CodeView `hello-debug-dll` case is in progress at
`rewrite_bp_relative_symbol`, after packing two global type records. There are 69
accepted application functions / 22,928 extent bytes, plus 621 associated switch-data
bytes. This is 36.59% of the 62,662-byte whole-text denominator, including CRT/unknown/padding;
it is neither non-CRT nor executed-branch coverage. Four CRT bodies (400 bytes)
and four import thunks (24 bytes) remain separate support.

## Build and validation

```
make -j4 all native
make -j4 check audit-native
```

`all` uses the shared repository Wine prefix for the original compiler only.
Fixtures, GDB, dependency responses and regression checks run natively. Never
rebuild concurrently with an immutable pair check: build once, freeze its input
content and PE/MAP/ELF hashes, then run validators. Shared-source changes may
require native-only rebuild/capture. Reuse original raw only when its target,
mask, effective candidate, configuration and execution inputs are unchanged;
strict current sidecars bind both generations and regenerate current metadata.

The source retains standard `main`; its documented native target-TU alias
`cvpack_main` separates it from the host driver. `src/crt-imports.h` preserves the
original direct `_iob` data import instead of adding the SDK accessor.

The shared Rust audit checks candidate-inclusive CFG/RET, qualified imports,
function references, ownership and complete extents. Encountered switch tables
have exact COFF/relocation/linked-byte proofs. Narrow provider/SDK preservation
witnesses resolve cached EBX/ESI/EDI/EBP imports. Main additionally requires all
five original cached free calls and only the genuine checksum callback as
unresolved. Actual GDB DWARF formals independently check argument count/names,
integer/pointer kind, width and known pointee size before typed state comparison.

## Cases and evidence

Case definitions and immutable original inputs/masters live under `cases/`.
`hello-debug-dll` was prepared by the actual 5.10 compiler/linker with `/Z7`,
`/DEBUG /DEBUGTYPE:CV /PDB:NONE /NOPACK /INCREMENTAL:NO`. Its NB05 input and NB11
master are specific to this toolchain; no VC1 output is reused.

The current prefix compares 336 type records and all 384,000 initialized table
bytes, buffer/record aliases, actual reads and allocations, complete mapped-file
contents, defined symbol/source-module buffers, and 47 owned counted type-name
cache nodes with their bucket/segment aliases, plus initialized arena prefixes,
global pointer slots and interned-node ownership. The default dispatch row and
next global index share their original storage, with actual DWARF offset proof. Uninitialized allocation tails
remain native owned; explicit positive controls permit changing them. The first
module's type-table contract fails closed on reuse until that lifetime is observed.

The reviewed MSPDB WidenTi case provider is outside application coverage and has
not yet been traversed by the reconstruction. It enforces 21 actual lifetimes,
complete inputs/aliases/sequences/capacities and actual thiscall interfaces,
writing only observed prefixes and preserving each native allocation's own tail.
The unreached Record slot fails closed. See `cases/hello-debug-dll/DEPENDENCY.md`.

`cases/hello-debug-dll/REVIEW-50.md` records the independent source/graph/ABI review.
Each accepted function's `evidence/*/function-review.json` or `validation.json`
records fresh/reused controls and separate build/capture/check/fixture times.
Historical raw is never relabeled; incomplete and superseded trials retain their
status. Current native help runs in roughly 15 ms; the partial debug fixture is
about 0.7 s including GDB. Completion will use native entry traces, exact streams,
status and the cached NB11 master without launching Wine.
