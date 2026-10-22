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

The last completed coverage checkpoint has 400 frontiers: 90 help, 254 minimal-C,
3 hello-win32 and 53 hello-dll. Its clean parallel build, full frontier replay
and all four completed cases pass. Pending helpers and unexecuted branches need
later coverage.

Both new user-authorized coverage scenarios are active in the durable order in
cases/coverage-queue.json: shared buffered archive creation, debug DLL, then the
remaining library/dump/edit lifecycle. The initial reader checkpoint has 410
accepted frontiers. All ten additions replay; all four prior cases pass. Audits
check 285 call graphs, 276 callback sets and 242 contextual data definitions.
Original .text accepted extent is 112387/281634 bytes (39.9053%), up from
110787/281634 (39.3372%). This measures whole-function accepted extents, not
instruction or branch coverage. The current trap is the ordinary archive writer;
these new scenarios are not complete.

hello-dll (-link @link.rsp) exits 0 with exact streams and matching 2 MB hello.dll,
hello.lib and hello.exp. Normalization uses only the structurally identified
archive/COFF/export timestamp fields in the immutable fixture. The DLL exports
_HelloMain@16, ordinal 1, RVA 0x1090. Original, reconstructed and untouched-baseline
DLLs pass load, stdcall invocation, painting (300 dark pixels), WM_CLOSE,
unregistration, unload and exit 0. Normalized SHA256:
  hello.dll 322ba54da6475e88c38e583e208cfe9a29caff7c827cf4e009a8556d50a73102
  hello.lib a686c62d37c456499183fb0fc174ad393b2458f9df44ef57d6c8735f94ed6d57
  hello.exp b68237aab51557f79031447e6e234a29ad693f5f5e348a40701487dfde29ffd9
A separate paired longer-output-name variant also matches all three artifacts.
It checks the observable auxiliary-symbol stack residue model and DLL-name NUL
padding; cases/hello-dll-auxiliary.json records the original watchpoint evidence.

Shared fixtures under ../fixtures are immutable. cases/ records invocations,
normalization, generated-program checks and verification evidence.

Build and check help:
    make -j2 check
Check minimal-C, its output bytes and the generated program:
    python3 compare.py --case minimal-c --require-exit
Check hello-win32, its output bytes and GUI behavior:
    python3 compare.py --case hello-win32 --require-exit
Check the DLL, import library, export object and DLL-host GUI behavior:
    python3 compare.py --case hello-dll --require-exit
Replay every recorded frontier and finish with the DLL artifact check:
    python3 compare.py --replay --case hello-dll --require-exit

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
