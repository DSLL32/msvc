# C2.EXE reconstruction

The first `/?` diagnostic case is complete: exit 1, empty stdout and exactly
51 stderr bytes, with 19 application entries through exit cleanup. Its frozen
reference remains the native regression case. Root authorized the next case,
`numeric-control-flow`, compiled by original CL with `/O2 /G5`. Its fresh C2
argv/environment, four IL files and standalone 2347-byte COFF master are in
`cases/numeric-control-flow/`; only the COFF header timestamp is normalized.

Current reconstruction has 47 application functions (9979 bytes) and five CRT
helpers (438 bytes). The numeric case has decoded its complete IL header and
has initialized all five no-debug output sections and has reconstructed the23-kind symbol decoder; its first unsigned serial86 and full filename now match, and it stops at name registration. Four import thunks (24 bytes) and contextual
text tables (439 bytes) remain separate. The 10417 source bytes are 1.817% of
573183 `.text` bytes; that denominator includes CRT/data/padding and is not
non-CRT coverage. `cases/first-case-result.json` describes the finished first
case, and `cases/acceptance/` retains each subsequent promotion in order.

`make -j4 native` builds i386 Linux with shared shims; `make -j4 original`
checks the Win32 reconstruction with the shipped compiler and `/MD` runtime.
`make audit` uses the common Rust CFG/RET/ownership/extent tool with explicit
paths. `make -j4 fixtures` runs only the native executable against frozen streams and
19 original application entries. Current measurements: 2.7 ms execution, 9.2 ms
including fixture setup/checks. New capture helpers retain their actual executable
and immutable compiled inputs; newer captures also retain ELF/PE/MAP and
static-index snapshots in scratch for independent review. Earlier raw evidence
is never relabeled.
Original captures are explicit scripts and never fixture dependencies.

The initial full-text INT3 frontier was matched at `0x452797`, with the actual
loaded shipped MSVCRT file verified by hash. The whole startup, including its
cold SEH chunks, passed the common Rust graph audit and fresh original/native
state comparison. The orchestrator reviewed the candidate-inclusive protocol,
immutable compiled-source snapshots, raw GDB/ELF INT3 proof and 24 rejection
controls before promotion. Evidence is under `cases/acceptance/0001-startup/`.

## Target and dependencies

`toolchains/5.10/bin/C2.EXE` is a 660240-byte i386 PE32 console executable,
SHA-256 `e75aecaf4073b0817ffb638cae5fff636b2e2d1a090daabf8f68dbc954515fae`.
Its image base is `0x400000`, entry RVA `0x52797` (VA `0x452797`). It has no
exports, TLS directory, delay imports or base relocations. Its PE linker version
is 5.10; its embedded compiler banner says version 11.00. The binary version
must not be inferred from the folder name alone.

[cases/inventory.json](cases/inventory.json) lists the sections, data directories
and all import names. Direct imports are:

- `MSVCRT.dll`: runtime calls and data, including `_iob`, `_pctype`,
  `__mb_cur_max`, `__unDName`, `_fullpath`, `_controlfp` and ordinary stdio.
- `MSPDB50.dll`: 16 ILStore/ILMod, PDB and Stream APIs. The DLL is shipped;
  imported does not establish that an API executes in a no-debug case.
- `USER32.dll`: `LoadStringA`, for module-backed resource strings.
- `KERNEL32.dll`: `GetModuleHandleA` and `SetConsoleCtrlHandler`.

The toolchain includes `CL.EXE`, C1/C1XX DLL frontends, headers, `LIBC.LIB`,
`MSVCRT.LIB` and `MSPDB50.DLL`. Original compiler execution/actual frontend
inputs are being investigated with the CL/C1 workers. Startup source and named child stubs compile with the original toolchain;
application and CRT acceptance are tracked separately in `functions.json`.

## Proposed cases and actual-input capture

[cases/plan.json](cases/plan.json) records six source-only seeds copied byte for
byte from the corresponding vc1.0 case sources: minimal C, Win32 hello with
`/O2 /G5`, loops, register pressure, aliasing and floating point. It records the
origin and source hash. No vc1.0 intermediate file, object, trace or expected
result has been adopted as a vc5.10 oracle.

The original-only CL debugger capture in
`../cl/cases/original-probes/host-contract-2/` preserves all four intermediate
files before C2 starts, its working directory, raw `MSC_CMD_FLAGS` assignment and
the completed 275-byte object. The actual process argv contains only argv[0];
options printed by CL's `/Bd` are carried in `MSC_CMD_FLAGS`, including its final
space. `cases/plan.json` links the complete captured inputs and exact interface.
Standalone original C2 replay from these inputs remains unverified and must be
proved before treating this as an independently replayable backend case.

The original `/?` argument returns status 1, empty stdout, and 51 stderr bytes: `fatal error C1007: unrecognized flag '/?' in 'p2'` followed
by CRLF. It does not provide a help screen. Raw bytes and timing/hash receipt
are under `cases/original-help/` for the historical groundwork probe. The
canonical shipped-CRT capture now has a controlled 16-string environment, exact
argv, and streams under `cases/unsupported-help-diagnostic/`. The controlled environment also supplies the matching partial startup proof.

## Native host and outstanding questions

Use an i386 native executable with original-style Win32 C and `<recon.h>` last,
shared shim sources at final link, `-O0 -g`, and `make -j4`. C2 remains an EXE;
its inputs come from the captured original CL/frontend pipeline.
Preserve each recovered function ABI and distinguish import/runtime entries
from application graph entries. Imported MSVCRT startup or PDB APIs are not
application reconstruction coverage.

The shared maintainer supplies startup CRT contracts and a prepared module-resource
provider for encountered `LoadStringA` strings. MSPDB50/ILStore is not implemented. The reached canonical FILE layout, character flags, mutable environment
aliases and `_errno`/`strtol` coupling have fresh matching evidence. Native builds
select `RECON_CRT_PROFILE_MSVCRT_510` for the verified shipped CRT. Additional
stdio, locale or PDB contracts still require reached evidence. These are investigation items, not requests
for speculative compatibility code. Debug/PDB cases may need additional
shared support after a minimal no-debug standalone case is captured.

Source entry names are `main` and `mainCRTStartup`. Explicit driver `symbol`
aliases preserve earlier logical graph/trace names without modifying historical
captures. Only target native translation units alias `main` to `c2_main`; the
Linux host retains its own `main`. See `cases/analysis/naming-aliases.json`.

Unchanged protocol mutation controls may reuse a reviewed immutable baseline;
new or affected state checks still run with every reconstruction. The intrinsic
initializer checkpoint ran17 fresh controls and reused60 unchanged ones in
5.415s; its receipt lists both proof inputs and checker/contract hashes. The
previous complete74-control store checkpoint took21.022s.

The section-default checkpoint compares all five typed86-byte nodes, pool/hash/list
aliases, scalar fields, untouched bytes and CONST name/class address sharing.
The context at original49e300 is still UNMODELED; an approved incomplete-type
address anchor has no payload/extent/data credit and must be replaced before
any pointee use. Full107-control checkpoint time is recorded separately from
build, audit and original/native execution in its acceptance timing receipt.

The symbol-input checkpoint recovers variant fields, the64-byte transient record,
2058-byte name scratch, and the fifth source-name allocation pool. It reuses
existing option identities for constant-string blocks and frame-pointer flags.
The next unsigned reader has no formal parameters, verified from the actual
stopped ELF's DWARF; its caller state and complete transient storage match the
fresh original. The strict common audit includes all50 source vertices. This
checkpoint ran34 fresh rejection controls and reused87 reviewed controls; its
receipt also records the earlier33-control run separately.
