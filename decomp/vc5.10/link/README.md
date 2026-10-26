# VC5.10 LINK

401 application functions / 139,908 original code bytes are reconstructed
(33.73% of the 414,810-byte whole `.text` extent, which includes CRT and text
data). Thirteen shipped CRT/import boundaries add no application credit.
`binary.c` is the sole function, status and extent registry. The remaining
registry has 48 pending bodies with known extents totaling 7,143 bytes and
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

`make -j4 check unit` runs all eight cached native cases and their narrow checks;
fixtures never invoke Wine. The genuine DLL completion native replay took
58.7 ms (25.5 ms setup, 22.0 ms execution, 11.2 ms checks). The final parallel
regression/unit batch took 0.45 s; the successful full graph/RET/EH audit took
1.71 s. Necessary original/native builds also passed; these are incremental
closeout measurements, not clean-build timings.

`make -j4 win32 audit` verifies original-compiler compatibility and the full
414-vertex graph, imports, references, ownership and extents. Scoped return
checks cover 399 ordinary application bodies and six CRT bodies. Seven import
thunks remain in the graph without body-return claims. The two genuine C++
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
Optional Mac, VXD, PDB, callback, debug and librarian edit/extract domains retain
named traps or explicit native qualification guards. The diagnostic current_phase
owner supports the observed 36-byte prefix, with guards before copies and
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
measurements. Archive extraction and converted-member paths remain unqualified.

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
