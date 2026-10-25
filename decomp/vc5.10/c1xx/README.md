C1XX.DLL (Visual C++ 5.10)
=========================

`make -j4 check unit` runs the two numbered native cases and narrow forked tests
through shared recon-tool. `make -j4 win32 audit` builds the real DLL with 5.10
CL/LINK and checks the function graph, imports, individual ABIs and extents.
Native fixtures never invoke Wine.

The single registry is binary.c: 138 accepted application functions / 19974
code bytes. Historical startup support is separate (2 functions / 451 bytes),
as are four six-byte import endpoints (24 bytes, zero App credit). The 140
source-body audits exclude import RET-body credit. The whole .text extent is
751216 bytes, including runtime/data/padding; it is not a non-CRT denominator.

Application source lives in src/, retained startup variants and their rationale
in crt/, and native adapters/checks in harness/. The current minimal case uses
the small Win32 host in harness/original-host.c: real LoadLibrary, typed
_InvokeCompilerPass@12(argc26,argv,mode1), then FreeLibrary. Build that host with
`make -j4 original-host`; explicit references use shared original capture with
`--host build/host/c1xx-host.exe --progress --native-msvcrt`.

0000 checks completed DLL attach/export-presence/detach. 0001 compiles minimal.cpp
and currently reaches the genuine make_function_type INT3 at RVA3840e;
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

The 420-byte translation-unit initializer preserves its 375 hot and 45 cold
bytes. Its 32-byte root scope has the actual eleven-slot C++ virtual table;
original and rebuilt slots plus a native object check retain the pending ABIs.
The 572-byte translation-unit allocation exposes only encountered fields, with
+228 and unwritten scope slots left unmodeled. Short namespace symbols share
only their real common/named prefix; larger IR fields have a separate view.
The actual namespace symbol allocation default is 20 and joins reset/restore.
The first real allocator call precedes symbol initialization and payload use.

The 118-byte recycled-symbol allocator preserves its context bypass, size-class
head pop, owned-record clear and name/kind publication. The 26 record widths,
26 size classes and eleven NULL heads are semantic allocation metadata; cd674
is now its kind23 array element rather than duplicate storage. All entries join
the nonzero restore unit, with a separate owned-record reuse test. Record tails
remain semantically unmodeled. Values beyond the encountered table stop before
an unsupported consumer. The natural namespace call finds an empty free list.

The 31-byte Arena::Allocate member preserves the old cursor, advances it by
the actual request, commits only when it passes the current limit, and returns
the original pointer with thiscall RET4. No new state was introduced.

The 96-byte symbol constructor preserves the kind byte, common name/link
publication and distinct kind4/kind16/kind17 DWORD zero stores. The variant
views contain no intervening placeholders; later interpretations of those zeroed
cells remain unsupported. Current kind23 writes only its named prefix, then the
translation-unit initializer assigns the real namespace payload and scope.

The 26-byte symbol-group initializer calls the genuine six-argument hash-table
constructor and stores its result in the independently NULL-initialized cc5c8
owner. Own later readers establish a symbol-group cache rather than the earlier
provisional default-function-type name. That owner joins nonzero restoration.
Original raw stack bytes independently retain all six inputs:16,0,4,3,1,1.

The 158-byte generic hash-table constructor preserves both exact allocations:
24-byte header and bucket_count*4 storage from the selected arena. Its comparison
and hash callback selectors remain distinct, count narrows to WORD only for the
stored count, two mode bits preserve the rest of their byte, and entry_count
starts zero. The other mode bits, natural padding and entry pointees remain
unmodeled. No explicit bucket clearing is invented; current storage is owned
zero-backed arena memory. The real six arguments are16,0,4,3,1,1.

Original/native took 0.685 s / 10.677 ms (native setup 4.906, execution 4.112,
checks 1.659); build/audit/unit took 2.101 s. The 140-body strict audit and real
selected-report order/frontier match; compilation is still unfinished.
Full logs, immutable sources/binaries and failed attempts remain under
~/.ai-tmp/msvc2/vc5.10/c1xx/minimal-resume-20261025/.

Historical sources/proofs remain in Git and owned scratch. Full old proof
revalidation depends on those external artifacts. Legacy observer/replay and
identification frameworks are retired; current cases use shared native tooling.
