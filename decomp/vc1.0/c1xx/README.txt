C1XX3232.EXE, Microsoft C++ frontend, MSVC 1.0 NT / project vc1.0

Target: toolchains/1.0-nt/bin/C1XX3232.EXE
SHA256: 333d015dd85773df2f0f9ab541ca65c40da9a2ad9bc8df9e03815f7deb20b2f7
Original image base 0x10000; entry point VA 0x804e0.
Starting case: original CL.EXE /Od /c cpp-basic.cpp. The independently verified
frontend input/output oracle is ../fixtures/cpp-basic/manifest.json.

PARTIAL reconstruction: 96 accepted paired frontiers, including 43 driver
functions and three older CRT routines. Keyword interning, C++ context-stack
setup, and intermediate file creation are reconstructed. The next matching
named trap is _open_expression_intermediate at original VA 0x6c4d0. The complete
frontend case is not yet reconstructed or claimed equivalent.

Build and check the current frontier using the original toolchain:

    make -j2
    make -j2 check

make replay reruns every accepted frontier in order. compare.py --runtime
advances only exact original CRT/import matches, stopping at the next native
frontier. functions.json records the accepted order and original ranges.
identify-runtime.py matches library bytes while checking relocated direct-call
and jump destinations, so a similarly shaped but unrelated CRT routine is not
silently substituted. Driver call targets are audited by audit-graph.py.

Both on-disk original and reconstructed .text copies are masked with INT3.
Only accepted whole-function extents and explicitly recorded shared text data
are restored. The original toolchain files are never modified. The pair runs
concurrently with identical MSC_CMD_FLAGS, isolated working directories, closed
stdin, captured output and a debugger that terminates on the next trap. At
matching frontiers, visible file sets and contents are compared. At successful
completion, streams, statuses and all four intermediate files must also match
the independent original fixture exactly. Scratch and a private 32-bit Wine
prefix live under ~/.ai-tmp/vc1.0-c1xx.

The only inline assembly currently used in native source is INT3 in named
unreconstructed stubs, as required by the reconstruction protocol. Native
implementations use readable C; original CRT code is linked from the original
library after exact matching. No executable data sections are copied into C.

The original CRT __fmode is 0x8000 at VA 0xaa3bc. BINMODE.OBJ supplies that
same binary-default setting to this reconstruction; compare.py checks the
original and rebuilt globals against the recorded value. The PCH configuration
snapshot is represented by the typed CompilerConfiguration structure, with an
asserted size of 764 bytes. Its two path buffers and known flags are named;
remaining fields retain offsets until their consumers are reconstructed.

The launcher preserves the captured argv[0] identically for both images,
independently of the selected application image. audit-data.py checks all 98
option rules, their target sharing and defaults, the typed configuration, 81
language keywords and four __declspec entries against the original binary.
The older CRT _sopen, setvbuf and _freebuf bodies have readable C replacements;
audit-runtime-variants.py and runtime-variants.json record their complete code
and call/data alias correspondence with the shared older C1 variants. Assembly
is used only as analysis input to these audits, never as generated implementation.
