Visual C++ 1.0 for NT: LINK.EXE reconstruction

Target: toolchains/1.0-nt/bin/LINK.EXE. Reconstruction source is readable,
period Win32 C preserving individual graph vertices and calling conventions.
Pending functions are named INT3 stubs. There are no assembly ABI exceptions,
lifted implementation blocks or copied original data sections.

Current checkpoint:929 accepted frontiers,246011/281634 accepted .text bytes
(87.3513%). This union includes contextual inline data and alignment; it is not
branch coverage. The partial checked inventory distinguishes181110 accepted
target-body instruction bytes,27995 CRT instructions,258 import-thunk bytes,
1936 inline-data bytes,3052 alignment bytes and31660 accepted unclassified bytes.
Unclassified bytes are not all padding. cases/edit-section-attributes/components.json
records the partial accounting; it is not a full application-body denominator.

The58 completed native cases cover help; minimal console/GUI/DLL linking;
archive creation/list/extract/remove; COFF/archive/PE/FPO/symbol-map dumps; PE
editing/rebase/bind/release; ordinary OMF conversion/linking; old C++ undecoration;
x86 disassembly; Macintosh linking/relocations/debug/swapper/static/resource/pcode
paths; DEF handlers; absolute/weak externals; Win16 resources; and ROM/reproduction.
Exact arguments/environment, immutable inputs, streams, outputs and clock rules
are in cases/shared-prefix/. Only these fresh shared-prefix references are active.
Earlier acceptance/state/CFG and rejected-defect evidence remains analysis.
The public EDIT workflow now chains actual native rebase→bind→release artifacts
against a pinned local DLL, with independently valid whole-file checksums. The
separate compact-input variant preserves the original logical-size checksum/
physical-growth defect byte-for-byte. See cases/editor-rebase-bind/validation.json.
The legal386 diagnostic-monitor corpus adds81 readable C decoder callbacks and
14 contextual data tables/arrays. Both complete disassemblies and71+10 new entry
sequences match. The narrow setjmp/longjmp support proof requires the same saved/
current registration and scope0=0; no SEH handler/finalizer equivalence is claimed.
See cases/disassembler-x86-monitor/validation.json for actual state evidence,
original decoder quirks and the corrected x87 operand-slot error.
Further cases proceed one substantial root-reviewed workflow at a time under
user-authorized autonomous continuation; no reachability is inferred from percent.

Native build, fixtures and GDB

Run from this binary root:
    make -j4 native native-trace
    make -j4 check
    make -j4 replay
    python3 scripts/check-native.py --trace --case m68k-debug-swapper
    python3 scripts/check-native-boundary.py
    python3 scripts/check-native-rejections.py
    python3 scripts/check-native-repro.py
    python3 scripts/check-editor-flow.py
    python3 scripts/check-editor-checksum.py
    python3 scripts/check-module-memory.py
    python3 scripts/check-decoder-state.py
    gdb --args build/native/LINK.EXE /?

check/replay execute the instrumented i386 ELF against all46 cached originals.
They never invoke Wine, original executables, PE helpers or automatic capture.
Clang flags are -m32 -std=gnu89 -fms-extensions -fasm-blocks -O0 -g.
The same src/ reconstruction files build both ELF and Win32; shared lib/recon C
sources compile directly at final link, without a static archive. The native-only
scripts/native-module.c host unit registers the proved36-byte loaded workspace;
it is uninstrumented and is never compiled into the Win32 target. Only app
objects are instrumented. src/runtime.c, CRT/import/shim functions are excluded.
recon.h is last in each source/header include list and is empty for Win32.

scripts/native-traces.py validates every native application definition, including
unreconstructed stubs, against functions.json. It projects the original accepted
history, excludes CRT/imports, seeds each case's accepted application prefix and
binds each sequence to fresh original capture receipts/events. All46 cases check
727 new application entries; zero-new-entry cases reject unexpected first entries.
Synchronous native LINK selfchildren share seen-state and retain original argv.
Missing/stale references, undefined paths and unexpected child invocations fail.

cases/reference-cases.json selects LINK-owned active references. Cases use fixed
canonical S:\vc1.0\link\<case> cwd and R:\toolchains\1.0-nt\bin\LINK.EXE identity.
The native fixture exposes the repository as virtual R: and its private temporary
case namespace as virtual S:. These are native filesystem translations, not Wine
drive changes. Input and reference hashes are checked before execution. Streams
and status are exact. Only structurally proved timestamp fields are ignored;
where required the PE checksum is validated and derived from normalized clocks.
No path, padding, ordering or arbitrary-byte normalization is permitted.
Native receipts split setup, execution, output checks and trace comparison.

DLL children execute the actual native ELF. The external CVPACK response is a
strict captured boundary: invocation/environment and the entire normalized
prepack input must match before the frozen packed response is applied. Nonempty
captured child streams are rejected. See cases/native-boundaries/README.txt.
LINK_REPRO copy children use a separate pinned captured system response, verifying
full current inputs before replay; fixtures never launch a command processor.
GUI execution, DLL load/unload, SEH fault delivery and arbitrary process trees
are outside this ELF fixture suite. Actual GUI/console behavior was checked in
the original capture provenance; generated PE programs are never run by fixtures.

Explicit original-only compilation and captures

These commands use Wine outside fixture execution:
    make -j4 all audit
    python3 scripts/measure-coverage.py
    python3 scripts/measure-components.py
    make -j4 capture-helper
    python3 scripts/capture-shared.py --case help --output-root <new-reference-folder>

The original compiler/linker build remains the default target. Static checks
use the shared Rust CFG engine for676 source/runtime graphs, including13
evidenced runtime vertices, with return-cleanup, function-reference, application-
ownership and whole-extent checks. All841original/838rebuilt accepted extent
records (unique836/835) receive boundary checks. Retained Python checks cover
663callback sets,472contextual data definitions and41ECX-member return cleanups.
The quiet incremental Cargo helper builds once; audit invokes the executable
directly, preserving its JSON output/errors. No cargo run or Wine is introduced
by this host tool. See cases/graph-audit-validation.json and the explicit
Python baseline target audit-graph-python. Toolchain files remain
untouched. scripts/capture-shared.py is an explicit original-only analysis tool,
not a fixture dependency. src/capture-original.c is its debugger helper and is
not linked into the reconstruction. Capture plans mask the entire original
.text, restore accepted extents/contextual code data, and trap application first
entries while excluding CRT/imports. Masks are written only into stopped original
process memory. The original disk EXE and actual R: _pgmptr remain unchanged.
The debugger follows LINK selfchildren and snapshots the CVPACK boundary.

All original tools use the existing shared ~/.ai-tmp/vc1.0-capture/wine prefix:
R: is /root/msvc2; S: is ~/.ai-tmp/msvc2. These mappings are asserted, never
created/remapped by LINK. Scratch reservations are S:\vc1.0\link\<case> and
S:\vc1.0\link\capture.lock. The shared server is never stopped. LINK's obsolete
private prefix/server and custom drive configuration were retired after ownership
and idle-service inventory. Raw earlier analysis was not relabeled.

The shared link-debug-dll/link-omf/link-undecorate capture tools are explicit
original-only diagnostics. OMF and undecoration retain exact shared frozen
outputs after migration. The old shared debug-DLL reference embeds old tool paths
in verbose stdout/debug content; shared-prefix invocation cannot match it. Its
shared manifest/oracles remain untouched. LINK's freshly captured active debug
DLL reference replaces only LINK's own former transport/oracle. CVPACK owns
independent byte-identical copies of its earlier input/output, so its current
case has no dependency on LINK's active boundary directory.

Migration results and reconstruction limits

cases/shared-prefix-validation.json records all35 original captures and repeat
runs, native checks/negatives, static build/audits, timings, changed path-dependent
artifacts, shared capture-tool limitations and exact private-prefix retirement.
Mac debug embeds its new absolute S: output path byte-for-byte. Full-debug DLL
stdout and DLL bytes also change with real paths; its packed output is71756bytes
rather than the earlier71800. These are fresh original references, never patched
or additionally normalized. Old own output caches/transport manifests were removed.

Target defects remain explicit: /absolutesfrom may write an External union before
removing an undefined node; a forced unresolved DIR32 case leaks a process pointer
into its output. Rejected original/native artifacts and state remain under
cases/absolute-weak/capture/. No pointer normalization or false acceptance was
introduced. The accepted /include no-relocation companion preserves the original
LNK4016 warning. The56-byte External layout separates archive/Mac/fallback fields;
all prior Mac artifacts remain regressions. Redirected stderr buffering is modeled
in shared CRT from stopped-original FILE state, without partial-stream exclusions.

The approved OMF and large-symbol/wildcard families are recorded in
cases/omf-symbols-wildcards-queue.json. OMF is complete (ten functions/1936extent
bytes,1864instructions and72alignment); its source/alias/provenance/validation
are in cases/omf-records. The second family uses4096ordinary external definitions
compiled by original CL and a relative unit?.obj operand. Further cases proceed one substantial root-reviewed workflow at a time under
autonomous continuation. Known-body estimates are opportunities, not branch-coverage or
99% promises. SOURCE-NOTES.txt retains historical checkpoints; current metadata
supersedes them.

Public EDIT rebase/bind/release checkpoint835

Four frozen cases add14 application functions,6541 instruction bytes,112 switch
bytes and227 alignment bytes. The two624-byte shipped strtoul/strtoxl extents and
ten6-byte IAT thunks are separate support, with zero application credit. C source
retains distinct32-byte REBASE and36-byte BIND layouts in one contextual union;
compiler assertions and original reader/writer displacements prove cache fields.
Fresh masked-original/native GDB stops preserve complete physical partial streams.
Actual static IAT callers are mandatory when native APIs also serve earlier GPA
calls; nonempty public strtoul input/result/endptr/errno inventory proves the CRT
boundary without claiming native internal-helper ABI equivalence.

Original setup builds real relocatable/importing EXE and exporting provider DLL.
Repeated original public phases prove3HIGHLOW words rebased+100000, preferredbase
400000→500000, actual IAT3046→601000/provider timestamp/forwarderchainffffffff,
then stack reserve200000/commit3000. The combined-mode probe exits after REBASE;
it is retained as actual original early-exit behavior, not BIND/release coverage.
Every positive raw2MB checksum is checked independently before normalizing only
COFF TimeDateStamp136..139 and recomputing its derived checksum. Native later
stages consume actual prior outputs, validating full normalized input bytes first.
The compact3072-byte original input grows2MB while keeping a checksum over3072;
that explicit fourth case requires full physical bytes exact, no ignored fields,
a valid logical3072 checksum and an explicit whole-physical-file mismatch.

The original resolves ntdll!LdrProcessRelocationBlock successfully; its observed
stdcall4HIGHLOW/ABSOLUTE boundary is shared support. The internal fallback remains
trapped. BIND's actual FlushViewOfFile receives its loaded .bss workspace, not the
file view, and returns success. Host-only registration binds exact target SHA,
ImageBase10000, VA6d470/RVA5d470 and36-byte owned typed global; unknown ranges fail.
Native GDB independently verifies pre/post errors, successful API results, UTC
SYSTEMTIME→FILETIME conversion and actual last-write file time against cached
original observations. SearchPath fallback, error helpers, SEH faults and unentered
ordinal/forwarder/verbose branches are not claimed dynamically exercised.

All44 native cases/646 application entries pass3.467s external (3.210s case loop),
including both zero-entry variants. Canonical -j4 build/native trace/static checks
pass in29.372s:676Rust graphs,663callbacks,472data definitions and41ECX routines.
The true RET4 mutation negative rejects after fixing a test generator that had
mistaken inline switch bytes for RET. Raw failed observer/test attempts remain
explicit in validation.json. No reconstruction divergence was hidden.

Initial output-only three-stage9.237s was dominated by scalar Python checksum
checks (~1.83s/stage and again in prior-input setup), with directELF10–14ms.
The independent oracle now uses little-endian array word sums, checked against
its retained scalar algorithm on33bounded odd/even/random/carry images plus seven
actual images. Raw checksum/defect semantics are unchanged. Focused output-only
after timing is.201/.288/.285s, compact.095s; full splits and measured/unmeasured
build/capture/debugger phases are in cases/editor-rebase-bind/timing.json.

Large initialized-data DLL checkpoint (928 frontiers)

Ten readable C functions add1186 actual instruction bytes: numeric options,
default MAP name, undecorated stdcall entry matching, debug24 order-file rewrite,
mapped/buffered growth, FIXUP debug records and three486 operand callbacks.
3MB mapped4MB/buffered3156992-byte outputs and5MB buffered5259776-byte output
match complete repeated originals, including COFF/FIXUP debug records, three
HIGHLOW relocations, exact physical padding, MAP and rewritten .ord files.
Only PE COFF/two debug timestamps and the MAP clock line are normalized.

The original long-COFF-name diagnostic failure, obsolete inline CMPXCHG compiler
encoding and XADD/byte-CMPXCHG display quirks are preserved. Standard486 bytes
are explicit fixture input only, never reconstruction code or executed code.
See cases/large-image-fixups/README.txt and its validation/timing/components JSON. All52 cached native trace/output cases pass4.109s
external; native -j4 rebuild6.993s measured separately. Canonical compiler,
Rust graphs, callback/data/ECX checks and mutation guards pass.

    make -j4 native-trace-check
    python3 scripts/check-large-image.py

At that checkpoint the next workflow was public EDIT section rename/attributes;
remaining OMF internal-g and unclassified ranges remain pending, not CRT.

Public section-edit checkpoint (929 frontiers)

One1176-byte whole C function plus40-byte inline table/24-byte alignment now
preserves section rename, cache/page/read/write/share flags, alignment, per-letter
negation and missing-section diagnostics. Six repeated original phases and all
actual native edited/dumped4MB files match with zero normalization; release raw
checksum is independently correct. Later phases consume actual native artifacts.
All58 cached trace/output cases pass5.380s;768 Rust graphs,755 callback routines,
486 data and41 ECX checks pass. See cases/edit-section-attributes/README.txt and
validation/timing/components JSON. Remaining17887 unclassified unaccepted bytes
are not proven CRT. Next root-approved single case creates/inspects i860/MIPS
import archives from conventional DEF; it does not link/run those target images.

Cross-machine import-archive checkpoint (932 frontiers)

Three C bodies add748 instruction bytes. Six public i860/MIPS all-DATA generation,
list and dump phases pass; later phases consume actual native-created archives.
The genuine i860 helper and descriptor/lookup/export relocations are inspected,
not executed. Captured external time makes complete raw native outputs exact;
no archive suffix/padding normalization. A separate valid function-export probe
exposes an original uninitialized heap-pointer auxiliary tail; native rejects that
unsupported buffered route125, while prior mapped scalar-residue cases pass.
Exact36-byte machine descriptors are now proved by data layout and fresh GDB.
All64 cached native cases/741appentries pass5.390s;771 CFG graphs,758 callback
sets,490 contextual data definitions and41ECX routines pass. Extent coverage is
246759/281634=87.6169%;17197 unaccepted unclassified bytes are not CRT. Detailed
scope, limitations, time splits and evidence: cases/cross-machine-imports/README.txt.

Mac multi-entry loader/MAP checkpoint (937 frontiers)

Five C bodies add305instruction bytes+11alignment, including the genuinely new
26280Mac section/value qsort callback. A valid long-name COFF module produces two
CODE resources, actual per-resource duplicated code and two far jumps. Full RAW
native image/MAP equality is required using captured external time input; public
header/symbol/relocation dump consumes actual prior native image. Historical
short-name failure and odd original MAP rows remain untouched limitations.
All66 native cases746appentries pass5.725s, native rebuild7.158s separately;
canonical compile/audits26.290s pass776graphs763callbacks490data41ECX. Extent is
247075/281634=87.7291%;16914unaccepted unclassified bytes remain pending, not CRT.
See cases/mac-loader-map/README.txt, validation/timing/components JSON. The next
root-approved single workflow tests named-resource language identities and
ordinary duplicate-identity error recovery, with actual no-SEH-unwind proof gate.

Named resource identity/recovery checkpoint (942 frontiers)

Four whole C bodies add342 instruction bytes;16bytes of shipped _unlink support
remain separate CRT credit. A valid SAMPLE/Café resource with languages0409/0411
links exactly; the companion duplicate exits26 with exact diagnostics and a real
zero-byte temporary artifact. Explicit per-child TMP input proves true tempnam
counter/path/error behavior. Native longjmp returns actual-1 with matching live
errno and no-SEH-unwind context. A separately retained stale LastError mismatch
has an original armed-import/native no-reader proof through successful cleanup
and exit; ambient1224 equivalence is not claimed. All68 native cases/750entries
pass5.696s, native build7.303s separately;781graphs767callbacks490data41ECX pass.
Extent247433/281634=87.8562%;16912 unaccepted unclassified bytes remain pending.
See cases/resource-identity for typed ABI evidence, raw limitations, timings and
negative controls. Next root-approved single workflow: public EDIT rejection of
ordinary valid images and file failures; no forced internal OMF debug entry.

Public EDIT rejection/file-failure checkpoint (946 frontiers)

Three whole C bodies add165instruction bytes, while bounded _snprintf support
adds103CRT instructions+9alignment with zero application credit. Valid /FIXED
rebase rejection, a valid no-export provider bind rejection and ordinary absent
file match actual original exit54/53/7, streams and complete file inventories.
All consumed inputs/callers/output ownership and defined bounded result are
observed; omitted/changed packets fail. All71 cached native cases/753appentries
pass6.307s after7.596s build. Canonical compile/audits26.596s pass785CFG graphs,
770callback sets,490data definitions and41ECX routines. Extent247710/281634 is
87.9546%;16838 unclassified remaining bytes are not CRT. See cases/editor-errors
for precise limits, original setup correction, negative checks and split timing.

LastError attribution clarification: resource-identity/last-error-attribution.json
records an unattributed dead ambient discrepancy. Original CloseHandle preservation
is observed; no native CloseHandle1224-to0 transition was proved. Current shared
CloseHandle already preserves successful-call LastError. No provider fix is claimed.

OMF continuation/displacement/LINSYM checkpoint (952 frontiers)

Five whole C bodies add371application instruction bytes+2internal alignment;
shipped-vfprintf support55CRTbytes+9alignment has zero app credit and no new
handwritten CRT. Valid continuedCOMDAT, repeated/nonzero segment targets and
LINSYM16/32 produce exact native archives; USE16 data emits the actual warning.
A separate dump consumes actual prior native archive. Original stale auxiliary
length8 versus16Brawspan is preserved. Distinct controlledTMP cases prove true
counter/access/errors; the earlier uncontrolled errno mismatch remains raw.
Qualified public vfprintf input/output FILEbuffer,3arguments/errors and full
outputs match. All74native cases/758entries pass6.037s, build1.841s separately.
791CFG/775callback/490data/41ECX checks pass. Extent248147/281634=88.1097%;6996known
OMF debug-gated bytes and16827unclassified bytes remain pending, not CRT. See
cases/omf-continuation for scope, whole-body evidence, failures and split timing.
