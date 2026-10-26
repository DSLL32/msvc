RC.EXE 5.10 -- finished application reconstruction

The sole registry is binary.c: two implemented application functions, 277/277
original code bytes. No new functions or cases were accepted by migration.
Historical handwritten CRT support remains separately classified (350 reached
bytes); the unreached 3-byte math helper is still an INT3 stub. Import extents
and all CRT classifications award no application reconstruction credit.

make -j4 native-bin       ordinary native RC with statically bundled RCDLL
make -j4 check unit       native-only replay of three existing public invocations
make -j4 original audit   original5.10 compiler plus common Rust graph/ABI audit

One O0/debug i386 .build/libbinary.so is shared by numbered case libraries.
Reconstructed src/ files are unchanged Win32 C; only their native compilation
aliases main to rc_main for host isolation. The original main symbol is _main.
Original builds retain /MD /O2 /Oy- for rc.c and /MD /Od for crt/startup.c.

The three captures retain help, rich resources, and missing%source.rc error
oracles. RCDLL is a strict in-process response replay, not native RCDLL coverage.
Each capture contains the actual dependency argv/environment, callback buffers
before/after (including bytes after inserted NULs), pointer-alias mutations,
input bytes and resulting files/status/streams. Rich input conservatively pins
all428 original SDK headers; this is not a claim each header was read. Its
Windows process environment was losslessly projected from ASCII UTF16 values,
not independently observed CRT-heap environment data. Stable historical S: cwd
labels map to private relative-input workspaces; they are not new captures.

harness/provider.c is native host glue, not reconstructed target code. It denies
unknown requests/inputs, checks callbacks and complete post-call pointer effects,
and observes the actual shared console-handler shim via typed linker wrapping.
Real target exit runs a completion assertion after the disable call. Its checked
completion/dependency files are harness assertions, distinct from original RC
user-visible streams and resource artifacts. Units retain two narrow negative
requests per existing invocation; no old acceptance-mutation campaigns survive.

Common graph checking retains CFG, RET, qualified import, reference, ownership
and extent checks. Registry/MAP endpoint identities keep dormant CRT references
named without auditing the INT3 stub as an accepted body. Explicit existing
cdecl/stdcall preservation contracts feed
common register-import must-analysis for the two cached-import owners. The
RC-specific _adjust_fdiv pointer/scalar load witness is separate from call
qualification. No local graph discovery, runtime identifier or Wine runner is
retained. Historical proof packages remain in Git history and the pre-migration
scratch snapshot, not in cases or another binary-local evidence directory.

Layout: src/ contains only the two application bodies and their interface.
crt/startup.c retains the previously verified executable-specific CRT startup,
initializer/argument hooks and x87 precision ordering; MSVCRT.LIB is an import
library and does not supply this exact startup body. crt/startup.h contains only
its target ABI declarations. This preserves existing support and adds no CRT
reconstruction. Native dependency replay and its narrow static imported-data
predicate live in harness/. scripts/ contains only the import-library generator
and its DEF input. Function bodies, identities, original RVAs and captures are
unchanged by this separation.

The toolchain coordinator's make -j4 native-bin-rc target builds
build/native/bin/RC.EXE under decomp/vc5.10. It is an ordinary i386 Linux ELF.
Root entry.c uses the shared native bootstrap for actual writable arguments,
environment, command line and Z: identities. It initializes the one MSVCRT510
file-mode state to zero, registers the real RCDLL exports and embedded encountered
message resources, attaches its actual DLL CRT, then invokes RC's retained CRT
startup. Normal target exit detaches the DLL. Fixture hosts/providers stay out
of this link; original Win32 sources and builds are unchanged.

Ordinary /help matches the cached 589-byte output even with an empty environment.
The existing rich invocation, with the SDK INCLUDE directory supplied normally,
executes the real RCDLL and produces the exact 3672-byte resource output. No
physical RCDLL or extracted message companion is needed. This does not expand
RCDLL's reconstructed domain: the extra missing-file probe traps on its still
unencountered message 1110. The three numbered RC fixtures continue to verify
their original strict dependency-response contracts independently.
