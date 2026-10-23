# C2.EXE reconstruction

Only the first case is released: `/?`, an unsupported-option diagnostic (status
1, empty stdout, 51 stderr bytes). The minimal intermediate-input case remains
queued and unauthorized. The complete 345-byte CRT startup is accepted; no
application function has been accepted yet.

`make -j4 native` builds i386 Linux with shared shims; `make -j4 original`
checks the Win32 reconstruction with the shipped compiler and `/MD` runtime.
`make audit` uses the common Rust CFG/RET/ownership/extent tool with explicit
paths. Native fixture targets remain fail closed until this first case completes.
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
only the CRT startup currently has acceptance credit.

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
its driver relationship still depends on the new DLL frontend workflow.
Preserve each recovered function ABI and distinguish import/runtime entries
from application graph entries. Imported MSVCRT startup or PDB APIs are not
application reconstruction coverage.

The shared maintainer supplies startup CRT contracts and a prepared module-resource
provider for encountered `LoadStringA` strings. MSPDB50/ILStore is not implemented. The current shim has legacy stdio,
ctype and locale state, but this binary's imported MSVCRT layouts and actual
calls must be proved before reuse. These are investigation items, not requests
for speculative compatibility code. Debug/PDB cases may need additional
shared support after a minimal no-debug standalone case is captured.
