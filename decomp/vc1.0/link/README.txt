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

Win16 resource diagnostic callback checkpoint (955 frontiers)

Three whole readable-C callbacks add1549 instruction bytes+10alignment+24inline
class-table bytes. These are explicitly external direct code tests; the public
LINK adapter never enables embedded CVTRES diagnostics. Typed original table,
two actual cdecl formals, complete payload/guards/CRT/stdout/errors and saved-
register/ESP return evidence match. Signed predefined-class bug is preserved.
Twice originals and fresh masked/native runs produce exact123/349/910byte streams.
The initial unusedTAB ctype mismatch is retained; maintainer correction29b61167
now makes all257 original/native table entries match without recapturing refs.
All77cached native cases/761appentries pass (74CLI+3direct); prior74 external6.080s,
direct3trace.342s, output-only3.090s. Build1.861s and GDB.754s separate; canonical
8.926s/794graphs/778callbacks/490data/41ECX and negativecontrols pass. Extent is
249730/281634=88.6718%, not branch coverage or onlyCRT completion. Remaining15244
unaccepted unclassified bytes are not CRT. See cases/legacy-resource-diagnostics
for raw proof, exact scope, failures, split timing and cached-only commands.
Separate public cancellation transport remains HOLD with no handler/cleanup
coverage; read-only Wine source diagnosis is not target evidence.

IDE callback-protocol checkpoint (958 frontiers)

Three bundled IDE functions now reconstruct channel initialization, valid tagged
message formatting/sink delivery and actual _MSC_IDE_FLAGS pointer output. This
is an explicitly external direct test, with a VC1-compiled C callback DLL loaded
through a named driver phase for original capture; it is not public CLI coverage.
Native cached fixtures link the same external C callbacks directly. Complete
1392-byte protocol, real callback formals/owners/returns, all buffer bytes/guards,
CRT environment alias and ABI state match; invalid/assertion paths stay unvisited.
All78cached native cases/764appentries pass, external6.544s including the new
trace fixture.306s (actual ELF4.84ms). Canonical797CFG/781callbacks/492data/41ECX
checks and36 host-only negative controls pass. Extent250163/281634=88.8256%;
14811 unaccepted unclassified bytes remain pending, not CRT. See
cases/ide-callback-protocol/README.txt and timings.json for scope/evidence/splits.

Direct COMDAT diagnostic checkpoint (963 frontiers)

Five readable C functions now print a caller-owned dependency graph through real
iterators and the custom variadic diagnostic wrapper. Explicit direct test only,
not CLI coverage: actual fixed input mapping/pointers, full4096 bytes/guards,
68 typed call/return packets and raw287-byte output match. The real plain-%p
provider discrepancy was corrected centrally in edc071fc; failed raw evidence is
retained and no output is normalized. Native observer return sites now arm only
for outstanding calls, handling Clang's optional-call join without target changes.
All79 cached native cases/769appentries and802CFG/786callbacks/494data/41ECX plus
negative controls pass. App-only extent220258/281634=78.2072%; support-inclusive
250701/281634=89.0166% is separate. Full nonCRT denominator is unavailable;14273
unaccepted unclassified bytes and6996 known gated OMF bytes remain pending. See
cases/comdat-graph-diagnostics/README.txt/timings.json for scope and measurements.

Dispatcher assistance checkpoint (965 frontiers)

Public -helper and the separately proved direct driver_usage now preserve actual
cdecl inputs/returns, Tool callbacks and all mutable name tails. Two originals per
phase freeze exact163/71-byte stdout, exit3 and empty file inventory. A live zero-
capacity FILE mismatch held promotion until shared correction525d34e37; unchanged
original packets now match pending bytes/errors and full native output. Failed
state is retained without normalization or new handwritten CRT. All81 native
cases/771appentries,804CFG/788callbacks/500data/41ECX and26 ABI negatives pass.
App-only extent220515/281634=78.2984%; support-inclusive250958/281634=89.1078% is
separate. Full nonCRT denominator is unavailable, and14173 unclassified bytes
remain pending. See cases/dispatcher-assistance for direct/public scope, ordinary
raw evidence, timing and the approved standalone converter format investigation.

Direct C6 converter checkpoint (983 frontiers)

Eighteen readable C functions translate old C6 type/symbol records through the
embedded standalone parser's real -g/-o argv. Direct external support coverage,
not public LINK CLI: corrected17type/16symbol input, actual formals/owners and
cdecl returns, original18-entry transcript, exact1648B COFF (only timestamp4..7),
108B warnings/exit0 and inventory pass. The original line-bearing variant has a
proven line-vector use after free; it remains explicitly unsupported125 natively
at a host-only guard. Target source/allocator/output stay untouched. The valid
no-lines version removes only optionalLINNUM and preserves original quirks.
All82cached native cases pass7.656s; native/trace build8.420s. New fixture .2224s
includes actual ELF4.76ms; output-only .01842s. Canonical822CFG/806callbacks/
511data/41ECX and negative controls pass. App-only227735/281634=80.8620%, support-
inclusive258178/281634=91.6715%;789appfunctions (759public/30direct),16 historical
CRT separate. Full nonCRT denominator unavailable;13768 unclassified bytes remain
pending. See cases/direct-cvtomf-codeview for ordinary raw evidence and timings.

Direct file resize checkpoint (987 frontiers)

Three readable C bodies now resize the custom cached-file backend through genuine
API-owned mapped/buffered setup. Explicit direct support only: actual two formals,
40B slot/page ownership, cdecl/SDK returns and full close/reopen files match. The
mapped shrink failure remains-1/Last1224 with5242881physical bytes; buffered output
is4097bytes. A real final-seek error-state discrepancy was fixed centrally in
c2a12ca40; only the new direct host opts into the existing53-bit startup policy.
Earlier error/precision/observer failures remain raw with separate correction
proof. No target repair, normalization or new CRT C body. All84cachednativecases,
826CFG/809callbacks/511data/41ECX and focused denials pass. App-only union
228012/281634=80.9604%; support-inclusive258615/281634=91.8266%. There are792app
functions (759public/33direct),16historicalCRT separate;13462unclassified bytes
remain pending, not CRT. Full suite8.289s; actual new traced ELF calls6.02/5.11ms.
See cases/direct-file-resize for split accounting and ordinary evidence.

Direct symbol-table checkpoint (989 frontiers)
---------------------------------------------
Two readable C support functions print populated/empty custom symbol tables and
project only defined names into a new table, whose entries intentionally remain
undefined. Their actual1formal/return/owned graph/source immutability and exact
1274-byte stdout/1432-byte driver report pass fresh original/native observation.
This is external direct support, not public CLI. Only reviewed dead x87 C0 differs
between FCOMP/status and FUCOMIP/EFLAGS compiler sequences; all live FP state is
strict and raw evidence/negative controls remain. No new CRT implementation.

All85 cached native cases pass8.539s; newcase directELF5.75ms (trace total.228s).
CanonicalVC1-j4 and828CFG/811callback/511data/41ECX audits pass. App-only extent
228320/281634=81.0698%; total including support258923/281634=91.9360%. There are
794appfunctions (759public/35direct) and16 historical CRT overrides separately.
13154 unaccepted unclassified bytes remain pending; no full nonCRT denominator.
See cases/direct-symbol-tables for precise scope, timings, proofs and references.

Direct module/library diagnostic checkpoint (991 frontiers)
----------------------------------------------------------
Two readable C functions now parse populated/empty default-library lists and print
actual module-debug metadata after the genuine full-CodeView main returns. This
is external direct support with live owners, actual formals/hidden inputs, saved
main result and normal CRT continuation; public CLI reach is not claimed.
Fresh raw EXE/MAP/203-byte stdout/433-byte report match with captured clock input,
zero normalization. An earlier baseline-versus-fresh clock mismatch is preserved
and corrected by one native-only clock-aligned observation. Narrow inherited dead
LastError/C0 differences retain raw state and reviewed no-reader guards through
exit; all consumed state stays strict. See cases/direct-module-diagnostics.

All86 cached native cases pass8.827s; native/trace build11.415s. New traced ELF
6.477ms (fixture total.274s), output-only ELF5.367ms (total.052s). CanonicalVC1-j4
and830CFG/813callback/512data/41ECX audits pass. App-only extent228478/281634=
81.1259%; support-inclusive259081/281634=91.9921%. There are796app functions
(759public/37direct),16 historical CRT separate. Unclassified12996bytes remain
pending; the full nonCRT denominator is unavailable. No new CRT implementation.

Direct image-helper checkpoint (995 frontiers)
---------------------------------------------
Two readable C bodies finalize the actual mapped .idata section and calculate or
write a full-file checksum through a genuine file/mapping lifecycle. External
direct support only: actualBoundImage owners, third-pointer mode, all50 entry/
return observations,7rawstage files,280B scalar/errorreport and missing-file result
match with no normalization/state exclusions. Shipped __fileno/__filelength add
only verified publicCRT support, zero application credit; no new handwritten CRT.
All87nativecases, originalVC1-j4 and833CFG/815callback/512data/41ECX checks pass.
App-only228780/281634=81.2331%; support-inclusive259511/281634=92.1448%.
798appfunctions(759public/39direct),16historicalCRT separate;12678unclassified bytes
remain pending, with no complete nonCRT denominator. Traced newfixture.352s/ELF
40.75ms; output-only.155s/ELF37.84ms includes7externalstage copies. Full make wall
12.978s includes unmeasured olderhost relinks; perfixture timings excludebuild.
See cases/direct-image-checksum for exact effects, rawrefs and split accounting.

Direct priority archive checkpoint (996 frontiers)

insert_other_input's real16-byte owners feed ordinary main's dump continuation in
priority/ordinary order. Ten fresh ABI/defined-state observations, full36586-byte
stdout,352-byte externalreport/four unchanged archives and exit0 match. The raw
unwritten+8 node word is neither initialized nor consumed/serialized. The initial
by-value-parameter observer error is retained separately from corrected complete
baseline; no target repair/state qualification/normalization. All88cachednative,
originalVC1-j4 and834CFG/816callback/514data/41ECX checks pass. App-only229012/
281634=81.3155%, support-inclusive259743=92.2271%;799app(759public/40direct),
16historicalCRT separate.12446unclassified remaining bytes are not CRT. Details:
cases/direct-priority-archives/README.txt and its validation/timing/coverage files.

Direct standalone BIND parser checkpoint (997 frontiers)

The independent230-byte parser uses genuine loader-zero/persisted BIND globals and
four live owned argv/string blocks; its actual filename/mode feed real map/unmap/
close before grouped verbosity and usage/unknown phases. Fourteen fresh state/ABI
observations, exact27Bstderr/1344Bexternalreport/unchanged2MBimage/exit0 match with
strict errors/FP and initialized BoundImage12..32 only. No public caller or private
list setup is invented; unused first3workspace words stay raw without outputclaim.
All89cached native, originalVC1-j4 and835CFG/817callback/514data/41ECX checks pass.
App-only229242/281634=81.3971%; support-inclusive259973=92.3088%;800appfunctions
(759public/41direct),16historicalCRT separate.12216unclassified bytes remain notCRT.
See cases/direct-bind-parser/README.txt and its validation/timing/coverage records.

Direct diagnostic-log lifecycle is complete (cases/direct-debug-log). Real
initialization, missing-directory failure, lazy open, replacement, route toggles
and final close match 36 fresh observations and complete streams/files/report.
open_debug_log and set_debug_flag add 125 application extent bytes; _perror and
_setbuf are verified shipped support, with no new CRT C or application credit.
The native runner uses the original regular-file redirection contract. The first
PIPE failure and explicit native status-125 denial remain preserved. The corrected
new case and prior 89 pass on identical frozen ELFs; that checkpoint covered90cases.
At that checkpoint there were1001frontiers/802application functions (759public,43direct):
229367 application bytes / 281634 = 81.4415%; with support, 260306 / 281634 =
92.4271%. The existing 16 CRT overrides remain separate. The 11883 unaccepted,
unclassified bytes are not called CRT. Original compiler, 839 CFG graphs,
819 callbacks, 514 data definitions, 41 ECX ABIs and focused controls pass. See the
case's receipts, timing/coverage records and historical debug_flags limitation.

Direct relocation fallback is complete (cases/direct-rebase-fallback): one
whole204-byte stdcall callback plus24-byte semantic table,12-byte padding excluded.
Its real REBASE fallback assignment establishes ownership; direct tests do not
force NTDLL failure or claim public CLI reach. Count0, cumulative positive/negative
six-type relocations with a valid HIGHADJ pair, and clean type6 NULL return have
exact original/native page/records/guards/return/errors/FP/report evidence.
All91 native cached trace/output cases pass; check/replay remain native-only.
Current1002frontiers/803application functions (759public,44direct),16historical
CRT overrides separate. Application extent229595/281634=81.5225%; support-inclusive
260534/281634=92.5080%. Partial known-body inventory194443/195120;11655unaccepted
unclassified bytes remain. Full nonCRT denominator unavailable, unknown notCRT.
