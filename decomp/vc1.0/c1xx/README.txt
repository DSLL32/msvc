C1XX3232.EXE, Microsoft C++ frontend, MSVC 1.0 NT / project vc1.0

Target: toolchains/1.0-nt/bin/C1XX3232.EXE
SHA256: 333d015dd85773df2f0f9ab541ca65c40da9a2ad9bc8df9e03815f7deb20b2f7
Original image base 0x10000; entry point VA 0x804e0.
Starting case: original CL.EXE /Od /c cpp-basic.cpp. The independently verified
frontend input/output oracle is ../fixtures/cpp-basic/manifest.json.

PARTIAL reconstruction: 48 accepted paired frontiers, including 16 native
source functions covering driver/options setup, scope/packing state, permanent
allocation, checksum initialization and environment argument transport. The next
matching named trap is _new_vector@8 at original VA 0x4e810. The complete
frontend case is not yet reconstructed or claimed equivalent. The option table
is still a placeholder protected by its named trapping parser frontier.

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
