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

Run make -j4 check trace-check process-check adoption-check for shared checks.
No archive is built. A target's final native link compiles src/*.c directly
alongside reconstructed objects, without instrumentation, using -m32 and the
same native preprocessor/include settings. Add both include/ and native/ to the native include
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
encountered in the reconstructed tools. Layout assertions require i386 pointers and long. This is
not a replacement Windows SDK. Add declarations as targets encounter them.

Execution and paths
-------------------

Ordinary native regression tests consume captured reference streams/files.
Reference capture is a separate explicit operation; there is no automatic Wine
fallback, original-executable lookup, reference refresh or PE interpreter here.
The process wrappers accept native i386 ELF executables and reject PE files and
scripts. They do not search for Wine. Explicit captured-response tests may replace
an external tool at its spawn boundary, as described below.

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

Compatibility scope
-------------------

Streams preserve text
CRLF translation and Ctrl+Z input EOF; binary files remain binary. File mappings
extend writable files to their declared size and remain valid after mapping
handles close. Truncation fails while a mapping handle is open, preserving the
mapped file sizes seen in the captured outputs. Text reads are bounded by raw
bytes so reconstructed ftell/fseek keep their original buffer-position rules.
Formatting delegates to host libc after stream adaptation, with the encountered
VC1 trailing-percent behavior preserved. `_snprintf` retains the VC1 exact-fit
nontermination and -1 truncation result. Its ordinary conversions use host
formatting; `%n` explicitly fails until its stop-on-overflow side effects are
implemented. `_setmode` changes only the legacy descriptor text bit and returns
the previous mode. Integer scanning wraps at 32 bits as
the old CRT does. qsort preserves the VC1 pivot/shortsort tie ordering because
equal-key ordering affects LINK's serialized import tables.

Native builds do not implement Windows SEH: ordinary __try bodies execute, but
faults/traps fail the process rather than entering the reconstructed __except
handler. Full Windows console delivery, arbitrary process trees, cross-process
file-sharing rules, named mappings and asynchronous/overlapped I/O are outside this initial
scope. Specialized fixtures stay explicit; unsupported modes must not be counted
as successful native cases. Win32 ABI/function-graph validation remains on the
canonical build. Output-only native fixture validation is separate from the
application first-entry comparison supplied by the tracing adapter below.

Native application entry tracing
-------------------------------

src/trace.c supplies __cyg_profile_func_enter/exit for reconstructed objects
compiled with -finstrument-functions. Compile trace.c itself and compatibility
sources without that flag. The callbacks also explicitly suppress instrumentation.
Naked functions use RECON_TRACE_NAKED_ENTRY(name), empty in the Win32 build;
it preserves registers, flags and floating-point state around the entry callback.

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
single-thread fixtures and synchronous same-program native children. Missing/bad
configuration, missing or ambiguous symbols, and trace write failures fail
explicitly with status126 and diagnostics.

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
See LINK's cases/native-trace-validation.json for initial before/after measurements
and cases/native-all-validation.json for the complete eighteen-case native suite.
trace-validation.json here records the initial shared runtime checks;
process-validation.json covers the current runtime including synchronous children.

Synchronous children and captured process responses
--------------------------------------------------

Set RECON_SELF_SPAWN=1 when a program launches itself through its canonical
Windows executable path. Only a requested path resolving to recon_pgmptr is
routed to /proc/self/exe; its original argv and requested environment are passed
unchanged. Other actual child executables still require native i386 ELF. This
keeps canonical diagnostic/argument spelling without executing the original PE.
The supported spawn mode is _P_WAIT. Native children inherit normal descriptors.

For a global first-entry history across synchronous same-program children, create
one zero-filled regular file of 32 + the number of application filter names
bytes, keep its descriptor inheritable, and set RECON_TRACE_STATE_FD to it. Pass
the same trace/filter/seen configuration to every child. Each process resolves
its own ELF pointers, while the shared bitmap suppresses names encountered by
any process. A header checks version, filter size and name signature. Reusing a
state file never resets it; each test case gets a fresh file. No trace/state
configuration is consulted if RECON_TRACE_FD is unset. Records still contain
names only. This supports synchronous children, not a deterministic ordering
claim for concurrent programs or threads.

spawn.py supplies SpawnBroker(callback) for explicit process-boundary fixtures.
Pass broker.fileno() to the target as an inherited descriptor and add
broker.environment() to its environment (RECON_SPAWN_FD). Before an actual spawn,
the shim sends the requested mode, caller PID, executable spelling, cwd, argv and
explicit-or-inherited environment over that socket. Returning None allows real
native spawning. Returning an integer 0..255 supplies a captured process status;
the callback must first validate invocation and input bytes, then apply any
captured file changes. Raising rejects the request and the native target exits
125. There is no automatic mock, reference capture, Wine fallback or successful
return for an unknown dependency. Broker records omit the full host environment;
adapters should record only fixture-relevant environment values.

Use a context manager or close() after the target exits, then check() to report
broker errors. requests records each accepted invocation and whether it used a
native child or replay. This is a test boundary, not an implementation of the
external tool: input and response data must be backed by independent original
captures, never by the reconstruction currently under test. Captured replies may supply status, raw stdout/stderr bytes and callback-applied
file effects. Stream bytes pass through the logical inherited descriptors without
text conversion. Captured spawn failures supply status=-1 and errno. Each adapter
must validate the complete prepared input and requested invocation first.

The socket protocol is local little-endian RSP1: magic plus uint32 payload size,
then int32 mode, uint32 PID/argc/envc and length-prefixed executable, cwd, argument
and environment byte strings. Replies contain magic, int32 action (0 native,
1 status replay, 2 reject, 3 stream replay, 4 spawn-failure replay), and int32
result. Stream replies append bounded stream lengths and bytes; failure replies
also include the captured errno. It is opt-in; unset RECON_SPAWN_FD sends no
requests. The Python adapter and C shim enforce bounded message/list sizes.

Run make -j4 process-check for real self-exec with canonical argv/environment,
child artifact/PID proof, shared and local first-entry behavior, accepted prefix,
input-checked response replay, rejection, PE refusal and disabled tracing. Source
is compiled directly with the shims at final link. process-validation.json
records the shared checks and build/check elapsed times.

Adopted tool compatibility
--------------------------

RECON_DEFAULT_FMODE=0x8000 selects the BINMODE.OBJ default for C1/C2; explicit
fopen b/t modes override it. Legacy FILE layouts, x87 controls, 32-byte setjmp
buffers, counted NT strings and CP932 conversions are shared here. Decimal
helpers preserve the encountered x87/12-byte formats; unusual invalid literal
syntax and Windows best-fit character conversion are outside tested scope.

RECON_CANONICAL_CWD preserves a captured absolute DOS working-directory spelling
only after its drive mapping resolves to the actual host cwd. `_getcwd` uses the
same validated DOS path, preserves caller buffers on ERANGE, and allocates at
least the requested capacity for a NULL buffer (zero selects the required size).
RECON_MKTEMP_SUFFIX
can supply a captured six-character suffix; otherwise mktemp uses the original
PID-derived naming and collision sequence. RECON_NAMED_PIPE_ROOT maps simple
Windows pipe names to prepared native FIFOs.

Buffered flushes preserve the old CRT's observable FILE state: a successful
positive write clears write mode on update streams; every flush resets the
buffer pointer/count, including failed writes. Internal buffer-full output
resumes write mode before appending the next byte.

RECON_LOCAL_CLOCK=YYYY-MM-DDTHH:MM:SS.mmm/step_ms supplies an advancing local
calendar clock for CL timing fixtures, using reconstructed timezone/DST hooks.
RECON_FIXED_TIME and RECON_FIXED_MILLISECONDS provide fixed clock inputs.
RECON_SPAWN_REQUIRE_EXISTS=1 makes missing executable candidates fail ENOENT
before a captured response is requested, supporting original PATH search.

Native signal fixtures test observable handler/output behavior. They do not
claim equivalence of Windows console callback threads or SEH fault dispatch.
Shared changes belong to the orchestrator; binary workers request additions.
