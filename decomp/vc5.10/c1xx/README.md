C1XX.DLL (Visual C++ 5.10)
=========================

`make -j4 check unit` runs the two numbered native cases and narrow forked tests
through shared recon-tool. `make -j4 win32 audit` builds the real DLL with 5.10
CL/LINK and checks the function graph, imports, individual ABIs and extents.
Native fixtures never invoke Wine.

The single registry is binary.c: 132 accepted application functions / 19125
code bytes. Historical startup support is separate (2 functions / 451 bytes),
as are four six-byte import endpoints (24 bytes, zero App credit). The 134
source-body audits exclude import RET-body credit. The whole .text extent is
751216 bytes, including runtime/data/padding; it is not a non-CRT denominator.

Application source lives in src/, retained startup variants and their rationale
in crt/, and native adapters/checks in harness/. The current minimal case uses
the small Win32 host in harness/original-host.c: real LoadLibrary, typed
_InvokeCompilerPass@12(argc26,argv,mode1), then FreeLibrary. Build that host with
`make -j4 original-host`; explicit references use shared original capture with
`--host build/host/c1xx-host.exe --progress --native-msvcrt`.

0000 checks completed DLL attach/export-presence/detach. 0001 compiles minimal.cpp
and currently reaches the genuine CompilerTranslationUnit::Initialize INT3 at RVA3aee6;
compilation is unfinished. Its stable cwd is T:\vc5.10\c1xx\0001-minimal-cpp.
Only the consumed MSC_CMD_FLAGS absence is declared. Native setup binds the
forwarded argument vector and that finite environment scope, without old private
HOST vectors or LastError seeding. The system-info profile is a harness asset.

The compilation wrapper preserves its full hot/cold SEH graph; native execution
currently exercises the normal try body only. The driver projects 756 known
semantic bytes of the original 1052-byte PCH settings transfer. The other 296
bytes have no placeholders or equivalence claim. Pending semantic consumers
must recover further fields before running. The frontend initializer likewise
models only encountered type/object fields; its later branches currently have
source/graph coverage only. A focused nonzero unit checks modeled reset/restore.

The 179-byte input factory preserves separate primary/secondary interfaces at
owner and owner+4, with original 8/20-byte allocations. symbol-aliases.json binds
PlainText::Reset to the single pending Lexer::Reset body. The shared alias helper
emits code-free COFF metadata and a native linker response; both production
vtables resolve to one physical function. The alias unit inspects a real object
without invoking pending methods. These aliases add no code credit. Only the
initializer/factory TUs use /Ob1 for helpers proved inline in the original.

The 2210-byte IR writer preserves its 1459 hot and 751 cold code bytes,
with 297 separate switch-data bytes across six exact intervals. Its full
operand-format, symbol, constant and string paths are reconstructed; the
current invocation stops at its first source-line helper before operand
serialization. The expression and symbol views contain only encountered fields.
New initialized defaults and BSS members join the focused nonzero restore unit.
The type-class fallback retains the original reused argument-slot value; no
address-byte output equivalence is claimed for that unexercised path.

The 370-byte source-line helper preserves its 18 hot and 352 cold bytes,
including inline buffer-room checks and signed-WORD delta encoding. Its source
line delta and last-emitted delta join reset/restore with exact zero defaults.
The 188-byte Grow member preserves signed half-capacity growth, page rounding,
genuine scalar new/delete, used-byte copying, zeroing of the complete new tail,
and relocation after publishing the replacement buffer. The new unit covers
8192-byte first allocation, 24576-byte regrowth, copied bytes, zero tail and
callback receiver/context/publication. The first actual delete receives NULL;
shared d6c43a2b5 supplies the reviewed declaration and NULL no-op, with existing
nonNULL ownership checks unchanged. The paired ELF consumed those exact frozen
bytes before publication, and no publication-only replay was needed.

The 132-byte Flush member preserves all suppression flags, the actual fwrite
boundary, a fresh post-call byte-count comparison and unconditional cursor
reset after a nonempty flush. No new fields were needed. The partial expression
file contains the exact six-byte 5b/80/00000484 record; its later zero marker is
still stdio-buffered at this trap, so no final file-size claim is made.

The 202-byte type-table initializer allocates four head arrays from arena zero,
with exact 8192/4096/4096/2048-byte requests. Own lookup masks establish
2048/1024/1024/512 pointer heads; the four entry payload types remain opaque.
Their original initialized NULL owners join snapshot restoration and the focused
nonzero unit. No entry layout or broader table contents are inferred here.

The 216-byte type interner preserves the low-WORD pointer hash, WORD auxiliary
comparison, duplicate identity, 20-byte pool nodes and conditional debug-cache
reset. CompilerType has 14 semantic bytes in a 16-byte natural layout; original
+a..b remain unmodeled padding. Typed transfer does not claim byte equivalence
for that padding. The first real builtin loop supplies zero-backed 16-byte
prototypes; broader unknown consumers stay behind their reconstruction frontiers.

The 21-byte symbol-arena initializer clears the saved arena frame pointer and
selects arena four. Own readers establish the arena stride and linked saved
contexts. These two BSS fields join the existing nonzero reset unit; the frame
pointee remains opaque until a semantic consumer is reached.

The 42-byte builtin initializer preserves the 62 semantic name/destination pairs
and stops at the first genuine returning name interner. The existing cc698 cell
is now correctly named bool_identifier, including its actual lookup argument;
it is not duplicated in the 61 new initialized name slots. Earlier zero/unvisited
observations retain their narrower meaning. All slots join the reset unit.
A common next/name/flags header now describes both generic 16-byte and keyword
20-byte allocations; only keyword records expose token/category fields.

The 380-byte returning interner shares only the common header, buckets and
current-name pointer with keyword interning. It preserves the hash, content
check, move-to-front identity, actual 16-byte records from own pool six, aligned
arena strings and folded low-WORD flag update. No token/category field or
unmodeled generic tail is accessed. All 62 builtin names now execute before
the genuine translation-unit initializer trap.

Original/native took 1.438 s / 11.420 ms (native setup 6.584, execution 3.009,
checks 1.826); build/audit/unit took 2.485 s. Actual selected-report order,
partial files and the genuine member frontier match. The typed-only _wcsupr
bytes consumed by this build are published ca070c16a; no publication-only replay.
Full logs, immutable sources/binaries and failed attempts remain under
~/.ai-tmp/msvc2/vc5.10/c1xx/minimal-resume-20261025/.

Historical sources/proofs remain in Git and owned scratch. Full old proof
revalidation depends on those external artifacts. Legacy observer/replay and
identification frameworks are retired; current cases use shared native tooling.
