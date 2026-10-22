Native reconstruction test shims
================================

This library supplies the Linux test-build boundary for reconstructed Win32
command-line programs. The canonical build remains a Win32 executable compiled
with the original toolchain. RECON_NATIVE_TEST enables this header only on Linux;
without it, recon.h is empty.

Requirements: an i386-capable C compiler and 32-bit libc development files.
On this Debian host gcc-multilib and libc6-dev-i386 supply the missing support.
The shims themselves compile with GCC or Clang. LINK's existing Microsoft inline
assembly requires Clang's -fms-extensions -fasm-blocks in its native build.

Run make check here to build and execute the shared checks. No archive is built.
A target's final native compiler/link command supplies src/crt.c and src/win32.c
directly alongside its reconstructed object files, using -m32 and the same native
preprocessor/include settings. Add both include/ and native/ to the native include
path; add only include/ to the original Win32 build.

Header contract
---------------

Include <recon.h> at the end of each reconstruction source/header include list.
The header consumes host stdio/stdlib and other ordinary header declarations
before mapping names. It defines the 32-byte Microsoft-style ShimFILE structure,
matching fields the reconstructed CRT and application access, and remaps FILE,
stdio, Microsoft CRT names, and the supported Win32 calls.

Define RECON_SHIM_IMPL before including recon.h in a shim implementation. Types
and shim declarations remain available but original names are not remapped, so
implementation code can call host libc. Do not pass ShimFILE to a host FILE API.
Reconstructed target CRT functions retain their definitions through the name
mapping; shared fallback definitions for these functions are weak so the target's
reconstructed implementation wins. Ordinary host libc calls remain outside the
native application's reconstructed function graph.

The native Windows overlay contains the types, PE/COFF layouts and constants
encountered in LINK. Layout assertions require i386 pointers and long. This is
not a replacement Windows SDK. Add declarations as targets encounter them.

Execution and paths
-------------------

Ordinary native regression tests consume captured reference streams/files.
Reference capture is a separate explicit operation; there is no automatic Wine
fallback, original-executable lookup, reference refresh or PE interpreter here.
The process wrappers accept native i386 ELF executables and reject PE files and
scripts. They do not search for Wine. External CVPACK and other unreconstructed
PE dependencies therefore remain unsupported in native tests.

Backslashes are translated at filesystem boundaries. RECON_DRIVE_N=/some/path
maps N:\\... into that directory; the same convention works for other drives.
Existing path components are resolved case-insensitively; nonexistent output
names retain their requested spelling. Ambiguous host case variants fail.
Z defaults to the host root. Unknown drives and drive-relative paths such as
N:relative fail instead of guessing a drive CWD. RECON_PROGRAM_PATH supplies the
canonical diagnostic program path when needed. RECON_FIXED_TIME supplies epoch
seconds for a deterministic captured-reference case; otherwise time is real.

LoadLibrary/GetModuleHandle recognize the shim kernel32 module. GetProcAddress
resolves the supported mapping exports to their native implementations. Missing
exports (including LINK's IsTNT probe) return NULL with the Windows error code;
unsupported DLLs fail. No dynamic PE loading is involved.

Initial scope
-------------

The first implementation targets ordinary LINK fixtures. Streams preserve text
CRLF translation and Ctrl+Z input EOF; binary files remain binary. File mappings
extend writable files to their declared size and remain valid after mapping
handles close. Truncation fails while a mapping handle is open, preserving the
mapped file sizes seen in the captured outputs. Text reads are bounded by raw
bytes so reconstructed ftell/fseek keep their original buffer-position rules.
Formatting delegates to host libc after stream adaptation, with the encountered
VC1 trailing-percent behavior preserved. Integer scanning wraps at 32 bits as
the old CRT does. qsort preserves the VC1 pivot/shortsort tie ordering because
equal-key ordering affects LINK's serialized import tables.

Native builds do not implement Windows SEH: ordinary __try bodies execute, but
faults/traps fail the process rather than entering the reconstructed __except
handler. Full Windows console delivery, process trees, cross-process file-sharing
rules, named mappings and asynchronous/overlapped I/O are outside this initial
scope. Specialized fixtures stay explicit; unsupported modes must not be counted
as successful native cases. Win32 ABI/function-graph validation remains on the
canonical build. Output-only native fixture validation is separate from the
application first-entry comparison supplied by the tracing adapter below.

Native application entry tracing
-------------------------------

src/trace.c supplies __cyg_profile_func_enter/exit for reconstructed objects
compiled with -finstrument-functions. Compile trace.c itself and compatibility
sources without that flag. The callbacks also explicitly suppress instrumentation.
The canonical Win32 build and source function bodies need no tracing changes.

Set RECON_TRACE_FD to an inherited writable descriptor for a separate trace file.
Set RECON_TRACE_FILTER to a newline-separated list of the target's native
application function names, and RECON_TRACE_SEEN to a newline-separated list of
application functions accepted before this case's historical segment. An empty
seen file is valid. The target adapter builds these lists from its audited
classification; CRT overrides, imports and shim functions are outside the
application comparison on BOTH sides. Unmapped application functions must not
be silently excluded by the adapter.

Startup reads the running i386 executable's ELF symbol table once, resolves
selected global and local functions, accounts for PIE/ASLR, and builds a pointer
to name cache. Keep the executable unstripped; -g is already used by native
builds. Names, not numeric logical IDs, are written one per line. Repeated calls
and previously accepted prefix functions produce no records. The exit hook is
empty. The adapter compares the emitted names to the cached original sequence
projected onto the same application boundary, and checks the terminal status and
fixture outputs. No original execution, Wine or reference refresh is involved.

Records are written immediately with write(), separate from stdout/stderr and
Microsoft stdio. First-entry writes preserve errno and survive an ordinary crash
or INT3 without a later stream flush. This initial runtime supports the existing
single-thread ordinary fixtures. Missing/bad configuration, missing or ambiguous
symbols, and trace write failures fail explicitly with status126 and diagnostics.

When RECON_TRACE_FD is UNSET, both callbacks immediately return. Startup reads no
name files or ELF symbols and allocates no tracing state. Instrumented call
instructions still have overhead, even when callbacks do nothing.

Run make -j4 trace-check to exercise first-encounter names (including static
functions), prefix exclusion, repeated calls, fastcall arguments, PIE and non-PIE
addresses, errno preservation, disabled tracing and configuration/symbol errors.
The source callback test target is instrumented; the tracer final-link source is
compiled directly without instrumentation. No archive is built.

LINK provides make -C decomp/vc1.0/link -j4 native-trace-check from the repository
root. Its application-name cache is projected from the existing original-backed
acceptance metadata. The runner checks that cache and all native application
definitions before launch, then compares the emitted segments and captured
outputs. make -C decomp/vc1.0/link -j4 native-check keeps the output-only build.
See LINK's cases/native-trace-validation.json for before/after measurements;
trace-validation.json here records the shared runtime checks.
