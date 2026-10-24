Direct C6 CodeView converter checkpoint — 983 frontiers

Eighteen whole application functions are reconstructed in src/omf-debug.c.
They collect old debug records, translate aggregate/array/procedure/primitive
and bitfield types, generate COFF symbols/auxiliaries and interleave line records.
The public LINK adapter passes only -o/-co; this case instead gives the embedded
standalone cvtomf parser its real five-word -g/-o argv from an explicit external
cdecl driver after ordinary target CRT startup and a real public setjmp context.
It is direct support coverage, not public LINK CLI reachability. No private
converter mode seed or new handwritten CRT body is involved.

The fixture generator makes ordinary checksummed OMF test input, not claimed
compiler output. Microsoft's published C6 cvdump definitions establish the old
linkageBYTE/lengthWORD/leafBYTE type framing and BYTE symbol framing. The newer
original-C2 CodeView object was independently recognized as incompatible and was
not knowingly passed to this older decoder. Primary-source hashes/URLs and
actual caller/layout evidence are under analysis/; manifest.json preserves the
original line-bearing variant and variants/no-lines/manifest.json the completed
version. Its21records/1110bytes retain all17types,16symbols,8ordinaryfixups,
14debugfixups,34codebytes and100databytes. Only the optional60-byte LINNUM record
is removed by sources/without-linnum.py; no output is repaired.

Two untouched-original no-lines baselines and a fresh masked original18-entry
transcript freeze exit0, exact108-byte warnings, empty stderr and full1648-byte
COFF. Native fixtures compare every byte except the proven header timestamp4..7,
all streams/status and the complete input/output inventory. The native projected
first-entry sequence exactly matches the new variant's actual original events.
The first965 accepted entries form its seen prefix. Wrappers, CRT/imports and
shims are excluded; an unexpected application entry fails. The caller-owned
source input remains unchanged. Run from the binary root:

    make -j4 native-trace-check
    python3 scripts/check-cvtomf.py
    python3 scripts/check-cvtomf.py --trace --unsupported-lines
    python3 scripts/check-cvtomf-controls.py

These commands run only ELF and cached references. The explicit original helper
build/capture commands are separate diagnostics and never fixture dependencies.
The original compiler and graph/ABI/data checks already passed; no canonical
rebuild was needed for the later native-host guard alone.

Preserved limitations and evidence

The original line-bearing input is valid and succeeds, but its symbol translator
frees the512-byte line vector before omf_write_lines reads its same dangling
pointer/count again. Actual original49b95/49b9a and492c0 observations, corresponding
native GDB release/consumer packets and full failed artifacts are preserved in
analysis/line-lifetime/. Nine6-byte typed rows at8-byte stride consume54defined
bytes spanning70bytes; the72-byte raw sample's18padding bytes are not compared.
The original retains row(0,5), while native free changes the first row and those
changed bytes appear in the serialized duplicate at file368..373. This remains
an original lifetime-ended read even when its old bytes remain accessible.

The uninstrumented native host wrapper calls the real symbol translator once,
then stops125 on mode2/nonzero-line input, before the finisher/stale consumer.
Disassembly, exact unsupported stderr/no output and the17-entry prefix are
checked. Status125 is native unsupported policy, not original behavior. The
Win32 source, allocator and original output stay unchanged. There is no delayed
free, heap transport, source repair or extra five-byte normalization.

The no-lines original still emits two procedure-start(0,0) line rows. The first
six allocated bytes were actually observed zero before the real procedure reads,
and native matched without seeding; no allocation-initialization guarantee is
claimed. Other retained quirks are two valid EAX-register16 warnings/FFFFFFFF
values and the original .bb38/.eb50 block-offset addition. The initial semantic
layout probe used bit64/96 field offsets; it is retained separately and never
presented as a coherent Node layout. The corrected byte8/12 version preserves
16-byte Node, next12 and bitfield bit65. Its changed type-writer/constructor
states were freshly rechecked. The unused anonymous-name workspace was corrected
from a17-byte maximum-format estimate to its actual12-byte span before the next
head global; oversized formatting behavior remains unexercised.

Original/native FILE refill placements are retained raw. The separately reviewed
binary-read live-byte/logical-position relation is narrowly documented in
analysis/input-file-representation.json, including historical ftell's def-use;
it is not a blanket FILE-layout exclusion. Freed type tables/fixups are marked
lifetime-ended rather than read as live outputs. Return checks preserve cdecl
ESP/nonvolatile registers; WORD helpers compare AX only, and void EAX is not
invented. The failed C89 controller build/stale-helper attempt is retained,
earns no acceptance, and the capture runner now rejects a stale helper.

Validation and accounting

All82 cached native cases pass, including the eighteen new direct entries.
Canonical822CFG/806callback/511contextual-data/41ECX checks and graph negatives
pass. Four new fixture controls reject changed input, missing reference, changed
non-clock output and changed original entry provenance. Build and source hashes,
ordinary raw receipt paths and phase timings are in native-validation.json,
native-source-snapshot.json, audits.json and timings.json. No recursive toolchain
or auditor archive is introduced. The final native/trace build took8.420s;
82-case trace/output suite7.656s. New positive fixture .2224s comprises .2081s
setup (.1997s trace preparation included), .00476s ELF, .00116s output checks and
.00015s trace comparison. Output-only was .01842s. Earlier explicit original
captures and GDB observations, including failures, are separately accounted;
canonical build calls not stopwatch-measured are labeled unmeasured.

There are789 reconstructed application functions:759 public and30 direct-only;
16 historical local CRT implementations remain separate. The new18 add6828
CFG instruction bytes,108alignment and284inline-table bytes, total7220extent.
App-only accepted extent is227735/281634=80.8620%; all accepted support-inclusive
extent258178/281634=91.6715%. Neither is branch coverage. The partial known
application inventory is192638/193315bytes; it is not a complete nonCRT denominator.
Unaccepted13768unclassified bytes remain pending, not CRT. Three known OMF bodies
remain unvisited (LHEADR15B, error107B, deferred-fixup registration59B), without
public-reach or branch-credit claims. The completed case does not mean only CRT
remains; the next substantial case requires root review.
