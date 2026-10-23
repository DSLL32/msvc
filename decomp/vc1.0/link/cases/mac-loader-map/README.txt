Mac multi-entry loader, duplicated contributions and MAP

A498-byte ordinary M68K COFF object defines startup/loader markers, two helper
functions in one CODE resource and two distinct duplicated code contributions.
CTOT16 references create two far jumps and DUPCON16 references request actual
per-resource copies. The copies contain genuine CTOABSD32 fixups to the helpers.
Distinct long COFF target names avoid the original shared short-name scratch
alias. The older rejected two-source short-name input at
../m68k-dupcon/capture/rejected-two-source remains unchanged; this successful
layout is separate and does not claim every older two-source input now works.
The fixture writer documents20/40/18-byte bundled COFF layouts and Motorola
instruction words; they are inspect-only inputs, never reconstruction code.

Two untouched original public /machine:M68K /nodefaultlib /entry:main /fixed
/debug:none /map runs succeed. All physical2MB output bytes and full MAP match
between runs after only the structural COFF clock and its exact MAP rendering.
The actual link_passes time(&TimeDateStamp) input at1b099/1b09e is independently
captured with nonnull out pointer, equal result/output, unchanged stack/errno.
Creation-only existing RECON_FIXED_TIME supplies that external value; normal
native fixtures require complete RAW image and MAP equality against its capture,
in addition to timestamp-normalized cross-run comparison. The MAP ctime line
is checked against the captured value and the original PST8PDT timezone.
No path/padding/output ordering normalization or output rewriting is used.

A second public /dump /headers /symbols /relocations phase has two original
captures and consumes the actual successful native-generated image. It does not
replace it with a frozen PE. Structural checks independently prove two CODE
resources, two distinct copied instruction sequences in both resources, and two
far-jump entries resource2/offsets40+44. The original MAP's duplicated source
publics have section0/offsetfffffcf8; these actual rows are retained, not repaired.
No Macintosh code or image is executed.

Reconstruction

Five fresh masked-original/native GDB firstentries produce five whole readable
C functions,305 real instruction bytes plus11 alignment bytes. Four nominated
bodies handle external-name comparison, source-contribution comparison, jump
ordering and cached Mac symbol sorting. The genuine fifth qsort callee at26280
sorts by Section.index then wrapped final value; it was not in the old driver
inventory. Its original null-first-owner branch returns1 even when both are
null, preserved exactly. The byte-return name comparator preserves the original
shared short-name buffer behavior; the successful case uses long names.
No assembly implementation, extra runtime/import credit, contextual data snapshot
or source-layout padding is introduced. Full extents and individual cdecl ABIs
remain distinct from dynamically exercised branches.

All66 cached native cases/746 application entries pass5.725s external,5.447s
internal. Native-j4 rebuild7.158s is separate. Suite sums: setup1.998s,
directELF.685s, output checks2.709s, trace checks.00708s; trace preparation.183s.
Canonical original-toolchain compile/audits26.290s pass776CFG graphs,763callback
sets,490contextual data definitions and41ECX ABIs. Graph negatives18.461s and
reference/caller/CRT controls pass. Native cross-import pointer-leak guards still
deny125 and previous mapped scalar cases pass. Seven Mac semantic/flow negative
controls reject changed entry/code/jump/resource and missing/incorrect actual
prior-stage evidence. A first checker attempt mistakenly included two extra
bytes in its expected startup slice; this was corrected, not an output exclusion.

After the66-case freeze, the normal captured-clock creation oracle was tightened
to RAW size/hash equality. A focused eight-phase trace/output run covers all
three registered clocked creations and their actual-native inspections; it passes
without another full66 run or any executable/original change. Its times are
separate in timing.json; full66 timing remains the earlier unchanged executable
measurement. All historical capture and fresh acceptance receipts/hashes remain
in validation/timing JSON and acceptance/. Large raw native receipts stay in
recorded scratch paths. See checks.json and components.json for semantic proof
and honest partial body accounting.

    make -j4 native-trace-check
    python3 scripts/check-mac-loader-map.py --native-receipt <receipt>

Accepted .text extent is247075/281634=87.7291%,937frontiers. Partial inventory now
has182163accepted/190625known target-body bytes,27995/37088CRT,258/348imports,
1936verified contextual data and3063alignment. The16914remaining unaccepted
unclassified bytes are not proven CRT or padding. The large OMF internal-g
closure remains inaccessible through the normal public wrapper; it is pending,
not accepted or dismissed. The next root-approved public resource-identity
workflow is recorded in next-proposal.json and the durable ordered queue.
