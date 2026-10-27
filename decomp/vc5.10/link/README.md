# VC5.10 LINK

615 application functions / 234,136 original code bytes are reconstructed
(56.44% of the 414,810-byte whole `.text` extent, which includes CRT and text
data). Thirteen shipped CRT/import boundaries add no application credit.
`binary.c` is the sole function, status and extent registry. The remaining
registry has 42 pending bodies with known extents totaling 4,434 bytes and
185 entries whose body extents remain unknown; this is not a complete non-CRT
denominator.

The `/help`, Win32 hello, DLL/DEF, standalone import-library and ordinary
COFF archive cases are complete. Hello produces the matching 24,064-byte executable. The DLL workload
links an original-compiled stdcall entry point and functions, a shared data
export, DEF aliases, explicit ordinals and a NONAME export. It produces the
matching DLL, EXP and LIB files. Its actual synchronous `/lib @response` child
executes the reconstruction in a fresh native module with shared first-entry
state; the reference supervises the real same-staged-EXE child. Parent and child
both exit 0, with 375 combined application first entries. The separately captured
standalone `/lib` invocation exits 0 with 249 entries; it is retained as its own
oracle, not composed into the combined reference.

The SELF policy validates the complete invocation and response-file input. The
fixture retains the actual initial target OS environment separately from the
CRT snapshot; the child request must equal that block plus exactly the reached
`LINK_REPRO_NAME` insertion. Native temp-file identity is carried through the
real child and deletion. The original staged T: executable and native canonical
R: executable denote the same-program role, not byte-equal physical pathnames.
The encountered ID 9001 resource uses the real LoadString path and an
identity-bound minimal STRINGTABLE.

Outputs compare the complete file inventory and all bytes except identified
clock fields: hello EXE COFF TimeDateStamp; DLL COFF and export-directory
TimeDateStamp; EXP COFF and export-directory TimeDateStamp; and the LIB archive
member Date fields and contained COFF TimeDateStamp fields. No general mapping,
pathname, relocation, padding or symbol-table normalization is used. The DLL
completion reference retains raw artifacts in scratch. The earlier EXE reference
retains a full SHA with a documented single-DWORD timestamp derivation, rather
than claimed raw artifact bytes.

`make -j4 check unit` runs all thirty-one cached native cases and their narrow checks;
fixtures never invoke Wine. The genuine DLL completion native replay took
58.7 ms (25.5 ms setup, 22.0 ms execution, 11.2 ms checks). The final parallel
regression/unit batch took 0.45 s; the successful full graph/RET/EH audit took
1.71 s. Necessary original/native builds also passed; these are incremental
closeout measurements, not clean-build timings.

`make -j4 win32 audit` verifies original-compiler compatibility and the full
628-vertex graph, imports, references, ownership and extents. Scoped return
checks cover 608 ordinary application bodies and six CRT bodies. Seven import
thunks remain in the graph without body-return claims. The seven genuine C++
lifetime owners retain separate normal-return, cleanup-action and qualified
frame-handler checks. Ordinary cached imports retain finite calling-convention
contracts; they do not grant provider or pending-helper body credit.
`/OPT:NOICF` keeps distinct pending trap entries from being folded.

The two DIRECT cases qualify the authentic 12-byte and 20-byte buffer owners
with NULL state and genuine allocations; the latter also checks Set with live
spans and a NULL callback. They establish internal lifecycle behavior. Only the
justified lifetime/operation TUs use special optimization and instrumentation
settings, with one physical cleanup owner and actual Win32 EH actions.

The completed workloads cover ordinary x86, nonincremental, no-PDB output.
Unreached Mac, PDB, callback, packed CodeView and further librarian edit/extract domains retain
named traps or explicit native qualification guards. The diagnostic current_phase
owner supports the observed 41-byte prefix, with guards before copies and
concatenations; its original full capacity is unknown. Physical DWORD flag
transports remain distinct from proven low-byte consumers and genuinely
unwritten bytes. The shared tokenizer supports only its qualified default-C,
same-thread, live-line domain.

Application source and original constructor registration are in src/;
harness/startup.c owns native setup/invocation. crt/crt-imports.h is a shipped
CRT interface declaration, with no implementation or application credit. Root
policy files contain live finite audit contracts. Historical raw captures,
failed candidates and earlier scope limitations remain in scratch/Git history.

The ordinary archive case creates `widgets.lib` from two original-compiled
COFF objects with cross-object function/data references and debug sections.
It exits 0 with 149 application first entries and a matching 2,758-byte archive.
The recovered route includes member removal traversal, object loading, both
archive symbol indexes, member copying and string/hash-table cleanup. It uses
neither the pending CoffLibrarianContext virtual bodies nor dependency replay.
Only the two index-member ten-digit Date fields are normalized; member dates
come from the fixture's fixed object mtimes, and all remaining bytes and the
complete file inventory compare strictly. Raw reference artifacts remain in
scratch. The genuine completion native replay took 15.9 ms (9.7 ms setup,
3.6 ms execution, 2.6 ms checks). Case-end regression/unit checks took 0.93 s;
the successful full graph/RET/EH audit took 1.71 s. These are incremental
measurements. Archive extraction and other conversion formats remain unqualified; the
short-import consumer below qualifies its reached conversion domain.

The archive-replacement case loads that unchanged original-produced archive,
replaces `left.obj`, retains `right.obj`, and adds a long-named cross-referencing
object. It exits 0 with 158 first entries and a matching 4,142-byte `updated.lib`,
including the real replacement diagnostic, long-name table, retained member
and rebuilt symbol indexes. Only the three identified index/long-name Date
fields are normalized; fixed input mtimes, member modes, payloads and inventory
remain strict. This path exposed a missing `%ho` text literal in the original
mask; its corrected four-byte ownership has no code credit, and failed evidence
remains in scratch. Encountered resource 9021 uses the existing identity-bound
STRINGTABLE route. Native completion took 16.5 ms (10.0 ms setup, 3.4 ms execution,
3.1 ms checks); case-end regression/unit checks took 0.45 s and the full audit
1.69 s. Explicit `/remove`, listing, extraction and conversion remain separate
unqualified domains.

The archive-backed executable case selects a real three-member reference chain
from `updated.lib`, excludes default libraries, removes an unused COMDAT and
folds two identical function bodies. The retained output disassembly confirms
both calls target the same folded body. It exits 0 with 269 first entries and
a matching 2,560-byte executable, normalizing only its identified COFF
TimeDateStamp DWORD. This is chiefly integration coverage; it adds the
177-byte default-library exclusion and 80-byte miscellaneous output-symbol
bodies. The diagnostic owner now supports exactly the reached 41-byte prefix,
including NUL, with its original total capacity unknown and guards retained
before longer writes. The affected boundary unit checks both this supported
archive/member name and rejection beyond the prefix. Native completion took
13.4 ms (8.2 ms setup, 2.9 ms execution, 2.3 ms checks). The final nine-case
regression/unit batch took 0.51 s; the full graph/RET/EH audit took 1.69 s.

The new-format COFF import-library case uses the original-compiled DLL object
and DEF aliases, stdcall names, explicit ordinals, NONAME and data exports with
`/lib /newimplib`. It exits 0 with 235 application first entries, producing the
matching 1,032-byte EXP and 2,522-byte LIB. Five genuine short-import records
carry the reached name/ordinal/type encodings; both symbol indexes, strings,
member padding and complete output inventory compare strictly. Only identified
archive Date fields and COFF/export/short-import timestamp DWORDs are normalized.
Raw outputs and the exact clock-field derivation remain in scratch. The newly
recovered Prepare, Finish, short-import helper and virtual Emit bodies total
1,256 original code bytes. The sole recovered Emit caller ignores its provisional
DWORD return; the source preserves the actual even-cursor/final-write paths
without a wider return-value claim. Other pending interface methods remain
named traps. Native completion took 33.7 ms (18.1 ms setup, 4.8 ms execution,
10.8 ms checks). The ten-case regression/unit batch took 0.98 s; the full
original-build/graph/RET/EH gate took 1.71 s.

The short-import client links an original-compiled x86 object against the unchanged
original-produced new-format import library. Its actual stdcall DllMain call and
imported data reference exercise named and ordinal imports; additional /include
records are archive roots, not client calls. Both runs exit 0 with 321 application
first entries and a matching 1,536-byte executable. Its final import table retains
hello.dll, the named _DllMain@12 import and ordinal 11. Only the identified COFF
TimeDateStamp DWORD at PE+8 is normalized; all other bytes and the complete file
inventory remain strict. Actual temp files are independently created, opened
with delete-on-close and removed, without pathname seeding or masking. Raw
reference output and failed partial comparisons remain in scratch.

This case adds 16 bodies / 3,351 original code bytes: short-header validation,
conversion lookup and construction, lazy payload owners and their actual cleanup
callbacks, converted section dispatch and copying, and decorated entry resolution.
The new-expression failure action retains its actual saved allocation, scalar
delete and C++ handler. Only encountered short-import section kinds 0..8 are
supported by the converted dispatch tables; other format kinds stop before
indirection. Delayed conversion, decorated-name undecoration and incremental
entry bookkeeping remain genuine pending traps. AddSymbol and ConvertShortMember
are void on the closed consumed call chain, preserving thiscall and RET32; this
does not settle the separate virtual Emit return. Three relocation emitters now
use the original second stack argument for their destination; earlier NULL/NULL
writer observations did not distinguish those slots. Defined carried DWORDs
remain distinct from unwritten upper bytes and their narrower consumers.

The final fresh native completion took 18.2 ms (11.2 ms setup, 4.3 ms execution,
2.7 ms checks). The eleven-case regression/unit batch took 1.08 s, and the full
original-build/graph/RET/EH gate took 1.85 s. These are incremental measurements;
the complete suites ran only at case completion. The source and case are native
fixtures, with original builds and captures using the shared toolchain launcher.

The mixed archive conversion case uses the unchanged original-produced short-import
library and ordinary archive containing a retained member, replacement object and
long-named object. `/lib /convert` emits a matching 9,380-byte archive with 182
application first entries and exit 0. The five short-import members become ordinary
COFF members; ordinary object payloads, member metadata, both symbol indexes,
long-name table, padding and complete inventory compare strictly. Only the two
index-member Date fields and long-name-table Date field are normalized over their
ten decimal digits. Raw mapping and final artifact copies remain in scratch.

The converted-member writer and its one-argument buffer Reserve overload add two
bodies / 1,335 original code bytes. The writer retains its actual section-header
updates, shared section-kind dispatch tables, symbol/string reads, final header
rewrite and 20-byte scoped owner. Its normal path inlines scalar deletion; the
exceptional action enters the same qualified canonical buffer destructor. The
bounded EH residual checks these distinct roles, RET32, the actual Set call and
qualified handler. The recovered debug-section flags are 0x42100040 and
0x42101040; this case exposed an earlier alignment-bit error whose previous
non-triggering observations remain limited. No repair credit or output mask was
added. Native completion took 14.3 ms (8.7 ms setup, 2.6 ms execution, 3.0 ms
checks). The twelve-case regression/unit batch took 1.13 s and the full
original-build/graph/RET/EH gate took 2.06 s, both only at case completion.


The LE/VxD serialization case links an original-compiled x86 object with code,
initialized data, BSS, internal fixups, a dynamic VXD definition, description and
ordinal exports. Its genuine synchronous librarian child and parent both exit 0,
with 324 shared application first entries. The final 4,769-byte VxD, 298-byte EXP
and 1,454-byte LIB match. The separately observed librarian invocation also
completes, with 206 entries; it remains an independent oracle. Actual temporary
names pass through response-file creation, SELF and deletion without seeding or
pathname masks. The ordinal-1 export is a function: the writer consumes its
actual 80-byte payload prefix for device/version fields. This qualifies the
original-accepted serialization route, not an operational DDB or loaded driver.

Thirteen bodies / 6,229 original code bytes cover VxD initialization, definition
text/export parsing, buffer Copy, warning dispatch, LE header sizing, object and
page tables, module names, entry bundles, section-index lookup, grouped fixups
and final headers. The LE header is one typed 196-byte owner with its actual
scalar defaults, including DDK version 0x400. Buffer/cache and section fields
retain their existing physical owners. Header growth/relaunch and terminating
warning-as-error helpers remain named traps; large page-count and unsupported
conversion domains remain guarded. No driver execution or wider PDB claim is
made.

Raw reference artifacts and failed partial comparisons remain in scratch. The
VxD output is byte-exact without normalization. EXP/LIB comparisons normalize
only the identified COFF TimeDateStamp DWORDs and five archive-member ten-digit
Date fields; every other byte and the complete file inventory remain strict.
The final fresh native completion took 40.4 ms (14.6 ms setup, 22.5 ms execution,
3.3 ms checks). The fourteen-case cached regression/unit batch passed in 1.07 s;
the successful full 451-vertex / 440-return-contract / four-EH-owner gate took
2.13 s. The audit's initial missing cached toupper identity was closed with its
ordinary documented cdecl EBP-preservation contract and exact site inventory.
No regression or reference capture was repeated for that metadata correction.


The COFF debug/map/order case links two original-compiled `/O2 /Gy /Z7`
objects with cross-module calls, identical packaged functions, initialized
pointer relocations and a merged data section. The order file places StepRight,
StepLeft and the stdcall DebugEntry; unused code is discarded and the identical
functions share an output address. Both runs exit 0 with 315 application first
entries and matching 4,096-byte EXE and 1,595-byte map. The map includes public
symbols, source lines, relative-fixup deltas and the 16-byte ICF saving. The
original warns that `/pdb:none` disables `/debugtype:fixup`; this case does not
qualify that disabled debug format.

Thirty-two bodies / 10,153 original code bytes add order-file reading and
placement, discarded/folded COMDAT reports, value-sorted symbols, COFF symbol
emission and line processing, map records/presentation and mapped-file length
adjustment. The folded report reuses the qualified 12-byte scoped buffer;
its normal path inlines scalar deletion while its genuine exceptional action
enters the single canonical destructor. Its fifth bounded EH check retains
normal RET0, action/handler and FuncInfo roles independently. Private line
records retain their WORD line, section pointer and DWORD offset; alignment
bytes are not semantic fields or output comparison exceptions. The module's
line-file list is the actual +0x80 owner, and the debug-line factories remain
separate owners from the scoped buffer. PDB and CodeView children remain named
traps, with no wider debug-state claim.

The saved final raw artifacts bind the comparison-only clock derivation. Only
the EXE COFF TimeDateStamp, three IMAGE_DEBUG_DIRECTORY TimeDateStamp copies,
and the map's corresponding eight hexadecimal digits and 24-character `ctime`
text are normalized. All remaining bytes, lengths and file inventory are strict.
Failed raw captures preserve the missing `.debug$C`/`.debug$E` literal ownership
and omitted line-format terminator; these were repaired in the mask, without
output relaxation or application code credit. The map caller's earlier stub
had mislabeled its second argument as a stream; the recovered caller/body use
the real output filename and retain the separate global map stream.

Fresh native completion took 17.1 ms (10.0 ms setup, 3.7 ms execution, 3.4 ms
checks). The final fifteen-case regression/unit batch took 0.53 s and the full
original-build/graph/RET/EH gate took 2.45 s. The interrupted closeout attempts
and expected-fixture clock encoding correction remain in scratch; successful
prior cases were not recaptured. Full regression and audit ran only at case
closeout, after the current-case pair completed.


The raw CodeView case links two original-compiled `/Od /Gy /Z7` C modules with
structures, unions, enums, arrays, locals and cross-module function/data references.
`/debugtype:both /pdb:none /nopack` retains uncompressed CodeView alongside COFF
debug and the source-line map; it invokes no CVPACK dependency. Both runs exit 0
with 291 application first entries and matching 7,168-byte EXE and 1,695-byte map.
Twenty-six bodies / 6,955 original code bytes cover per-module debug tracking,
pooled file/section/line records, size calculation and serialization, contribution
spans, public symbols, segment maps and the CodeView subsection directory.

The existing 68-byte debug-module table has one shared definition, and the
module's existing line-buffer and file-offset fields retain their physical owners.
The serializer preserves actual WORD/BYTE count and length conversions, list
order, padding stores and release behavior. No debug payload, names, padding,
allocation state or file inventory is normalized. Long public names still reach
the authentic MSPDB50 `TruncStFromSz` boundary, whose shared native surface is
currently typed denial only; this case does not call it or qualify truncation.
PDB-backed module output remains a separate pending path. The raw EXE difference
was confined to the identified COFF TimeDateStamp and its three debug-directory
copies; the map normalizes only its corresponding eight hexadecimal digits and
24-character `ctime` text. The original raw artifacts and comparison derivation
remain in scratch.

This case exposed a previously unvisited caller read: `EmitDebugDirectories`
seeks to the debug section's stored absolute end at offset +8 before CodeView,
not its raw size at +12. The correction adds no code credit; the failed raw pair
and the corrected native comparison against the unchanged reference are retained.
Fresh native completion took 13.8 ms (8.2 ms setup, 3.0 ms execution, 2.6 ms checks),
and the original took 1,255.5 ms. The sixteen-case cached regression/unit batch
passed in 1.21 s; the full 509-vertex / 497-return-contract / five-EH-owner gate
passed in 2.50 s. Full checks ran only at case completion.


The definition-settings DLL case combines original-compiled stdcall code, shared
initialized data and BSS with DEF heap/stack sizes, a version, section attributes,
named/ordinal-only/data exports, an existing original-produced MZ stub input and
an export map. The genuine librarian child and parent both exit 0, with 366
shared application first entries. The final 6,656-byte DLL, 989-byte EXP,
3,412-byte LIB and 1,155-byte map match. Its separately observed librarian
invocation also exits 0, with 236 entries, and remains an independent oracle.
This qualifies file serialization, not execution of the DLL or DOS stub.

Six bodies / 4,688 original code bytes cover DEF size/section parsing, the
attribute lookup, numeric switch parsing, section-characteristic application and
relocation-based map export names/ordinals. The section attribute owner remains
distinct from section merging despite its provisional function label. The map
writer preserves actual COFF relocation and ordinal-table roles, borrowed names,
forwarded-export formatting and original buffer/file cleanup; this workload
reaches named, ordinal-only and data exports, not a forwarded-export runtime.
Cached tokenizer and map imports use finite ordinary nonvolatile ABI contracts.
PDB, incremental and Mac dependencies remain pending or explicitly guarded.

Raw original artifacts and failed authored-alias/clock-reference comparisons
remain in scratch. Only generated DLL COFF/export timestamps, EXP timestamps,
LIB Date/generated COFF timestamps and the map's corresponding eight hexadecimal
digits and 24-character ctime text are normalized. The existing MZ stub's own
bytes, all padding, symbol records, names and final inventory remain strict.
Fresh native completion took 40.3 ms (14.8 ms setup, 22.3 ms execution, 3.2 ms
checks); the supervised original took 1,805.8 ms. The single case-end parallel
cached regression/unit/original-build/full-audit batch passed in 2.76 s, including
all eighteen fixtures, the 517-vertex graph, 503 return contracts and five
separate C++ EH-owner checks. That combined wall time is not a separate audit
benchmark or clean-build measurement.


The LINK_REPRO package case links the existing original-compiled entry object
against its original-produced archive, using separate base/address and order
files. It creates a real reproducibility directory containing all four consumed
inputs and a 233-byte response file with rewritten relative paths. An existing
entry.obj destination remains unchanged; the actual exclusive-create collision
loop chooses entry.2.obj and writes that name into the response. Both sides exit
0 with 293 application entries and matching 2,560-byte EXE, 1,281-byte map,
copied inputs and complete directory inventory. This establishes package
serialization; it does not claim that the package was subsequently relinked.

Two bodies / 541 original code bytes add the copier and response closer.
CopyFileA uses the reviewed shared explicit-5.10 regular-file domain after the
caller reserves and closes an empty destination. Original error-path LastError,
security streams, non-normal attributes and wider file domains are unclaimed.
Only the new EXE's COFF TimeDateStamp and its map hex/ctime presentation are
normalized. Copied object/archive timestamps, response bytes, directory names,
existing destination bytes and every other artifact byte remain strict.

This case also repaired two previously limited state models without extra
credit. The actual logo BYTE starts at 1, allowing an early LINK_REPRO warning
before /nologo parsing. The actual 47eeb8 stream has one source owner, initialized
to stdout by main, replaced by the map open and closed by the linker caller.
Earlier source had split that owner and closed the wrong stream; natural exits
had hidden the pending map flush. The failed prefix/regression and original
writer/reader evidence remain in scratch. No NULL-fclose shim or extra reference
run was used for the repair.

The supervised reference took 1,563.0 ms. Final corrected native replay took
15.2 ms (9.1 ms setup, 2.9 ms execution, 3.2 ms checks). The initial 2.56 s
case-end batch retained its passing unit/cached checks but caught the stream
identity mistake in hello. Only hello and the current case were replayed after
the repair; the remaining cached checks and current full original-build/audit
passed in 2.67 s. All nineteen cached cases and unit checks are covered, with a
517-vertex graph, 505 return contracts and the five existing EH-owner checks.
These batch times combine work and are not isolated audit benchmarks.


The Mac PowerPC import-library case uses the authentic `/lib /machine:MPPC`
route with an authored DEF containing function and data exports, explicit
ordinals, initializer/terminator names and distinct current/code/API versions.
Both sides exit 0 with 160 application entries and matching 1,959-byte EXP and
1,434-byte LIB files. The EXP contains the original 676-byte `.ppcshl` header,
its character buckets and five 200-byte name records. Archive members carry
ordinary COFF `.drectve` import/version text and short/long symbol records.
This qualifies serialization; no Mac executable or runtime invocation is claimed.

Five bodies / 3,866 original code bytes recover version merging, initialization
and termination name decoration, the shared-library header writer and the Mac
member emitter. The actual code-version owner is 4670b4 and the API-version
owner is 4670b0. Earlier Prepare source had conflated these initially equal
owners; its real OldAPIVer consumer now uses the API owner. Two formerly
unvisited caller/header fields were also corrected without credit: `.ppcshl`
has no relocation pointer, and the writer's sixth DWORD is its section file
offset, not a symbol count. Failed comparisons and raw bytes remain retained.
Per-library version lists and incremental history retain genuine pending
append calls. Native header names longer than the emitted 52-byte field remain
guarded; the Win32 source retains its original unchecked copy.

Only generated COFF TimeDateStamp DWORDs and the ten decimal digits in archive
Date fields are normalized. Spaces, padding, version text, buckets, symbols,
complete lengths and final inventory stay strict. The masked completion also
matches the separately retained untouched original master outside the same
clock fields. The original took 1,701.1 ms; fresh native completion took 13.9 ms
(8.8 ms setup, 2.4 ms execution, 2.7 ms checks). The single case-end parallel
cached regression/unit/original-build/full-audit batch passed in 3.63 s, covering
twenty fixtures, the 528-vertex graph, 516 return contracts and the five existing
C++ EH-owner checks. This batch time combines work and is not an isolated audit
or clean-build benchmark.

Six additional whole bodies / 1,237 bytes retain their genuinely matched
incremental-prefix evidence: support checks, old-state loading/database
initialization and the OS/address-selection helpers. They do not establish ILK
creation or reuse. The attempted incremental case is excluded from passing
fixture discovery because its first original fixed mapping at 3fff0000 fails
with error 487 while the identical native request succeeds. A single read-only
original diagnostic observed that first request, NULL return and the target's
immediate GetLastError; the earlier final retry's 1132 is a distinct observation.
The unaccepted mapper remains a named entry trap with implementation 0; its
whole source draft, authored inputs, raw pairs and blocked-case snapshot remain
in scratch. No collision was seeded or emulated, and these prefixes make no
incremental-completion claim.


The Alpha resource-only DLL case uses a genuine original RC/CVTRES-produced
Alpha COFF object with English/German string tables, version information and
a named binary resource. Both sides exit 0 with 248 application first entries
and matching 2,560-byte DLL and 406-byte map. The final masked reference also
matches the separately retained untouched master outside the same clock fields.
Only the DLL COFF TimeDateStamp DWORD at PE+8 and the map's corresponding eight
hexadecimal digits and 24-character ctime text are normalized. Resource trees,
UTF-16 strings, payloads, fixups, padding, lengths and complete inventory remain
strict. Raw artifacts and failed clock-only comparisons stay in scratch.

Six whole bodies / 4,095 original code bytes recover Alpha backend setup,
relocation classification/sizing/application, section data sizing and code-range
sizing. Existing section, contribution, incremental-code and GP owners are reused.
The actual reader retains five cached SigForPbCb sites under finite ordinary EDI
preservation contracts. Seven-slot fixup calls retain RET28; code-range sizing
retains its receiver, three stack slots and RET12. The pending branch-target,
auxiliary-fixup and diagnostic children remain named entry traps with zero body
credit. This workload exercises resource REFLONGNB serialization, not Alpha
instruction execution, GP-relative code, large-image branch islands, ROM output
or incremental linking. The original branch helper consumes only the low BYTE
of its physical absolute-state DWORD; no equality of the caller's unwritten
upper scratch bytes is claimed.

The ordinary resource producer used the original RC source retained in case.h;
failed /nologo and dummy-header-only guarded-input attempts remain excluded.
The genuine completion original took 1,273.4 ms. Its corrected native comparison
took 17.3 ms (10.2 ms setup, 3.8 ms execution, 3.2 ms checks). The single case-end
parallel cached regression/unit/original-build/full-audit batch passed in 3.71 s,
covering twenty-one fixtures, the 534-vertex graph, 522 return contracts and the
five existing C++ EH-owner checks. These are incremental timings; the batch is
not an isolated audit or clean-build measurement.


The MIPS Windows CE resource-only DLL case uses original RC/CVTRES output
with three languages, version data and a named payload, plus COFF debug and
a map. The period producer requires /WINDOWSCE for MIPS, and LINK requires
/SUBSYSTEM:WINDOWSCE; the earlier rejected non-CE attempts are retained without
completion credit. Both successful runs exit 0 with 229 application entries and
matching 4,096-byte DLL and 515-byte map. The separately retained untouched
master matches the masked completion outside the same exact clock ranges.
Only the COFF TimeDateStamp, its two IMAGE_DEBUG_DIRECTORY copies, and the
map's corresponding eight hexadecimal digits and 24-character ctime text are
normalized. Resource trees, UTF-16 payloads, debug bytes, relocation output,
padding, lengths and complete inventory remain strict.

Four bodies / 2,697 original code bytes recover the MIPS backend, classifier,
reader and fixup processor with their actual stdcall cleanups and five cached
checksum import sites. Existing section, GP, incremental-code, error and
auxiliary-fixup owners are reused. The resource path exercises REFWORDNB; it
does not qualify MIPS instruction execution, GP-relative code, incremental
patching or other machine relocation domains. Their genuine pending child
entries remain trapped. Ordinary SDK spellings replaced authentic numeric
values before closeout; native .text remained byte-identical to the matched
completion build.

The full-body comparison also corrected two unvisited Alpha tails with zero
credit: paired debug-fixup sites use the contribution RVA after accounting
for the preceding stack push, and a missing GP diagnostic skips the current
relocation after the error counter. The prior Alpha resource-only observation
reached neither tail; its REFLONGNB qualification remains unchanged. Current
narrow static checks and the ordinary cached Alpha regression pass, without
a retrospective original capture or wider error-domain claim.

The completion original took 857.1 ms; native comparison took 13.7 ms
(7.6 ms setup, 3.5 ms execution, 2.6 ms checks). The one case-end parallel
regression/unit/original-build/full-audit batch passed in 3.36 s, covering
twenty-two fixtures, 538 graph vertices, 526 return contracts and the five
existing C++ EH-owner checks. Raw artifacts and failed clock comparisons are
retained in scratch. Batch timing is incremental, not a clean-build or
isolated audit benchmark.


The ARM Windows CE resource-only DLL case uses genuine original RC/CVTRES
output with three-language strings, version information and a named payload,
COFF debug and a map. Both sides exit 0 with 232 application entries and
matching 4,096-byte DLL and 514-byte map. The masked result also matches the
retained untouched master outside the same clock fields. Only the COFF and
two debug-directory TimeDateStamp DWORDs, and the map hex/ctime rendering,
are normalized; all resource, relocation, section-header, padding and inventory
bytes remain strict. The reached /ALIGN warning uses its authentic resource.

Four bodies / 1,235 original code bytes recover ARM initialization,
classification, relocation reading and processing with their actual stack
cleanups and checksum sites. Private relocation names follow the encountered
target table; this period's shipped SDK does not declare ARM relocation constants.
The resource ADDR32NB route is qualified, without ARM instruction execution,
branch encoding, small-data or incremental-domain claims. Pending auxiliary
and debug-fixup children retain genuine named entry traps.

The case repaired a split global with zero credit: original 47f100 is one
small-section-alignment BYTE written by backend initialization and read by
section layout and BSS flag conversion. Its formerly separate source owners
caused a real .reloc VirtualSize mismatch; the failed raw output is retained.
One shared owner, a distinct-state unit and the unchanged-original native
comparison close that repair. Cold Alpha/MIPS auxiliary calls also now forward
the actual relocation VirtualAddress, rather than patched payload data; earlier
resource observations did not reach that small-data domain.

The completion original took 1,114.4 ms; native comparison took 16.3 ms
(10.2 ms setup, 3.2 ms execution, 2.9 ms checks). The single case-end parallel
regression/unit/original-build/full-audit batch passed in 3.75 s, covering
twenty-three fixtures, 542 graph vertices, 530 return contracts and the five
existing C++ EH-owner checks. Raw artifacts and failed comparisons remain in
scratch. These are incremental timings, not clean-build or isolated audit costs.


The PowerPC Windows CE resource-only DLL case uses genuine original RC/CVTRES
output with three languages, version data, a named binary resource, COFF debug
and a map. Both sides exit 0 with 234 application entries and matching
4,096-byte DLL and 518-byte map. All bytes outside the three identified COFF/
debug TimeDateStamp DWORDs and the map hex/ctime text match both the masked
reference and retained untouched master. Complete artifact inventory, resource
contents, relocation results, padding and lengths remain strict.

Four whole bodies / 3,449 original code bytes recover the PowerPC backend,
classifier, relocation reader and processor. The actual backend symbol-map BYTE
remains distinct from Mac-target state, while small-alignment behavior uses
the shared owner qualified by the ARM case. Physical argument slots, RET8/16/28
cleanups, paired fixup records and finite cached-checksum import sites are
preserved. The branch-target helper and other unvisited children remain named
traps with zero body credit. This workload qualifies resource ADDR32NB output,
not PowerPC execution, branch-island allocation, GP-relative or incremental
semantics. SDK constants replaced authentic numeric values without changing
native .text bytes; target-only relocation 21 remains explicitly identified.

The completion original took 1,101.1 ms; native comparison took 17.4 ms
(9.8 ms setup, 4.2 ms execution, 3.4 ms checks). The single case-end parallel
regression/unit/original-build/full-audit batch passed in 4.23 s, covering
twenty-four fixtures, 546 graph vertices, 534 return contracts and the five
existing C++ EH-owner checks. Raw artifacts, the failed compiler declaration
and clock-only comparisons remain in scratch. These are incremental timings.


The SH4 Windows CE resource-only DLL case uses genuine RC/CVTRES SH4 output
with three-language strings, version data, a named payload, COFF debug and a
map. Both sides exit 0 with 232 application entries and matching 4,096-byte
DLL and 514-byte map, also equal to the untouched master outside the same
clock fields. Only the three COFF/debug timestamp DWORDs and map hex/ctime
text are normalized; the complete remaining artifacts and inventory are strict.

Four bodies / 2,014 original code bytes recover the shared SuperH initializer,
classifier, reader and processor. The backend's authentic SH3 diagnostic text
is preserved even for SH4. Private relocation names follow the actual target
table rather than an unverified SDK declaration. The reached DIRECT32_NB
resource route is qualified; SuperH instruction execution, other operand
encodings and section-name fixups are unclaimed. The latter's genuine lookup
child remains a named pending trap. Current source preserves complete field
widths, RET8/16/24/28 and debug-fixup call order.

Original completion took 913.0 ms; native comparison took 14.1 ms (8.7 ms setup,
2.6 ms execution, 2.8 ms checks). The one case-end parallel regression/unit/
original-build/full-audit batch passed in 3.95 s: twenty-five fixtures, 550
graph vertices, 538 return contracts and the five existing EH-owner checks.
Raw originals, native diagnostic artifacts and clock-only failures remain in
scratch. These are incremental timings, not isolated audit costs.


The archive/PE Dumper case runs `/dump /all /nologo widgets.lib resource.dll`
against the unchanged original-produced ordinary COFF archive and SH4 resource
DLL. Both sides exit 0 with 126 application first entries and exactly 28,091
stdout bytes; there are no output files. The corrected masked reference also
matches the untouched master on complete streams, inputs and inventory, with
no normalization. Dumper does not support `/nologo`; its authentic logo and
warning 4044 remain part of the strict output.

Forty-two whole bodies / 23,918 original code bytes recover option parsing,
archive indexes and members, COFF/PE dispatch, file and section headers, raw
data, COFF relocations, line numbers, FPO, symbols and auxiliaries, debug
directory display, base relocation records, symbol indexing and summary sorting.
The original 12-byte COFF dumper object uses genuine inline construction and
destruction, with separate failed-new actions and qualified frame handlers.
Only its actual owner TU has the justified optimization/instrumentation settings.
One empty physical RET12 body has a code-free stdcall alias for an otherwise
thiscall entry; it consumes neither receiver nor formals and adds no body credit.

The readonly debug loader reopens the live read/share-read input through the
tracked `_fsopen` lease while its readonly view remains live. This input has
COFF and MISC debug records without CodeView; PDB interfaces are declarations
only and the cold OpenValidate boundary remains denied. Provider objects are
never manufactured. Separate debug files, PDB lookup, disassembly, further
machine relocation names and unvisited debug-content children retain named
traps or precise native guards. The 328-byte per-file scratch, shared nine-byte
symbol-name buffer, 12-byte export records and existing 16-byte NB10 header each
retain their actual storage identity and field widths.

Failed raw comparisons remain in scratch. Newly encountered format-literal
NUL extents were corrected before final qualification, without code credit or
output masks. The completed original took 1,513.7 ms; fresh native comparison
took 18.9 ms (11.6 ms setup, 3.9 ms execution, 3.4 ms checks). The single case-end
parallel cached regression/unit/original-build/full-audit batch passed in 4.97 s,
covering twenty-six fixtures, 592 graph vertices, 578 return contracts and seven
separate C++ EH-owner checks. These are incremental timings, not isolated audit
or clean-build benchmarks.


The import/export Dumper case runs `/dump /all /dependents hello.dll client.exe`
with unchanged raw original-produced inputs from the completed DLL and import
client workloads. The DLL has named aliases, explicit ordinals, a NONAME export
and exported data; the client imports the DLL by name and ordinal. Both sides
exit 0 with 93 application entries and exactly 10,907 stdout bytes. Complete
streams and input inventory match the separately retained untouched master,
without normalization or invocation of either input image.

Three bodies / 4,085 original code bytes recover dependency names, import
descriptors and thunks, and export names, ordinal slots and unnamed entries.
The source preserves independent lookup/address tables, actual file seeks,
byte reads, unsigned RVA checks and RET4 contracts. Ordinary stdcall read/seek
preservation facts cover only the actual cached-import sites. The bound-import
header is absent in these inputs, and forwarded exports are not exercised;
those source paths gain no separate dynamic qualification. Malformed indices
and unterminated or out-of-range native reads remain outside the supported
input domain. Period SDK pointer-typed thunk/address fields are transported
through their actual DWORD RVA interpretation, without a native layout fork.

The completion original took 1,680.3 ms; fresh native comparison took 18.8 ms
(12.2 ms setup, 3.5 ms execution, 3.1 ms checks). The one case-end parallel
regression/unit/original-build/full-audit batch passed in 4.09 s: twenty-seven
fixtures, 595 graph vertices, 581 return contracts and seven separate EH-owner
checks. Raw references and build failures remain in scratch; timings are
incremental rather than isolated audit or clean-build measurements.


The PE editor case runs `/edit /nologo /release /section:.text=.code,erw`
with stack `0x200000,0x4000`, heap `0x300000,0x8000`, console subsystem 4.1,
image version 3.7 and OS version 5.1 on the unchanged original-produced
`hello.dll` and `client.exe`. Both sides exit 0 with 74 application first
entries. The modified 3,072-byte DLL and 1,536-byte client match the untouched
master exactly, including computed PE checksums, with no normalization.

Eight whole bodies / 7,179 original code bytes recover the editor driver,
input precheck, writable-file preparation, option dispatch, section parsing,
symbol-name updates, stack/heap number parsing and shared checksum method.
Section records retain the original spare first slot. The editor consumes the
existing ordinary-file list, distinct from the link/dump list; the initial
wrong-list comparison remains failed evidence in scratch. The two real parser
switch tables and all RET20/RET12/RET8/RET0 roles pass the ordinary owner checks.
The same context byte has separate editor signature-check and dumper header
selection roles in their independent context instances.

This case exercises PE header edits and in-place mapped checksum computation.
Bind/rebase, OMF conversion, COFF symbol rewriting and file-checksum fallback
success remain unvisited; pending bodies keep named traps, and cold IMAGEHLP
APIs deny natively before effects. Numeric size parsing guards the original's
unwritten commit-local domain on invalid leading-comma input. The authentic
large-address-aware bit remains numeric in ordinary source because the shipped
VC5 header lacks that spelling; native SDK declarations do not change the
original build's headers. Earlier uppercase section flags were rejected by the
original; the supported workload uses its actual lowercase syntax.

The completed original took 1,674.9 ms; fresh native comparison took 18.5 ms
(12.4 ms setup, 3.1 ms execution, 3.0 ms checks). The single case-end parallel
regression/unit/original-build/full-audit batch passed in 4.82 s: twenty-eight
fixtures, 603 graph vertices, 589 return contracts and seven separate EH-owner
checks. Raw reference artifacts and failed build/comparison evidence remain in
scratch. These measurements are incremental, not clean-build benchmarks.

The legacy-image dumper case completes with 79 application first entries and
strict streams and file inventory, without clock normalization. It reads the
shipped OS/2 NE WBIND executable and an unchanged original-produced LE/VxD
image. The NE path emits the original unsupported-format warning; it does not
decode NE tables. The LE path prints its header, object records and page maps,
then reads and dumps the genuine object bytes, retaining zero-filled pages and
the shorter final file page. The object record's reserved DWORD aliases the
writer's existing four-byte tag at the same offset. Native malformed-page
bounds stop before unsafe pointer arithmetic or writes. Completion replay took
18.6 ms (12.0 ms setup, 3.5 ms execution, 3.1 ms checks); the parallel case-end
regression, unit, original-build and full graph/RET/EH batch passed in 4.74 s.
These are incremental measurements.

The disassembly workload remains dependency-blocked and excluded from passing
fixture discovery. Its complete inputs, raw references and scoped patch are
retained in scratch. Two matched prefix bodies (directive token printing and
disassembly dispatch) are preserved with their reached-prefix qualification;
the actual MSDIS-dependent section wrapper at RVA 22c40 remains a named trap
with no implementation credit. The shipped provider lacks some dormant exact
imports, and no decoder, substitute vtable or provider-success behavior is
claimed. The new completion carries these two prefixes and six legacy dumper
bodies, adding eight functions and 2,428 code bytes since the PE editor case.

The import-archive/map inspection case completes with 129 application first
entries over the genuine original-produced DLL import archive, an optimized
original-compiled COFF object, and the DLL/client images. COFF symbol maps retain
the actual auxiliary-record walk and next-record length calculation. The import
collector uses the real `.idata$5` word and `__imp_` symbol, preserving ordinal
presence and the shared short-name storage. With no ordinal, its original local
ordinal slot and three local flag-padding bytes are unwritten; native zeros are
representation only, and the real consumer tests the presence BYTE before
reading the ordinal. Defined pointer bits carried in the reused input formal
are distinct from those unwritten local bytes. The section-search helper and
external decoder remain pending because this workload did not reach them.

All streams, input bytes and final inventory match the untouched reference
without normalization. Completion replay took 21.1 ms (12.3 ms setup, 5.4 ms
execution, 3.3 ms checks). The parallel case-end regression, unit,
original-build and full graph/RET/EH batch passed in 4.19 s. This adds two
whole bodies and 828 code bytes; the current 30-case graph has 613 vertices,
599 scoped return contracts and seven distinct EH role checks. The attempted
separate-debug input producer remains separately blocked: shipped REBASE
returned error 120 while splitting symbols, so no `.DBG` fixture or decoder
success was invented.

The buffered-file case uses the original `-db44` control with the genuine
cross-module COFF-debug/map/order/merge link inputs. It completes with 323
application first entries and produces the matching 4,480-byte executable and
1,595-byte map. Its 15 whole bodies / 2,432 code bytes recover the 30-buffer
pool, ordinary CRT open/stat, seek, page assignment and LRU update, physical
read, chunked copy/write, flush/release/close, resize and sequential page advance.
The 4,136-byte cache record contains one 4,096-byte data page plus its authentic
metadata and links. FileRecord's existing offset 16 storage has explicit Win32
handle and CRT descriptor views; its 40-byte layout is unchanged.

The original page-boundary transition occurs even when a request ends exactly
there. Sequential buffer reuse retains its existing bytes, and the write path
marks the next page dirty before decrementing the remaining count. The fatal
exit flag is one original owner at 48a738; its initially zero normal-cleanup path
is observed, while its pending fatal writer retains no new qualification.
Cache eviction under pressure and page-table growth remain genuine named
traps. This case does not qualify malformed ranges, IO failures, or every
unvisited branch merely because its complete source is present.

Raw original and native artifacts differ only at the already-identified COFF
and three debug-directory timestamp DWORDs and the map's hex timestamp/ctime
rendering. All other bytes and complete inventory also equal the untouched
master. The derived comparison keeps the genuine original report and capture
unchanged; the master retains its historical 315-entry trace from the earlier registry,
while the final reference observes the eight newly registered reached helpers as part of 323.
Normal completion replay took 17.7 ms (10.6 ms setup, 3.8 ms execution, 3.3 ms
checks). The separate native GDB raw-file diagnostic took 202.3 ms and is not a
fixture benchmark. The single parallel case-end regression, unit,
original-build and full graph/RET/EH batch passed in 4.66 s. Two missing
stdcall declarations during the source split were caught by original linking
before execution; those failed candidates remain excluded and retained in
scratch.
