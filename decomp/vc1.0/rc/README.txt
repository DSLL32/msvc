RC.EXE reconstruction, Visual C++ 1.0 for Windows NT
===================================================

Target: toolchains/1.0-nt/bin/RC.EXE
SHA-256: eb2e4bb5994303a37326cc60107ee07be01ca267daca1f9116e3149d9ea31591
Image base: 0x01000000; entry: 0x01011950.

Initial case:
    RC.EXE /dCASE_FROM_COMMAND=7 /Iinclude /foresources.res resources.rc

The read-only case and exact original oracle are in
../fixtures/resources-rich. Expected success has empty stdout/stderr and an
exact 3,684-byte resources.res with 19 records. Original RCPP is intentionally
used as RC's child; a separate reconstruction owns that executable.

The initial rich resource case completes with exactly the captured output. The
binary remains partly reconstructed: pending functions have typed named INT3
stubs, and this case does not establish coverage of every input or branch. The
accepted order and original function/code-data extents are in functions.json;
validation.json records the complete durable replay and remaining named stubs.
The rich case, seven local follow-ups and eight shared follow-ups complete; this is not a claim
that all original functions have been identified or all branches exercised.

Build and differential checks
-----------------------------
    make -j2
    make -j2 check
    make replay

Builds use the original CL.EXE and LINK.EXE through Wine. wine-tool.py creates
its own prefix under ~/.ai-tmp/vc1.0-rc/wine and maps T: to the original
1.0-nt toolchain so header paths are stable. Each debugger run gets a separate
copy of the case under ~/.ai-tmp/vc1.0-rc. The local build/rcpp.exe symlink points
to original RCPP.EXE; neither original binary nor fixture is edited.

compare.py masks the entire backed code section in both executables and restores
only accepted functions plus explicitly recorded adjacent text data. It launches
both masked processes concurrently and rejects a different logical trap or an
interior trap. --accept NAME first verifies the matching current frontier,
records that whole function, and reruns. --advance-runtime advances exact CRT
and import matches one verified frontier at a time. make replay repeats every
accepted frontier from a fully masked entry in the case that first reached it,
then checks all completed cases. --case NAME selects one case; make check checks
every completed case.
Successful final runs require exact raw streams, status and captured product bytes,
and the exact complete case file set, including unchanged input files and no
unexpected products or leftover temporary files. The verbose-names case has
the explicit two-field native-PID stdout comparison described below.

Early PE section headers have VirtualSize=0. binary.py uses backed raw sizes
for the code extent and returns zeros for uninitialized sections; BSS
is not interpreted as executable-file bytes at raw offset zero.

Follow-up cases
---------------
The local cases/ directory retains exact original streams/products and input
hashes for seven additional cases:
  help             /?; all 424 help bytes go to stdout, exit 0.
  invalid-switch   /z; fatal diagnostic, exit 1.
  missing-input    Missing source; diagnostic, exit 1, no output/temp files.
  alternate        Expressions, explicit 0o octal, custom dialog CLASS,
                   adjacent VERSIONINFO strings, FONT/FONTDIR and both old
                   2.03 image-header forms; exact 940-byte resource product.
  lexical-error    Unknown resource keyword; RW2001, exit 1.
  parser-error     Invalid dialog caption; RW2002, exit 1.
  resource-error   Invalid legacy image format; RW2003, exit 1.

    python3 capture-original.py
    python3 compare.py --case alternate

capture-original.py reruns the untouched RC.EXE and verifies saved oracle files
and manifests. --record deliberately refreshes those local captures. It never
writes the shared rich fixture. cases/alternate/make-assets.py generates the
small input images and font metadata; no binary data came from an executable.
The font input exercises RC's observed 148-byte directory-header reader and
name offsets. It is a deliberately minimal parser fixture, not a renderable
font: RC copies its file bytes and extracts directory information without
validating glyphs. Uninterpreted font metrics stay in a contextual input-header
structure alongside the two file offsets RC actually uses.

The parent-owned read-only ../fixtures/resources-encoding suite adds:
  kanji                /x /l411 /Iinclude /foencoding.res encoding.rc
                       Native lead-byte ranges, Japanese byte strings in menus,
                       dialog and STRINGTABLE, and escaped wide RCDATA. Exact
                       480-byte output, exit 0, no diagnostics.
  wide-long            /x /l411 /fowide-long.res wide-long.rc
                       A 300-character wide string and Unicode escapes. Exact
                       700-byte output, exit 0, no warning.
  preprocessor-error   /x /fofailed.res preprocessor-error.rc
                       Missing include: exact child RC1015 on stderr (100 bytes),
                       RC RW1016 on stdout (69 bytes), exit 1, no products/temps.

The parent-owned ../fixtures/rc-targeted suite adds:
  rcinclude            /x /forcinclude.res rcinclude.rc
                       Mixed-case native RCINCLUDE, bare filename boundaries,
                       intentional leading/trailing whitespace, two parts and
                       nested ordinary includes/macros. Exact 344-byte output.
  scanf-pushback       /x /l411z /fopushback.res pushback.rc
                       The original accepts trailing z after hexadecimal 411.
                       Non-EOF pushback preserves the string-stream byte and
                       produces an exact 68-byte resource with language 0x411.
Both exit 0, have empty streams and remove all temporary files. Source whitespace
is pinned and copied byte for byte, including whitespace in rcinclude.rc.

Shared manifests, source/output bytes and original RCPP identity are pinned
and independently verified by capture-original.py. All source/ files are copied
into isolated case directories. No host codepage/locale changes, PID overrides
or stream normalization are used. --record refuses shared-suite mutation.
The runtime harness also requires the child symlink to resolve to original RCPP.

The native KANJI scanner preserves lead/trail pairs and its own escape/quote
rules. Unlike the ANSI scanner, its token length includes the terminating NUL.
Its bounds occupy signed WORD fields at offsets 0/4/8/12 in a 16-byte object;
source compile-time checks and the instruction/layout audit preserve that shape.

Remaining measured limitations include fonts' untested duplicate-ID branch,
verbose resource reports, other encoding/error branches and exceptional CRT
paths. The scanf engine is complete recovered C, with its native helper graph;
mbtowc remains a named trap because these cases do not reach it. ungetc is
now fully recovered C, retaining all original flag/buffer branches and its
separate _getbuf call; the new case exercises matching-byte string pushback. Unaccepted named vertices remain masked. The list in validation.json
counts known vertices only, not every unidentified original function.

Recovery and audit
------------------
The process driver is recovered C with original option mutations, INCLUDE
processing, temporary names, RCPP command construction, error paths and exception
handling. Custom CRT code retains RC's older command parser, Win32 process-heap
allocator, path helpers, locale conversion, shared file opening, 512-byte stream
buffers, formatted output and buffered write behavior. Ordinary literals remain
in their use context; no data section or machine-code implementation is copied.

RC predates the allocator in shipped LIBC.LIB and LIBCMT.LIB. Its allocator
calls GetProcessHeap/HeapAlloc/HeapFree, whereas both shipped libraries use
region/block allocation. The old routines are reconstructed explicitly rather
than substituted with a different allocator graph. identify-runtime.py searches
both libraries and checks relocation-aware code and direct call/jump targets.
Matching fragments do not authorize a changed private neighbor: differing
routines are reconstructed separately when encountered. Explicit source records
override candidate CRT aliases, preventing a small generic library body from
unmasking an unrelated function with the same candidate name.

audit-graph.py walks source-function control flow, switch-table targets and SEH
filters/handlers. It checks a bijection of direct call vertices and function
address references and compares RET stack cleanup and indirect-call presence.
It does not claim branch coverage or prove dataflow equivalence by itself;
paired execution and final byte oracles remain required. It also checks the
encountered FILE/stream aliases, C-locale classification table and decimal point,
six floating callback slots and signed KANJI field offsets. Callback targets
that are identified but unaccepted remain masked, including __fptrap.

Recovered resource logic includes the dispatcher and type interning, identifiers,
LANGUAGE, numeric expressions, keyword lookup, memory flags, nested menus,
dialogs and controls, string tables, accelerators, icon/cursor splitting, Windows
and OS/2 bitmap headers, raw/custom data, DLGINCLUDE, VERSIONINFO and the .res
record writer. The debugger snapshots payloads without changing execution: the
first menu matches all 190 captured bytes and the first dialog matches all 506
bytes, including its 12 controls. Deferred multilingual string tables and normal
cleanup produce the complete 3,684-byte product with 19 resource records.

RC also predates shipped LIBC's termination state, scanf engine and private
stream-flushing helper. Those variants are recovered C, preserving the shared callback walker,
exit wrappers and stream graph. The graph audit separately verifies all CRT
initializer/termination callback sequences, including the single __endstdio
pretermination callback. Exact matched routines keep their original library
implementation only where their complete bytes and named call targets agree.

Dialog reconstruction preserves the original class/control parser functions,
WORD-packed scratch layouts, style handling and private wide-number helpers.
The graph audit follows a known tail-call target when deriving stack cleanup,
so an original optimized tail jump can be compared with an idiomatic C wrapper
without introducing assembly or accepting an unknown return convention.

RC calls NTDLL's counted-string conversion routine directly. The shipped SDK has
no NTDLL import library. ntdll.def and import-library.py use original LIB.EXE to
generate one, retaining stdcall COFF link symbols and removing their @12 suffix
only from generated DLL lookup-name records. The script copies no target data
or code and adds no conversion-wrapper function to the original graph.

Source INT3 instructions are pending-function traps. The debugger helper is
analysis tooling, separate from the target reconstruction. Assembly listings
under ~/.ai-tmp/vc1.0-rc are analysis artifacts, not reconstruction source.

Real console interrupt
----------------------
The readonly ../fixtures/rc-interrupt case uses a fresh private two-process
console and a real group-zero CTRL_C_EVENT. Native successful registration,
actual OS-created callback thread and argument zero are observed with debugger
breakpoints. The primary remains behind a lexer-entry barrier while Wine
services pending SIGINT. There is no callback injection or PID substitution.
The whole 95-byte BOOL WINAPI interrupt_resource callback is recovered in
finish.c with its original distinct _fcloseall/remove/exit call graph and RET 4.
Exact status 1, empty streams and deletion of output plus both temporary files
are required. Its font-directory branch is recovered but not exercised here.

The analysis-only helper is compiled from the fixture and excluded from RC's
link. It uses ~/.ai-tmp/vc1.0-rc-interrupt/wine; other cases retain the existing
private prefix. The fixture also labels the Wine 10 CTRL_BREAK limitation,
where no registered callback runs. Failed initial delivery-race evidence is
retained under evidence/console-interrupt-race; the durable replay uses the
fixed lexer barrier and never accepts or normalizes those failed outcomes.

Current verified coverage is 46,113 / 53,248 original code-extent bytes (86.60%).
This is the union of accepted source/CRT extents, shared text data and import
thunks, clipped to original sections using VirtualSize or backed raw fallback;
it is not branch coverage. Validation replays all 252 accepted frontiers and
16 final cases (268 paired outcomes), with 191 source-function graph/ABI checks.

Measured remaining-code cases
-----------------------------
The immutable ../fixtures/rc-remaining suite adds two original-backed cases.
verbose-names uses /v with both named resource types and names; its exact
180-byte product reaches wide_to_ansi and the native NTDLL conversion import.
Every stdout byte is retained. Only six native PID digits at offsets161 and173
are symbolic; length349, prefixes RC/RD, all surrounding bytes, stderr, status,
product and complete case file set are exact. The debugger records the actual
CreateProcess PID and reads native temporary_input/output objects at successful
console-handler registration. Fresh pinned source directories prove no first-
candidate collision. The immutable capture identifies native _mktemp and the
original fprintf writer; no PID override or broad stream normalization occurs.

path-fallback stages RC alone, with no sibling/current-directory RCPP. A
controlled PATH starts with a nonexistent directory, then a quoted directory
with a space containing the pinned original RCPP. Its streams are empty and
its product is exactly68 bytes. spawn-search.c recovers the whole older
_spawnvp/_spawnvpe/_getpath group as distinct C functions with the original
ABIs and native error/UNC/quoted-PATH branches. The native search parser extent
is129 bytes; the public wrapper26 and search walker395, excluding alignment.

The audit now requires link-map proof of both name/address for reconstruction
CRT candidates, preventing RET/padding aliases from becoming fake callbacks.
Its instruction-end guard decodes beyond each accepted extent and rejects an
end inside an instruction, including CRT/import bodies on both images.

audit-uncovered.py inventories the executable-byte complement rather than
ranking aliases or one-byte pending stubs. At the original14-case checkpoint,
7,777 bytes remained:3,622 recognized padding plus4,155 function/region bytes.
The two new cases recover642 bytes. Remaining7,135 bytes comprise the same
3,622 padding bytes and3,513 conditional/unused CRT function/region bytes.
No bytes remain unclassified in this native extent inventory. It distinguishes
locale-only paths, exception callbacks, startup allocation failure, conditional
commit/text-resize paths and unused linked CRT siblings from public RC cases.
The detailed original gap map is frozen with the fixture; uncovered-code.json
is regenerated by make audit using the current accepted union.
