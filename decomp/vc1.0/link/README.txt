VC1.0 LINK.EXE reconstruction

Build the native i386 shared library and run the cached cases from this folder:

    make -j4 native
    make -j4 check unit

The shared recon-tool harness discovers the 93 numbered case directories. Native
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
set now has 810 application functions (759 public-reached, 51 direct-support),
with accepted App extent union 230353/281634 (81.7916161%). The resource-support
workflow adds three whole functions/331 bytes after the four-method demangler
checkpoint. Support-inclusive historical accepted extent is 261292/281634
(92.7771505%). The 1009 historical frontier count includes shipped CRT/import
support; 16 old handwritten crt/runtime.c overrides remain separate, without new
application credit. A complete non-CRT denominator remains unavailable; 11196
bytes remain unaccepted and unclassified under the preserved partial
body/data/alignment inventory. The prior 11403 decreases by 207 newly accepted
resource-helper bytes outside that inventory; resource_usage was already classified.
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

Current shared validation checks 847 graphs for calls, RET cleanup, live function
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
