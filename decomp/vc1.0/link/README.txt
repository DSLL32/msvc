Visual C++ 1.0 for NT: LINK.EXE reconstruction

Target: toolchains/1.0-nt/bin/LINK.EXE. Build with its original compiler,
linker, headers and libraries through Wine. Program logic is readable period C;
all 70 formerly lifted implementations and their generators were removed. Only
INT3 statements in pending named stubs remain. No assembly ABI exception exists.
Data is contextual and typed; no original section snapshots are retained.

C sources and headers live in src/; executable Python tooling lives in scripts/.
Run make and script commands from this binary root. Metadata, cases and build/
retain their existing roles. analysis/ is non-executable evidence only.

Nineteen cases are complete:
  /? (exit 3, exact 884-byte stdout, empty stderr)
  minimal-c, hello-win32, hello-dll
  library-create, hello-debug-dll
  library-list, library-extract, library-remove, library-list-trimmed
  library-dump-library, library-dump-object, library-dump-dll
  library-edit, library-dump-edited
  omf-library, omf-link, cpp-undecorate, dump-disasm-data

The two latest assigned families use immutable original-backed fixtures
link-omf and link-undecorate (86a7f87). The valid 32-bit OMF input covers segment,
public/external, explicit-allocation COMDAT, LEDATA and FIXUPP records. Both
archive conversion and direct linking match; the runner checks external 20 +
ordinary data 7 + COMDAT 15 = 42 and exits 0. Converted COFF preserves the original
symbol records, including its duplicate COMDAT definition/undefined records.
C++ archive dump/undecoration matches all 5511 stdout bytes, empty stderr and
exit 0 without normalization. Its original-toolchain source covers constructors,
destructors, inheritance, operators, qualifiers, overloads, arrays, references,
member pointers and function pointers. Templates were rejected by the original
compiler and are not claimed. Unreached demangler branches remain named traps.

Current verification: original-toolchain make -j4, all 21 cached native
entry/status/stream/artifact regressions, 553 direct/indirect/tail-call checks,
544 callback checks, 422 contextual-data checks, 707 original extent instruction
boundaries and 38 ECX-member return-cleanup checks pass. The previous 640-frontier
Win32 history remains historical evidence; current function acceptance uses a
masked original-only capture paired with native GDB.
cases/omf-undecorate-queue.json records case provenance and verification hashes.
SOURCE-NOTES.txt retains historical checkpoints and recovered implementation
choices; later completed checkpoints supersede earlier active-frontier notes.

The accepted .text extent union is 203445/281634 bytes=72.2374% (701 frontiers), up from
160229/281634=56.8926% at 528 frontiers and 110787/281634=39.3372% at 400.
This includes accepted inline tables/alignment, not dynamic branch coverage.
cases/disassembler-x86-rich/validation.json records the current union;
cases/coverage-after-omf-undecorate.json is the preceding checkpoint. Separate partial
body accounting at the preceding minimal-code checkpoint is reproducible with scripts/measure-components.py and the analysis-only
Ghidra inventory: 131627 accepted known target-body bytes, 26904 known CRT-body
bytes and 192 import-thunk bytes. Another 40008 accepted extent bytes are outside
that old body inventory. All one-byte inventory entries are excluded. The
unclassified remainder includes uninventoried bodies, inline data and padding;
no exact padding total or target-wide body percentage is claimed.

The expanded earlier cases cover C++ archive/member lifecycle and COMDATs,
full COFF/CodeView/FPO debug DLL linking, native CVPACK, resources, DEF/order
parsing, map/verbose output, archive/COFF/PE dumps and PE stack/heap editing.
Generated console programs exit 0. GUI executables and DLLs pass load/call,
300 painted pixels, WM_CLOSE, exit 0 and DLL unload where applicable. Streams,
status and artifacts match the frozen fixture and untouched-original oracles.

Normalization remains limited to structurally proved clocks and the independently
validated PE checksum derivation where an earlier fixture requires it. No path,
padding, output-order or arbitrary-byte normalization is used. OMF fixtures
normalize only proved archive/COFF/PE clock fields. Undecoration is raw-exact.
The editor's 71800-byte edited.dll is raw-exact, including stack 0x180000/0x2000
and heap 0x140000/0x2000. Native /release sets checksum 1 then emits LNK4058:
checksum calculation is unsupported on this OS. No success behavior is invented.

The debug DLL uses default mapped IO. Archive lifecycle/OMF/undecoration use
LINK32_DB=27, the buffered backend. Under Wine original mapped archive creation
otherwise leaves its 2 MB allocation and subsequent listing reports corruption.
Inputs are not trimmed or patched. Extracted alpha is raw-exact and the trimmed
archive omits beta. Native DEF limitations remain: NONAME and forwarder fixture
inputs exercise parsing, not effective ordinal-only/forwarder output.

Normal native fixtures and cached entry replay (all twenty-one cases):
    make -j4 check
    make -j4 replay
Select one cached native fixture:
    python3 scripts/check-native.py --trace --case cpp-undecorate
    python3 scripts/check-native.py --trace --case omf-link
Fixtures never invoke Wine, originals, PE helpers or automatic reference capture.
Original-toolchain compilation and Win32 reference/diagnostic work are separate
explicit operations; see the original-only build and analysis commands below.
Measure accepted extents and partial native-body components:
    python3 scripts/measure-coverage.py
    python3 scripts/measure-components.py

New acceptance uses an explicitly captured masked original and a native GDB
session. The old paired Win32 runners and their replay/reference targets have
been removed. No fixture or replay target can launch them. Original toolchain
files are never modified; pure scripts/indexes.py analysis can mask a scratch
original copy for a separately authorized original-only capture.

Runtime members come from relocation-target-verified original LIBC objects.
Older CRT differences, including text-mode ftell/fseek, are readable C in
src/runtime.c. The native C parser rejects __thiscall; demangler methods instead
use __fastcall(ECX this,ignored EDX,native stack arguments). This preserves
native ABI and individual graph vertices without wrappers or assembly. Recorded
symbols and scripts/audit-abi.py verify native callee return cleanup.

Both assigned families are finished; no further cases are queued. Analysis-only
cases/remaining-after-omf-undecorate.json ranks possible public-input follow-ups:
i386 -dump /disasm (~3707 uncovered known body bytes/12 functions) is practical;
valid Macintosh/m68k COFF linking (~16534 bytes/57 functions in a named-root
closure) is a higher-effort alternative requiring original-backed input first.
Remaining OMF closure 9800 bytes includes internal -g conversion code not selected
by the ordinary LINK wrapper (-o/-co), so that total is not promised for public
OMF debug input. Remaining Mac/ROM/bind/rebase/disassembler/diagnostic/dormant CRT
paths and unclassified bytes mean 99% whole-binary coverage is not established.
Shared fixtures remain immutable. Scratch/evidence/private Wine prefixes live
under ~/.ai-tmp/vc1.0-link. New cases require the user's next selection.

Original-only compilation and static analysis

These explicit build commands use Wine solely for the original compiler/linker;
they are never prerequisites of normal native check/replay:
    make -j4 all audit
    python3 scripts/measure-coverage.py
Historical receipts retain evidence of the removed Win32 replay infrastructure;
their former commands are provenance, not supported fixture commands.

Native Linux output checks

The original-toolchain Win32 build remains the default. An additional i386 ELF
build compiles the same reconstruction sources with the shared lib/recon shims:
    make -j4 native
    make -j4 native-check
    python3 scripts/check-native.py --case cpp-undecorate

Clang, 32-bit libc development files and a working -m32 compiler are required.
The native flags are -std=gnu89 -fms-extensions -fasm-blocks -O0 -g; the Microsoft
extensions express the existing calling conventions and pending INT3 stubs.
RECON_NATIVE_TEST enables the header mappings. Every reconstruction C/header
includes recon.h last; the header is empty in the canonical build. Native object
files live in build/native and the output link compiles lib/recon/src/crt.c and win32.c directly
alongside those objects; the traced link additionally compiles trace.c. No shim archive or alternate reconstruction is used.

scripts/check-native.py launches only the native ELF. It verifies captured input
and output hashes before launch, compares exit and streams exactly, and compares
artifacts using the fixtures' existing structural timestamp rules. Missing
captured references fail. Toolchain libraries and the editor message catalog are
read-only inputs with previously captured hashes. It never invokes Wine, an
original executable, reference-refresh tooling, generated PE programs, or the GUI
and DLL-load helpers. The native clock runs normally; only the timestamp fields
already declared by the immutable fixture may differ. Pacific timezone settings
match the captured NT CRT rendering, including daylight saving.

The native set comprises all eighteen existing cases: help, archive tools, dump
and editor phases, OMF conversion/link, C++ undecoration, minimal C and all three
GUI/DLL link fixtures. DLL self-spawn and the explicit CVPACK boundary are
described below. Each phase starts independently with captured
inputs. Fresh private working directories and timing receipts are written under
~/.ai-tmp/vc1.0-link/native-check-*. Receipts separate setup, native execution and
output checks; build timing is labeled unmeasured unless separately recorded.
Produced executable bytes are checked but never executed by this runner.

The uninstrumented output checker does not validate function entry order. The
optional native entry checks below retain the canonical ABI/graph and paired-
frontier audits as separate validation. Windows SEH fault
paths, console-signal delivery and arbitrary process-tree tests remain outside
its scope; registered synchronous LINK selfchildren are supported below.
DLL import-library children execute the native LINK ELF. Full-debug linking uses
the explicit verified-input CVPACK response boundary below. Actual GUI paint and
DLL load/call/unload remain separate Windows execution checks. Unknown external
PE dependencies fail rather than automatically running Wine.

cases/native-validation.json records the completed native adoption: all fifteen
registered phases passed with exact captured streams/status and artifacts under
the existing timestamp rules. It records per-case setup/execution/oracle times,
three earlier diagnostic fixture batches, measured build receipts and explicitly
unmeasured diagnostic/build work. Raw receipts and logs remain in the recorded
scratch directories. The original-toolchain parallel compile/link and all static
call-target, callback, contextual-data, ABI and extent-boundary audits passed.
The accepted 640 frontiers and 193671/281634-byte extent metric are unchanged.

Native application entry checks

Use a separate instrumented ELF while retaining the uninstrumented target:
    make -j4 native-trace
    make -j4 native-trace-check
    python3 scripts/check-native.py --trace --case cpp-undecorate

Only application objects use -finstrument-functions. The reconstructed CRT
overrides in runtime.c and final-link shared CRT/Win32/tracer sources remain
uninstrumented. The shared tracer resolves ELF function pointers to names once
per process, logs first entries through an inherited descriptor, and suppresses
the supplied accepted prefix. With no descriptor, callbacks are no-ops. Log
records contain newline-delimited native names, without numeric logical IDs.

scripts/native-traces.py derives all application definitions from native object
symbols, validates their source/identity against functions.json driver metadata,
and projects accepted/frontier_cases order after excluding CRT overrides, indexed
CRT routines and imports. C/fastcall decorations are normalized only for app
identities. All 581 application definitions, including unreconstructed stubs, are
in the filter; unmapped definitions fail rather than disappearing from a trace.
The existing metadata classifies 640 accepted identities as 466 application,
142 CRT and 32 import entries. No compiler-generated application thunk needed
a special exclusion in this build. The initial 15 native phases checked 365 new application entries; the current
18-case set checks all 466 accepted application entries. Four zero-new phases
require an empty new-entry transcript
and their existing concrete output/exit oracles. Earlier accepted app functions
are seeded as seen, including prior specialized case acceptances.

cases/native-traces.json is a cached projection, not a new original capture.
Metadata hashes and current native object definitions are checked before launch.
After deliberate acceptance-metadata updates, explicitly regenerate the cache:
    python3 scripts/native-traces.py --write
This operation reads metadata/objects only. A divergence preserves trace.seen,
trace.expected and trace.entries beside streams and receipt.json. An alternate
--trace-cache path supports scratch expected-sequence diagnostics. First-entry
segments do not compare repeated calls, earlier prefix order, callers or sampled
states. Unsupported external dependencies, Windows SEH and actual GUI execution remain
outside these native checks; registered LINK selfchildren are handled below.

cases/native-trace-validation.json records five matched before/after runs plus
one disabled-tracer run, per-case component medians and raw receipt hashes. The
first five requested baseline runs are retained separately: help initially lacked
LIB, while the cached original environment set it. Adding the captured search
path corrected entry order without changing help output; both matched timing
groups use the same LIB environment and the identical uninstrumented ELF.
No original/Win32/Wine reference was executed or refreshed for this work.

Measured whole-suite median including Python startup was 0.225415s uninstrumented
and 0.371439s traced (five runs each). The internal case-loop medians were
0.148648s and 0.201966s; separate trace-cache/object-symbol preflight was 0.088673s.
Median summed setup/execution/output-check times were respectively
0.033082/0.045927/0.063523s before and 0.051112/0.069779/0.069630s after; traced
sequence comparisons added 0.001855s. These are medians of per-run sums and do
not sum exactly to independent medians. The disabled instrumented run passed all
15 cases in 0.230985s externally (0.149876s internal; one sample). The measured
instrumented build took 4.467905s using the actual make -j2 command and is excluded
from suite timings; future builds above use the requested -j4. Deliberate wrong
expected names and missing app-definition classification both fail as required.

Historical context, without a new Wine run: the old 16-case ordinary continuous
Win32 adapter totaled 58.080934s. Selecting the same 15 case names as the native
suite from that receipt totals 48.923779s, after excluding hello-win32. That old
selection includes paired original/reconstruction traces (507 logical entries
including CRT/imports), a fresh untouched-original output reference and generated
PE execution where applicable. The new suite checks one ELF, cached app-only
segments and captured outputs. This is a historical scope comparison, not a
controlled cross-mode speedup. Two specialized DLL/CVPACK histories (182.604980s
previously) are excluded from that initial 15-case timing comparison. No reconstruction, coverage or acceptance changed.

All twenty-one native fixtures

The current commands build/run all twenty-one LINK cases, including hello-win32,
hello-dll and hello-debug-dll:
    make -j4 native-trace-check
    make -j4 native-check
    python3 scripts/check-native-boundary.py

Both DLL fixtures dispatch their exact -lib command to the actual native ELF,
preserving canonical Windows argv, relevant inherited environment, stdout/stderr
and trace descriptors. An explicit self-spawn option maps the canonical logical
LINK path to /proc/self/exe. A strict fixture broker validates the command and
allows that native child; it rejects any unregistered dependency. Child-only
application entries and independently generated import-library/export-object
bytes provide the case-specific execution evidence. Parent and child map one
613-byte seen-state file and append to the same name trace, matching the cached
global first-encounter history without suppressing new child functions.

The full-debug fixture reaches external CVPACK. Its test double verifies the
exact captured invocation, relevant environment and complete 2MB prepack DLL,
allowing only the existing 39 structural clock fields. The prepack checksum
sentinel is compared unchanged. Only after that check does it apply independently
captured packed bytes, transplant corresponding input clocks and recompute the
dependent checksum. Final DLL/lib/exp/map bytes still pass the existing immutable
fixture comparisons and checksum validation. Captured empty packer streams and
status 0 are checked; a nonempty captured stream is rejected because this boundary
does not replay streams. The real CVPACK implementation is not reconstructed.

No trusted prepack bytes remained in the original fixture/scratch evidence. One
explicitly coordinated original run captured them at the CVPACK process-create
debug event, before its code ran, and captured the response at process exit. Its
command, process tree, exit0 and empty child streams were recorded; all four final
artifacts and exact root streams matched the existing frozen references.
cases/native-boundaries/hello-debug-dll contains the pre/post data, events and
pinned tool/helper provenance. This capture is separate from native regression
execution; the runner has no original capture, Wine or reference-refresh path.

cases/native-all-validation.json records the final single-sample 18-case run:
all status/stream/artifact checks and 466 application first entries passed.
External suite time was 1.149772s, including 0.085628s trace preflight; the internal
case loop took 0.981600s. Summed setup/native execution/output-oracle/trace-check
times were 0.183790/0.160448/0.624741/0.002718s, with remaining overhead recorded
separately. CVPACK replay time is a subset of execution, not additional time.
A focused uninstrumented run of the three added cases also passed (0.855197s
external). Native final-link integration used make -j4; its 0.653580s measurement
and earlier 0.786518s traced-link measurement are separate from suite timing.
The necessary original boundary capture took 1.735003s, with helper build 2.208802s
separate; later capture-reference validation was unmeasured. No repeated benchmark
or new reconstruction/acceptance is claimed.

The host-only boundary rejection command mutates a non-clock .text byte and sends
a wrong CVPACK option in separate checks. Both reject before applying the packed
response, and neither launches a process. Actual Windows GUI painting, DLL
load/call/unload, SEH and console behavior retain their separate Windows evidence.
The initial 15-case adoption and before/after timing receipts above remain
historical records; the subsequent one-function-native-validation receipt adds the nineteenth case.

One-function native reconstruction trial

initialize_disassembler is readable cdecl C in src/disassembler.c. Its complete
19-byte original body stores the supplied fatal/warning callbacks, with no
callees or inline code data. A data-only original-compiled COFF object selects
/dump /disasm without entering the pending instruction decoder. Fresh original
entry/return capture and named native GDB break/finish agree on callback state
and caller-owned argument cleanup; output is exact. The cached nineteenth case
contains no Wine or capture path. See cases/one-function-native-validation.json.
That trial reconstructed exactly one original function. The subsequent code-bearing
disassembly case below completes its reached instruction path.

Code-bearing i386 disassembly checkpoint

cases/disassembler-code/inputs/test.obj is the original-compiled minimal-C object,
with18 code bytes. Its untouched-original /dump /disasm output was captured once
outside fixture execution. Twenty-three further functions now decode and format
its PUSH/MOV/SUB/JMP/POP/LEAVE/RET sequence exactly, preserving distinct operand
helpers and the original mode/flag/register effects. Native GDB and masked-original
frontiers agree on54 scalar state words plus instruction bytes and relocated
pointer relationships. The final full streams and exit0 match the fixed reference.

The20 native fixture suite checks490 application entries in1.384s external wall
(single sample); its internal loop is1.193s and trace preparation0.093s. The
canonical compiler build/audit takes31.629s; native plus trace build3.724s.
See cases/disassembler-code/validation.json for individual measured phases,
per-case timing, accepted extents/ABIs and hashes of private scratch evidence.
The19-case midpoint regression is also retained in scratch; no fixture runs Wine.

Remaining disassembler handlers are individually named traps. The old Ghidra
inventory's zero remaining i386 entries is incomplete: a separate original CFG
survey finds at least7565 then-unaccepted instruction bytes in102 decoder bodies,
with five unresolved indirect switches. The completed small case consumes some
of those leaves. This is not an exhaustion or branch-coverage claim.
cases/disassembly-queue.json records the user-selected next stages: richer x86
addressing/branch/prefix/x87 disassembly, then original-validated Macintosh/m68k
COFF linking. No public-input reachability is claimed before original capture.

Richer x86 disassembly checkpoint

The original-compiled661-byte COFF object contains181 code bytes across four
functions. Thirty-seven further readable-C functions complete its addressing/SIB,
16-bit operand/address overrides, arithmetic/shift/branch/immediate, string and
seven x87 opcode groups. Complete status and output match the untouched original
capture. Only reached functions are accepted; their other recovered branches
remain statically reconstructed, not dynamically claimed. Other handler targets
remain named traps. The fixture input's annotated16-bit _emit bytes and original
repnz/ST spelling quirks are documented in cases/disassembler-x86-rich/README.txt.

All21 cached native trace/output fixtures pass, checking527 application entries.
Single-sample external wall is1.373s, case loop1.188s, trace preparation0.098s;
summed setup0.303s, execution0.190s, output oracles0.681s, trace checking0.003s.
Canonical build/audit takes31.886s and native plus trace build3.936s. See
cases/disassembler-x86-rich/validation.json for per-case and original capture
accounting; fixture execution never invokes Wine or a PE child.

The old partial inventory misses genuine decoder code. The reproducible read-only
scripts/measure-disassembler.py reports61 accepted decoder/output bodies with9406
actual instruction bytes,152 inline switch-table bytes and216 verified alignment
bytes. Its5699 instruction bytes outside the old inventory expand the known
accepted target-body union to137326 bytes. Known accepted CRT bodies occupy26904,
import thunks192;39023 accepted extent bytes remain unclassified. These are partial
component counts, not a target-wide body denominator or whole-binary padding total.
The old inventory's zero remaining i386 entries remains an inventory limitation.

The richer x86 stage is complete. cases/disassembly-queue.json retains the selected
next stage: original-validated Macintosh/m68k COFF linking, covering only loader,
relocation and resource paths actually reached by valid original inputs.

Minimal Macintosh link checkpoint

The original-validated 84-byte m68k COFF case now completes through 26 additional
C functions. It emits the exact captured 2 MB Mac-resource PE, excluding only the
structurally identified four-byte COFF timestamp. Its banner, stderr and exit
status match. No Macintosh runtime execution is claimed. Contextual symbol,
fixup, jump and resource structures preserve the original graph and data layout;
the External archive-member and weak-default slots have explicit Mac unions.

All 22 cached native cases pass with 553 checked application entries. The single
measured suite takes 1.352s externally; canonical build/audit takes 33.315s and
native trace build 4.053s. Coverage is 211113/281634 = 74.9601%,727 frontiers.
Audits pass579 call graphs,570 callbacks,445 contextual data definitions,733
instruction boundaries and38 ECX cleanup checks. Commands remain make -j4 check
for captured-only native regression and explicit make -j4 audit for original
compiler compatibility. Detailed timing and original/GDB evidence are in
cases/m68k-minimal/validation.json. The frozen m68k-reloc-loader case is next.

Mac relocation/loader checkpoint

The queued 368-byte object now closes through thirteen more C functions. Its
CTOT16, CTOABSC32, CTOABSD32 and DTOABSD32 relocations produce the original A5
thunks, patched code/data, compressed fixups, loader prologues and resource map.
Fresh masked-original/native-GDB runs exit 0 and match the exact frozen 2 MB
artifact after only the COFF timestamp is ignored. The full backend and classifier
retain their other static branches; this case does not claim to exercise them.

All 23 native cases pass with 566 application first entries. After the shared
getcwd addition, the final native/native-trace build takes 6.336s and the final
single-sample suite takes 1.388s externally (1.214s loop,0.091s trace preparation;
summed setup0.298s,execution0.183s,oracles0.720s,trace checks0.003s). The prior
pre-header-change run is retained, not hidden as a timing replacement. Canonical
build/audit takes16.071s;592 calls,583 callbacks,449 contextual data definitions,
746 instruction boundaries and38 ECX cleanup checks pass.

Current accepted extent is217053/281634=77.0692%,740 frontiers. All39 accepted Mac
bodies have13203 genuine CFG instruction bytes,268 inline data and137 alignment.
Augmenting the older partial inventory with Mac/disassembler CFG evidence proves
150529 accepted target-body bytes,26904 CRT bytes and192 import-thunk bytes;
38655 accepted extent bytes remain unclassified after separately identifying420
inline-data and353 alignment bytes. Unclassified bytes are not all padding, and
these partial inventories provide no whole-target body percentage.

See cases/m68k-reloc-loader/validation.json,body-inventory.json and components.json.
The authorized code disassembly, richer x86 and Macintosh stages are complete.
No further case is running; remaining.json records suggestions for user selection.

DEF directive checkpoint

The approved next queue is cases/definition-mac-debug-queue.json. Its first family
now closes through four C functions: HEAPSIZE/STACKSIZE parsing, SECTIONS/SEGMENTS,
the shared attribute lookup and IMPORTS consumption. The old linker explicitly
warns LNK4053 and ignores IMPORTS; no imported-symbol emission is claimed. Both
full2MB library/export objects preserve the original emitted directives and match
captured references after only structurally identified clock fields.

All24 native cases pass with570 application first entries. The measured loop is
1.508s with0.100s trace preparation; combined canonical/audit/native-trace build
is16.786s. Coverage218415/281634=77.5528%,744 frontiers. Graph596,callback587,
data450,boundary750 and38 ECX cleanup checks pass. See cases/definition-directives
for frozen inputs, complete outputs, validation, CFG and partial component counts.
The next approved family is original-validated Macintosh COFF debug and swapper.

Mac COFF debug and swapper checkpoint

The second approved family now completes through four contextual C functions in
src/mac-debug.c. A derived401-byte m68k input defines ___InitSwapper and emits the
original full COFF debug directory/symbols, Mac debug records and swapper resource.
The captured absolute DOS output spelling is preserved using validated N: cwd
mapping; only the three structural clock fields are normalized. There is no Mac
runtime or CodeView-packer claim. A separately sampled uninitialized relocation
cursor is documented with producer/non-reader/serializer evidence, never used as
an output exclusion. Complete fresh original/native artifacts still match.

All25 native cases pass with574 application first entries. Single-sample elapsed
suite1.801s externally:1.610s loop,0.104s trace preparation; summed setup0.351s,
execution0.197s,oracles1.047s,trace checks0.003s. The canonical original compiler,
audits and native-trace build take14.768s. Checks pass600 graphs,591 callbacks,
453 data definitions,754 instruction boundaries and38 ECX cleanup checks.

Current extent219208/281634=77.8344%,748 frontiers. Partial body accounting proves
154101 accepted target instruction bytes,26904 CRT bytes and192 import-thunk bytes;
632 inline-data and482 alignment bytes are separately identified. Accepted36897
bytes remain unclassified by the old partial inventory, not all padding. Known
remaining target bodies occupy21212 bytes, plus30874 unclassified remaining bytes;
these figures provide no complete native-body percentage or99% promise.

See cases/m68k-debug-swapper/{validation,coverage,body-inventory,components}.json.
Both families in cases/definition-mac-debug-queue.json are complete. Work is paused;
remaining.json suggests original-validating Mac static/resource-ID relocations
(1766 known bytes/10 bodies) or old C++ undecoration grammar(1012/8), with no new
case started. Standard regression command remains make -j4 check (native ELF and
cached references only); make -j4 audit explicitly checks original compilability.

Mac static/resource/relocation checkpoint

Three original-validated cases now close static-symbol and near/static jump
references, explicit CODE/INIT resource IDs, standalone-code validation, packed
native/P-code references, __nep entry aliases and duplicated code fixups. Twelve
C vertices cover all ten nominated bodies plus two comparator callbacks. A prior
standalone/no-loader layout condition was corrected against the original; full
native regression passes afterward. Identical qsort/bsearch comparator bytes
retain separate native graph identities. Resource-ID assignment quirks remain.

All28 native cases pass with586 application first entries. Single-sample suite
1.923s external:1.732s loop,0.102s trace preparation; combined canonical original
compiler/audit/native-trace build14.374s. Audits pass612 graphs,603 callbacks,455
contextual data definitions,766 accepted boundaries and38 ECX cleanup checks.
Accepted extent221068/281634=78.4948%,760 frontiers. The55 Mac CFG bodies contain
15828 instructions,268 inline data,165 alignment bytes; no branch coverage claim.

cases/m68k-dupcon/family-validation.json records the family and per-case evidence.
A failed two-source original probe remains separate from the successful DUPCON
fixture. The first stage in cases/mac-special-undecorate-queue.json is complete;
old __based/template/vbtable undecoration is next, with original compiler-backed
__based and vbtable input preparation already successful. Fixtures remain native
ELF/cached-reference only; explicit original captures/builds occur outside them.

Old C++ undecoration checkpoint (approved queue complete)

Three new frozen cases close compiler-produced __based/vbtable names and authored
old-template/truncated-name inputs. The compiler rejects template source syntax;
the formatter inputs explicitly preserve raw-name fallback, __based(NYI:*),
quoted __segname syntax and " ?? " status markers. Seven full C helpers were
reached; the nominated57-byte merge-error helper stays pending because all11
native calls require flags absent from LINK's sole fixed0x800 wrapper. Ordinary
literals remain contextual; member ABIs retain RET4/RET8 via ignored-EDX fastcall.

All31 native cases/593 application entries pass. Single-sample suite1.966s
external:1.770s loop and0.104s trace preparation. Summed setup0.377s, execution
0.217s, oracle1.156s, trace checks0.004s (trace check is within other totals).
The native-trace rebuild/static audit step took17.168s; canonical original-toolchain
compilation passed earlier. Intermediate build/preparation times were not all measured. Audits pass619 graphs,
610 callbacks,455 data definitions,773 boundaries and41 ECX cleanup checks.
Accepted extent222078/281634=78.8534%,767 frontiers. New C++ instruction bytes955
are distinct from1010 accepted extent bytes (36 switch table,19 alignment).

cases/cpp-pointer-status/family-validation.json records evidence/timing/limits;
reachability.json explains the pending helper. Component inventory remains
partial:156891 accepted target instruction bytes,26904 CRT,192 import thunks,
920 contextual inline data,1482 alignment and35689 accepted unclassified bytes.
The unclassified portion is not all padding. Both approved families are complete;
worker pauses. remaining.json suggests FPO/symbol-map dumping and absolute/weak
symbol resolution as possible next public-input cases, without starting them.
