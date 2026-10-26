# VC5.10 LINK

438 application functions / 152,336 original code bytes are reconstructed
(36.72% of the 414,810-byte whole `.text` extent, which includes CRT and text
data). Thirteen shipped CRT/import boundaries add no application credit.
`binary.c` is the sole function, status and extent registry. The remaining
registry has 45 pending bodies with known extents totaling 6,628 bytes and
167 entries whose body extents remain unknown; this is not a complete non-CRT
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

`make -j4 check unit` runs all fourteen cached native cases and their narrow checks;
fixtures never invoke Wine. The genuine DLL completion native replay took
58.7 ms (25.5 ms setup, 22.0 ms execution, 11.2 ms checks). The final parallel
regression/unit batch took 0.45 s; the successful full graph/RET/EH audit took
1.71 s. Necessary original/native builds also passed; these are incremental
closeout measurements, not clean-build timings.

`make -j4 win32 audit` verifies original-compiler compatibility and the full
451-vertex graph, imports, references, ownership and extents. Scoped return
checks cover 434 ordinary application bodies and six CRT bodies. Seven import
thunks remain in the graph without body-return claims. The four genuine C++
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
Unreached Mac, PDB, callback, debug and further librarian edit/extract domains retain
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
