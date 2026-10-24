# RCDLL reconstruction

Target: `toolchains/5.10/bin/RCDLL.DLL` (the toolchain contains no RCDLL.EXE).
The first `RC` export `/help` case is complete; the rich resource invocation is
in progress. Conventional entries are `main`, `DllMain`, `_DllMainCRTStartup`
and `_CRT_INIT`. Historical logical names are mapped by bounded aliases without
editing raw evidence; [naming verification](cases/help/conventional-entry-names.json)
records unchanged ABIs and function extents.

Ninety-four application functions (25,168 original code bytes), two embedded CRT functions
(555 bytes), and two import-thunk boundaries have been accepted. The rich case
currently matches at the next named trap, `rc_parse_macro_parameters`.

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
bytes. Current accepted application extent coverage is 37.96% (94 functions). This denominator
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

The character-token setter is accepted (30 bytes), matching all four control
characters plus `$` before its character-class update. Only encountered lexical
entries are defined and compared. [Verification](cases/resources-rich/character-token-verification.json)
records the original byte-index behavior, 29 fresh controls and retained captures.

The character-class setter is accepted (30 bytes). The `$` class becomes16 and
execution reaches hashing `CASE_FROM_COMMAND`, with the entire macro buffer and
explicit definition-list cursor matching. [Verification](cases/resources-rich/character-class-verification.json)
records 29 fresh controls and separate build/capture/audit timings.

Macro-name hashing is accepted (26 bytes), matching the first definition call
with replacement `7`, length3 and parameter kind-2. The actual AL return,
macro buffer/length/hash and initially empty bucket table are preserved.
[Verification](cases/resources-rich/macro-hash-verification.json) records 30 fresh
controls, full typed frontier inputs and split timings.

Macro definition is accepted (374 bytes), reaching its zero-argument lookup
with all implicit name/hash/bucket inputs checked. The full body covers record
insertion, protected names and replacement storage behind named callees.
[Verification](cases/resources-rich/define-macro-verification.json) records 32 fresh
controls; the newly encountered SEH copy helper remains unaccepted.

Macro lookup is accepted (58 bytes), reaching the first copy with an initialized,
bounded 18-WCHAR name and a verified zeroed 16-byte pending record.
[Verification](cases/resources-rich/find-macro-verification.json) records allocation
ownership, 36 fresh rejection controls and timings. The SEH copy remains gated.

Macro-text copying is accepted (226-byte enclosing SEH owner) through the
[trailing-space variant](cases/resources-rich-trailing-space/README.md). Every
copy read fits initialized owned storage, the trim write is observed, and all
three macro records match at source opening. [Verification](cases/resources-rich-trailing-space/copy-verification.json)
records 40 gate controls, seven SEH-role controls, a native source-guard negative
and timings. The earlier no-space input retains its unsupported overread limit;
no allocator defaults were changed and native exception recovery is unclaimed.

Source opening is accepted (616 bytes), reaching the zero-argument refill with
its full source stack, file state and allocated buffer checked. The original
reuses a parameter slot for the new buffer; entry arguments and subsequent
ownership are checked separately. [Verification](cases/resources-rich-trailing-space/open-source-verification.json)
records 38 fresh rejection controls, the exact 512-byte CRT buffer at position
zero, and separate build/capture/audit timings. Refill remains a named trap.

Source refill is accepted (487 bytes), reading all 3,381 prepared WCHARs and
reaching filename duplication. Full destination bytes, terminator, initialized
tail and file state match. [Verification](cases/resources-rich-trailing-space/refill-verification.json)
records 45 fresh rejection controls and the shared VC5 fread correction: large
reads use the caller buffer directly, retaining the existing 512-byte stream
buffer. The correction reused the unchanged original capture with a fresh native
observation; no original recapture was needed.

Filename duplication is accepted (37 bytes), retaining the actual pointer
returned by wcscpy and binding the copied allocation to the source frame.
[Verification](cases/resources-rich-trailing-space/duplicate-source-name-verification.json)
records exact UTF-16 input/output ownership, 45 fresh rejection controls and
timings. Execution now reaches source-line emission.

Source-line emission is accepted (88 bytes), reaching the first output write
with exact `#line 1 ` bytes and all four write parameters checked.
[Verification](cases/resources-rich-trailing-space/emit-source-line-verification.json)
records full scratch/error state and 13 fresh format/write rejection controls;
unchanged transport, file and source-owner controls reuse their recorded hashes.
The shared formatter now supports the separately probed exact space suffix.

Buffered preprocessor output is accepted (49 bytes). Its first 16-byte write
returns one element and remains in the 4,096-byte stream buffer at physical
position zero. Only the initialized written prefix is compared; unwritten
allocator tail bytes are unclaimed. [Verification](cases/resources-rich-trailing-space/write-preprocessed-verification.json)
records 12 fresh output/quote-frontier controls, hash-bound prefix reuse and
timings. No shared fwrite change was required for this write.

Filename quoting is accepted (69 bytes), reaching character classification
with the actual 16-bit `S` argument, source/destination cursors and first two
written WCHARs checked. [Verification](cases/resources-rich-trailing-space/quote-source-filename-verification.json)
records nine fresh controls and the contextual 256-WORD token table. Its
pristine original-compiler data matches the original, including the separate
class[0] alias used by token index256. Other class defaults remain unknown
storage placeholders; a static scope control verifies they are unclaimed.

Character-token lookup is accepted (43 bytes), completing escaped filename
output and reaching macro listing. The marker's actual local ownership, name,
mutable empty replacement, initialized fields and disabled-listing input match;
its two uninitialized padding bytes remain unclaimed.
[Verification](cases/resources-rich-trailing-space/character-token-lookup-verification.json)
records ten fresh controls, exact buffered output and hash-bound prefix reuse.
The getter preserves the BOM, above-table default and index256/class0 cases.

Macro listing is accepted (195 bytes), preserving the APSTUDIO option handlers
and listing-record formatter. This case takes the disabled-listing return:
its marker is unchanged, ret4 restores the stack, and line scanning begins with
the same cursor, source stack and output buffer.
[Verification](cases/resources-rich-trailing-space/list-macro-verification.json)
records nine fresh controls, retained observer corrections and separate timings.
The unvisited general formatter remains unsupported in native execution.

Line-start scanning is accepted (281 bytes). It consumes the prepared source's
leading `#`, advances the cursor one WCHAR and increments the line before reaching
the zero-argument directive handler. The complete scanner retains comment, splice,
refill, progress and newline-output paths behind named unaccepted helpers.
[Verification](cases/resources-rich-trailing-space/scan-line-start-verification.json)
records seven fresh character/cursor/line controls and hash-bound prefix reuse.
Unknown character-class defaults remain unconsumed on this path.

Directive dispatch is accepted (1,892 bytes), reaching the zero-argument
nonspace reader with expansion/directive flags zero and source state unchanged.
The full body preserves conditional, include, definition, line, diagnostic and
VC5 code-page pragma branches behind named helpers.
[Verification](cases/resources-rich-trailing-space/handle-directive-verification.json)
records five fresh controls and the separately restored 14-entry switch table.
Unencountered diagnostic token-descriptor slots are unavailable storage
placeholders; their zeros are not valid descriptors or compared target data.

The nonspace reader is accepted (97 bytes), returning the actual UTF-16 `l`
from the prepared `#line` directive. Its caller and next identifier formal agree
on the 16-bit value, four-byte stack slot and advanced source cursor.
[Verification](cases/resources-rich-trailing-space/read-nonspace-verification.json)
records five fresh controls and separate build/capture/audit timings.
Comment, splice and input-resumption helpers remain named traps.

Identifier scanning is accepted (361 bytes), reaching character classification
for `i` with partial scratch `li`, hash76 and exact source/destination cursors.
[Verification](cases/resources-rich-trailing-space/read-identifier-verification.json)
records nine fresh controls, the saved source-frame ABI and an independently
reviewed contextual class table. All256 default WORDs match the original; setup's
`$` assignment is checked separately. The class getter remains trapped, and its
index256 alias into the adjacent `union` keyword remains unclaimed.

Character-class lookup is accepted (31 bytes) for the supported table scope.
Actual `i`/`n`/`e`/space calls return16/16/24/64 through AX with ret4, completing
`line` and its hash40. [Verification](cases/resources-rich-trailing-space/character-class-lookup-verification.json)
records nine fresh controls and an upper-EAX scope control. Native index256
executes a source INT3 before its unavailable word is read; a plain Linux probe
links the production object and verifies SIGTRAP, corroborated by GDB. The
Win32 boundary word's mutable alias into `union` remains unclaimed.

Directive-name lookup is accepted (101 bytes), recognizing `line` as code10
through AL with ret4. Its encountered dictionary contains29 group boundaries,
13 suffixes and13 result bytes; both Win32 images match these typed entries.
[Verification](cases/resources-rich-trailing-space/directive-code-verification.json)
records ten fresh controls, an upper-EAX scope control and exact lexer entry
inputs. Native high-character indices trap before unavailable adjacent data;
the plain Linux probe links the production object. The next frontier is the
zero-argument lexer, with prior token kind1, null token value and cleared modes.

The lexer is accepted (2,831 bytes), reaching the number parser on the prepared
`#line 1` marker. Its actual WCHAR49 argument, digit class29, cursor14 and
inactive alternate numeric-buffer input match; the lexer has not returned yet.
[Verification](cases/resources-rich-trailing-space/next-token-verification.json)
records eight fresh controls and separate build/capture/audit/native times.
The full body preserves operator, quote, comment, identifier and source-boundary
paths behind named helpers. Only44 encountered descriptor rows are defined;
their kind is one BYTE, and padding/other keyword rows remain unclaimed.
A plain Linux guard probe links the production lexer object with an explicitly
controlled number dependency and confirms unavailable descriptor128 traps in the
lexer itself; descriptor7 returns94. This does not accept the number parser.

Number scanning is accepted (668 bytes), reaching the logical-character reader
after writing digit1 with radix10 and provisional token7. The body retains hex,
octal, floating continuation, suffix and integer-storage paths behind named
helpers. [Verification](cases/resources-rich-trailing-space/read-number-verification.json)
records eight fresh controls and exact initialized scratch/owner/cursor inputs.
The alternate buffer is only an address identity, with no contents, capacity or
data-extent credit. Native alternate mode traps before any dereference; mode0
reaches the named reader stub. No complete integer result is claimed yet.

A correction to acceptance0049's descriptive hidden-input field is recorded
[here](cases/resources-rich-trailing-space/corrections/0049-source-word/verification.json):
the next character is quote34, not space32. Both full raw source buffers and
cursors were already checked; the old proof incorrectly constructed32 in its
report. The current validator derives the word from both captured buffers.
Old source versions, raw captures and acceptance0049 remain unchanged; its eight
controls were rechecked against the archived ELF without running either binary.

Logical-character reading is accepted (65 bytes). Its actual quote34 input and
AX return, ret0 and two-byte cursor advance match; the number parser then
rewinds and terminates scratch `1` before integer conversion at radix10.
[Verification](cases/resources-rich-trailing-space/logical-character-verification.json)
records eleven fresh controls, including raw source-word mutation with a
matching declared word, plus an upper-EAX scope control. Descriptor guard probing
also passes after adapting its auxiliary dependency object to the defined number
parser. Source-boundary and splice behavior remain behind named helpers.

Integer conversion is accepted (238 bytes), reaching digit conversion with the
full32-bit argument49, radix10, original scratch cursor and zero accumulator/
previous value. [Verification](cases/resources-rich-trailing-space/integer-value-verification.json)
records seven fresh controls, including rejection of an argument whose low16
bits still match. The complete source retains signed decimal overflow checks,
octal/hex bounds and diagnostic paths. No integer result has returned yet.

Digit conversion is accepted (54 bytes): full32-bit character49 returns1 with
ret4, and integer conversion then returns1 with ret8. The next decimal-kind
helper receives that exact four-byte value. The caller's other four local bytes
remain uninitialized and unclaimed.
[Verification](cases/resources-rich-trailing-space/digit-value-verification.json)
records nine fresh return/cleanup/formal/storage controls and separate timings.
The full digit body preserves the alphabetic UTF16/towupper path.

Decimal integer-kind selection is accepted (28 bytes): value1 returns AL7
with ret4. The signed comparison and wrapping magnitude behavior are retained.
[Verification](cases/resources-rich-trailing-space/decimal-kind-verification.json)
records nine fresh controls and independent upper-EAX/token-spill scope controls.
The next storage function receives a physical four-byte token and a pointer to
four initialized value bytes in an eight-byte local. Valid numeric codes consume
only the token low byte; both raw DWORDs remain recorded, with no padding seeded.
Storage remains trapped; its full raw fallback needs its own guarded proof.

Integer storage is accepted (216 bytes), reaching type interning. The actual
node has token7, flags1 and value1; the separate20-byte type has flags0x1003.
Both private28-byte allocations, zero initialization, exact writes and the
interner's256 null buckets match.
[Verification](cases/resources-rich-trailing-space/integer-storage-verification.json)
records ten fresh controls, the independent review and native fallback guard.
The physical DWORD token remains intact in the Win32 fallback; native unknown
fallback codes trap before uncharacterized spill bits are consumed.

Type interning is accepted (159 bytes). Empty bucket3 receives one owned
24-byte entry; its20-byte type copy, cleared WORD16 and returned type/node alias
match. The next zero-argument filename-string reader sees the actual source
cursor16 and word114, with header/output modes0 and directive mode1.
[Verification](cases/resources-rich-trailing-space/intern-type-verification.json)
records eleven affected controls and the exact current input/FILE/heap state.
Earlier number-frame storage has expired and is not reread; unchanged prefix
controls remain bound by their prior record hashes. The comparator stays trapped.

String reading is accepted (290 bytes): `resources.rc` and its NUL occupy26
bytes, with the remaining4,064 bytes unchanged. Exact original cursor/terminator
bounds establish a contextual2,045-WCHAR work area. A two-byte union preserves
identifier hash/length bytes and the string-length WORD. The prior numeric
address anchor now aliases element1024 of that area; its earlier opaque proof
history is unchanged, and alternate numeric mode still traps in production code.
[Verification](cases/resources-rich-trailing-space/read-string-verification.json)
records fourteen fresh controls, the actual linked-object layout/alias probe and
renewed plain/GDB mode guard. The next end-of-directive helper receives `#line`
and sees source cursor42/CR13 with output and directive modes1.

Directive-end checking is accepted (75 bytes): context `#line`, ret4, and
CR/LF handling advance the cursor by two bytes before normal scanning resumes.
The next comparator has distinct owned20-byte descriptors with flags0x1003.
[Verification](cases/resources-rich-trailing-space/expect-end-verification.json)
records ten affected controls. The fresh pair caught an omitted scanner CR
literal in the mask; its exact four-byte shared ownership was corrected, with
no source change or code credit. The failed run and old acceptance remain
unchanged; the new physical mask differs by exactly those four bytes.

Type comparison is accepted (42 bytes): distinct owned20-byte types compare the
flags WORD, key DWORD8 and key DWORD4, returning1 with ret8. The next angle-name
reader has no formals and sees the actual source cursor84/word119 at
`windows.h>`, the full scratch buffer and parser flags.
[Verification](cases/resources-rich-trailing-space/equal-type-verification.json)
records eleven affected controls and timings. The native help fixture takes
9.73ms total; the rich case remains in progress. Nonempty include-marker writes
remain untraversed until their contextual storage proof is complete.

Angle-bracket include names are accepted (171 bytes). `windows.h` occupies20
bytes of the shared scratch; the other1,004 bytes are unchanged. The following
CRT copy writes20 bytes to contextual257-WCHAR marker storage, leaving494 bytes
unchanged. That selected native model fits verified loader-zero storage before
the header-mode DWORD; the original C array size and intervening two bytes remain
unknown. A production native guard stops names exceeding the selected model.
[Verification](cases/resources-rich-trailing-space/angle-name-verification.json)
records sixteen affected controls and the plain/GDB guard proof. Native-only
boundary256 succeeds within the model;257 stops before copying, without claiming
original long-path capacity. The next include-directory lookup sees the complete
100-slot list/pointees and the existing owned string-buffer interior scratch.

Include-directory search is accepted (241 bytes), trying local/local/include/SDK
paths and entering the SDK header. Three failed opens return0 with the real
error states; the fourth is pending at the comment scanner's actual INT3. The
new frame owns4,098 decoded characters plus a terminator. Its FILE has4,096-byte
capacity but only512 initialized buffer bytes; the remaining3,584 are unclaimed.
[Verification](cases/resources-rich-trailing-space/include-search-verification.json)
records fourteen affected controls, including matching invented `None` results,
and a positive unknown-tail control. A missed native boundary was corrected by
the shared maintainer: glibc `wcschr` had searched past16-bit NULs and omitted
separators. Canonical-provider probes and the shared UTF16 implementation restore
matching paths; the unchanged original capture was reused. The failed native
runs remain retained, and no CRT/data bytes are counted as application code.

Comment dispatch is accepted (41 bytes), reaching the logical-character matcher
with the full32-bit formal42 and the source cursor on the SDK comment star.
[Verification](cases/resources-rich-trailing-space/scan-comment-verification.json)
records ten affected controls; unchanged source/FILE/SEH controls are reused by
exact prior hashes. Both builds and native help pass (8.54ms total). The
dispatcher has not yet returned; neither comment scanning nor rich output is
claimed complete.

Logical-character matching is accepted (31 bytes): the whole32-bit argument is
compared with the decoded WCHAR; a match returns1/ret4 and advances the cursor
from2 to4. [Verification](cases/resources-rich-trailing-space/character-match-verification.json)
records eight affected controls, the actual typed return, and the next
block-comment scanner at plus43. Both builds and cached native help pass.

Block-comment skipping is accepted (112 bytes). The SDK opening comment reads
434 initialized bytes, advances13 lines and appends52 exact CRLF bytes before
returning with ret0. [Verification](cases/resources-rich-trailing-space/block-comment-verification.json)
records twelve affected controls and the next conditional-stack helper at
`#ifndef _WINDOWS_`, with nesting-1 and condition1. The encountered four-byte
CRLF literal is contextual data with no code credit. EOF/resume remains trapped.

Conditional-stack pushing is accepted (78 bytes): nesting-1 becomes0, the
function returns0/ret0, then its caller stores1 in condition slot0.
[Verification](cases/resources-rich-trailing-space/push-condition-verification.json)
records eleven affected controls and the actual next `#define _WINDOWS_`
parser frontier. The new caller branch has no initialized condition local to
claim; source/frame/FILE and macro state remain captured. Overflow is unvisited.

Define dispatch is accepted (307 bytes). Empty `_WINDOWS_` creates an owned
24-byte node and30-byte name block with NULL replacement text; its name copy
reads20 initialized scratch bytes. The next `WINVER 0x0400` definition is
pending in the replacement reader, with parameter count0 and the existing
4,090-byte string-buffer identity. [Verification](cases/resources-rich-trailing-space/parse-define-verification.json)
records fifteen affected controls, including matching invented pending returns.
The earlier six-copy observer limits stopped explicitly on the seventh copy;
those raw failures remain retained. Capture execution now holds the shared
original-capture lock, with wait time recorded separately.

Macro replacement scanning is reconstructed (902 bytes), reaching its first
buffer check before any write. It has consumed the first `0` of `0x0400`;
the next source word is `x`, with output pointer at the existing buffer start
and both operator flags0. [Verification](cases/resources-rich-trailing-space/replacement-reader-verification.json)
records ten affected controls and the typed32-bit bounds pointer. No length
return or numeric replacement completion is claimed. Parameter lookup and
quoted-copy dependencies remain named traps; cached native help still passes.

Replacement bounds checking is accepted (61 bytes): seven cursor checks below
buffer+4,086 return with ret4. The reader writes14 bytes (`0x0400` plus NUL),
leaving4,076 bytes unchanged, then reaches parameter lookup for `x0400` at
buffer+2 with parameter count0. [Verification](cases/resources-rich-trailing-space/macro-space-verification.json)
records fourteen affected controls. Original module dumping now reads each
actual debug-event hFile with explicit OVERLAPPED offsets and exact bounded
chunks; target/CRT/driver full hashes match. Offset/short/missing/size mutants
reject. This was verified in the ordinary fresh pair, with no extra probe.

Macro parameter lookup is accepted (64 bytes). Count0 returns-1/ret4 without
reading the parameter table; all three SDK object-like definitions complete.
The exact26-byte WINVER text owner contains the initialized numeric replacement
and serialization marker. [Verification](cases/resources-rich-trailing-space/parameter-lookup-verification.json)
records sixteen affected controls and the next no-formal `#if` evaluator at SDK
line71, including source/cursor, macro buckets and conditional state. Output
contains the source-derived `#line 23` marker; an erroneous expectation was
corrected without editing raw captures, and a matching wrong marker on both
sides is rejected. Native02 adopts the published host-only shared clock getter;
unchanged original01 is reused. Capture execution is concurrent again after
the shared explicit-offset transport release. Parameter-table traversal and
conditional evaluation remain pending; native help passes in12.35ms total.

The conditional evaluator wrapper is accepted (195 bytes): it resets the actual
parenthesis counter0 and expression lookahead1, then reaches the no-formal conditional parser
without consuming another source word. [Verification](cases/resources-rich-trailing-space/evaluate-condition-verification.json)
records nine affected controls, exact caller and hidden state, both builds and
cached native help. New original-event negatives refresh their dependent receipt
pin and must change bytes, so semantic checks cannot pass through unrelated
receipt rejection. Diagnostic branches and expression results remain unvisited.

Conditional-expression parsing is reconstructed (67 bytes), reaching the first
logical-OR child before any ternary dispatch or result. The source cursor and
all live expression state remain unchanged. [Verification](cases/resources-rich-trailing-space/conditional-expression-verification.json)
records nine affected negatives, each attributed to its typed state predicate,
and both builds plus cached native help. Retired or uninitialized parser locals
are not treated as observed inputs. Ternary branches remain untraversed.

Logical OR is reconstructed (41 bytes). Its first logical-AND call is the next
actual named trap, with unchanged source and expression state. Both parser
operands remain explicit calls before boolean combination. [Verification](cases/resources-rich-trailing-space/logical-or-verification.json)
records nine affected semantic negatives, current graph/ABI checks and native
help. Neither an operator matcher return nor an expression result is claimed.

Logical AND is reconstructed (45 bytes), reaching the actual bitwise-OR child
with unchanged live expression input. [Verification](cases/resources-rich-trailing-space/logical-and-verification.json)
records nine affected semantic controls, original/native builds with identical
shared-source hashes before/after compilation, current graph/ABI checks and
cached native help. Operator matching and expression results remain pending.

Bitwise OR is reconstructed (32 bytes); its first bitwise-XOR child is the
actual named frontier. [Verification](cases/resources-rich-trailing-space/bitwise-or-verification.json)
records nine affected semantic negatives, unchanged complete live expression
input, both builds, graph/ABI checks and cached native help. No expression or
operator return is claimed.

Bitwise XOR is reconstructed (32 bytes), reaching the first bitwise-AND child
with unchanged actual expression state. [Verification](cases/resources-rich-trailing-space/bitwise-xor-verification.json)
records nine affected semantic negatives, both builds, current graph/ABI and
cached native-help checks. No operator or expression return is claimed.

Bitwise AND is reconstructed (32 bytes), reaching the actual equality-parser
child with unchanged live expression input. [Verification](cases/resources-rich-trailing-space/bitwise-and-verification.json)
records nine affected semantic controls, both builds, graph/ABI checks and
cached native help. Operator matching and expression results remain pending.

Equality parsing is reconstructed (64 bytes), preserving the original single
comparison instead of a loop. The first relational call is the next actual
frontier. [Verification](cases/resources-rich-trailing-space/equality-verification.json)
records nine affected semantic controls, unchanged live input, both builds,
graph/ABI checks and cached native help. The shared typed-only surface was
built coherently and accepted only after publication with identical bytes.

Relational parsing is reconstructed (114 bytes), preserving signed comparisons
and single-comparison dispatch. Its first shift-parser child is the next named
frontier. [Verification](cases/resources-rich-trailing-space/relational-verification.json)
records nine affected semantic negatives, unchanged live expression state,
both builds, graph/ABI checks and cached native help. Operator matching and
expression results remain pending.

Shift parsing is reconstructed (54 bytes), reaching the actual additive-parser
child before any shift operand/result. [Verification](cases/resources-rich-trailing-space/shift-verification.json)
records eleven affected semantic negatives, both builds, graph/ABI checks and
cached native help.

The evaluator's expression lookahead byte at original RVA149dc is distinct from
the lexer byte at144f4. Earlier source wrongly reused the lexer variable; both
normal values happened to be1, so old value-only receipts missed that alias.
The [approved correction](cases/resources-rich-trailing-space/expression-storage-correction.json)
introduces distinct storage, binds exact original/rebuilt readers and writers,
and proves noncoincident reset sentinels in original and production native runs.
Wrong-read/write mutants reject. Historical raw and receipts remain unchanged
and are explicitly limited; the correction adds no code or data extent credit.

Additive parsing is reconstructed (52 bytes), reaching the actual multiplicative
child before arithmetic operands/results. [Verification](cases/resources-rich-trailing-space/additive-verification.json)
records eleven affected semantic negatives, current distinct lookahead/lexer
symbols and widths, both builds, graph/ABI checks and cached native help.
The coherent shared snapshot was accepted only after identical-byte publication.

Multiplicative parsing is reconstructed (85 bytes), preserving the signed i386
multiply/divide/remainder loop and reaching the actual unary-parser child.
[Verification](cases/resources-rich-trailing-space/multiplicative-verification.json)
records eleven affected semantic controls, unchanged live input, both builds,
graph/ABI checks and cached native help. No arithmetic result or division
exception behavior is claimed by this checkpoint.

Unary parsing is reconstructed (65 bytes), reaching its first token matcher
with actual token55: a one-byte formal, four-byte physical stack slot and full
raw `37000000` retained. [Verification](cases/resources-rich-trailing-space/unary-verification.json)
records seventeen affected semantic controls, both token globals' distinct
storage/widths, full unchanged live source state, both builds, graph/ABI checks
and cached native help. Matcher prefetch/result and primary/unary operands
remain pending; the147-byte primary parser is a named dependency.

The [lexer storage correction](cases/resources-rich-trailing-space/lexer-storage-correction.json)
binds expansion depth16f00 separately from include depth14a10, and unifies
command/listing/marker state25058 as `rc_apstudio`. Earlier source confused
these identities. A bounded26-control address/width review, opposite-state
production-object branch tests and coherent wrong-storage negatives check the
correction. Historical raw stays unchanged; this adds no reconstruction credit.

Expression token matching is reconstructed (197 bytes), reaching the actual
475-byte `defined` helper with the16-byte UTF16 identifier in shared scratch.
[Verification](cases/resources-rich-trailing-space/expression-match-verification.json)
records20 affected semantic negatives, the saved BYTE55/four-byte matcher slot,
source cursor4678 at `(`, distinct expression/lexer/value storage, both builds,
graph/ABI checks and native help. Helper and expression results remain pending.

The `defined` operator is reconstructed (475 bytes), reaching its first159-byte
whitespace dependency before source/buffer writes. [Verification](cases/resources-rich-trailing-space/defined-operator-verification.json)
records20 affected controls, the actual name and two initialized parent locals,
buffer identity, original/native builds, graph/ABI checks and native help.
Alternate numeric mode traps in the production native body. The record also
documents a repaired gate-report sequencing error; historical raw and acceptance
records remain unchanged. Operator and expression results remain pending.

Expression whitespace skipping is reconstructed (159 bytes). Both actual calls
return1/RET0 with the complete4090-byte work buffer, cursor and line unchanged.
The caller then consumes parentheses and finds `RC_INVOKED`, prefilling value1
and expression lookahead7 before the primary parser trap.
[Verification](cases/resources-rich-trailing-space/expression-space-verification.json)
records24 affected controls, both builds, graph/ABI checks and cached native help.
The original default unary tail jump and native C call retain different raw
frames; exact instruction/return evidence binds their logical chains explicitly.
Primary and final conditional-expression results remain pending.

Primary-expression parsing is reconstructed (147 bytes). Two invocations return1
and0 for the SDK's defined tests; the next invocation enters parenthesized
`WINVER` evaluation and reaches the real macro-expansion stub.
[Verification](cases/resources-rich-trailing-space/primary-verification.json)
records22 affected semantic controls, seven native workspace-guard probes,
nested include thread/ESP/LIFO checks, both builds, graph/ABI checks and native
help. The new macro-output cursor uses its actual initialized work-buffer alias;
[its field mapping](cases/resources-rich-trailing-space/primary-hidden-inputs.json)
records the original pointer relocation and the rejected zero assumption.
The third primary result and complete rich resource case remain pending.

Macro expansion is reconstructed (921 bytes), reaching its first context-push
child with the owned `WINVER` record. The initialized parent result1/parameter0,
distinct saved pointers, line and argument counter, and untouched1280-byte frame
array match. [Verification](cases/resources-rich-trailing-space/macro-expansion-verification.json)
records17 fresh semantic negatives, an opposite-state production-ELF store probe,
both builds, graph/ABI checks and cached native help. Six named dependencies
remain traps; no completed expansion or rich-case result is claimed.

Macro-context push is reconstructed (155 bytes), returning with RET4 and saving
the first typed20-byte frame. [Verification](cases/resources-rich-trailing-space/macro-push-verification.json)
records the actual counter/pointer transitions, untouched frame/reserved bytes,
18 affected negatives, a nonzero-state production probe, both builds, graph/ABI
checks and native help. The next no-argument selector and whole expansion remain
pending. The record retains an earlier15-second debugger timeout and native-only
retry; native fixtures still take about9ms.

Macro-text selection is reconstructed (28 bytes), switching to owned `WINVER`
text and changing its flag0→1. The newly reached number-reader mode writes exactly
14 initialized bytes into the existing work area and returns1/RET4.
[Verification](cases/resources-rich-trailing-space/macro-selection-verification.json)
records22 fresh semantic controls, nine production bounds probes, unchanged
Win32 section/relocation bytes, exact initialized D-marker provenance, both
builds and graph/ABI/native checks. The next no-argument input-resume helper
remains a trap; rich output and complete expansion are pending.

Input resume is reconstructed (279 bytes). It consumes the initialized D marker
and reaches the actual185-byte frame-finishing stub, preserving the frame, flag
and pending outer state. [Verification](cases/resources-rich-trailing-space/macro-resume-verification.json)
records17 affected negatives, exact marker/cursor/stack roles, the child’s noarg
ABI and live controls, both builds, graph/ABI/native checks and unchanged-source
shared publication. No frame pop, resume result or complete expansion is claimed.

Macro-frame finish is accepted (185 bytes). Its actual D-marker path writes
NUL/R, returns1 and allows the expansion to return; the later R-marker restores
the SDK source cursor and releases the live macro flag. Both runs reach
`rc_based_integer_kind(1024)`. Only four bytes of the caller’s eight-byte value
object are initialized; its raw tail remains unclaimed.
[Finish/lifetime verification](cases/resources-rich-trailing-space/macro-finish-verification.json)
records32 fresh affected rejections, four production-binary state/guard checks
and explicit original reuse after correction of an unvisited pending call edge.
The native cached help fixture takes about9ms; the rich case is still in progress.

Nondecimal integer classification is accepted (23 bytes): actual input1024 returns
BYTE7 with RET4. The signed comparison follows the original; caller AL consumption
and raw upper-return bits are recorded separately. Both runs now reach
`rc_skip_inactive(5)` with matching current WINRESRC source, FILE and2664 buffered
output bytes. [Integer-kind verification](cases/resources-rich-trailing-space/based-kind-verification.json)
records16 fresh rejections and the finite auxiliary-observer scope. Retiring old
observers after the actual leaf return brings the native debugger capture back
to14.9s, while application entry tracing and production traps remain active.

Inactive-block skipping is accepted (159 bytes), reaching its genuine directive
scanner with no arguments. Mode5, nesting0 and branch byte11 are freshly bound;
the scanner result and skip return are pending.
[Inactive-skip verification](cases/resources-rich-trailing-space/inactive-skip-verification.json)
records16 affected rejections, exact source/FILE/output state, the preserved
failed call-graph diagnosis and a narrowly bound original-reference reuse.
Whole-candidate graph preflight now runs after building and before new captures.
