Direct custom-symbol diagnostics and defined-name projection
==========================================================

This is an explicit external direct-support test, not a public LINK CLI case.
The two whole cdecl functions are print_external_table26510..265b0 (one
SymbolTable* formal, void) and copy_defined_names2dcf0..2dd84 (one SymbolTable*
formal, newly owned SymbolTable* result). Their custom40-byte table/44-byte hash/
56-byte external-symbol objects and actual constructors/enumerators establish
LINK ownership. No CRT body or external test-driver code receives application
credit. The prior read-only inference that25cd0 was define_export was wrong:
it is the already accepted lookup_external; the projection copies names only.

The4096-byte owned input contains32 distinct short/long names, including an
exactly8-byte name; real lookup/set-defined calls leave21 defined and11 undefined.
Print source, project, print destination, construct/print/project/print an empty
source, then enumerate all four tables through the real API. The projection
creates21 ordinary undefined lookup entries; flags/types/values are not copied.
The exact1274-byte stdout and1432-byte external tables.bin report preserve this
behavior and counts32/21/0/0. Empty stderr, exit0, input immutability, full owned
file inventory and guards are required. No output normalization occurs.

The external debugger controller executes original startup to main, then makes
actual typed calls using owned stack/payload/sentinel transport. The sentinel,
public fflush and report serialization are driver operations, not invented
original callers. One untouched original baseline sufficed; three subsequent
masked captures established first printer trap, printer return/next projection
trap, and complete projection return/empty workflow. Native GDB observed the
same actual formal, owned table graph/defined scalar/name bytes, cdecl ESP and
nonvolatile returns, live stdout FILE/pending state/errors and within-image full
source immutability. Raw observations and concise comparisons are in acceptance/.
The reserved four-byte string-buffer size prefix has no writer/reader in this
workflow; raw bytes remain recorded and same-image immutable, never seeded or
serialized. Cross-image comparison uses the actually written names at offsets>=4.

Both native driver links select the existing startup53 constructor before app
entry; actual architectural CW027f/tagffff/TOP0/exception status match. Original
find_hash_entry computes an exact count/1024 ratio with FCOMP[3.0], then immediate
FNSTSW/TEST AH,41; native uses FUCOMIP/EFLAGS. Original status0100 versus native0
is only dead C0. Parent independently checked the canonical13 body hashes and
producer/read sequence. The approved qualification is ONLY bit0100 for this
workflow/integer formats/callees. Raw status and initial mismatch stay preserved;
all other status/CW/tags/live state remain strict. Reopen at any new status reader
or unknown callee. Fifteen other status-bit mutations are rejected; no FP seeding,
blanket-status masking or cross-image byte-equality claim is made.

Normal cached commands:
  make -j4 check
  python3 scripts/check-symbol-tables.py --trace
These execute native ELF only with immutable original references, no Wine,
debugger, compiler, shell replay or reference refresh. The explicit original
capture helper/driver lives separately; canonical make -j4 all and audit remain
explicit original-compiler/host-analysis operations. Normal target dry-run has
no Wine/original/capture edge. Missing/changed references fail before execution.

All85 cached cases pass. New functions add308 accepted extent bytes:301 actual
CFG instruction bytes plus7 alignment, not branch coverage. Checkpoint989
frontiers/794 appfunctions (759public,35direct),16 historical handwritten CRT
overrides separate. App-only228320/281634=81.0698%; support-inclusive258923/281634
=91.9360%. Partial known target instructions193208/193885, CRT28298/37088, imports
258/348; full nonCRT denominator unavailable. The13154 unaccepted unclassified
bytes remain pending, not CRT. All828 canonical graphs,811 callback routines,
511 contextual data and41ECX checks pass, including accepted endpoint boundaries.

Single final sample: native/trace build9.991s; cache projection1.146s; all85
external suite8.539s. New trace case.2280s = setup.2109 (trace preparation.2039
included), directELF.00575, output oracle.00084, trace check.00015 plus receipt
hash/accounting overhead. Initial output-only.01863s (ELF.00291s). Canonical
build20.918s; host audits6.931s; graph-positive/negative controls18.376s. Four
explicit original captures total5.879s; three native GDB observations total.585s.
Initial controller/intermediate builds and analysis were unmeasured. A stale
trace-cache preflight correctly failed without running fixtures; corrected host
projection followed. Exact paths, per-case timings and source/ELF hashes are in
native-validation.json/timings.json/audits.json. Shared61155226d published the
unchanged tested candidate source; no publication-only rebuild was required.
