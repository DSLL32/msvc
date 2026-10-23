# RCDLL reconstruction

Target: `toolchains/5.10/bin/RCDLL.DLL` (the toolchain contains no RCDLL.EXE).
The first `RC` export `/help` case is complete; the rich resource invocation is
in progress. Conventional entries are `main`, `DllMain`, `_DllMainCRTStartup`
and `_CRT_INIT`. Historical logical names are mapped by bounded aliases without
editing raw evidence; [naming verification](cases/help/conventional-entry-names.json)
records unchanged ABIs and function extents.

Nineteen application functions (7,599 original code bytes), two embedded CRT functions
(555 bytes), and two import-thunk boundaries have been accepted. The rich case
currently matches at the next named trap, `rc_preprocessor_main`.

`make -j4 native` builds the instrumented Linux i386 static DLL host.
`make -j4 fixtures` compares cached original output, callback effects and application
entry order without Wine. `make -j4 original` explicitly invokes the original
5.10 compiler to build a real Win32 DLL. `make graph-audit check-abi check-plan`
checks the shared Rust function graph, DLL interface/resources and input provenance.
`python3 scripts/check-completion.py` revalidates the frozen completion pair and
11 rejection controls without changing live configuration or invoking Wine.

## First case

The original host is the actual RC.EXE with static imports. It loads the masked
DLL, performs CRT/application attach, calls
`RC(0, 0, diagnostic_callback, 0, argc, argv)` for `/help`, then performs process
exit detach. The native host reproduces that sequence outside the DLL graph.
Both complete with status 0, identical 589-byte stdout, empty stderr, four exact
callback before/after buffers and nine matching application first entries.
[Cached reference](cases/help/reference/manifest.json),
[acceptance history](cases/help), [verification/timings](cases/help/verification.json).

| Export | Ordinal | ABI |
| --- | ---: | --- |
| Handler | 1 | stdcall, DWORD, ret 4; unaccepted trap |
| RC | 2 | stdcall, six 32-bit slots, ret 24 |

Diagnostic callbacks are stdcall with three slots, with mutable ANSI text in
slot three. The RC.EXE callback removes a paired trailing CR/LF and escapes `%`
when printing. The fourth RC argument remains opaque and must be NULL in this
case. Handler/control-thread behavior and resource callbacks remain untested.

The original target and actual loaded MSVCRT are verified from debugger hFile
bytes. Target SHA256 is recorded in [inventory](cases/pe-inventory.json); the
canonical provided MSVCRT SHA256 is
`e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210`.
The DLL was loaded at preferred base 0x71c20000, without TLS. Nonpreferred loading
is untested. Process-startup reserved context is nonNULL and opaque; detach uses
reserved=1. The reconstructed startup code only forwards these values.

## Evidence and native contracts

The original debugger guards the nonrelocated first entry byte before execution,
saves relocated text at entry, masks code, restores only accepted/candidate ranges
from that snapshot, and verifies mask readback. Exact loader metadata and encountered
text-data exceptions are indexed. Each acceptance binds the same candidate to raw
original/native evidence and the Rust graph check. Completion requires actual
process exits on both sides; a pair of absent frontiers is insufficient.

Native module identity is registered through `lib/recon`, with the actual captured
S: module path, original SHA and no-TLS flag. The provider contains only encountered
message-table entries 10001/20001, verified against source hash/offset/encoding.
The same readable `src/help.mc` payloads are encoded for the native provider and
compiled through the original RC into the real Win32 DLL; no original resource
section is copied.
Shared FormatMessage support preserves literal message text, inserts, CR/LF,
NUL/tail bytes and observed LastError. Private heap ownership and the zeroed
528-byte allocation (eight-byte header plus 520-byte buffer) are checked.

VC5 uses a 16-word jump buffer. The help path's `_setjmp3(...,0)` is an opaque
save using a genuine native caller frame; original register/SEH bytes are retained
as original evidence rather than equated to glibc bytes. Target longjmp/unwind,
nonzero counts, unknown resources, window-message paths and unvisited heap modes
fail closed in shared compatibility support.

`capture-original.py`, `message-probe.c` and `heap-probe.c` are explicit reference
helpers. `capture-native.py` and `fixtures.py` run only Linux programs. GDB observes
frontiers, typed state and actual completion. Shared shims/host callbacks are outside
the application trace; embedded target CRT functions remain reconstructed/audited.
The Rust LINK metadata adapter is used only for explicit symbol aliases and paths,
with no VC1 defaults. No Python graph discovery is used. DLL export/entry policy
is checked separately.

## Counts, timing and future inputs

The provisional Ghidra inventory discovers 299 application functions / 66,295 code
bytes. Current accepted application extent coverage is 11.46% (19 functions). This denominator
is analysis discovery, not a claim that every remaining original extent has passed
an ownership audit. [Inventory](cases/function-inventory.json) excludes separately
identified embedded CRT and import thunks; switch/literal data is excluded from
accepted code bytes.

Measured full `-j4` rebuilds, including resource assets: original 4.566 seconds,
native 1.671 seconds. Rust audit of all 11 accepted functions: 0.037 seconds. Last fresh masked original completion:
1.468 seconds. Native fixture: 0.0026 seconds execution / 0.0081 seconds total,
including setup and output/callback/trace checks. Exact receipts and current binary
hashes are in verification metadata; historical raw captures are not relabelled.

[cases/plan.json](cases/plan.json) preserves ordered source-only VC1 seed provenance
for future resource/preprocessor cases. These inputs include menus, dialogs,
strings, icons, cursor/bitmap, accelerators, raw/custom/message/version resources,
includes, macros and conditionals. No VC1 outputs became VC5 reference oracles.
The resources-rich second case is authorized and in progress. Original capture scratch uses the shared R:/S: prefix
under `~/.ai-tmp/msvc2/vc5.10/rcdll`; fixtures never invoke Wine.

The rich source compiles unchanged with original VC5 RC: status 0, empty streams,
and a 3,672-byte resource file. The immutable boundary and complete source/header
input hashes are under [cases/resources-rich/reference](cases/resources-rich/reference/manifest.json).
The capture includes actual loaded RC/RCDLL/CRT identities, the six export slots,
mutable argument pointers, Windows environment and working directory. RC and
CVTRES consume this verified reference independently of RCDLL reconstruction.
Its original capture took 0.666 seconds; reconstruction progress is recorded below.

Current entry names use the conventional `main`, `DllMain`,
`_DllMainCRTStartup` and `_CRT_INIT` identities. These names are inferred from
function roles, not recovered debug symbols. Native target translation units
alias `main` to `rcdll_command_main` so the static harness keeps its own entry.
Historical captures retain their original labels; bounded metadata aliases
map those names for comparison. [Naming proof](cases/help/conventional-entry-names.json)
records unchanged ABIs/extents and original/native build checks.

Resources-rich now accepts `rc_release` (31 bytes); the matched next frontier is
`rc_prepare_input`. [Verification](cases/resources-rich/release-verification.json)
and [22 rejection controls](cases/resources-rich/release-gate.json) cover the full
captured target environment, mutable arguments, live private-heap allocations,
temporary names, 20 preprocessor arguments and the empty partial resource file.
Target environment bindings exclude native host controls. Individually bounded
main literals needed by the rich path were added after the complete-buffer gate
caught masked string data; no data section is copied into source.

The detailed original API census is opt-in (`--api-census`), separately accounted
in [api-census.json](cases/resources-rich/api-census.json). Ordinary captures use
finite observer counts; native fixtures never run that census or Wine.

`rc_prepare_input` is now accepted (1,143 bytes). Its first 46-byte UTF-16
`#line` directive, complete filename buffer, file state and empty temporary output
match at `rc_write_bytes`. [Verification](cases/resources-rich/prepare-verification.json)
records the 1.618-second original pair, 0.228-second native GDB capture and 27
rejection controls. The provided 47-byte `__chkstk` is separate compiler support:
its archive object matches both PEs, and actual original entry/return state proves
its custom EAX/ESP ABI. Native execution uses its own compiler-owned frame and
checked local-array bounds; it makes no synthetic helper call. This covers normal
non-overflow execution, without claiming Windows stack-overflow/SEH behavior.

The 44-byte `rc_write_bytes` helper is accepted; it returns the exact written
count, reporting the original fatal diagnostic on short writes. Its success
proof matches all 46 buffered bytes and typed FILE state before encoding
detection. [Verification](cases/resources-rich/write-verification.json) includes
22 rejection controls and separate build, capture, audit and fixture times.

`rc_detect_encoding` is accepted (151 bytes), matching at `rc_is_text_unicode`.
The 250-byte sample and its terminator, initial flags, source-file position and
all meaningful buffered input bytes match; [verification](cases/resources-rich/detect-verification.json)
records 23 rejection controls and separate timings.

The internal Unicode test is accepted (931 bytes), matching classification flags
and result before `rc_read_wide_line`. [Verification](cases/resources-rich/unicode-verification.json)
retains the 24 rejection controls and exact `GetCPInfo(0)` field/padding effects.
The native build now selects the shared MSVCRT 5.10 profile, matching its observed
512-byte input-buffer capacity after `fseek`. The original pair was reused for
the native-only profile rebuild.

The line reader is accepted (355 bytes). Its first UTF-16 line, complete temporary
allocation before/after use, ownership removal, classification result and all
meaningful input-buffer bytes match at `rc_prefix_equal`.
[Verification](cases/resources-rich/line-verification.json) records 27 rejection
controls and timings. Original debugger buffer reads are now batched without
changing the retained raw format; fixture execution remains native-only.

The case-insensitive prefix matcher is accepted (85 bytes). The fresh pair
matches at progress `(1,64)`, including all 3,776 buffered UTF-16 output bytes,
input-file state, exact 16-bit `towupper` results and legacy line-format effects.
[Verification](cases/resources-rich/prefix-verification.json) retains 27 rejection
controls, canonical CRT probes and timings. Native API observers keep bounded
samples while the already verified line-processing loop runs normally.

The progress helper is accepted (46 bytes). With the observed NULL window, both
versions finish all 126 input lines and match at `rc_preprocess`, including the
complete 6,762-byte UTF-16 intermediate file, 20 arguments, restored codepage and
closed input FILE. [Verification](cases/resources-rich/notify-verification.json)
records 25 rejection controls and timings; window-callback behavior is unexercised.

The 28-byte preprocessor wrapper is accepted. Its original stdcall-three-slot
interface forwards the same count and vector to the argument-widening helper,
then returns the named preprocessor body's result. [Verification](cases/resources-rich/preprocess-verification.json)
retains the unchanged intermediate artifact, typed forwarding proof and 24
rejection controls.

Argument widening is accepted (159 bytes). All 20 UTF-16 strings, pointer offsets,
terminator and exact owned-allocation contents match at the preprocessor body.
[Verification](cases/resources-rich/widen-verification.json) records the 566-byte
allocation, bounded original/native observation counts, 25 rejection controls
and separate timings. Historical ANSI arguments retain their raw identities;
only the two current temporary-path slots are mapped explicitly for this case.

The embedded preprocessor entry is accepted (449 bytes), matching at its first
argument fetch. Its 39 contextual settings, 31 option descriptors and nine nested
letter switches agree with the original; the complete packed argument allocation
and prepared UTF-16 input remain unchanged. [Verification](cases/resources-rich/preprocessor-main-verification.json)
records 25 fresh rejection controls, compiler builds and split timings.

The argument getter is accepted (31 bytes), matching at the option scanner with
19 remaining arguments and the exact `-CP` alias. Its actual cdecl callback pointer,
descriptor-table identity and duplicate flag are retained in the [verification](cases/resources-rich/next-argument-verification.json).
Thirty fresh rejection controls cover the newly observed scanner contract.

The option scanner is accepted (177 bytes). It selects `-CP#`, consumes `1252`
through the real getter callback, and matches at the independently trapped option
dispatcher. [Verification](cases/resources-rich/scan-options-verification.json)
records the exact descriptor/operand aliases, remaining count, 30 fresh rejection
controls and separate timings.

The option dispatcher is accepted (405 bytes), matching the numeric helper's
exact `1252` operand and descriptor ownership. [Verification](cases/resources-rich/apply-option-verification.json)
records 29 fresh rejection controls. Unvisited `_wcsdup` branches use a typed
fail-closed shared import; numeric CRT behavior is still independently gated.

The numeric option helper is accepted (135 bytes). The complete option loop now
matches at preprocessor setup, including three definitions, four include paths
and exact input/output aliases. [Verification](cases/resources-rich/option-number-verification.json)
records fresh numeric/error-state evidence and the retained investigation timings.

[Host I/O](cases/resources-rich/host-io-verification.json) uses libc for logs and
control files, leaving only standard descriptors open at DLL/RC entry. Actual
callback output still uses the Win32 CRT shim for matching CRLF bytes. The new
error proof also corrected shared successful-path errno leaks. The original
help completion and its rejection matrix pass with the current native host,
reusing the unchanged original reference.

The rebuilt dispatcher's compiler-generated switch data is [verified from COFF](cases/resources-rich/dispatcher-switch-verification.json),
including exact relocation targets, index bytes and linked bounds. This supplies
the existing Rust metadata interface; code/RET/ownership checks remain strict.

Preprocessor setup is accepted (626 bytes), matching at the first character-token
assignment with its four forwarded parameters, parsed option lists and error
state intact. [Verification](cases/resources-rich/open-preprocessor-verification.json)
records 30 fresh rejection controls and separate build/capture/audit timings.
Macro creation and source opening remain behind named trapped callees.
