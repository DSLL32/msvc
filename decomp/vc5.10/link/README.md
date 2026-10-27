# VC5.10 LINK

689 application functions / 253,634 original code bytes are reconstructed
(61.14% of the 414,810-byte whole `.text` extent, which includes CRT and text
data). Shipped CRT/import boundaries add no application credit.
`binary.c` is the sole function, status and extent registry. The remaining
registry has 38 pending bodies with known extents totaling 4,066 bytes and
173 entries whose body extents remain unknown; this is not a complete non-CRT
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

`make -j4 check unit` runs all forty-three cached native cases and their narrow checks;
fixtures never invoke Wine. The genuine DLL completion native replay took
58.7 ms (25.5 ms setup, 22.0 ms execution, 11.2 ms checks). The final parallel
regression/unit batch took 0.45 s; the successful full graph/RET/EH audit took
1.71 s. Necessary original/native builds also passed; these are incremental
closeout measurements, not clean-build timings.

`make -j4 win32 audit` verifies original-compiler compatibility and the full
702-vertex graph, imports, references, ownership and extents. Scoped return
checks cover 681 ordinary application bodies and six CRT bodies. Seven import
thunks remain in the graph without body-return claims. The eight genuine C++
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
Unreached Mac, PDB, callback, packed CodeView and further librarian listing/edit domains retain
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
measurements. That observation did not qualify archive extraction or other conversion formats; the
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
1.69 s. That observation did not qualify explicit `/remove`, listing, extraction or
conversion.

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

The failed-link case uses two genuine original-compiled objects in buffered-file
mode, with duplicate strong data/function definitions and unresolved references.
One undefined symbol has two referring modules, exercising the real four-slot
reference block; the other retains its single-module owner. Both duplicate
warnings, all three unresolved-reference diagnostics and the fatal summary
match the untouched master exactly. Both executions retain the full DWORD exit
status 1120, with an empty map and no executable. No normalization is used.

Six whole bodies / 539 original code bytes recover duplicate reporting,
reference-enumerator initialization/advance, ordinary error and fatal wrappers,
and terminating cleanup. The third fatal formal keeps its physical DWORD
transport and actual low-BYTE cleanup meaning; exit receives the diagnostic
number. The single fatal flag at 48a738 is set before file cleanup, so buffered
pages follow their original discard path. Incremental/PDB cleanup children and
warning-as-error resource 1218 remain unvisited; genuine pending children retain
entry traps. Ordinary cdecl nonvolatile ESI contracts qualify only the two
actual cached unlink sites across unlink/access calls, with exact site checks.

The completed original took 1,664.6 ms; the fresh native comparison took
16.5 ms (11.3 ms setup, 2.1 ms execution, 3.1 ms checks). All capture fields equal
the separately retained untouched master. The single parallel case-end
regression, unit, original-build and full graph/RET/EH batch passed in 4.17 s:
32 cases, 634 graph vertices, 620 scoped return contracts and seven distinct
EH-owner checks. Earlier single-reference inputs/prefixes, rejected producer
flags and failed static metadata remain separately retained in scratch. These
are incremental measurements, not clean-build timings.

The architecture-inspection case dumps all records from the five unchanged
original RC/CVTRES resource objects used by the Alpha, MIPS, ARM, PowerPC and
SH4 link cases. It completes with 107 application first entries and exactly
48,640 stdout bytes. Status, streams, all input bytes and complete inventory
equal the untouched master; the final capture itself is byte-identical to that
master. No normalization, disassembly or input-code execution is involved.

Five whole bodies / 1,781 code bytes recover the architecture relocation-name,
width and symbol-presence decoders. Physical DWORD inputs retain their actual
WORD interpretation; PowerPC separately consumes the low type BYTE and next
flag BYTE. Its formatter has one mutable owner with a guarded 45-byte supported
output extent derived from all fixed names and suffixes, not an assertion of
original capacity. This input exercises unflagged resource relocations; dormant
flag combinations, other relocation variants and the Mac decoder remain
unqualified. Original and rebuilt switch tables retain exact ownership and
RET12 checks.

The first failed pair exposed one shared CRT difference: timestamp zero was
rendered as `Jan 01` by the original and `Jan  1` by host libc. The ordinary
explicit-5.10 ctime boundary now preserves the Microsoft day formatting, and
the saved deeper reference passed without another original. Failed streams
and the separate native GDB diagnostic remain in scratch; no clock field was
masked. Historical DST and cross-API time-buffer aliasing are not claimed.

Completion took 1,575.8 ms in the original and 21.0 ms natively (11.0 ms setup,
6.8 ms execution, 3.2 ms checks). The single parallel case-end regression, unit,
original-build and full graph/RET/EH batch passed in 4.85 s: 33 cases, 639 graph
vertices, 625 scoped return contracts and seven separate EH-owner checks.
These are incremental measurements.

The Alpha ROM case serializes the unchanged original RC/CVTRES resource object
with COFF debug and a map. It exits 0 with 223 application first entries, a
3,072-byte ROM image and a 464-byte map. The real 56-byte ROM optional header,
read-only/data section type conversion and debug-directory terminator are
retained. The newly recovered 256-byte converter preserves the other machine
branches and the BSS-base store, but this resource-only input qualifies Alpha
read-only/data serialization; it makes no firmware/runtime, code-section or
other-machine execution claim. A prior MIPS proposal was rejected by the
original backend and remains separately retained, not a ROM qualification.

Raw original/native/master artifacts differ only at the COFF timestamp DWORD
at file offset 4, the two actual `.rdata` debug-directory timestamp copies,
and the map's exact hex/ctime fields. The derived reference preserves the
untouched raw capture/report and compares every other byte and inventory
strictly. Completion replay took 17.6 ms (11.1 ms setup, 2.9 ms execution,
3.7 ms checks); its original took 1,206.3 ms. The native GDB raw-file diagnostic
is separate from that timing.

All 34 cached cases, unit checks and original builds passed in the initial
1.80-second case-end batch. Its full audit exposed undeclared emitted switch
data at the former end of the section-layout TU. Exact rebuilt table/byte-map
ownership for the unchanged merge body and new ROM helper corrected that
metadata; the audit-only retry passed in 3.93 s, without another case campaign.
The current full graph has 640 vertices, 626 scoped return contracts and seven
separate EH-owner checks. These are incremental measurements.

The buffered-pressure case links 33 genuine original-compiled cross-referencing
COFF modules with an original RC/CVTRES resource object containing a 5,243,003-byte
binary payload. The ordinary `/include` root retains the module graph without
claiming a client call or executing image code. Buffered mode exhausts the real
30-page pool and grows the output page table beyond its initial 4 MiB estimate.
The completed 5,517,824-byte DLL contains the complete payload unchanged; its
7,708-byte map, streams, status and full inventory also match the untouched
master outside only the identified COFF/debug timestamp DWORDs and map time
fields. Both executions exit 0 with 262 application first entries.

Two whole bodies / 215 code bytes recover page-table doubling and LRU removal.
Reallocation retains existing page pointers and zeroes only the newly added
slots. Eviction skips pinned entries, removes the actual candidate from both
ends/neighbors as needed, clears only its LRU links and returns it to the
existing release/reassignment caller. Empty/all-pinned invalid internal states
remain outside the observed domain. Sequential resource copying alone reused
a buffer and reached only growth; that earlier input/master/prefix remains
separate from the expanded module-pressure oracle. No synthetic cache state or
allocator change was used.

Completion replay took 92.5 ms (18.1 ms setup, 22.5 ms execution, 51.8 ms checks);
the original took 1,856.6 ms. The raw-file GDB diagnostic is separate from those
measurements. The single parallel case-end regression, unit, original-build
and full graph/RET/EH batch passed in 4.27 s: 35 cases, 642 graph vertices,
628 scoped return contracts and seven distinct EH-owner checks. Raw references,
failed timestamp comparisons and earlier compiled input variants remain in
scratch. These are incremental measurements.

The large-export case builds one import library from 3,200 DEF exports with
sparse odd ordinals, internal aliases, data and NONAME entries, private records
and four identical repeated declarations. Two misplaced PRIVATE tokens produce
the original warning 4017; they are retained as ignored syntax, not claimed as
private exports. The encountered warning strings use the ordinary identity-bound
resource path. The original and reconstruction both exit 0 with 179 application
first entries, a 1,670,986-byte archive containing 3,181 members and a 359,344-byte
EXP object. Complete index, member, symbol, relocation, padding, stream and file
inventory comparisons match the separately retained untouched master.

Three whole bodies / 1,056 code bytes recover linear-hash bucket splitting,
identical-export comparison and mapped-file growth beyond the initial 512 KiB.
The split retains chain order while moving entries into the new bucket and
preserves the real key callback and allocation edges. Duplicate comparison keeps
physical DWORD transports and consumed flag bytes distinct. Mapping growth
unmaps the actual old view, doubles or raises its extent, then uses the existing
file-extension/remapping path. Directory exhaustion, duplicate forwarding-name
comparison and mapping failure branches remain outside this observed domain.

Raw artifacts differ only in the two EXP COFF/export-directory timestamp DWORDs,
the 3,181 archive Date digit fields and 3,179 contained COFF timestamp DWORDs.
The derived reference binds the untouched capture/report and all raw files;
every other byte is strict. Missing warning resources and earlier clock-only
failed comparisons remain in scratch, alongside the separate GDB raw-file
diagnostic. No additional original was run for the resource-only harness fix.

The final original took 1,197.3 ms; fresh native comparison took 502.6 ms
(23.2 ms setup, 451.6 ms execution, 27.8 ms checks). The single parallel case-end
regression, unit, original-build and full graph/RET/EH batch passed in 4.31 s:
36 cases, 645 graph vertices, 631 scoped return contracts and seven distinct
EH-owner checks. These are incremental measurements.

The short-import inspection case reads the unchanged original-produced new-format
archive and EXP, its five byte-exact extracted short members, and the genuine
original-compiled client object. One `/dump /all` invocation exercises archived
and standalone record decoding, code/data imports, named and ordinal encodings,
COFF symbols, relocations and archive export collection. Both executions exit 0
with 122 application first entries and exactly 27,640 stdout bytes. All target
input/reference fields equal the untouched master, with only the previously
omitted existing dumper-resource harness asset added. No output normalization
is used; input timestamps are already frozen. The original's warning that
`/nologo` is unrecognized by this dumper is preserved.

The 995-byte whole decoder reuses the qualified 20-byte scoped buffer, its real
Grow edge, inline normal scalar delete and exceptional tail to the one canonical
cleanup body. The export-name array uses its actual scalar new/delete calls,
retains all 12-byte records on growth and writes the defined flag padding.
An initial narrow graph check caught and corrected a provisional malloc/free
substitution before any new masked reference. Original `/O2 /Ob2 /GX` and scoped
native optimization after inlining preserve the actual lifetime and entry roles;
no helper vertex or additional destructor qualification was added. The eighth
bounded EH-owner check separately binds normal RET8, the actual stack owner,
action, FuncInfo/unwind data and authenticated frame-handler thunk. Allocation
failure and malformed record domains remain unvisited.

The original took 1,656.8 ms; fresh native comparison took 21.4 ms (14.3 ms setup,
3.7 ms execution, 3.4 ms checks). The single parallel case-end regression, unit,
original-build and full graph/RET/EH batch passed in 5.30 s: 37 cases, 646 graph
vertices, 631 scoped return contracts and eight distinct EH-owner checks.
The resource-setup failure, failed declaration builds, allocator graph failure
and unchanged raw references remain in scratch. These are incremental timings.

The archive-extraction case expands a single ordinary `upda*.lib` match and
extracts the unchanged long-name `replacement-long-object.obj` from the original
replacement archive. Both executions exit 0 with 83 application first entries
and exactly the archived 1,155 payload bytes, excluding the archive's alignment
padding. Streams, complete input/output inventory and object bytes match the
untouched master without normalization. Its earlier 82-entry master predates
registration of the newly discovered enumeration helper; that raw trace remains
historical rather than relabeled as the later complete catalog.

Three whole bodies / 1,111 code bytes recover wildcard argument expansion,
the separate Win32 enumeration helper and archive extraction. The helper keeps
one persistent 580-byte allocation whose consumed SDK ANSI find-data prefix is
320 bytes, with filename at offset 44. Only attributes and the name alias are
consumed; excess allocation is unobserved. It returns actual directory entries
to its own skip loop, retains the real search handle across argument parsing
and closes it on exhaustion. Shared FindFirst/Next/Close support uses independent
real directory handles and the existing DOS matcher; it adds no application
credit or captured directory replay. This input qualifies one ASCII regular-file
match, not multi-match ordering, short-name or general filesystem equivalence.

The extractor scans real archive headers and long names, uses the genuine
`_mbsicmp` comparison, derives its default output basename and copies through
the actual mapped destination. The original's success path frees its library
indexes, closes the files and calls `exit(0)`; it does not return to the librarian
caller. Its normal missing-member RET4 and buffered-copy branches remain
unvisited. Narrow import/extent checks corrected a provisional `_stricmp` name
before new execution. Finite ordinary stdcall/WinAPI nonvolatile contracts retain
the actual cached import sites, with zero opaque calls; no provider-body campaign
or behavioral claim follows from those ABI facts.

The final original took 1,265.7 ms; fresh native comparison took 18.5 ms
(12.2 ms setup, 3.2 ms execution, 3.0 ms checks). The single parallel case-end
regression, unit, original-build and full graph/RET/EH batch passed in 6.14 s:
38 cases, 649 graph vertices, 634 scoped return contracts and eight distinct
EH-owner checks. All raw references, original-produced archive/member identity,
pre-support frontiers and failed static checks remain in scratch. These are
incremental measurements.

The procedure-table inspection case dumps a genuine original-compiled COFF object
and its original-linked DLL. Three authored five-DWORD procedure records carry
actual function relocations in a 60-byte `.pdata` section. The x86 linker retains
that section but publishes an empty exception directory; its attribute warning
4078 is retained in producer evidence. This qualifies serialized metadata and
inspection, without claiming valid runtime unwinding or a RISC machine identity.
The image procedure-table reader remains a genuine pending entry because this
image does not reach it.

Two whole bodies / 514 original code bytes recover the object procedure-table
reader and its separate machine-format selector. The current x86 input reads
and prints all three ordinary records. Compact PowerPC/Windows CE encoding,
other machine interpretations, empty and malformed tables remain unvisited.
The full physical section-index DWORD retains its actual signed low-WORD use.
The original and native executions exit 0 with 113 first entries and exactly
21,986 stdout bytes. Every capture field equals the untouched master, with no
clock, pathname, payload or output normalization. The earlier master has one
fewer cataloged first entry because the format selector was newly discovered;
its historical raw trace is preserved.

The completed original took 1,766.8 ms; fresh native comparison took 20.7 ms
(12.9 ms setup, 4.7 ms execution, 3.1 ms checks). The single parallel case-end
regression, unit, original-build and full graph/RET/EH batch passed in 5.35 s:
39 cached cases, 651 graph vertices, 636 scoped return contracts and eight
EH-owner checks. Failed declaration builds and strict narrow extent/import/RET
checks remain in scratch, alongside raw master/progress artifacts and producer
source/objects. These are incremental measurements.

The native-driver serialization case links two original-compiled x86 objects
with `/driver:uponly /subsystem:native`, code/data pointers, COFF debug and a map.
The entry also consumes the linker's authentic `_ImageBase_` and section-number
symbols for `.text` and `.data`. Both executions exit 0 with 262 first entries,
a 4,096-byte SYS image and a 1,207-byte map. This establishes file serialization;
the generated driver is never loaded or executed and makes no kernel-runtime
claim.

Three whole bodies / 1,595 original code bytes recover synthetic-symbol
collection, native section preparation and symbol-value serialization. The
collector's four physical arguments are contribution/list outputs, correcting
provisional count/opaque names without changing thiscall or RET16. Section
references use the actual two separate eight-byte allocations and preserve
lookup/fallback order. The writer emits real section indices and image base,
updates symbol values, releases both list forms and appends the actual base
relocation. INIT/PAGE/discardable section logic is retained; the input does not
claim all dormant section-name, debug-fixup or other-machine domains. Their
pending callees remain genuine traps. The narrow import check corrected a
provisional `_strnicmp` to the actual case-sensitive `strncmp` before execution.

Only the COFF timestamp DWORD, two debug-directory timestamp copies and map
hex/ctime fields differ. The raw PE checksum changes deterministically with
those clocks. Original, master and native complete raw-image checksums were
independently verified. A native output checker validates the untouched raw
checksum and exact fields, preserves an ephemeral raw copy, then transforms only
the post-exit comparison view: three clock DWORDs become zero and the checksum
is recomputed. The normalized complete image SHA matches; the checksum is never
masked. A focused corrupted-payload check rejects before transformation. Generic
full file/inventory assertions remain active. Durable qualifying raw native
bytes were saved separately through the existing GDB exit-group copy route;
the callback's supervisor-root copy is explicitly not durable evidence.

The final original took 1,655.1 ms. The fresh GDB qualification took 196.0 ms
including debugger execution. Ordinary native replay took 18.7 ms (11.0 ms
setup, 2.5 ms execution, 5.2 ms checks). The single parallel regression/unit/original-build and
full graph/RET/EH batch passed in 5.54 s: 40 cached cases, 654 graph vertices,
639 scoped return contracts and eight EH-owner checks. Raw masters, both authored
input versions, failed hash/static/build results and finite clock/checksum
receipts remain in scratch. These are incremental measurements.

The polymorphic C++ case links two genuine original-compiled translation units,
a real virtual table and the shipped MSVCRT import library. The original compiler
emits a class-105 vector-deleting destructor weak external whose actual fallback
is the scalar-deleting destructor, with library search mode 2. No COFF symbol,
alias, dependency reply or provider body is fabricated. The fixture links and
inspects the resulting files; it does not execute the input DLL or its destructor.
Both sides exit 0 with 317 first entries, a 4,096-byte DLL and a 1,841-byte map.
Only the exact COFF/debug timestamp DWORDs and map hex/ctime text are normalized.
All import records, virtual-table pointers, code/data, symbols, padding and full
inventory otherwise match the untouched master. Its earlier 316-entry raw trace
predates registration of the newly discovered insertion helper and remains
historical evidence rather than relabeled.

Three whole bodies / 949 code bytes recover the eight-argument weak-external
collector, default insertion/update and masked resolution. The collector reads
the real auxiliary index/search prefix, temporarily seeks to its referenced COFF
symbol and restores the original cursor. The twelve-byte default record shares
its already-known cleanup owner. This input qualifies ordinary search-2 weak
resolution; alias/no-library alternatives, malformed records, copied-input bases,
Mac attributes and incremental branches remain unvisited. The field at symbol
+0x36 is preserved as a copy-only WORD with no inferred Mac meaning. The separate
weak-default lookup body remains a named pending trap because it was not reached.
A narrow graph check corrected the malformed-record diagnostic to its actual
fatal callee before execution.

The first post-insertion native pair exposed an older counter identity error.
Original 47f080 is the weak-external count: the collector, two retirement writers,
PassOne's resolver gates and FullLink's incremental condition use one source
object. The general unresolved count at 47ef48 remains separate. Earlier zero-only
observations did not distinguish them. Correcting only these actual readers and
writers makes the unchanged reference reach the real resolver; it adds no credit,
reset, synthetic state or historical replay. The failed pair and exact original
operand witnesses remain in scratch, alongside the corrected source/symbol binding.

Final original completion took 1,846.3 ms; fresh native comparison took 20.5 ms
(12.3 ms setup, 4.4 ms execution, 3.8 ms checks). The separate GDB raw-file
comparison is not included in that timing. The single parallel case-end
regression, unit, original-build and full graph/RET/EH batch passed in 5.48 s:
41 cached cases, 657 graph vertices, 642 scoped return contracts and eight
EH-owner checks. These are incremental measurements.


`cases/0043-omf-conversion` completes genuine flat32 OMF-to-COFF conversion and
an ordinary executable link. The shipped Watcom assembler produced the unchanged
233-byte OMF input: separate code/data segments, public symbols, a data pointer
and real FIXUPP records. Its source, producer command and thirteen valid record
checksums are retained in scratch. LINK's embedded converter runs directly; no
external converter response is replayed. The final image is 4,096 bytes and its
map is 730 bytes. This qualifies serialization/linking, not execution of the
input code.

Thirty-seven whole bodies / 8,969 original code bytes recover conversion entry,
record dispatch and byte/WORD/DWORD/index readers, names, SEGDEF/group/public
records, actual hash nodes, growable section/symbol/string buffers, FIXUPP parsing,
second-pass relocation emission, COFF writing and cleanup. The two hash tables
retain distinct 24-byte named-symbol and 16-byte symbol/displacement nodes.
One-based logical segment IDs and zero-based output section arrays remain
separate. The auxiliary-index table at 478a78 is distinct from record length
478a74; an apparent negative-base overlap was rejected before implementation.
The shared identifier separately matches the original stack probe to shipped
`MSVCRT.LIB` `__chkstk`, with zero application credit.

The index getter returns the actual AX WORD consumed or ignored by the recovered
callers; its upper EAX bits are unspecified volatile carry, not a zero guarantee
or uninitialized-storage defect. Still-pending wider-looking callers must close
their actual consumption before use. Section creation instead preserves its
full physical DWORD result: stored characteristics in the upper half and section
ordinal in AX, narrowed only at the real consumers. Current input qualifies its
ordinary small indices, two segments, public names and direct target fixups.
Other record types, nonzero target-offset interning, line data, malformed input,
weak/comment alternatives and exceptional recovery remain unvisited or named
pending entries as applicable.

The genuine Win32 `__try` plus caller-frame `setjmp` emits the period compiler's
`_setjmp3`/SEH-unwind imports and actual scope table. Full rooted graph checks
retain the compiler filter/handler; a bounded residual separately proves normal
RET8 and filter RET0 rather than using their union as public ABI proof. Native
uses its real host `jmp_buf` in the same source frame and does not claim Windows
SEH exception behavior. Eight earlier C++ cleanup owners retain their distinct
normal/action/handler checks.

The original and native create independent real `lnk` temporary names and carry
them unchanged through open/conversion/delete. Earlier partial comparisons fail
honestly while those names remain live; final cleanup leaves only the expected
EXE/map inventory. No pathname is seeded or masked. Durable raw original,
native and untouched master artifacts match everywhere except the identified
PE COFF timestamp at [184,188), COFF/MISC debug-directory timestamps at
[1540,1544)/[1568,1572), and map timestamp/ctime text at [22,30)/[32,56).
Those fields derive from the actual image timestamp writer; no other output
bytes, payload, checksums or inventory rules are relaxed. Raw hashes, exact
field provenance and labelled comparison derivation remain in scratch.

Final fresh original completion took 984.2 ms with 289 first entries. The explicit
saved-reference native comparison passed in 15.6 ms (8.7 ms setup, 3.1 ms execution,
3.8 ms checks); the separate GDB raw-copy diagnostic is excluded. The single
parallel case-end regression/unit/original-build batch passed all 42 cached
cases. Its first full audit exposed missing ownership of rebuilt inline switch
tables; declaring the exact six compiler-generated `.text` data spans required
no source or oracle change. The corrected static gate passed in 4.65 s with
694 graph vertices, 679 scoped return contracts and nine distinct EH/SEH owner
checks. Regression/unit results were retained without repeating the campaign.


`cases/0044-omf-cross-module` completes an executable link from two genuine
Watcom-assembled debug OMF modules. Cross-module public references, a shared
communal declaration, uninitialized section storage, 4 KiB of repeated
initialized bytes and real line-number records exercise one coherent conversion
and link. The assembler expands nested DUP syntax into ordinary LEDATA; this
case does not claim LIDATA coverage. Both original and native exit 0 with 307
application first entries, an 8,192-byte EXE and a 1,150-byte map. The input code
is not executed.

Eight whole bodies / 1,518 original code bytes recover external-name records and
lookup, communal-size parsing, variable-length numeric reads, line records,
function auxiliary symbols and the two line-list append operations. The numeric
reader defines its complete DWORD result; index reads retain their established
AX-only role. Function auxiliary emission preserves all six physical arguments,
including the unused third slot, and writes the real packed 18-byte COFF records.
Line nodes retain one 12-byte owner; only their defined offset, line and next
fields are written, and serialization excludes the uninitialized padding word.
Malformed records, other numeric encodings and dormant record handlers remain
unvisited or genuine named pending entries as applicable.

Original and native temporary names remain independent until actual conversion
cleanup removes them. Raw original, native and untouched master outputs match
except the PE timestamp DWORDs at [184,188), [1540,1544), [1568,1572), and map
hex/ctime text at [25,33)/[35,59). The same qualified timestamp writers explain
those fields; no pathname, symbol, line, relocation, data or inventory mask is
used. Diagnostic raw artifacts and the explicitly derived comparison reference
remain in scratch.

The final original took 1,734.7 ms. Fresh native comparison passed in 16.5 ms
(9.6 ms setup, 3.6 ms execution, 3.2 ms checks); the separate GDB raw-copy
diagnostic is excluded. One parallel case-end regression/unit/original-build
and full graph/RET/EH batch passed in 5.27 s: 43 cached cases, 702 graph vertices,
687 scoped return contracts and nine distinct EH/SEH owner checks. Failed
partial temporary-file and clock-only comparisons remain preserved. These are
incremental measurements.
