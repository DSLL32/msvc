Direct cached-file resize lifecycle

Three whole readable C bodies extend LINK's own FileSlot/page subsystem. This is
an explicitly external direct-support caller, not public LINK CLI reach. The
actual files@5c000 table, flags/length fields and mapped/buffered dispatch connect
the bodies to the established custom file backend; no CRT-lookalike inference
or private mode/global seed is used. The external controller runs real CRT startup
and calls files_initialize/open_file/write_file/seek_file/read_file/close_file.

Frozen binary-mode inputs write12289 deterministic bytes, resize to12289, grow
to5242881, read, shrink to4097, read, close/reopen/read/close. The initialized
subsystem owns every slot/page/name/view. The external callback is real file_failure;
no callback fires in the positive phases. Each phase has one untouched original
baseline. An earlier text-mode arbitrary-byte probe failed before any resize;
its172-byte LNK1107 diagnostic and12304-byte artifact remain under probes/.
The corrected explicit _O_BINARY phase is separately identified, not a relabel.

Mapped growth returns1. Shrink returns-1 with ERROR_USER_MAPPED_FILE1224 while
updating logical length4097. Close also attempts truncation while its mapping
object remains open; actual physical size stays5242881 after close/reopen. These
original failure semantics and all bytes are the oracle, not a repaired success.
Buffered resizing returns0 and leaves the exact4097-byte prefix after close/reopen.
The first buffered request12289 sees flushed logical length12288 while its final
byte is still pending in an owned page, so it takes the grow path; only mapped
phase4 is the actual equal-length no-op. Both phases preserve complete status/
streams/inventory, no normalization.

Actual cdecl formals are int handle/long length and FileSlot*/long length. The
owned FileSlot is40 bytes, with length+12, contents+24, cursor+32 and next+36.
The first observer incorrectly read48 bytes: failed raw packets remain referenced
in analysis/observer-bound-correction.json, and their final8 unowned bytes have
no comparison or credit. Original-compiler size/offset assertions and corrected
fresh native/original packets use40 bytes. Populated page headers/table sentinel
states, cursor offsets and edges match. detach_file_page returns pool-owned pages
and its flush helper replaces table cells; no actual Page allocation is freed.
Historical page identities remain owned when LRU links refer to detached pages.

The new original support trap __setmode57bb0..57c50 is byte-exact shipped
SETMODE.OBJ, retained with its original/canonical graph/ABI/extent. Correspondence
is at already accepted public _chsize: actual fd3/length12289,5242881,4097, results0,
errors, FP and full file effects. No native internal-helper ABI claim, new CRT C
body or application credit is assigned to that160-byte support extent.

Two real provider/prestate differences held promotion. Actual final successful
SetFilePointer64 erased native LastError1224 but original kept it. The precise
seek/SetEndOfFile/seek pre/post sequence establishes attribution, unlike the older
withdrawn CloseHandle lead. Shared correction c2a12ca40 preserves successful
NULL-highword/unambiguous-seek LastError only; old cases/defaults pass. The initial
native x87 CW037f also differed from original027f. Only these new driver final
links select existing RECON_NATIVE_STARTUP_CONTROLFP_53 before app entry. No late
state/debugger reseed occurs. Corrected native runs match unchanged originals:
CW027f/status0/tagffff and all8empty zero payloads; raw reserved FSAVE fields and
the old mismatch packet remain intact with separate correction provenance.

Fresh original masked/native GDB evidence covers10 mapped and12 buffered actual
application entry/returns, plus6 API/publicCRT pre/post rows per phase. Complete
actual formals, named owners/callers, defined returns, nonvolatiles and ESP match.
The external INT3 sentinel and results.bin report are test transport, not original
callers or application vertices. The60-byte report derives14 defined API returns
from original events plus one reservedzero for void files_initialize, not its EAX.

All84 cached native cases pass, including both new phases. Normal commands:
  make -j4 check
  python3 scripts/check-file-resize.py --trace
The ordinary fixture runs only native ELF and cached references; no Wine, original,
debugger, compiler, capture, shell replay or reference refresh. Unknown/missing
artifacts fail. The actual API/report/file comparisons retain both original
failure and successful buffered outcomes. Explicit original capture and canonical
compiler/audit targets remain separate; make-n check/replay shows no Wine edge.

Checkpoint:987frontiers,792application functions (759public/33direct-only),
16 historical handwritten CRT overrides separate. Three new bodies add277extent
bytes:269CFG instruction and8alignment, not branch coverage. App-only union
228012/281634=80.9604%; support-inclusive258615/281634=91.8266%. Partial known-body
inventory is192907/193584 target instruction bytes,28298/37088 CRT and258/348 import.
The full nonCRT denominator remains unavailable;13462 unaccepted unclassified
bytes remain pending, not CRT. All826canonicalCFG graphs,809source callback
routines,511contextual data definitions and41ECX ABI checks pass. Twenty-three
host-only evidence denials and common wrong-call/RET/runtime-tail controls pass.

Single final timing sample: native/trace build9.741s; trace-cache preparation
1.138s; all84 suite external8.289s; new output-only command external.148s.
New trace phase internal times .2831mapped/.2246buffered; output-only .0631/.0112.
Per-case setup/ELF/oracle/trace splits are in native-validation.json. Canonical
original compiler build20.952s, final host audits5.489s, graph controls18.500s.
Twelve explicit original capture invocations total19.044s, including preserved
probes/failed states;10nativeGDB observations total1.742s. Original-controller
build times were unmeasured. Two failed metadata cache preflights executed no
fixtures and are retained in timing accounting. No benchmark median is claimed.
