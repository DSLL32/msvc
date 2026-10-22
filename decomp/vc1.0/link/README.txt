Visual C++ 1.0 for NT: LINK.EXE reconstruction

Target: toolchains/1.0-nt/bin/LINK.EXE, built with its original 1.0-nt compiler,
linker, headers and libraries through Wine. Program logic is readable C. All 70
formerly lifted routines were replaced; assembly includes/generators and bulk
section snapshots were removed. Only INT3 statements in pending stubs remain.
No assembly ABI exception was needed.

Completed cases:
  /? exits 3, stdout 884 bytes and stderr empty, exact against the untouched target.
  stdout SHA256:
  957c1782d7ebeed2733e7596f4502ffb6ffd5ffc01858add77f54c7fb4b8dff4
  minimal-C (-link @link.rsp) exits 0 with exact streams and a matching 2 MB test.exe.
  Only the four-byte PE COFF timestamp specified by its fixture is ignored.
  Normalized executable SHA256:
  92b9d5585fb0a921da34f207353ce52f4ae46d9bb59745ae994446350ebc9d16
  The generated program returns 0 with empty streams.
  hello-win32 (-link @link.rsp) exits 0 with exact streams and a matching 2 MB hello.exe.
  The same four-byte timestamp normalization applies. Normalized executable SHA256:
  17f34769a7c0089cf4f74f79175082d30709175839af2c8cae7b7cbf2a8b97ca
  Original, reconstructed and untouched-baseline programs pass the GUI checker:
  HelloWindow, Hello, world! painting (300 dark pixels), WM_CLOSE and exit 0.

There are 383 accepted frontiers: 90 help, 254 minimal-C, 3 hello-win32 and 36
from the active hello-dll case. Full 383-frontier replay and all three completed
cases pass. Static audits check 260 source/runtime call graphs, 251 source
callback-reference sets and 226 contextual data definitions. Pending helpers
trap and unexecuted branches need later coverage.

The active hello-dll case reaches import-descriptor emission in the -lib child.
Both archive indexes, name lists and headers have been emitted. The partial
2 MB import archives match after only their two archive member dates are ignored.
Its DLL, import library and .exp outputs have not yet been reconstructed to
completion. The immutable original DLL passes artifact and load/call/GUI checks
in this harness. Its manifest lists the sole allowed timestamp normalizations.

Shared fixtures under ../fixtures are immutable. cases/ records invocations,
normalization, generated-program checks and verification evidence.

Build and check help:
    make -j2 check
Check minimal-C, its output bytes and the generated program:
    python3 compare.py --case minimal-c --require-exit
Check hello-win32, its output bytes and GUI behavior:
    python3 compare.py --case hello-win32 --require-exit
Replay every recorded frontier and finish with the hello-win32 artifact check:
    python3 compare.py --replay --case hello-win32 --require-exit

The harness masks the original's entire code section with INT3 and restores only
accepted function extents and encountered inline data. The reconstruction's entry
and unaccepted routines are masked too. Each pair runs concurrently and must trap
at the same logical function before another function is accepted. Each tool copy
has its original LINK.EXE basename and passes its exact path as argv[0], so the
library-manager subprocess runs that same masked image. The debugger follows the
whole child tree, records image bases and loader breaks per process, and stops
all debugged processes when one reaches a frontier.

Data is named and typed with its owning code. Ordinary literals remain at use;
shared mutable buffers, tables and observable sentinels retain their roles. The
native CRT comes from verified original LIBC.LIB members, with older differences
in runtime.c. SOURCE-NOTES.txt describes interfaces, private layouts and limits.
Scratch, disassembly evidence, logs and this reconstruction's private Wine prefix
live under ~/.ai-tmp/vc1.0-link. Original toolchain files remain untouched.
