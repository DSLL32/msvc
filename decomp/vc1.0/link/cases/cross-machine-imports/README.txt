Cross-machine import archives (932 accepted frontiers)

The successful public workflow creates conventional i860 and MIPS import
archives from a DEF containing LIBRARY, CODE/DATA attributes and three named
DATA exports with explicit ordinals1/7/12. LINK32_DB=27 selects the original
buffered library backend. The i860 archive contains the genuine16-byte bundled
helper-thunk COFF member; the MIPS archive has no helper. Their target words are
serialized output data, never executed on Linux or claimed as full cross-machine
image linking. Actual symbols, ordinal table, descriptor/lookup/export relocation
kinds, helper bytes and every complete archive/export-object byte are checked.

Each creation, /lib /machine:<target> /list, and /dump /headers /symbols
/relocations invocation has two untouched-original captures. All subsequent
native inspections consume actual preceding native-created .lib/.exp bytes.
The two original time(&out) calls at build_import_library and build_archive_header
are captured independently, including results/out-pointer/errno/stack effects.
Case-scoped existing RECON_FIXED_TIME supplies that external input natively;
no output bytes are patched. This makes the created outputs RAW byte-identical
to clock-input captures and the later dump streams exact. The archive date's
last two bytes, '0 ', come from the original fixed template and are not clocks.
Only first10decimal date digits and structural4-byte COFF/export stamps may
normalize when comparing separate captures; suffixes/padding/order stay exact.

Three complete C functions add748 real instruction bytes, with no text padding
or inline-table credit: separate29-byte CODE/DATA token-drain handlers and the
690-byte i860 helper-member writer. The latter exposed a prior private type
abstraction: ImportMachine actually has fiveUSHORTs, pointers/sizes at12/16,
relocation pointer/DWORD count at20/24, helper pointer/size at28/32, total36.
Original descriptors are40bytes apart because of trailing alignment, not an
extra field. Compiler assertions and fresh original/native complete36-byte
memory comparison prove all scalars, implicit interior padding and named data
pointers. The three descriptors and16-byte helper are contextual typed globals;
no section image or executable byte payload is retained.

Function-export variant limitation

The initially valid conventional DEF exported functions Ping/Worker and DATA
Shared. Both original machines succeed and repeat exact non-clock artifacts,
but two auxiliary records leak low16bits of an original heap pointer. A hardware
watchpoint proves final writer _lseek@55bb1 push edi, called by flush_file_page
@3c94c while closing i860.exp. SavedEDI is that .exp FileSlot, whose pointer
survives in the uninitialized auxiliary bytes16..17 read at2cb51. The original
initializes only bytes0..14 and serializes all18. This differs from the previously
proven mapped-close residue, a defined scalar seek length. The new buffered
pointer leak is not seeded, normalized, or declared equivalent natively.

probes/function-export-auxiliary preserves the raw writer/reader trace, both
initial watch attempts (the first analysis helper lacked single-step dispatch),
and explicit proof. Original function-export inputs/full outputs/repeats remain
under inputs/cross.def, reference/import-archive-{i860,mips}, and their manifests;
they are NOT active passing fixtures. The successful all-DATA variant is separate
under inputs/all-data, reference/*-data, and active shared-prefix references.
Its fresh original segment repeats the same three first-entry functions reached
by the initial variant; replay-cases frontier_case preserves that actual initial
acceptance provenance rather than relabeling it. Positive archive completion
applies only to the all-DATA variant.

An outside-graph native host --wrap=write_import_members checks the original
actual defined/non-synthetic/function predicate when mapped_files==0 and rejects
that unsupported route before function-member emission. Status125 is a native
unsupported result, NOT original behavior. Both i860/MIPS negative runs prove the
guard, while all-DATA buffered and prior mapped scalar-residue fixtures pass.
The guard does not claim coverage of unvisited mapped-open failure fallbacks.
No original function graph/ABI or application coverage credit comes from it.

The initial /list probe omitted /machine and correctly rejected these archives
as conflicting with defaulti386. Those exact diagnostics remain under
probes/list-default-machine; explicit matching-machine list phases pass.

Verification

All64 cached native trace/output cases pass,741 app firstentries,5.390s external
and5.131s internal. Sums: setup1.842s, directELF.596s, output checks2.643s,
trace comparisons.00715s; trace preparation.171s. Final make-j4 was up-to-date
.017s, not a measured rebuild; earlier real compile/relink elapsed is unmeasured.
Canonical original-toolchain build/graph/data/callback/ABI checks pass6.327s:
771 CFG graphs,758 callback sets,490 contextual data definitions and41 ECX ABIs.
Graph mutation and reference/caller/CRT-state controls pass; check-cross-import
also rejects incorrect clock proof/suffix/state/layout/actual-input-flow evidence.
Fresh original/native GDB entries and full descriptor state live in acceptance/.
Compact immutable hashes, original capture/GDB/native/audit timing categories and
source inventories are recorded in validation.json, timing.json and final-run.json.
Raw large execution receipts remain at their recorded scratch paths.

    make -j4 native-trace-check
    python3 scripts/check-cross-import.py --native-receipt <receipt> --run-denials

Coverage is246759/281634 .text extent bytes =87.6169%; this is separate from CFG
body accounting and dynamic branch coverage. Current partial inventory has
181858accepted/190353known target-body bytes,27995/37088 CRT,258/348 import thunks,
1936verified contextual data and3052alignment. Another17197 unaccepted unclassified
bytes remain pending, not proven CRT or padding. Public LINK never selects the
embedded OMF converter's internal-g debug path; no forced reachability is claimed.
