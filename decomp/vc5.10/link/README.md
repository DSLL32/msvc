# LINK 5.10 groundwork

Target: `toolchains/5.10/bin/LINK.EXE`, PE32 i386, image base `0x400000`,
entry RVA `0x65280`. There are no exports or TLS directory. No function has
been reconstructed, unmasked or accepted. `src/` is intentionally empty.
Reconstruction waits for root release after the Rust graph audit and DLL workflow
investigation. The original compiler and companion DLLs exist; the CL worker
proved a minimal original compilation with CL 11.00.7022. This folder's own
link/debug build probes remain incomplete.

`make inspect` prints the read-only PE inventory. `native`, `original`, `fixtures`
and `check` fail explicitly while reconstruction is unreleased. Source-only cases
in `cases/plan.json` copy the vc1.0 GUI hello, stdcall DLL and debug DLL themes.
They contain no copied output, trace, object, reference hash or accepted frontier.

## Original-only probes

After the root shared-Wine health hold is lifted, explicitly run
`python3 scripts/capture_original.py --capture`. This invokes original tools;
it is never a regression target. It verifies the fixed shared R:/S: mappings,
uses a fresh `S:\vc5.10\link\<UTC timestamp>` working tree, and records exact
commands, input/output hashes, raw streams, loader/process logs and elapsed
times in scratch. Native shipped MSVCRT, MSPDB50, MSDIS100 and IMAGEHLP are
selected explicitly. Build probes cover GUI EXE, DLL, `/Z7` plus ordinary link,
`/debug /debugtype:cv /pdb:none`, and `/debug /pdb:hello.pdb`.

The initial `/help`-equivalent `/?` probes did not reach LINK: shared-prefix
prestartup health failures also affected other tools. One timed-out run's raw
logs and receipt remain in scratch, summarized by
`cases/original-probe-incomplete.json`. Its timeout is not LINK's expected result.
No compiler, linker artifact, PDB or child invocation has been dynamically
validated. All own Wine processes are stopped; the shared server was untouched.

## Dependency findings

`cases/inventory.json` records every imported symbol. Unlike vc1.0, this LINK
imports MSVCRT, MSDIS100's C++ disassemblers, three MSPDB50 functions and four
IMAGEHLP functions, as well as KERNEL32. Static analysis also locates dynamic
MSPDB50/MSPDB41 selection and resolution of `PDBExportValidateInterface` and
`PDBOpen`. These are in-process interfaces; subprocess response replay alone
cannot implement their object ownership and state.

`cases/dependency-evidence.json` records original static call sites for the
synchronous CVPACK and CVTRES child paths. The visible CVPACK argv is `cvpack`,
`/nologo`, quoted output path; there is no `/cvnew` in this call. CVTRES receives
machine, `/nologo`, `/readonly`, optional Windows CE flags and quoted input.
Both have sibling-tool lookup and PATH fallback. These observations establish
candidate contracts, not that any seeded case reaches them. The CVPACK and
CVTRES workers were notified; actual 5.10 captures must settle full argv,
environment and file effects before building strict native child replay.

The shared recon maintainer confirmed that MSDIS100, MSPDB50 and IMAGEHLP have
no current shim implementation. No speculative implementation was requested.
Root must choose the in-process dependency policy using reached ABI/state
evidence. Resource conversion probing awaits the RC worker's original `.res`
capture and shared Wine recovery.

The executable `.text` section also contains IAT/import metadata and string
data (the IAT starts at RVA `0x1000`). Section executable flags do not establish
instruction ownership; future masking must preserve verified non-code ranges.
All regressions will run the native i386 reconstruction with cached original
references, never Wine. Reconstruction source remains original-style Win32 C,
includes `<recon.h>` last and must also compile with the original toolchain.
