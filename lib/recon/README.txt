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

Native EXE/DLL hosts can register a verified module with recon_register_module
and a ReconModuleSpec containing its name, captured loaded-image DOS path, image SHA256 and
RECON_MODULE_NO_TLS flag. The case adapter must verify that provenance before
launch; registration only retains metadata and never opens a PE image.
RECON_MODULE_PRIMARY selects GetModuleHandleA(NULL)'s EXE identity.
The loaded path can be an S: transport path and is independent of argv0 and
the repository source path; the case retains their provenance separately.
DLL hosts keep their real reconstructed attach/export/detach calls; registration does not
run initialization or load a DLL. Registered modules are process-lifetime tokens.
DisableThreadLibraryCalls validates the token, records disabled notifications
and preserves errno/LastError on success. Native hosts can inspect retained
metadata and state with recon_module_identity/thread_calls_disabled. Unknown
handles return failure with ERROR_MOD_NOT_FOUND, as the explicit original probe
does. Unregistered VC1 hosts retain their existing kernel32-only module behavior.
Run make -j4 module-check for native identity/lifecycle checks and the frozen
original DisableThreadLibraryCalls success, repeat and invalid-handle comparison.

The canonical VC5 _initterm boundary walks an exclusive initializer range in
address order, skips NULL slots and reads each callback when reached. Imported
_adjust_fdiv is a mutable int32 seeded to the verified canonical provider's zero;
the target CRT retains its own separate copy and initialization logic.
RECON_NATIVE_ENTRY_CONTROLFP_53 opts a verified EXE host into the original CRT's
recon_controlfp(0x10000,0x30000) before application entry. Record this in build and
initial-state metadata; it is not applied to DLL hosts or other tools implicitly.
The existing RECON_DEFAULT_FMODE setting can seed a verified initial CRT mode.
Run make -j4 initterm-check for callback order/mutation and entry precision checks.

Compatibility scope
-------------------

Streams preserve text
CRLF translation and Ctrl+Z input EOF; binary files remain binary. File mappings
extend writable files to their declared size and remain valid after mapping
handles close. Truncation fails while a mapping handle is open, preserving the
mapped file sizes seen in the captured outputs. Text reads are bounded by raw
bytes so reconstructed ftell/fseek keep their original buffer-position rules.
Redirected stderr is buffered: explicit flush or normal exit writes pending bytes;
an abrupt trap leaves them in the legacy FILE buffer.
Formatting delegates to host libc after stream adaptation, with the encountered
VC1 trailing-percent behavior preserved. `_snprintf` retains the VC1 exact-fit
nontermination and -1 truncation result. Its ordinary conversions use host
formatting; `%n` explicitly fails until its stop-on-overflow side effects are
implemented. `_setmode` changes only the legacy descriptor text bit and returns
the previous mode. `_stat` supplies the VC1 36-byte layout and UTC seconds,
including drive-index devices, legacy mode bits and untouched alignment bytes.
Creation time follows the Linux Wine capture environment's mtime fallback;
POSIX inode-change time is not a Windows creation time. Integer scanning wraps at 32 bits as
the old CRT does. qsort preserves the VC1 pivot/shortsort tie ordering because
equal-key ordering affects LINK's serialized import tables.
`mbtowc` writes 16-bit wide characters and preserves the original C-locale byte
mapping and zero-length/no-source behavior. Nondefault LC_CTYPE conversion
explicitly fails pending codepage-specific support.
The VC1 `_find` filename boundary retains its persistent result pointer and one
active search, including replacement/exhaustion errors and unchanged CRT errno.
ASCII wildcard matching is case-insensitive and preserves the captured Wine
name order; DOS question marks can match no character at a dot/end, and `.*`
also matches an extensionless name. This implements the encountered CRT filename
contract, not public FindFirstFile/FindNextFile APIs or generated 8.3 aliases.
Run make -j4 find-check for native-only comparison with the frozen original
probe, including reverse file creation order, case, paths and search replacement.

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

The broker transports the actual host cwd. RECON_CANONICAL_CWD affects target
DOS path APIs, not this transport field. Adapters must verify the exact host
working directory and separately verify that a captured DOS cwd maps to that
directory; comparing only a directory basename is insufficient.

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
The dedicated /root/recon_lib_maintenance agent maintains this shared layer.
Binary workers contact it directly for shim changes and compatibility advice;
reconstruction acceptance, boundary policy and cross-project decisions remain
with the orchestrator.


Canonical MSVCRT startup imports can use `recon_crt_bind_arguments(argc, argv,
 target_environment)` in the native host before target startup. Both vectors
must be mutable, NULL-terminated and live for the process lifetime. They are
borrowed unchanged; target environment is independent of Linux shim settings.
`__getmainargs` supports only the observed wildcard-zero path and leaves its
one-DWORD startup-info input unchanged. The fmode/commode/initenv accessors
return stable mutable state; imported app-type, newmode and matherr callback
state remain inspectable. `_fsopen` has its SDK ABI but deliberately fails closed
until its sharing behavior is exercised; `_tzset` uses the existing shim.

Encountered resources
---------------------
Native hosts register their target module, then call
`recon_load_module_resources(module, prepared_path)`. The shared Python
`resources.write_resources(path, original_target_sha256, entries)` prepares the
provider. Each entry has kind (`string` or `message`), numeric id, language 1033,
and ASCII text (str or bytes). Preparation must independently validate the
owning contextual source/cached resource and original evidence: hash, offsets,
length, language and encoding. This provider is not a substitute for that proof.

The RMR1 file contains magic, little-endian entry count, 64 ASCII owner-SHA bytes,
then records of four little-endian DWORDs (kind1/string or2/message, id, language,
text length) and exact logical text bytes without NUL. The host loader validates
module identity, complete structure and EOF; native execution never reads a PE.

LoadStringA handles the encountered ASCII subset and sufficient positive output
capacity. NULL module selects the registered primary image. FormatMessageA
supports observed flags0xA00, language0/0409 and no argument array, copying literal
insert tokens. Both preserve LastError, errno and untouched output tails.
Unknown entries/modes fail explicitly. USER32's LoadStringA can be resolved by
LoadLibraryA/GetProcAddress; unrelated Wine loader LastError side effects are not
synthesized. Resources remain local to their registered owner.

Full terminal status
--------------------
Fixtures needing a full Win32 DWORD status can use `status.TerminalStatus()`.
Pass its `environment()` overlay and `fd` with subprocess `pass_fds`; after wait,
`read(child_pid, posix_returncode)` validates owner, complete receipt and low8
consistency, returning the full DWORD. Missing/malformed receipts fail. Compare
that value to the frozen original independently of stdout and entry traces.

`RECON_STATUS_FD` names a regular read/write file initially containing24zero
bytes. The shim writes six little-endian DWORDs: magic0x31535452, version1,
ownerPID, ready1, code, reserved0. The initial process owns the slot; synchronous
fork/exec children cannot publish into it. Duplicate publication fails.
`exit`, ExitProcess and the optional native entry wrapper publish automatically.
Custom native host entry adapters which return normally must call
`recon_record_terminal_status((uint32_t)result)` once before returning, or mirror
the original CRT exit call. An unset descriptor leaves ordinary behavior intact.

VC1 jump buffers default to8DWORDs. Native VC5 profiles may select
`RECON_LEGACY_JMP_WORDS=16`, matching their SDK storage size. Host saved context
still lives out of line; this setting adds no `_setjmp3` or SEH semantics.
`_assert` currently has its original typed ABI and fails explicitly on execution;
its diagnostic/abort branch has not been characterized.

The encountered `system(command)` CRT path uses the existing SpawnBroker with
mode0, executableCOMSPEC, argv[COMSPEC,"/c",command] and inherited environment.
Only captured successful responses are supported. The fixture must verify the
whole invocation, cwd, relevant environment and complete inputs before replaying
streams/file effects. Missing broker, allow-native replies, NULL commands and
unreviewed failure/fallback outcomes fail explicitly. No host shell is launched.

`crt_operator_new` maps to the distinct weak `recon_cpp_new` native provider.
The original C-friendly identifier may use reviewed COFF SEARCH_ALIAS metadata
to bind the actual MSVCRT `??2@YAPAXI@Z` import without adding a function. Cases
must retain that decorated import/ABI and prove the provider's NULL new-handler
state; mutable malloc-newmode is separate and does not control this import.
The supported path adjusts zero to1, returnsNULL above0xffffffe0, rounds the
provider allocation to16bytes and preserves observed errno/LastError on normal
success. Requested size and initialized bytes remain the application's ownership
bounds, irrespective of allocator capacity. Host allocation failure is explicitly
unsupported until handler/failure state is characterized.

The native flag `RECON_NATIVE_SETJMP3_COUNT0` (with16word storage) selects the
observed MSVCRT `_setjmp3` intrinsic for ordinary source `setjmp`. It validates
64byte storage/count0 and saves a genuine context in the caller's frame. The
legacy object's address is an opaque owner token; original raw register/SEH
writes and untouched tail remain in case evidence, not equated to glibc bytes.
Cases must prove no application raw-field interpretation. Count/unwind variants
and longjmp from these contexts fail explicitly until characterized. Ordinary
VC1 setjmp/longjmp is unchanged. RC's unvisited IsValidCodePage/GetACP/CharNextA
imports have SDK call ABIs and explicitly unsupported bodies, not guessed locale
behavior.

FormatMessageA also supports the observed FROM_STRING0x400 path when source
content matches a registered encountered ANSI message: exactly one `%1` string
and one `%2!d!` signed32 insert, passed through the actual va_list-pointer ABI.
It formats the supplied arguments, preserving NUL, return count, untouched tail,
errno and LastError. Other templates/grammars/capacity modes fail explicitly.
The separate wsprintfA/HeapDestroy imports currently retain SDK ABIs with
unsupported bodies for unvisited source branches.

Unvisited CL multibyte parser imports `_mbsncmp`, `_mbsinc`, `_mbccpy` and
`_ismbcspace` retain SDK declarations/remaps and fail explicitly on execution.
The native mbstring.h/mbctype.h overlays add no guessed codepage behavior.
Standard access-violation/stack-overflow constants are available for readable
original SEH filters; this adds no native exception execution support.

The encountered RC private heap supports HeapCreate(1,0,0) and successful
HeapAlloc(owner,9,528). A native opaque handle owns typed `native_heaps` records;
each block records owner, flags, requested size, payload and actual mapped size.
GDB can inspect these without calls. The requested528bytes are initially zero;
allocator metadata/page padding are outside application byte ownership. Original
errno and LastError are preserved on the observed successful paths. Other
owners/options/sizes/failure/free/destroy modes fail explicitly pending evidence.
The unvisited SendMessageA branch has the SDK ABI and an unsupported body.

GetTickCount returns boot-clock milliseconds modulo32bits and preserves
errno/LastError. Fixtures normalize only identified clock fields, retaining
relations such as equal initial/start values. Classic i386 CONTEXT (204bytes)
and FLOATING_SAVE_AREA (112bytes) follow the original SDK field layout for
readable target code; declaring them adds no native Win32 exception emulation.

`_onexit` keeps its cdecl int-callback ABI, returns the registered callback and
runs genuine callbacks in reverse registration order. One host atexit entry per
callback preserves interleaving with ordinary atexit. Callback return values are
ignored. Normal return/exit runs callbacks before final shared stdio flushing;
ExitProcess bypasses both. NULL/failure, __dllonexit and registration/exit nested
inside callbacks are explicitly unsupported until characterized. The optional
terminal receipt records the exit request before dispatch; acceptance also
requires matching normal POSIX completion, so a callback trap cannot pass as a
completed exit. Callback-driven exit replacement remains unsupported.

LINK's encountered NTDLL module lookup resolves LdrProcessRelocationBlock using
its actual stdcall4/ret16 boundary. It handles HIGHLOW relocations with unaligned
DWORD modulo arithmetic and skips ABSOLUTE padding, returning the next record.
Other types/exports fail explicitly. GetFullPathNameA supports the encountered
ordinary relative filename with validated canonical cwd and sufficient output
capacity, returning the length excluding NUL and a basename pointer into that
output. Both preserve observed errno/LastError; native fixtures compare full
cached original buffers. No target fallback or generic DLL loading is forced.

Default-C ASCII multibyte calls now support `_mbsncmp`, `_mbsinc` and `_mbccpy`.
Scope requires codepage0, MB_CUR_MAX1 and C LC_CTYPE; target evidence also verifies
clear ASCII lead-byte bits. Comparisons return canonical-1/0/+1; count0 accepts
NULL inputs. Advance includes NUL, copy changes exactly one byte including NUL.
Unknown codepages/locales/non-ASCII fail explicitly. `_ismbcspace` and `_mbschr`
remain unsupported. The separate `recon_mb_codepage` state is not LC_CTYPE's
codepage and must be observed rather than inferred from MB_CUR_MAX alone.
