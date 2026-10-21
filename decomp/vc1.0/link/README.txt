Visual C++ 1.0 for NT: LINK.EXE reconstruction

The target is toolchains/1.0-nt/bin/LINK.EXE, built with the original 1.0-nt
CL.EXE, LINK.EXE, headers, and libraries under Wine. All 70 formerly lifted
program routines have been replaced with readable C. The assembly includes,
lifting generator, generic naked stubs, and instruction-reassembly audit have
been removed. Only the required INT3 statements in pending C stubs remain.

The completed /? case exits 3, writes 884 bytes to stdout, and writes nothing
to stderr. Both streams and status match the unmodified original byte for byte.
The stdout SHA-256 is:
957c1782d7ebeed2733e7596f4502ffb6ffd5ffc01858add77f54c7fb4b8dff4

The ordered history contains 185 accepted frontiers. The active minimal-C link
case stops in both programs at attach_comdat_node, original address 000238e0.
It has not yet produced a validated executable. The hello-win32 link fixture
is queued after minimal-C. The fixtures are shared, immutable inputs under
../fixtures; cases/ records the queue and exact invocation.

Build and check help:
    make -j2 check
Replay every recorded frontier using its recorded case, then check help:
    make replay
Continue the current differential case without accepting another function:
    python3 compare.py --case minimal-c

The original binary's entire code section is masked with INT3. The harness
restores accepted function extents and runs both variants concurrently, asserting
that each trap names the same logical function. The reconstruction's entry point
and all unaccepted functions are masked as well. The clean parallel build and full
case-aware replay pass, with the minimal-C frontier unchanged.

The static audits compare direct calls, tail calls, indirect-call presence, and
relocated callback references. The current all-C build checks 95 source/runtime
call graphs, 86 source callback-reference sets, and 157 contextual data objects.
These checks and the frontier replay cover the reached paths; pending helpers
still trap, and unexecuted output-format branches need later case coverage.

Scratch, logs, and this reconstruction's private Wine prefix live under
~/.ai-tmp/vc1.0-link. Original toolchain files remain untouched. Useful evidence
from before cleanup is preserved outside the repository under pre-c-cleanup.
SOURCE-NOTES.txt describes data ownership, interfaces, and continuation rules.
