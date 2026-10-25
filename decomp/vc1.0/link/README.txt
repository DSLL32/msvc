VC1.0 LINK.EXE reconstruction

Build the native i386 shared library and run the cached cases from this folder:

    make -j4 native
    make -j4 check unit

The shared recon-tool harness discovers the 97 numbered case directories. Native
fixtures use only captured original inputs/results. They never run Wine or
refresh references. The original compiler and graph/ABI audit are separate:

    make -j4 win32 audit

src/ contains application reconstruction only. harness/ contains native setup,
artifact/process policies and other host glue. crt/runtime.c retains the 16
historical binary-specific overrides: LINK's older stdio buffering/positioning,
low-level open/seek, locale case conversion, timezone/DST and process argument
handling differ from the shipped LIBC objects. Their existing body/ABI proofs
and canonical graph checks justify retaining them; migration adds no CRT code.
crt/anchors.c selects the original static CRT termination-table objects and is
link scaffolding, not an application function or new CRT implementation.

binary.c is the canonical original-RVA function registry. Its implemented App
set now has 816 application functions (759 public-reached, 57 direct-support),
with accepted App extent union 230745/281634 (81.9308038%). The resource resize/release
lifecycle adds two whole functions/66 bytes after the OMF header/error
checkpoint. Support-inclusive historical accepted extent is 261684/281634
(92.9163382%). The 1015 historical frontier count includes shipped CRT/import
support; 16 old handwritten crt/runtime.c overrides remain separate, without new
application credit. A complete non-CRT denominator remains unavailable. At the prior cleanup checkpoint the old
partial body/data/alignment inventory left 11108 unaccepted bytes unclassified;
that historical inventory was not recomputed for this case and is not a current
registry classification count. It includes already classified, unreached CRT such as __cwild and helpers.
The current registry leaves 8983 bytes outside known App/CRT/import extents,
including the explicit Unknown rows. This gap includes code/data/alignment and
incomplete placeholder extents, not a reliable application denominator; no gap
has been relabeled as CRT merely because of its location or appearance.
These extent totals are not a count of executed instructions. Unknown provenance
is not treated as CRT. Direct callbacks remain explicitly
external test inputs and do not establish public command-line reachability.

Each capture preserves its original application first-entry suffix and explicit
previously-seen set, full status/streams, inputs and output inventories. Ordered
historical workflows now use independently frozen prior-stage inputs: each
producer checks its complete output and each consumer receives the same saved
contract. The migration no longer claims a live file handoff between invocations.

harness/fixture-artifacts.c is native test support. It validates structural clock
field inventories and raw PE checksums before the harness compares all remaining
bytes. The compact release variant retains the original physical 2MB/logical 3072
checksum defect. Host-only guards retain the unsupported buffered function-export
heap-pointer serialization and C6 LINNUM stale-use routes; Win32 source behavior
is preserved. No allocator repair or pointer/padding normalization is applied.

Case 0091 resumes the approved direct DemangleName reference-composition
workflow: plus-reference, append-reference, assign-reference and error merging.
The real public demangler installs allocator/release callbacks; the first actual
512-byte callback runs the owned-name workload while its outer allocation stays
live. Nested allocations and stack name/reference/string owners stay live through
every read. Normal demangler cleanup frees every owned block exactly once.
No parser globals, allocator failure or internal mode are seeded.

The typed original probe uses normal Win32 file reads and the target's already
initialized malloc/free at RVAs 41160/40fa0. It does not initialize or pretend to
use a private helper CRT. This version 3 dependency differs explicitly from the
older initialized-helper v2. Fresh masked originals reached the four methods in
order, and the complete original v3 3856-byte report is byte-identical to v2:
SHA256 b3bb977c4c6a71fc8aba18a10684ebef58595bf6d75a723d49247b9a2e84d32f.
Native checks the complete report/status/streams with no output zero ranges.
The original probe completes through the shared helper, without claiming a target
CRT-continuation run; the real demangler's own parser/cleanup does return normally.

The prior failed full-temporary-flags comparison remains preserved. Only 403f0's
actual 8-byte local at original entryESP-8/current native EBP-14 has unconsumed
upper 26 bits. Its head and low six bits are strict; copy/status/reference/node
callee closure consumes only those bits, and final copy transfers only the low
six into the caller's independently owned result. Raw upper bits are retained in
GDB packets, not claimed equal. All caller-owned flags/hidden results and every
serialized result byte remain full-width strict. Six actual detailed observations
match the retained original append-frontier packet under that exact qualification;
focused offline controls reject head/low-bit/caller-upper-bit changes. The old full
baseline did not observe the temporary at later calls, and is not relabeled as a
complete temporary-state oracle.

The original v1 setter quirk and observer-join failure remain distinct historical
evidence. Exact old WIP/source/raw results are retained at:

    /root/.ai-tmp/msvc2/vc1.0/link/standardization-pre-20261025/
    git-owned.diff SHA256 ccc2226bd8d338a57714839d6ab6881c3a7e4a5026391857cc80c47eae196869

Current ordinary scratch files, frontier receipts, native GDB packets, source and
timing references are at:

    /root/.ai-tmp/msvc2/vc1.0/link/demangle-resume/

The initial custom-helper CRT link failure, occupied historical cwd, one-byte
placeholder extent omission and diagnostic GDB-stop termination are retained.
They are not reported as target semantic failures or successful completed runs.

Current shared validation checks 853 graphs for calls, RET cleanup, live function
references, indirect topology, ownership and instruction boundaries. The 41 ECX
member declarations and existing layout assertions are retained. Historical
validation of 514 contextual initializers is preserved in Git; it is not a
recurring shared parity check. data-symbols.json retains useful contextual
ownership metadata. The superseded linear relocation scan is retired.

Migration validation passes all 91 cases and 7 narrow tests (91 unit-target
invocations, including empty test sets). The final parallel -j4 check+unit run
took 1.697s external wall time. Summed case times were setup 1.131s, execution
0.540s and checks 1.715s; summed unit-invocation times were 0.768/0.177/0.451s.
These component sums overlap across four workers and are not wall time. The
incremental native/case build took 3.656s. The layout-final native/original
build+audit+case-library retry passed in 8.976s combined (incremental, unsplit);
the final host-only 840-function audit took 1.464s. No original reference was
captured or refreshed during migration.

Both registered LINK selfchildren execute native code in distinct fresh
executors and share the application first-entry bitmap. The debug DLL case
checks the complete prepack input before replaying the captured CVPACK response;
the ROM case permits only its two captured copy commands and exact file effects.
The four policy-denial tests reject unknown invocations and changed input bytes.

Migration corrections restored the original unset-then-set LINK32_DB=27 input
for 15 buffered phases and the existing clock comparison ranges in three fixed
clock phases; no new normalization was introduced. The common executor's reviewed
errno boundary fix preserves fresh CRT/constructor/setup/return state across
host bookkeeping. It closes the four initial errno-only report differences
without a case seed, changed expected value or output mask.

Final runner: recon-tool e0116ffcb, SHA256
bd66eb34a22d7cfc42c071bed478292c842ee0e56791df1ddf5d61517bf397c4.
Full commands, initial failures, GDB writer proof, per-case reports and source
hashes remain in /root/.ai-tmp/msvc2/vc1.0/link/standardization/; see
final-summary.json, migration-accounting.json and final-reports-v3/.
Existing evidence removed from the active layout remains available through Git
history. Current cases contain only descriptors, typed drivers, narrow tests
and capture.bin. Nine retained project-specific input generators take explicit
input/output paths; regression tests consume captures without regenerating them.
The three obsolete shared LINK capture scripts were retired with this migration.

Reference-composition checkpoint validation: all 92 native cases and seven
focused tests pass in 1.729s external parallel wall time. Native/case build took
3.752s; the canonical original-toolchain build took 22.826s after the obsolete
stub translation unit was removed; final 844-function graph/ABI audit took
2.218s. The final batch used unchanged shared C/H before/after. Four fresh masked
original frontier captures and one completed progress run took 2.016/1.686/
1.927/1.847/1.910s respectively, excluding driver builds. The failed one-byte
extent attempt is excluded from those successful timings. Complete native GDB
observation took 281.532ms including setup/execution/checks. Per-command raw logs
and checkpoint-summary.json remain in demangle-resume/ above.

Case 0092 directly exercises the resource image-group and ANSI-identifier helpers,
then the actual usage/abort path through convert_resources(argc=1). It is not a
LINK command-line reachability claim. Its owned input derives from the previously
frozen icon/cursor groups, ordinal and named-resource payloads. Three groups and
two identifiers run at each of four output alignments with immutable inputs and
full guard/tail checks. The original ordinal expansion is FF00, preserved exactly.
The complete 4096-byte report matches SHA256
724cba5c152b9b7df361b300f6ad556f03a4d6da4d706f9b92a4789ea9adbf96;
usage stdout, empty stderr and status match without comparison masks.

The usage probe establishes a genuine public setjmp context before the accepted
converter calls usage. Actual saved/current registration is 0029ff2c, both
scopes are zero, and the original post-longjmp observation returns to that same
registration/scope with value -1. Native returns -1 using the already qualified
host context. This demonstrates only the existing no-SEH-unwind contract, not
filter/finalizer/global-unwind equivalence. External report emission and explicit
fflush are test-driver operations, not invented application edges. The original
helper completes this direct probe without claiming a target CRT-continuation run.

Three fresh masked original frontiers match native order, followed by complete
original/native output equality. Native GDB records 41 actual observations:
twelve group entry/returns, eight identifier entry/returns and usage entry.
Actual formals/pointer ownership, cdecl stack/nonvolatile preservation, errors
and x87 control/status/tags are checked. Void-return EAX is not asserted.
The ordinary native case took 17.304ms (setup 11.741, execution 2.103,
checks 3.459); complete GDB observation took 246.059ms. Baseline original capture
took 1.192s, frontier captures 1.940/1.900/1.625s and final progress completion
1.607s, excluding driver builds. Final incremental native/case build took 0.210s,
all 93 cases plus seven focused tests passed in 1.718s parallel wall time, the
original-toolchain build took 25.231s and the 847-graph/ABI audit took 2.068s.
Shared C/H hashes remain identical across the final native batch. Raw captures,
GDB packets, body/provenance bindings and timing/source hashes remain in:

    /root/.ai-tmp/msvc2/vc1.0/link/resource-support/

Case 0093 directly exercises the sorted defined/undefined diagnostic wrapper over
real create_image/lookup_external/set_external_defined owners. Its coherent
lifecycle includes an empty table, twelve mixed short/long names with nonzero
COFF values, and a subsequent definition reversal. Each actual External56B and
ImageOptions72B remains unchanged across diagnostics; the owned 512-byte input
and complete 2048-byte report are strict. Report SHA256 is
48e423a5ad0853722d1b61e2e41e946a3359898afbc992e5dbe047dfb519ef48.
This is direct-support coverage. Zero-valued unresolved-symbol warnings and
nonzero undefined-counter restoration remain unexercised; the observed counter
stays at actual loader zero. No post-main stack owner, catalog or global seed is
used. The first test-driver version omitted real invalidate_symbol_sorts after
the empty dump and then inserted into a table with a cached empty sort; its
original AV is retained. Version 2 supplies that genuine API step before insertion,
without repairing target code or assigning cache fields.

One corrected untouched-original baseline took 1.086s. Fresh masked first entry
at RVA12f70 took 1.226s and its native mate 17.274ms; masked completion took
0.889s and exactly matches baseline inputs/report/status/streams. Ordinary native
completion took 16.415ms (setup 10.311, execution 3.115, checks 2.989).
Native GDB took 191.813ms and checks three actual entry/return pairs: both live
formal pointers, cdecl ESP/nonvolatiles, errors and CW 027f/SW 0/empty tags. Void
EAX is unclaimed. Failed debugger symbol/return-observer attempts are retained
separately; they did not change reconstruction source or original references.
The original compiler plus 848-function graph/ABI audit passed in 15.357s combined
(unsplit). Raw captures, live observations, source/ABI binding and timing files:

    /root/.ai-tmp/msvc2/vc1.0/link/sorted-external-diagnostics/
Final sorted-external checkpoint: all 94 native cases and seven focused tests
pass in 1.750s parallel wall time, after a 0.537s incremental native/case rebuild.
The batch uses published shared bbd1bf5f6 with identical C/H before/after hashes.
No original invocation occurs during these cached checks.

Read-only residual classification now identifies the complete 192-byte shipped
___setargv at VA523f0..524b0 through shared relocation-qualified runtime matching
and an independent full-function masked-byte check. It is CRT, with no native
implementation, App credit or new historical frontier/accepted extent. Its
object's neighboring parser/find code differs and remains separately unclassified.
The larger float-conversion and multibyte gaps remain provenance leads, not App
reconstruction candidates. Classification source hashes and proof remain in:

    /root/.ai-tmp/msvc2/vc1.0/link/cleanup-lifecycle/

Case 0094 covers synchronous direct link_cleanup over real file-subsystem owners,
not the held public console-handler/thread transport. Real initialization and
three mapped writes followed by a normal middle close create two live outputs
and an actual vector hole. Real argument constructors register one temporary
original and a same-name retained control. The 512-byte precleanup report checks
all sixteen vector positions, defined FileSlot fields (40 bytes), live written bytes,
zero tails, pointer/argument ownership and actual CW 027f/SW 0/LastError1224.
No private vector, flags, error state or callback pointer is seeded.

The original and reconstruction unmap each live output, retain the original
zero-truncate-before-mapping-close ordering, close both handles, delete the file,
remove temporary originals and genuinely ExitProcess(1). The closed retained
output remains exactly 2097152 bytes, SHA256
44a068d376a9b3943df73bcda71c708d4e9ee141bd0371573be54b412d05dd69.
Precleanup report SHA256 is
5507a7fdb6221bae7e678f50a59a8eb8f78eb7ed0520f1d0485704269a0d2de0.
The two live outputs and temporary input are absent; all retained inputs and
complete final inventory are strict, with empty streams and no byte masks.
The nonwritable registered-slot branch remains unexercised because ordinary
writable mapping constructors do not produce that state.

Shared --direct-noreturn capture records the actual typed callback entry and
expected/actual DWORD 1, without reading helper globals after process exit or
fabricating completion. Both fresh pairs use explicit --capture and
--original-report bindings. Native GDB records 26 pre-exit observations covering
real global owners, both ordered cleanup API sequences, SDK stack cleanup and
nonvolatiles, FP state and final ExitProcess argument. Native SetEndOfFile returns
FALSE/1224 while mapping handles remain open. Individual original API returns
after cleanup entry were not sampled and are not claimed byte-equal.

Untouched-original baseline took 1.145s; masked frontier 1.325s and final completion
1.149s. The corresponding native frontier took 63.447ms (setup 11.130/execution 2.215/
checks 50.102); completed fresh pair took 30.072ms (8.298/2.918/18.855). The larger
checks component hashes the complete mapped/retained 2MB artifacts. Native GDB
completed in 235.603ms. Initial original compiler verification took 19.291s; the
final 849-function audit target passed in 22.296s including its shared-header
original rebuild (combined, unsplit). Final native/case build took 3.143s and all
95 cached cases plus seven focused tests passed in 2.139s parallel wall time with
identical published shared C/H before/after. Probe-DLL build time was unmeasured
and is excluded from capture timings.

The first native link exposed missing DeleteFileA support, now supplied by the
reviewed shared shim ecb2b3422. Its success contract is exercised by this real
owned-file lifecycle; no target-local deletion workaround or extra original API
campaign was added. Failed DSO-load and already-reserved artifact-directory
attempts were rejected before original execution and remain raw technical
failures. The empty old stubs.c translation unit is retired. Raw original files,
receipts, GDB observations, source hashes and timing/accounting are retained in:

    /root/.ai-tmp/msvc2/vc1.0/link/cleanup-lifecycle/


Case 0095 completes one direct embedded-converter header/error lifecycle, not a
public LINK invocation. Its valid LHEADR library-member record uses the counted
name and checksum framing documented by TIS OMF 1.1:
https://openwatcom.org/ftp/devel/docs/omf.pdf
The independent previously original-validated code/data/relocation/COMDAT record
bodies are unchanged. The actual 15-byte LHEADR handler only increments the
module count; the common skipper consumes its payload. Unlike THEADR, it emits
no FILE symbol. The converted COFF has three sections, twelve symbols and 479
bytes. Only its established header timestamp at bytes 4..7 is a clock comparison
field; every other byte remains exact, with an independent COFF layout check.

The same genuine converter command entry then diagnoses conflicting output and
multiple input arguments, no input argument, and a missing input file. Each
phase owns a real buffered FILE and a real setjmp(omf_abort) context. The 107-byte
error function emits its prefix/format/newline, closes all nonstandard streams,
and longjmps with actual value -1. Saved/current registrations agree and scopes
are 0; no SEH filter/finalizer/global-unwind equivalence is claimed. Native GDB
observes the actual literal-only and one-string variadic inputs, FILE owners and
CW 027f/SW 0. The complete 512-byte report and flushed file contents match, including
flags 10/capacity 4096/pending 19,17,19 bytes before closure and flags 0 afterward.
The error provider boundaries remain shipped CRT/shared shims, without new CRT
implementation or application credit. Deferred debug-fixup registration remains
separate from this valid workflow because of the prior offset/undefined-output
limitations.

Untouched baseline took 1.080s; masked LHEADR/error frontiers took 1.160s/0.693s.
Fresh masked completion took 0.455s and the matching native run 21.631ms
(setup 13.127/execution 2.891/checks 5.612). GDB execution took 202.958ms. The canonical
compiler/relink plus 851-function graph/RET/reference/ownership/boundary audit
passed in 22.014s combined. Final native/case build took 1.964s and 96 cached cases
plus seven focused tests passed in 1.974s parallel wall time. Initial native and
probe-DLL builds were unmeasured. The source inventory was unchanged during the
gate, including the Watcom-only compatibility addition subsequently published as
974310433 with identical bytes. It is not exercised by LINK.

Raw failures remain distinct: final-output checks were initially applied at the
first trap; partial timestamp rules initially omitted the established COFF clock;
and one completed original attempt could not publish because a prior emitted
observer input collided on re-emission. Corrected comparison/authoring uses the
same actual target workload; no target source, format bytes or expected output
was repaired. Receipts, full raw artifacts, GDB observations and timing/source
hashes remain ordinary scratch files at:

    /root/.ai-tmp/msvc2/vc1.0/link/omf-header-errors/


Case 0096 supplies a direct resource allocation, resize, conversion and release
workflow. The omitted 46-byte resource_reallocate uses actual realloc, not calloc;
its size formal is a WORD and NULL results take LINK's fatal-message policy.
The 20-byte resource_release conditionally frees and returns zero. Their real
callee and LINK program-name/error references establish application ownership;
no original caller/xref exists and no public CLI reach is invented.

The accepted resource_allocate first supplies zeroed 128/192-byte owners. The
new helper grows these to 160/224, shrinks them to 96/128, then grows to 160/224.
Each phase checks only defined retained prefixes, initializes new storage before
reading it, and converts one frozen valid legacy group with input/output guards.
Input groups 6/20/34 bytes produce 8/24/40 bytes. Both live owners and NULL are
released through the real policy; no payload is read after free. The complete
2048-byte report matches without masks, SHA256
5d8d92419d5262e0e4151b72f4b32a01c5be081d26fc2696d00f5305efedd5a6.
Zero-size/OOM/provider-failure paths remain unexercised, without new CRT code.

The initial external driver misidentified realloc as calloc and passed invalid
pointers 2/3; its actual access violation and source/frontier packets remain
explicit invalid-test evidence, not a target defect or accepted case. Corrected
live-owner setup obtained one successful untouched baseline in 0.508s. Fresh
resize/release frontiers took 0.471s/1.164s and completed original took 0.631s.
The completed native pair took 14.678ms (setup 9.384/execution 2.036/checks 3.258).
Nine GDB entry/return observations verify real formals, ESP, nonvolatiles,
CW027f/SW0/tagsffff and defined results; its complete run took 186.501ms. Original
compiler/relink plus 853-function graph/ABI audit passed in 22.403s combined.
Final native/case build took 6.018s; all 97 cached cases and seven narrow tests
passed in 1.967s parallel wall time. Full shared-source before/after hashes were
identical; the frozen unused Watcom-only lseek candidate is identified separately
from the reviewed Microsoft APIs actually exercised. Raw evidence and timings:

    /root/.ai-tmp/msvc2/vc1.0/link/resource-allocation/
