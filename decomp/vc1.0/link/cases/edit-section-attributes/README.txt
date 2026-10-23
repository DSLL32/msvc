Public EDIT /SECTION workflow

Root-approved single substantial case, following9c656734. One newly classified
original application body at28180 contains1176 actual instruction bytes,24 bytes
of internal alignment and a40-byte ten-entry jump table. All code and table
ownership are separate in functions.json. The old Ghidra body inventory omitted
this function; no previous application/CRT classification credit is invented.

Input is the complete original-backed4MB large initialized-data DLL from the
preceding workflow. provenance.json binds that immutable original. Every phase
was run twice on the untouched original outside fixtures and frozen BEFORE
acceptance. Complete files and stdout/stderr match with ZERO normalization;
no timestamp, path, padding or output-order field is ignored in this family.

Ordered edit stages:

  edit-section-rename:
    /section:.data=.store,rw /section:.text=.code,er
    /section:.reloc,!d /section:.rdata,a4

  edit-section-attributes:
    /section:.store,rs!w /section:.code,ek!p /section:.rdata,a8
    /section:.reloc,d /section:.miss,r

  edit-section-release:
    /section:.store=.data,rw!s /section:.code=.text,erkp
    /section:.rdata,!a4 /release

Each stage uses -edit and relative edit.dll, with inherited environment plus
pinned LIB. Exact commands live in *-spec.json/active manifests. Each has a
separate -dump /headers inspection. Later edits and dumps consume actual native
prior-stage bytes through the established from_case mechanism, never substitute
frozen expected bytes. The final receipt proves all five actual native flows.
The DLL is inspected only, not loaded/executed by fixtures.

The second stage preserves the actual LNK4039 missing-section warning. The final
!a4 leaves the existing8-byte alignment encoded in Characteristics: the original
negated-alignment implementation ANDs its pending clear mask, rather than
inventing an alignment-removal mask. Ordinary cache/page flags have the original
inverse meaning; per-letter negation resets after each attribute. These behaviors
are modeled directly in readable C, not fixed to more modern expectations.

Every changed byte is proved to belong to a section Name/Characteristics field
or, in release, the four-byte optional-header checksum. Data, relocations, debug
records, sizes, physical padding and all timestamps remain exact. The raw stored
release checksum is independently verified over the full actual4MB artifact on
every fixture run. artifacts-section-edit.py is pure and masks nothing.
check-section-edit.py rejects data/padding/checksum changes and missing/wrong/
changed actual-native flow proofs. No original, Wine, debugger or helper process
runs in fixtures; original captures and accepting GDB session are separate.

Fresh accepting frontier: original28180/native edit_section, exact partial streams
and named stack/caller. Source recovers fixed9/8-byte old/new-name handling,
all characteristic/alignment cases, original masked negation quirks, all matching
headers and exact section file offsets. Canonical graph preserves strncpy,
strncmp, seek_file, write_section_header, warning_message and fatal_message.
Original/compiler inline memory operations use supported intrinsic pragmas.

Final checkpoint:929 frontiers,246011/281634 original .text bytes=87.3513%.
All58 cached native trace/output cases pass5.380s external/5.115s internal; direct
ELF execution sums.668s, setup1.815s and output oracles2.585s. The new six stages
sum43.4ms direct ELF time; measured per-case totals include4MB input-flow checks.
The final make -j4 native/native-trace verification was already up-to-date. Its
.019s is not claimed as a rebuild; the initial incremental compile has a log but
no stopwatch. Canonical make -j4 all audit7.082s,768 Rust graphs,755 callback
routines,486 data definitions,41 ECX checks and meaningful negatives pass.
See validation.json, native-validation.json, timing.json and guard-validation.json.

Ordinary commands from LINK root:

    make -j4 native-trace-check
    python3 scripts/check-section-edit.py

Next root-approved single workflow is conventional DEF to cross-machine i860/
MIPS import archives and independent inspections. This does not claim full
cross-machine image linking/execution. Remaining OMF/internal/unclassified code
is explicitly pending, not CRT. No further original run is needed for this case.
