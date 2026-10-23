# VC 5.10 RC groundwork

Target: `toolchains/5.10/bin/RC.EXE`, resource compiler 5.00.1472.1.
This folder contains inventory, source-only case seeds and explicit unmodified
original probes. No target function has been reconstructed or accepted.
Reconstruction is gated on approval of the DLL workflow and the Rust graph auditor.

RC is a small console host, not the resource compiler implementation. It statically
imports `RCDLL.dll!RC` and `RCDLL.dll!Handler`; it does not resolve these exports
with LoadLibrary/GetProcAddress. Imports also include MSVCRT and KERNEL32.
The PE has no exports, relocations or TLS directory. Its executable section also
contains IATs, imports and debug metadata, so section size is not a code coverage
estimate. `cases/inventory.json` and `cases/pe-headers.txt` record the untouched PE.

## DLL boundary

Analysis of the original caller identifies `main` at VA `0x010011d2`. It registers
the imported Handler with SetConsoleCtrlHandler, then calls the six-slot stdcall
export `RC(0, 0, diagnostic_callback, 0, argc, argv)`. It unregisters the handler
and calls exit with RC's result. The imported Handler has one stdcall argument.
The diagnostic callback at `0x01001100` has three stdcall arguments and reads its
third argument as mutable narrow message bytes; it can modify trailing CRLF/LFCR
and percent characters, allocates an escaped-percent copy when possible, prints
via printf and returns zero. That mutation and allocation-failure path matter;
replaying only final diagnostic text would hide RC behavior.

The sibling RCDLL worker owns export bodies, CRT/DLL initialization and internal
preprocessing. A future native harness must preserve each module's graph and ABI,
perform audited module initialization before main, retain callbacks in the RC
module, and order detach at process completion. Module-qualified addresses and
callback IDs are needed for original/native evidence. No shared module lifecycle
or qualified trace-ID registry exists yet; the recon maintainer has been notified.
The orchestrator must decide that policy before implementation.

For RC-only early progress, a strictly checked cached DLL response might replay a
known export invocation plus callback sequence, complete mutable messages and
artifacts. Such a replay cannot earn RCDLL coverage or substitute for later
combined RC/RCDLL comparisons. The preferred eventual integration links each
reconstructed module's original-style C into a native process with an explicit
module harness. Unknown DLL calls or callback patterns must fail closed.

## Cases and commands

`cases/resources-rich/source` copies only input source/assets from VC1.0's rich
resource fixture: guarded nested includes, command-line macros, #ifdef/#if,
arithmetic, menus, dialogs, string tables, accelerator tables, icons, bitmaps,
cursors, raw/custom resources, message tables, version information and languages.
`cases/minimal/source/minimal.rc` is an independent small string-table probe.
No VC1.0 output, trace or acceptance metadata has been imported.

Run `make inspect` for read-only PE inventory. `make probe-help`, `probe-minimal`
and `probe-rich` explicitly invoke the untouched original through the shared Wine
prefix; they are never fixture dependencies. Probes use only the fixed R:/S:
mappings and scratch `~/.ai-tmp/msvc2/vc5.10/rc/<case>`. Native/original-build,
fixture and check targets currently fail closed rather than report false success.
Probes record elapsed command time and raw streams; these are groundwork, not
masked-original acceptance or regression references. Original /help succeeds.
The old `/nologo` option is unsupported; rejected attempts are preserved separately.
Resource compilation results are recorded in each probe receipt when available.

## Readiness at groundwork handoff

The minimal resource probe timed out after 30.032 seconds with empty streams and
no output. A short loader/exception diagnostic emitted only early Wine startup
information, which does not establish a defect in RC/RCDLL. The rich compile has
no successful baseline either. The orchestrator has held all further Wine probes
because other projects also stalled before application startup; no RC probe is
left running. No prefix settings or shared Wine server were changed. CVTRES must
wait for an actual new-toolchain .res capture before its conversion probe.
`--replace` is required to overwrite an existing groundwork receipt explicitly.
