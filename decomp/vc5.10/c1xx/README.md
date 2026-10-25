C1XX.DLL (Visual C++ 5.10)
=========================

`make -j4 check unit` runs the two numbered native cases and narrow forked tests
through shared recon-tool. `make -j4 win32 audit` builds the real DLL with 5.10
CL/LINK and checks the function graph, imports, individual ABIs and extents.
Native fixtures never invoke Wine.

The single registry is binary.c: 120 accepted application functions / 15211
code bytes. Historical startup support is separate (2 functions / 451 bytes),
as are three six-byte import endpoints (18 bytes, zero App credit). The 122
source-body audits exclude import RET-body credit. The whole .text extent is
751216 bytes, including runtime/data/padding; it is not a non-CRT denominator.

Application source lives in src/, retained startup variants and their rationale
in crt/, and native adapters/checks in harness/. The current minimal case uses
the small Win32 host in harness/original-host.c: real LoadLibrary, typed
_InvokeCompilerPass@12(argc26,argv,mode1), then FreeLibrary. Build that host with
`make -j4 original-host`; explicit references use shared original capture with
`--host build/host/c1xx-host.exe --progress --native-msvcrt`.

0000 checks completed DLL attach/export-presence/detach. 0001 compiles minimal.cpp
and currently reaches the genuine make_integer_constant INT3 at RVA3fc2;
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

The 249-byte OutputBuffer::Open now creates the expression output and selects
its 4096-byte stdio buffer. The actual original/native pair reaches
make_integer_constant(0x484): 1.316 s / 11.107 ms (native setup 5.611,
execution 3.908, checks 1.587), with the explicit original-report gate.
The build/audit/unit batch took 8.281 s. The former unnamed BYTE at original
+1c is now the encountered 260-byte filename array, ending before descriptor
+120. Constructors initialize its first byte; modeled BSS reset clears it, and
the affected nonzero unit checks the final byte resets correctly.
Full logs, immutable sources/binaries and failed attempts remain under
~/.ai-tmp/msvc2/vc5.10/c1xx/minimal-resume-20261025/.

Historical sources/proofs remain in Git and owned scratch. Full old proof
revalidation depends on those external artifacts. Legacy observer/replay and
identification frameworks are retired; current cases use shared native tooling.
