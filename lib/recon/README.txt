Native reconstruction test shims
================================

This library supplies the Linux test-build boundary for reconstructed Win32
command-line programs. The canonical build remains a Win32 executable compiled
with the original toolchain. RECON_NATIVE_TEST enables this header only on Linux;
without it, only the shared ABI/trace/inline annotations remain.

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
The stable include/recon.h retains shared ABI/trace/inline annotations. Its
Linux native branch includes <recon-remaps.h> from native/, where shim types,
declarations and remaps live. Original builds search include/ only; native
builds search both include/ and native/. Thus native-only API edits invalidate
native dependencies, while shared annotation/header changes still invalidate
original builds. This uses ordinary include resolution, without dependency
scanner exceptions or caches; the split itself requires one initial rebuild.
The native remap header consumes host stdio/stdlib and other ordinary header declarations
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
mapping and zero-length/no-source behavior. Explicit510 additionally admits
count1 ASCII1..127 after the existing isolated LC_CTYPE1252/SBCS selection and
full table/handle checks. It returns1 with optional aligned, disjoint caller-owned
WCHAR16 output, writing only two bytes. NonASCII/other selection/count/profile
remain denied on that nondefault path. NULL-source/count0/NUL retain prior
behavior. Focused controls cover every ASCII byte, tails, state/errors/semantic
FP, prewrite denials and unchanged CP0 byte mapping. The saved-original C1
420-entry wide-literal continuation matches through scan_floating_token
(native18.787ms); original per-call API state remains unmeasured.
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
remain unsupported. Explicit510 may resume an existing count0 context with
values1,2 or102 through the native caller-frame longjmp. Same-thread, still-live caller
and no intervening active SEH/C++ cleanup are caller preconditions; the registry
tag establishes the64-byte/count0 save, not those lifetimes. Other values and
profiles retain their denials for these opaque contexts. No Windows private
context, SEH unwind or original x87 effects are modeled. Focused checks compare
native control/status/valid-stack values, excluding incidental FP pointers and
empty-register contents. Ordinary VC1 setjmp/longjmp is unchanged.
The additional value1 role is exercised by RCDLL's typed native fatal unit,
starting from real initialization/output storage and its own live context; no
error counter is seeded to select a different result and no original probe is
used for that unit-only extension. Value102 is separately exercised by the
unchanged threshold unit:101 ordinary input-error calls from initialized0 reach
the real limit diagnostic, fatal cleanup and still-live saved unit frame. No
counter is seeded; this remains native-unit evidence, not a Windows private
context or original per-call-state observation.

FormatMessageA also supports the observed FROM_STRING0x400 path when source
content matches a registered encountered ANSI message: exactly one `%1` string
and one `%2!d!` signed32 insert, passed through the actual va_list-pointer ABI.
It formats the supplied arguments, preserving NUL, return count, untouched tail,
errno and LastError. Other templates/grammars/capacity modes fail explicitly.
The separate wsprintfA import retains its SDK ABI and an unsupported body for
unvisited source branches.

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
Unknown codepages/locales/non-ASCII fail explicitly. `_ismbcspace` uses the
canonical mask8 for ASCII space and TAB through CR, zero otherwise. The separate `recon_mb_codepage` state is not LC_CTYPE's
codepage and must be observed rather than inferred from MB_CUR_MAX alone.

Minimal native C++ target units may include shared headers with C linkage; shared
implementations remain C. RECON_THISCALL is empty for VC5 (implicit member ABI)
and explicitly selects i386 thiscall natively. Compile these narrow units with
-nostdinc++ -fno-exceptions -fno-rtti, then link through the C compiler alongside
shared C sources; no host C++ runtime is needed for this surface.

GetSystemInfo/GlobalMemoryStatus use an explicit RSE1 captured environment, loaded
by host recon_load_system_profile(path, reference_sha256). Prepare through shared
system_profile.write_profile using the verified capture/environment receipt SHA
and exact 36/32-byte results. Calls must match the observed one system query then
one memory query; unbound, repeated or unsupported inventories fail. SDK layouts
and all observed writes/errors are preserved. No Linux RAM query is substituted.
Only page size/total physical RAM are currently consumed; other volatile fields
are a captured sample, not invariants. New field consumption requires fresh proof.

GetFileSize supports the encountered regular-file success with NULL high word.
FlushViewOfFile supports an exact registered mapping base and zero-byte request,
flushing the registry's complete mapped length. Both preserve observed errno and
LastError on success; unknown handles/ranges/failures remain unsupported. A real
2MB original/native mapping-write probe checks streams, bytes and persistence.

Opt-in RECON_CRT_PROFILE_MSVCRT_510 selects reviewed canonical dynamic-CRT
setvbuf/freebuf transitions. Buffered sizes round even; explicit owned/user
buffers carry0x408/0x500, unbuffered mode carries4. Invalid mode/size returns
before mutation. Freebuf clears0x408 and pointers only for active owned buffers.
Default VC1 behavior is unchanged. Successful observed mode transitions preserve
errno/LastError; flush/allocation failure remains unsupported. Bind this opt-in
to the canonical MSVCRT provider and build receipt, not tool version guesses.

LocalAlloc/LocalFree support positive-size fixed allocations and same-owner
successful free. LMEM_FIXED0 uses ordinary malloc with unspecified initial
contents; LMEM_ZEROINIT zeroes every requested byte as before. The existing
native_local_memory records retain base, requested size and flags. WCC/WPP's
reached AH48 gateway adapter requests LMEM_FIXED0/65536B; no original zeroing or
release-path observation is inferred from that allocation. Successful free
returns NULL; both native calls preserve errno/LastError. The earlier zero-init
reference remains distinct from the new fixed-allocation case qualification.
Movable/unknown flags, zero size, unowned/double free and allocation failure are
explicitly unsupported; physical allocator capacity is not target-owned data.

The same verified CP0/SBCS ASCII scope supports _mbsdec(start,current): return
current-1 (including a leading NUL sentinel), or NULL at/before start. Preserve
errors and reject null/non-ASCII/unknown-state inputs rather than guessing DBCS.

A verified host can register encountered module data through
ReconModuleMemorySpec {sha256,original_rva,size,original_image_base,original_va} and recon_register_module_memory
(module,proof,native_base,native_size). Registration checks module identity, exact
extent, checked image_base+rva==va, nonoverlap and main ELF writable PT_LOAD
membership. It lives outside
the target graph and is bound to original image-data/source evidence in the receipt (including .bss).
Coordinate consistency is checked in the library; exact original section/global
identity comes from the host hash-bound reference validator, not from SHA lookup.
FlushViewOfFile accepts only the exact registered base with zero size, preserving
observed errors without persisting private image memory. Other image pointers
remain unsupported. This reproduces LINK's actual successful global-record flush;
its earlier static invalid-address prediction was superseded by real evidence.

SystemTimeToFileTime supports valid Gregorian UTC dates (1601..9999), preserving
100ns epoch arithmetic and observed errors. SetFileTime supports the encountered
valid regular file, NULL creation/access and last-write-only timestamp in the
native 1970..2038 range; actual filesystem mtime is changed and atime preserved.
Unknown/invalid dates, other update modes and failures remain unsupported. Tests
compare original UTC/epoch/leap conversions and actual last-write file metadata.

Default-C ASCII _mbsrchr scans through NUL and returns the last matching byte,
including the terminating NUL or NULL when absent. _mbschr returns the first
matching byte under the same scope. Both preserve errors and use
the existing strict codepage/locale guard. _mbspbrk and _mbsnbicmp currently have
SDK-typed surfaces; _mbspbrk remains unsupported. _mbsnbicmp now supports
the verified CP0/SBCS/C-locale ASCII path, folding A..Z and returning normalized
-1/0/+1 within bytecount/NUL bounds (count0 returns0 before dereferencing).

_mbsnbcpy supports generic Microsoft and explicit510 default-C ASCII, requiring
the existing LC_CTYPE/codepage/MB-width guard and MB gate0; Watcom is denied.
Canonical e390 MSVCRT ordinal358/RVA6daa gate0 branches through e843 to strncpy
89da (including cold padding path fa15): copy at most count, stop reading at NUL,
pad the remainder with zero, and return the destination alias. The shared body
prevalidates only that bounded read span before writing; byte[count] is never
required. Positive counts require nonNULL, disjoint valid spans without address
wrap; nonASCII and unsupported state deny before writes. Count0 returns the
alias without accessing either buffer after the profile/locale guard. Errors,
FP state, source and destination tails are naturally preserved. The reached
CVPACK request copies exactly15 nonterminated ASCII bytes from precompiled.obj;
the following newline is outside its read span. Provider effects are established
statically; original per-call state was not measured and no API-only original
run is claimed. `make mb-nbcopy-check` covers these bounded native contracts.

__unDName has a typed private-MSVCRT declaration/remap only. LINK's authentic
six-argument caller/IAT establishes cdecl char*(char*,const char*,int,
void*(__cdecl*)(size_t),void(__cdecl*)(void*),unsigned short), including24-byte
caller cleanup. No matching shipped SDK declaration was found. Every input
denies125 without accessing buffers or invoking allocator/release callbacks;
no successful demangling, allocation, or faithful provider diagnostic is claimed.
`make undname-surface-check` checks that boundary without an original run.

_mbsnbcmp is a separate SDK-typed cdecl int(const unsigned char*,const unsigned
char*,size_t) boundary (5.10 MBSTRING.H150). LINK's unvisited diagnostic branch
uses that genuine import, rather than strncmp. Every input, including NULL and
count0, denies125; no comparison/codepage/result semantics are supported.
`make mb-nbcmp-surface-check` verifies typing, denial and absence of a host import.

DeleteFileA supports translated nonempty, non-wildcard ordinary regular-file
paths with no concurrent namespace mutation. Writable files are genuinely
unlinked after excluding matching live views and current-process descriptor
aliases by device/inode. Existing View records retain that identity even after
their mapping/file handles close; an unrelated inode's live view is allowed.
Missing leaf/parent returns FALSE with LastError2/3; read-only mode or host
access denial returns FALSE/5. Success preserves LastError; all supported calls
preserve CRT errno/DOS naturally across host checks. Symlinks/directories,
open/mapped deletion and other failures deny125; there is no share/delete-pending
emulation. The VC1 LINK baseline establishes two real closed/unmapped writable
2MiB files disappear before ExitProcess, with BOOL ignored and no later
GetLastError consumer. Its per-API return/errors were not captured; success
LastError preservation is not claimed as an observed original effect. SDK
WINBASE.H and documented DeleteFileA semantics establish the public signature
and ordinary return/error scope. `make delete-file-check` verifies real deletion,
errors/FP/FILE/lowio preservation, same-inode denials and other-inode mapping.

_wcsicmp has the SDK STRING.H234/WCHAR.H689 cdecl int(const WCHAR*,const WCHAR*)
declaration/remap, with native16-bit units. RCDLL's actual IAT109c/calla139 and
eight-byte caller cleanup establish the boundary; signed EAX is consumed.
The generic profile still denies125. Explicit510 supports nonNULL readable
ASCII comparison prefixes under the existing default-C/LC_CTYPE/codepage/MB1
guard: fold A..Z by32, stop at unequal units or NUL, and return the unsigned16
folded difference. The authenticated canonical3fcd..40a4 zero-locale branch has
no calls/stores/error/FP work. NonASCII units that would be read and unsupported
state deny; unread tails are not inspected. Native checks cover source/tables,
exact ordering/differences, error/full108-byte-FP preservation and guard-page
stopping. Original per-call state remains unmeasured; RCDLL reuses its genuine
0143 receipt, without an API-only original or host wchar32 conversion. The
generic `wcsicmp-surface-check` retains denial/type checks; the explicit510
success/guards use `make wcsicmp-ascii-check`.

The Watcom100A profile alone remaps bare open to a distinct cdecl variadic
typed boundary and supplies encountered O_BINARY=0x0200 (O_RDONLY=0). Every
unqualified mode denies125 without fetching any variadic permission word;
additional admitted tuples are documented below. Exact0200
supports an ASCII nonempty/non-wildcard existing regular readonly path via the
existing binary descriptor backend (mode8000, OPEN_EXISTING/shareREAD|WRITE).
Host-only preflight preserves incoming errno; real descriptor publication,
initial position0 and later binary read/close use that existing namespace.
Unknown paths, nonregular files and open failures deny125 rather than exposing
Microsoft/host failure errno as Watcom. WLIB's actual two-argument resource-table
call, reviewed finite provider closure and shipped FCNTL.H establish the domain;
its actual legacy provider and CLIB3R openwnt member are not byte-identical.
The case supplies the exact captured staged self-image through the shared recipe;
the reached caller only stores/tests the opaque descriptor before FindResources.
Wider flags, errors/descriptor numeric identity, FILE26 and arbitrary private CRT
state remain unclaimed. Existing `make watcom-surface-check` retains typed denial
guards; `make watcom-open-check` verifies actual binary descriptor operations.

The Watcom profile also maps bare lseek to a distinct long(int,long,int)
normal-path adapter. It requires an open shared regular-file descriptor and
origin0/1/2, restores errno after host-only identity checks, and uses genuine
recon_lseek position changes. Only nonnegative signed32 results return;
unknown/closed/device/origin/failure domains deny125, without exposing MS error
numbers as Watcom failures. The Microsoft _lseek mapping is unchanged. Actual
WLIB fd3/-24/SEEK_END and WLINK resource traversal qualify the reached normal
path; numeric descriptor identity, failure errno and FILE26 remain unclaimed.
The Watcom-only long tell(int) callback spelling delegates to that same
lseek(fd,0,SEEK_CUR) operation and leaves the descriptor position unchanged.
The existing Watcom open checks cover the coupled seek/tell/read/close namespace.

Watcom-only setmode(int,int) accepts the reached O_BINARY0200 request on an
open shared descriptor, changes the real lowio text bit through recon_setmode,
and translates the previous Microsoft binary8000/text4000 result to Watcom
binary0200/text0100. Input text mode and unknown/closed descriptors deny125;
Microsoft _setmode and native FILE representation remain unchanged. WLINK's
normal startup requests binary on public fileno(stdin/stdout), ignoring returns.

Microsoft generic/510 _mbsnbcmp supports guarded default-C/CP0/MB1/gate0 ASCII
comparison, stopping at the first mismatch, NUL or byte count. It returns the
provider default branch's normalized unsigned-byte -1/0/1; count0 accesses no
buffer after the state guard. Only compared units are validated; unread tails
are not claimed. Watcom/unknown state, NULL positive-count and nonASCII compared
units remain unsupported. LINK's actual count9 import-name equality test is the
reached consumer; original live per-call locale/errors/FP were not measured.

Microsoft _ltoa(long,char*,int) supports the reached signed32 decimal domain
by reusing recon_itoa; successful output is returned by exact alias with its
NUL and preserved errors/FP. Other radices/NULL and Watcom remain unsupported.
C1XX's caller supplies owned text-workspace storage; the cold octal path stays
denied. The default VC1 _gcvt(double,int,char*) admits default-C digits16 and
exact signed32 integral binary64 values with caller-owned13-byte output. It
decodes the argument using integer operations and reuses the decimal backend,
retaining the old provider's decimal point (7.0 becomes "7.") and alias return.
Negative zero, fractions, out-of-range/nonfinite values, other precision/state
and explicit510/Watcom profiles deny125. Native Clang disables x87 for this
function to avoid an otherwise unused floating load of its double formal.
Focused full108-byte FP checks isolate callee effects from argument setup;
native errors are preserved, original per-call state remains unmeasured.

The private MSPDB NameMap::open(PDB*,int,NameMap**) static cdecl import has a
native declaration-only asmname binding to recon_mspdb_NameMap_open. Its opaque
struct types preserve the caller signature without a forwarding wrapper or
new App vertex; canonical Win32 keeps the original decorated import. The native
boundary always denies125 without reading database/output storage or changing
it. C1's current branch is unvisited; no database/name-map success is modeled.

MSPDB TruncStFromSz has an explicit510-only void cdecl(char*,const char*,size_t)
spelling backed by unconditional125 without argument access. The original LINK
IAT/caller and Microsoft public pdb.h bind typing; no local shipped SDK
declaration or successful truncation behavior is claimed. Current short-symbol
caller paths do not invoke it.

The Watcom profile exposes unsigned32 _amblksiz storage initialized to the
actual WCC DWORD16; main's assignment remains real source-visible state. This
does not tune host malloc or claim original CRT heap metadata. Native-only
__far is empty for the separately reviewed flat-DS semantic pointer projection;
original Watcom retains its keyword. No general segmented storage or selector
conversion is supported by this annotation.

Watcom-only bare memicmp/getcmd spellings have typed cdecl link boundaries:
int(const void *,const void *,size_t) and char *(char *), matching shipped
STRING.H83/PROCESS.H63. memicmp now implements the reached old unsigned-byte
comparison: fold only ASCII A..Z, return the full folded-byte difference at the
first mismatch or0 at count exhaustion. Count0 reads neither pointer, including
NULL. Nonzero count requires valid caller-owned nonwrapping spans; no locale
semantics or string-NUL stop is introduced. Other profiles and invalid spans
remain125. Native errors/full FP are retained; original per-call state unmeasured.
The native host may bind the exact original CreateProcessA input with
recon_bind_command_line(text,bytes), including the final NUL (ASCII, at most32767
bytes). The mutable buffer is borrowed for process lifetime, never reconstructed
from projected argv. GetCommandLineA returns that exact alias, denying unbound
use. Watcom getcmd supports the observed old whitespace-delimited ASCII parser:
skip the first token and following whitespace, then copy the exact remaining
text including its NUL to valid disjoint caller storage and return that alias.
Quotes are not interpreted or stripped, matching the old parser; malformed
bindings, output overlap and other CRT profiles deny125. No general command-line parsing or host
import fallback is supplied. Existing fixtures without raw input stay unbound.

GetTempPathA, GetTempFileNameA and SetEnvironmentVariableA have exact SDK WINAPI
declarations/remaps for LINK's complete unvisited DEF-processing branches.
Invocation always denies125 without reading input or touching output; no temp
file/path or process-environment mutation behavior is supplied.

The Watcom-only __STOSD spelling maps to a shared native DWORD fill returning
the end pointer. Valid unaligned destinations use ordinary memcpy per DWORD;
count0 leaves the destination untouched, and wrapping spans/counts deny125.
The binary's original header retains its genuine register pragma, independently
audited by the period build; native GCC does not emulate that register encoding.

Watcom _splitpath2 supports the reached ASCII relative leafname domain (<260
bytes, no drive or directory separators). It splits at the last dot and packs
requested empty-drive, empty-directory, name and extension strings into valid
caller workspace; NULL selectors neither copy nor advance the packed cursor.
Output pointers identify those exact NUL-terminated strings. Aliased input,
workspace or output locations, wrapping spans and other path domains deny125
before writes; there is no allocation, host path lookup or UNC interpretation.

Watcom compile spellings stdaux/stdprn identify existing native iob slots3/4;
this does not initialize their handles or qualify closing them. Bare isatty
uses the existing descriptor boolean helper. _IOLBF retains Watcom0200, with
setvbuf routed separately from the Microsoft40 implementation. Its reached
success domain is fresh nonterminal text stdout with a caller-owned128B or256B
buffer and mode0200. A disjoint256B user buffer may replace an established256B
buffer only when no bytes are pending; the old buffer is neither flushed nor
freed, preserving the observed permanent-allocation ownership. The exact supplied base/ptr is
retained as user-owned storage; separate Watcom line-buffer state makes newline
writes flush through existing fputc/lowio paths, with capacity and explicit
flush unchanged. Other streams/modes/sizes,128B replacement and pending-buffer
reconfiguration remain denied.
Blank auxiliary/printer slots still use benign generic fclose (EOF ignored,
no underlying handle close/free); no initialization/private FILE26 or general
auxiliary/printer runtime model is added. Original per-call errors are unmeasured.

The native conio.h surface and Watcom-only int getch(void) spelling admit only
unchanged registered stdin backed by the real /dev/null device, in its initial
text mode or after the qualified binary setmode transition. A direct
ReadFile into an initialized zero byte must succeed with count zero; getch then
returns zero without adding lowio EOF state. Host errno is restored after device
preflight; target errno and FP are untouched. Pipes, regular files, consoles and
other input/result domains deny125. This does not model private pushback or the
old ignored console-mode failures, nor claim original per-call LastError values.
WLIB's regular-stdout help never invokes its cold getch call.

The public IMAGEHLP staging route recognizes LoadLibraryA("imagehlp" or
"imagehlp.dll") as a built-in native module, alongside the existing system
module routes. Only after that load can GetProcAddress resolve the exact name
CheckSumMappedFile to its SDK IMAGEHLP.H58..65 WINAPI4/PIMAGE_NT_HEADERS entry.
The entry supports an exact live mapped base/FileLength bounded by its owned
view, DOS/PE32 headers and valid aligned disjoint external DWORD outputs outside
the whole view. It computes the standard little-endian WORD end-around checksum,
excludes the old checksum DWORD and adds FileLength (including odd final bytes).
It returns the exact base+e_lfanew NT-header alias, reports old/new sums and leaves
the image/tails/errors/FP unchanged. Unsupported inputs deny before output writes.
Unknown exports/ordinals/pre-load lookup and FreeLibrary remain unsupported;
no unloading/reloading/private lifetime or malformed-image failure is claimed.
There is no provider hash gate, PE loader or new generic provider framework.
The actual CVPACK main branch now reaches its genuine App file-size/mapping
frontiers instead of silently skipping checksum work. `make
imagehlp-surface-check` verifies routing, signature, state preservation and denial.
`make checksum-check CHECKSUM_INPUT=... CHECKSUM_MASTER=...` uses the retained
actual CVPACK18320B pre-call input/master0x4b2a oracle, plus small nonzero-old,
odd-length/carry/overlap/header controls. The API never places the checksum in
the image; the genuine caller performs that store. Original per-call errors/FP
were not measured and are not inferred from the native checks.

HeapReAlloc has only the shipped WINBASE.H1382..1389 WINAPI4 declaration/remap
LPVOID(HANDLE,DWORD,LPVOID,DWORD). RCDLL's unvisited growth edge supplies flags9
and its allocation base through actual IAT100c/callc9a0; the live reserve request
fits existing capacity. Every invocation denies125 without inspecting an owner,
moving/freeing storage or publishing a result. No reallocation/error/zero-fill
semantics or original API execution is claimed. `make heap-realloc-surface-check`
checks signature/denial/no plain host import.

Lowio alloc marks the selected OS-handle slot invalid (-1) without publishing
FOOPEN; set requires that sentinel, and free relinquishes ownership without
closing the OS handle. For fd0..2, set/free update the Win32 standard table at
those transitions. Close avoids double-closing an inherited stdout/stderr alias,
then frees ownership and clears flags. Fclose retains the original FILE index
and other untouched fields; freebuf changes only active owned buffers, and
freopen stops after close failure. Successful CloseHandle preserves LastError.
Original whole CRT bodies plus real closed/reopened stdout state prove this;
focused/default/stdio/onexit/stream checks and all95 finished native cases pass.

GetProcAddress preserves the explicit known export catalogue. Only evidenced
IsTNT absence returns NULL/127; arbitrary unimplemented KERNEL32/USER32/NTDLL
names and ordinal/null lookups fail explicitly, rather than silently forcing
application fallback paths. Original-only debugger exports are not native API
requirements. Four affected LINK mapping/editor cached cases pass unchanged.

_fsopen supports the observed isolated rb/share20 and wb/share10 success pair.
The actual Win32 share requests are READ and zero; recon_fsopen_shares retains
that descriptor provenance for debugger inspection. Existing inode aliases,
conflicting opens, dup/dup2 and mappings are outside this single-process scope
and fail before truncation. An active mapping/view also excludes _fsopen.
No cross-process sharing or HANDLE-inheritance equivalence is claimed. Under
RECON_CRT_PROFILE_MSVCRT_510, reopening a FILE retains _bufsiz/_charbuf as the
canonical provider does. Binary bytes, FILE state and error transitions have
an explicit original probe; native fsopen-check uses only cached references.

RECON_ACP=1252 explicitly selects the captured ANSI-code-page environment;
unset/other profiles fail. GetACP preserves errno/LastError. Private HeapAlloc
supports positive overflow-safe sizes with observed flags9 and owned heaps,
zeroing the full requested extent; failures remain unsupported. HeapFree flags1
accepts only a live exact-base block belonging to that heap, preserves errors,
and leaves other live allocations intact. No freed-byte or exception claim.

After recon_crt_bind_arguments, application getenv reads the mutable captured
target environment with canonical ASCII case-insensitive name matching and
returns a borrowed value pointer. Mutating it changes that environment entry.
Unbound consumers retain host-libc behavior. Native host adapters must use
recon_host_getenv for their Linux configuration controls after binding; shim
implementation files already retain host getenv through RECON_SHIM_IMPL.

_isatty returns the canonical FDEV mask (64) or0, with unsigned descriptor bounds
and unchanged errors; it is not a host Boolean tty test. CharNextA supports the
observed explicit ACP1252 ASCII path: next-byte alias for non-NUL, same pointer
at NUL, unchanged errors. Non-ASCII bytes/other profiles remain unsupported.

Strict one-call mktemp replay opts in with RECON_MKTEMP_TEMPLATE (exact input),
RECON_MKTEMP_SUFFIX (six captured ASCII alphanumerics), and
RECON_MKTEMP_PRESTATE='errno,doserrno,LastError'. These assert actual state;
no entry-state seeding occurs inside the shim. Existing parent/missing candidate
is required; collisions, repeated/child calls and other failures are unsupported.
Canonical _access sets doserrno/LastError2, then _mktemp restores saved errno.
RECON_MKTEMP_FD optionally records the typed recon_mktemp_observation (RMT1,
568 bytes) in a dedicated regular file. lib/recon/mktemp.py prepares/validates
that receipt, requiring one PID-owned invocation and matching aliases/strings/
errors. Fixtures must separately bind the original full directory inventory.
Unconfigured legacy mktemp behavior remains unchanged.

tmpnam has the SDK char* cdecl(char*) declaration and native remap, but every
invocation fails explicitly with status125, including NULL and caller storage.
This prevents unvisited target calls from escaping to host libc; no temporary
name, allocation, output-buffer or successful service behavior is supported.

towlower has the SDK WCHAR16 cdecl(WCHAR16) declaration and native remap.
Only explicit MSVCRT510 with default-C LC_CTYPE/codepage/MB state supports ASCII:
A..Z become a..z, and other ASCII code units are unchanged. The authenticated
provider's default path uses no calls, table access, global writes or FP work;
native errors and full x87 state remain unchanged naturally. Non-ASCII, changed
locale/MB state and other profiles fail125. Original per-call errors/FP/locale
were not observed in the reached RCDLL capture; static provider-path evidence
is distinct from native state observations. No original API-only probe was run.
towupper is unchanged.

_wcsupr has the SDK WCHAR16* cdecl(WCHAR16*) declaration and native remap,
with success only for explicit MSVCRT510 default-C LC_CTYPE/codepage/MB state
and nonNULL valid mutable NUL-terminated ASCII spans. It validates the whole
span before writing, uppercases a..z in place, returns the input pointer and
leaves the NUL and tails untouched. Errors and full FP state are naturally
preserved by the authenticated integer-only provider path. NULL, non-ASCII,
other profiles and changed locale/MB state fail125. Original per-call pointed
bytes/errors/FP were not measured in RCDLL's reached capture; static provider
semantics and native observations are separate. No API-only original was run.

One explicit MSVCRT510 headless CONOUT$ open is supported: exact name,
GENERIC_READ|GENERIC_WRITE, share0, NULL security/template, OPEN_EXISTING and
attributes0 return INVALID_HANDLE_VALUE/ERROR_INVALID_HANDLE6. The native
domain requires unchanged standard handle roles, /dev/null stdin, regular
stdout/stderr and no controlling terminal. Host checks preserve independent
caller errno; DOS errno, FILEs, lowio flags, FP and output buffers are untouched.
Other CONOUT$ tuples/environments fail125; other profiles retain their prior
path. This adds no console handle, viewport, CSBI success or controller.

_errno returns the real host thread-local errno address, so cached pointer writes
and native libc strtol updates refer to the same slot, preserving aliasing.

Successful WriteFile on a regular disk file preserves errno and LastError,
as actual Win32 sentinel probes and CL's banner-to-mktemp state demonstrate.
The correction does not seed application errors or broaden pipe/console scope.

The VC5 CRT profile preserves canonical read-prefetch state: fseek on an owned
read buffer without explicit-setvbuf400 temporarily sets _bufsiz512; the next
successful _filbuf restores4096 after reading512. Explicit buffers retain their
size. FILE count/pointer and logical/physical file positions have an exact
original probe. Default VC1 behavior is unchanged.

_fstat supplies the legacy36-byte descriptor structure, retaining alignment
bytes14..15. Regular files use dev/rdev0, readonly-derived permissions and
signed32-bit UTC times; the proved Wine environment uses mtime as creation.
Char/pipe handles use descriptor identity and zero sizes/times. Invalid bounds
preserve LastError/doserrno while setting errno9; closed handles set LastError6.
Success preserves errors. Directory/socket/error variants remain unsupported.

MapViewOfFileEx supports the observed whole-view NULL-base form through the
same typed mapping/view ownership model. Its successful call preserves both
error slots, proved with a full2MB zero-extension/lifetime/persistence probe.
Explicit MSVCRT_510 also admits a non-NULL, 64KiB-aligned preferred address
for a full owned writable mapping, FILE_MAP_ALL_ACCESS and zero offsets.
MAP_FIXED_NOREPLACE never overwrites another allocation; real collisions return
ERROR_INVALID_ADDRESS, misaligned bases ERROR_MAPPED_ALIGNMENT. Nonexact hint
fallbacks are unmapped and rejected. Successful views retain normal shared-view
ownership/unmap and preserve native error slots; original per-call error state
is unmeasured. Partial views and other-profile fixed bases remain unsupported.
The focused preferred-view check proves real success, collision/no-clobber,
alignment, file persistence, lifetime and profile isolation. Native address-layout
success does not establish equality with LINK's retained failed original view.

IsDBCSLeadByteEx supports explicit codepage1252 and ASCII bytes (includingNUL),
returningFALSE with unchanged errors; other pages/bytes fail. UTF16 wcscat uses
the canonical16-bit code-unit scan/forward append, returns the original pointer,
and writes only the appended units/NUL. The full canonical body has no locale
conversion. Actual8196-byte pre/post buffers across4 sentinel pairs prove the
encountered append, untouched tails, pointer alias and error preservation.

_mbsicmp supports the proved CP0/SBCS/default-C ASCII path, matching the
canonical _stricmp normalized sign, case folding, prefix/NUL termination and
unchanged errors. Unknown codepages/locales/non-ASCII remain unsupported.

With explicit RECON_ACP=1252, GetCPInfo(0) writes only its defined fields and
preserves errno/LastError plus each native caller's padding18..19. Actual20-byte
original buffers and alternate native padding controls verify this; unconfigured
legacy behavior and other codepages are unchanged.

The VC5 stdio profile initializes stdin from proved provider data: ptr/base alias
a shared static4096-byte buffer (originalRVA42780), count0, flags101, descriptor0.
It has user-buffer ownership and survives dup2/close/reopen without heap-free.
Untouched contents have no live-byte equivalence claim. Default VC1 startup is
unchanged; this is provider initial-state behavior, not a case/API reseed.

iswctype supports the proved default-C ASCII digit/space masks4/8, retaining
those exact mask bits and both error slots. All128 ASCII values across4 sentinel
pairs match the actual provider; non-ASCII/other masks/locales fail explicitly.
Explicit MSVCRT510 additionally supports exact masks128/0x107 with default-C/MB state:
For128,0..9/A..F/a..f return128 and other ASCII returns0. For0x107,
A..Z return1,a..z return2,0..9 return4 and other ASCII returns0, exactly
matching the pristine wide-table AND (ASCII letters lack bit0x100). Mask0x103
and other unvisited masks remain unsupported. The authenticated integer-only
ASCII branch and pristine canonical wide table prove these bits; original
selected per-call table identity/errors/FP remain unmeasured. No table is copied
into production code. Prior masks4/8 and generic-profile behavior are unchanged.

C++ import allocation ownership
------------------------------
Distinct operator-new/delete imports use host-only recon_cpp_allocations records
(base, requested bytes, rounded malloc request, generation, live state). Successful
delete and ordinary free retire a live record; successful realloc retires old-new
ownership, failure retains it. Scalar delete(NULL) is a no-op before ownership
lookup, preserving public errors, full FP state and every allocation record.
The authenticated provider delegates to free, whose NULL path returns without
entering the heap. Tracked realloc(size0), foreign/interior/dead
delete and unproved allocation failures are unsupported. Host malloc usable size
and unwritten bytes are not application-owned evidence.
A stale pointer numerically equal to a fresh allocation after reuse cannot be
distinguished by pointer-only delete. Fixtures bind allocation generations and
caller provenance; the runtime does not claim to detect that ambiguity.

The reached _access mode0/4 regular-file/directory path follows the canonical
attribute-based test: success preserves errno, doserrno and LastError. Missing
leaf maps errno2/DOS2/LastError2; missing parent maps errno2/DOS3/LastError3.
Internal Linux path-resolution errors do not leak into successful CRT calls.
Unknown failure classes/file kinds fail closed. Other modes retain the existing
uncharacterized behavior; this does not claim Windows write-permission coverage.

_searchenv supports the reached default-C ASCII path with a missing current-
directory basename and one unquoted absolute directory in the bound target
CRT environment. It performs the actual filename access, environment lookup,
candidate access and candidate copy in that order. The first missing-leaf check
sets errno/DOS/LastError2; a later successful check preserves that result.
Only the candidate text and NUL are written; caller buffer tails remain intact.
Lists, quotes, unavailable candidates, current-directory successes and other
unreviewed paths fail closed. The function does not hardcode a file or variable.

Proved eight-byte aggregate register returns
------------------------------------------
Only where original ABI evidence proves EDX:EAX, use
RECON_RETURN8_TYPE(PeriodType) for the declaration/definition return type,
RECON_RETURN8(value) inside the function, and
RECON_ASSIGN_RETURN8(destination, function_call(...)) at its caller.
RECON_RETURN8 includes the return statement and requires an addressable8-byte
object. The native macros copy exactly8 representation bytes through uint64_t
with constant-size builtin memcpy; they initialize no payload fields and add
no helper function or assembly. Return-object, destination and call expressions
evaluate once; compile-time checks reject other sizes. Canonical Win32 retains
the period type and ordinary return/assignment. This is an explicit ABI bridge,
not a global aggregate-return compiler setting or an initialized-byte claim.

Explicit native dependency exports
----------------------------------
A verified host adapter may attach ReconModuleExport records and an optional
context (required lifecycle callback) through recon_register_module_exports.
The spec SHA must match the already registered module. Names/ordinals are unique;
callbacks must point into the native main ELF executable segment. This validates
native ownership, not a C signature: the adapter separately proves the actual
typed calling convention and function identity. Export metadata is copied.
LoadLibraryA accepts the captured full module DOS path for one lease, calls the
host LOAD hook and exposes only the explicit exports. Each export lookup occurs
once; FreeLibrary requires the complete lookup inventory, calls the FREE hook,
and ends the lease. Repeated/refcount/reload/foreign/unknown paths fail closed.
Hooks preserve the measured errno/DOS/LastError state; violations are rejected,
not reseeded. Other complete visible state remains the case provider's duty.
Provider hooks are host configuration/replay code outside the target graph.
Cached native function pointers cannot physically be invalidated: every typed
Invoke/Abort callback must check recon_module_provider_state before use. No DLL
code or target dispatch function is synthesized by the shared registry. Module
source/hash provenance and the actual captured loaded DOS path stay distinct.
Existing modules without export providers retain their prior behavior.

The explicit host option RECON_TEMPNAM_PROFILE=libc100 selects the encountered
static CRT algorithm for _tempnam, separately from _mktemp. In default-C scope
it requires dir="\\", a short ASCII prefix and an existing absolute target TMP.
It checks TMP first, returns owned malloc storage, and uses the proved initialized
counter1/prefix-length0 globals (exposed as recon_tempnam_counter and
recon_tempnam_prefix_length). Prefix-length growth resets counter1, then ordinary
decimal increment makes the first tmp result tmp2. No filename is supplied by a
fixture seed. Only a missing initial candidate succeeds: it creates no file and
retains the direct access errno/DOS/LastError2. Fallback/collision/overflow/OOM
and other unproved paths fail closed. With no option, prior tempnam behavior is
unchanged. The independently verified errno-preserving mktemp paths are unchanged.

Encountered VC5 locale table state
---------------------------------
recon_crt_apply_locale_profile accepts ReconCrtLocaleProfile only under the
explicit MSVCRT510 build profile and the exact canonical provider hash. It
requires the proved CP0/static-table initial state, then validates all257 mb
bytes and257 uint16 ctype words supplied by a verified host adapter. The ctype
input includes EOF at element0; recon_pctype exposes element1 (base+2 bytes).
The shared mb-array stays static. The changed ctype allocation and locale
scalars1252/LC_CTYPE1033 persist across dependency unload. Old static table
storage remains alive. The apply API preserves errno/DOS/LastError; case replay
owns any actually observed error effect at its dependency boundary.
The tables are reconstructed as character flag properties, not a section image.
Period WINNLS/CTYPE flag meanings are retained; the captured uniform0200 NLS
bit is preserved without inventing a meaning absent from the period header.
This profile describes the encountered reference environment, not all Windows
CP1252 installations. The encountered default-C table in shipped VC1/VC5 LIBC and LIBCMT, and
canonical MSVCRT510, classifies TAB as CONTROL|SPACE(28), without BLANK.
Only ordinary SPACE has BLANK. Captured CP1252 locale TAB remains268. Existing multibyte/getenv
CP0 guards remain strict except for the separately verified ASCII API subset below.

Bounded target environment mutation
-----------------------------------
Under the explicit MSVCRT510 build profile, a verified native host can bind the
captured WCHAR16 process environment with recon_crt_bind_os_environment(block,
units,provider_sha256) after recon_crt_bind_arguments. Units include the final
double NUL. Both initial inputs are borrowed and must remain alive. The host
separately performs the proved startup assignment to *recon_p_initenv(); binding
the OS block does not invent that assignment. Host environ/controls stay separate.
Read-only observations are recon_crt_os_environment, its _units, the mutation
count recon_crt_environment_updates and recon_crt_environment_vectors[3].

Only the measured two-call sequence is supported. The initial absent-empty
assignment under CP0 copies the CRT vector and every string, retains __initenv
and its borrowed initial vector/strings, and leaves the OS block unchanged.
After the separately verified1252/en-US profile transition, insertion of the
same previously absent name creates a new CRT vector, retains the copied old
strings, and appends an independently owned copy of the actual assignment.
The caller may release its assignment afterward. A separate owned WCHAR16 OS
block retains prior order/content and appends that actual assignment. No key,
flags or response string is hardcoded. Both calls require the measured entry
errno/DOS/LastError2/2/2 and preserve those values; this is a guarded case contract,
not a general putenv error rule. Case adapters validate full actual input vectors,
assignments, ordering, module/profile identities and mandatory call inventory.

Vector history row0 is borrowed/live, row1 the first owned vector, row2 the
insertion vector. On insertion row1 becomes dead and its vector allocation is
freed; retained strings belong to row2. A dead row's address is historical numeric
metadata and must not be dereferenced. It does not distinguish allocator ABA
reuse. The borrowed initial vector/strings and initial OS block remain alive.
The native OS block is an explicit namespace representation; original process
heap allocation identity is not claimed. Unknown phases, replacement/removal,
nonASCII data, profiles, errors or states fail closed. Existing unconfigured
_putenv remains unsupported. Applying this namespace slice does not enable
unproved1252 getenv or multibyte API behavior.

Encountered1252 ASCII API subset
-------------------------------
After the verified locale apply, bound getenv and _mbsncmp/_mbsinc/_mbccpy/
_ismbcspace/_mbclen/_mbschr/_mbsspn accept ASCII under exactly that reference
CP1252/en-US state. Every entry checks scalars, all six handles, applied pctype
allocation identity and all257 mb/257 ctype entries. Native host libc locale is
not changed. getenv retains case-insensitive borrowed target-vector aliases and
rejects nonASCII returned values in this scope. Other multibyte, wide conversion
and searchenv routines retain their separate CP0 guards. No general1252/DBCS or
Unicode API behavior follows from this subset. Canonical _getmbcp() reporting0
for this SBCS state is distinct from the actual internal mb-codepageDWORD1252;
the original proof binds the latter plus complete tables and LC_CTYPE1033.

Target process environment and RSP2 wait-success response
-------------------------------------------------------
A NULL spawn environment inherits the separately bound OS namespace. Explicit
vectors stay explicit; with no bound target arguments the old host inheritance
path remains. Bound-but-missing OS blocks and unknown encoding fail closed.
ASCII WCHAR16 block conversion retains every string and its order, including
leading-equals process entries, without host controls. PATH search separately
reads the current target CRT environment through recon_getenv. recon_crt_arguments_bound
is exposed for read-only host observation. Host transport allocation/cleanup
must not alter the independently measured parent-state effect.

Only RECON_SPAWN_STATE_PROFILE=msvcrt510-wait-success-v1 enables RSP2. It requires
mode0, the exact applied1252/en-US profile and inherited target OS namespace.
The public outer spawn entry captures errno/DOS/LastError before any host helper.
RSP2 request fields are mode,pid,argc,envc followed by four DWORDs:
entry errno,DOS,LastError,namespace2; existing length-prefixed strings follow.
The callback receives parent_state={errno:0,doserrno:2,last_error:2} and
environment_source='target-os', in addition to the ordinary complete request.
Its response must be exactly status0/stdout bytes/stderr bytes/parent_state.
parent_state contains exact before/after maps with errno/doserrno/last_error:
0/2/2 ->0/0/2. Other keys, types (including bool), states or results deny.
RSP2 action5 carries those six state DWORDs, then existing stream lengths/bytes.
Native checks the full response and entry match, handles the streams, and only
then applies the measured poststate. Stream replay retains logical descriptor
routing; full FILE/handle/locale/files/process-result matching remains the case
adapter's responsibility. The adapter must validate full command, argv, CWD,
input artifacts, target OS environment, response and mandatory invocation count.
No partial matcher, native PE execution, original launch or fallback is implied.

RSP1 wire/reply/error-state semantics remain unchanged without that option.
The new profile cannot request native execution or a failed/unknown child result.
This is the measured wait-success seam, not a general spawn error-state rule.
Canonical _spawnvp65ba passes envpNULL through65d4/6612/66a6 to _cenvarg6709;
675a/6766 leave envblkNULL, passed at6974 to CreateProcessA697e. Explicit envp
uses the distinct cold construction path. The backend6946/6951 clears CRT errno
and DOS before CreateProcess; actual parent proof binds0/2/2 ->0/0/2. The native
implementation does not claim Win32 process creation or generic loader behavior.

VC5 fread direct reads
---------------------
The explicit MSVCRT510 profile uses the encountered canonical fread loop:
drain existing FILE bytes, read whole buffer-size multiples directly into the
caller destination, then use _filbuf for smaller remainders. The loop caches
capacity and refreshes it after _filbuf. Thus a seek's temporary512-byte
capacity remains512 when the request takes only direct reads, including a short
read followed by EOF. It changes to4096 only when the separately proved _filbuf
refill succeeds. Actual written bytes include an incomplete final item; the
return value counts only whole items. EOF/error flags follow the read result.
VC1/default fread implementation is unchanged. The current proof binds ordinary
binary file reads, owned/initialized FILE buffers, direct/buffered/remainder/EOF
and partial-item behavior with zero entry errors; unobserved argument, I/O error
and environment variants do not acquire a broader equality claim.

Legacy UTF16 source-line formatting additionally accepts the separately observed
exact L"#line %d " template alongside L"#line %d\r\n". Only those two complete
prefix/suffix forms pass. The signed32 argument is formatted into the caller's
UTF16 buffer, then its actual validated suffix/NUL are written. Default-C scope,
count excluding NUL, untouched buffer tails and measured error preservation are
retained. No additional formatting grammar, locale or capacity mode is enabled.

Reached _unlink regular-file deletion preserves entry errno, DOS errno and
LastError on success. Missing leaves map to 2/2/2; missing or non-directory
parents map to errno2/DOS3/LastError3 through the shared CRT error mapper.
Inputs and unrelated files remain unchanged. Wildcards, directories, symlinks,
other file kinds and unknown deletion failures are unsupported. This bounded
public _unlink contract is separate from the legacy recon_remove path.

A separately reached GetSystemInfo can use recon_load_system_info_profile(path,
reference_sha256), with system_profile.py encode_system_info_profile or
write_system_info_profile. RSI1 is exactly104bytes: tag4, lowercaseASCIIreference
SHA256(64), then the complete36-byte observed SYSTEM_INFO. The reference closes
an owning capture/environment receipt; the host validates actual provider,
inputs and output hashes before binding. This profile permits one GetSystemInfo
call and rejects GlobalMemoryStatus. It supplies no MEMORYSTATUS bytes or Linux
hardware values. Existing136-byte RSE1 and its ordered two-call inventory remain
unchanged. Both loaders and calls preserve measured independent error/FP state.

recon_apply_x87_empty_entry_profile(reference_sha256,control,status,tag) is an
explicit host input seam, called once before any target DLL lifecycle instruction.
Only captured CW027f/SW0020/tagffff/TOP0/all-empty is supported. The native prestate
must already be CW027f/SW0/tagffff (use the existing proved precision setup).
The reference hash closes the owning actual entry capture, validated by the host.
Only the two-byte architectural status word changes. Native IP/DP/selectors,
reserved transport bits and stale empty-register payload stay untouched, as do
independent errors. It neither performs target arithmetic nor resets later state.
Unknown/nonempty/inconsistent states and repeated application are unsupported.

The reached VirtualAlloc(NULL,positive_page_multiple,MEM_RESERVE|MEM_TOP_DOWN,
PAGE_READWRITE) subset creates a real private PROT_NONE region, trims an owned
reservation to64KiB alignment, and registers its exact length for existing
allocation lifetime operations. Host bookkeeping preserves independent errors
and x87 state. Fixed addresses, commit-with-top-down, other protections/flags,
non-page sizes/overflow and unobserved allocation failures are unsupported.
This bounded success contract preserves the actual opaque identity/size and
protection; it does not guarantee Windows' global highest-address ordering.
No reserved-byte contents or future address-order/low-bit consumers are claimed.
Ordinary VirtualAlloc paths are unchanged.

The reached default-C vfprintf pointer path accepts ASCII literals, one plain
%p and literal%% escapes. It consumes the actual32-bit void* and computes eight
uppercase zero-padded hexadecimal digits, including00000000 forNULL, with no
prefix. Existing ShimFILE buffering and byte/count/error routing are retained.
Pointer widths/precision/length modifiers, mixed/multiple conversions, malformed
prefixes and unproved locale/multibyte states are unsupported. Validation checks
the entire grammar before consuming the pointer. Formats without a pointer
conversion retain their existing formatter paths. This does not add %p handling
to the separate sprintf implementation or claim general pointer formatting.

RECON_CRT_PROFILE_WATCOM_100A exposes build-only Watcom 10.0a heap/signal/jump
surfaces to native target translation units. Beyond the separately qualified
first-main heap and normal jump contracts, unsupported operations deny125.
Signal registration supports only numbers1/4/6/7 in the single-threaded target
namespace: initial old tokens2/1/1/1, then exact old-handler return/new-handler
store. Nonnull handlers and IGN1/DFL2 tokens are stored without invoking them;
NULL/ERR3, other numbers (including FPE), raise and delivery remain unsupported.
No Linux signal/console wiring or private thread/raise-pointer bytes are modeled.
SIGNAL.H numbers1..12 and
SIG_IGN/DFL/ERR tokens1/2/3 remain Watcom values. jmp_buf is13 unsigned32-bit words
(52bytes), with no writes before denial. Host RECON_SHIM_IMPL translation units
retain Linux signal tokens and their actual libc context. Existing MSCRT jump
storage8/16 and signal behavior are unchanged. This supplies no Watcom FILE or
allocator implementation. Native GCC calling conventions are intentional;
original Watcom headers/compiler remain responsible for original ABI audits.
Run make -j4 watcom-surface-check for the explicit target/host split and denials.

The reached _mbsstr subset requires CP0, MB_CUR_MAX1 and the default C ctype
locale, with two nonNULL terminated ASCII strings. It returns the first borrowed
match in the actual text, the text base for an empty needle, orNULL for absence.
Complete input buffers, error channels and live x87 state are unchanged in the
bounded original/native matrix. NonASCII, other codepages and unproved locales
remain unsupported; the separately applied1252 profile does not enable this API.
The original reference probe records its ordinary startup ACP1252 state, then
establishes CP0 with _setmbcp(0) before measured calls. Its prior rejected startup
attempt remains retained. Run make -j4 mb-strstr-check (native only).

RECON_NATIVE_STARTUP_WATCOM_100A_FP is a separate native HOST startup option.
It uses the existing control87 translator once before main to select PC53 and
affine infinity (architecturalCW127f), matching the observed Watcom initializer's
FINIT/FLDCW127f chain. It preserves native status/tags, pointers/selectors and
all register payloads; no captured transport bits or instruction pointers are
installed. It does not implement target Watcom _control87 semantics or change
default startup. The existing MSCRT53-bit-only option retains its behavior.

One explicitly bound first-WLIB-main _heapchk path supersedes the build-only
trap above. Host recon_bind_watcom_wlib_first_main_heap(reference_sha256) requires
a verified closure proving no target constructors and that _heapchk is main's
first application call. It binds clean0/calls0/empty target allocation namespace.
Five profile-only malloc/calloc/realloc/free/strdup wrappers mark sticky target
activity and delegate existing native functions; internal/RECON_SHIM_IMPL host
allocations bypass them. The supported check requires bound1/clean0/calls0 and
no activity, then derives_HEAPOK from the empty namespace and records clean1,
calls1. Binding after activity, repeated/unbound checks and _nheapchk deny.
The exposed recon_watcom_first_main_heap state supports independent observation.
This diagnoses no glibc corruption, reconstructs no original free-ring headers
and grants no populated/mutated/later heap semantics. Exact source/adoption and
actual first-main state must be checked by the binary owner before enabling it.

Default CRT zero-capacity buffered output has one pending-byte slot when the
registered FILE already owns malloc backing (MYBUF/write flags0xa), has a valid
live descriptor and exactly the captured ptr/count shape. The owned allocation
must provide at least one usable byte. malloc_usable_size measures that existing
ownership invariant; it is not a validator for arbitrary pointers/forged FILEs.
The next byte writes the prior pending byte, then remains at base with ptr=base+1
and count=-1; ordinary flush/close drains it. This handles LINK's observed old
setvbuf(NULL,full,1) rounding to logicalzero while retaining real malloc0 storage.
It changes no malloc0 contract or target CRT source. Unregistered/user/unowned
zero-buffer states deny; negative/unbuffered/MSVCRT510 paths remain unchanged.
The original actual allocation/provenance and FILE lifetime are retained in the
LINK case, and native controls check next-byte/flush/close/errors/FP/tails.

Watcom ordinary setjmp/_setjmp calls with an actual52-byte array owner now
support normalreturn0 and the bounded live-context resume1 described below.
The macro invokes real libc _setjmp directly in
the live application frame; the existing registry keeps an opaque host context
out of line and tags its Watcom ownership. Windows'48-byte serialized register/FS
context and native host context are private differing CRT representations.
No captured Windows context is copied into native storage: all52 target bytes,
including finalDWORD/adjacent guards, remain their own native bytes. The actual
reached application uses only the owner address and result, not context fields.
Indirect/bare typed saves, pointer-sized/truncated/NULL owners and generic
recon_longjmp under this profile remain unsupported. The separate Watcom
longjmp route below adds no Windows SEH/unwind semantics.

Successful default/510 openfile preserves a reused FILE slot's _charbuf and
_bufsiz. The VC1 getstream/freebuf/openfile field writes independently confirm
this existing510 behavior also applies to the reached default C2 path. Pointer,
base and count are reset, so a retained capacity does not represent live buffer
storage; tmpfname and other successful-open fields still reset. Failed-open
behavior remains unchanged. Native controls cover cold slots and reused4096,
16 and512 capacities with independent charbuf sentinels, ownership and FP state;
the actual C2 writer-entry proof reuses its unchanged original capture.

Native hosts built with RECON_SHIM_IMPL can read
recon_local_clock_call_count(): the existing configured-clock sequence count
starts at zero and advances at each configured _ftime call. Its existing resets
on configuration change or leaving configured mode remain in _ftime. Fixtures
bind the clock once and assert their own exact call inventory at the appropriate
frontier/completion; the generic clock remains unconstrained.

The native mbctype.h surface exposes the encountered Microsoft _M1 bit and
_MB_CP_LOCALE selector. _mbctype aliases the existing shared257-byte table;
_setmbcp has its cdecl int(int) signature; unbound selections fail125.

An explicit MSVCRT510 native host can bind one isolated empty LC_CTYPE selection
with recon_crt_bind_empty_ctype_1252(canonical_provider_sha256, actual_receipt_sha256).
Binding leaves locale state unchanged. The actual setlocale(2, "") call requires
the captured initial tables/static pctype, MBcodepage/gate0, MBmax1, all handles0
and errno/DOS0/LastError203 or0. It allocates a writable514-byte ctype table and a
distinct persistent writable "English_United States.1252" result; only LCcodepage,
handle2 and pctype change. Old static ctype and all MB state remain unchanged.
Queries, repeat calls, other names/categories/prestates and mixing with the CL
combined dependency profile fail125. The read-only ctype-selection state exposes
the host reference, bound/call inventory and returned owner. Original ownership
proof covers observed initialized spans/lifetime, without claiming heap extents.
Focused checks use make isolated-ctype-check ISOLATED_CTYPE_REFERENCE=<retained-raw-directory>.

After that bound selection, one _setmbcp(-4) accepts only the captured C1
SBCS state: exact old/new tables, LCcodepage1252/handle2=409, internal MBcodepage
and gate0, MBmax1 and errors0/0/0 or0/0/203. It selects the established locale codepage
into recon_mb_codepage and advances mb_calls; all tables/owners, gate, result,
errors and FP remain unchanged. Other selectors, repeats, states and combined
CL-profile calls fail125. No general codepage conversion or _getmbcp API is
enabled. Focused checks use make locale-codepage-check
LOCALE_CODEPAGE_REFERENCE=<retained-C1-raw-directory>.

On the supported NULL-high/unambiguous successful SetFilePointer path,
LastError remains the caller's incoming value, including a preceding mapped
SetEndOfFile failure1224. The existing highword, ambiguous0xffffffff and failure
behavior remains separate. This changes no startup FP policy; direct native
drivers needing the observed53-bit startup select its existing opt-in flag.

_mbctolower has the canonical unsigned int cdecl(unsigned int) native link
surface. Every input fails125 until its own reached locale/codepage mapping
contract is reviewed; it never substitutes host tolower.

The default FILE refill restores a temporary512-byte read capacity to4096 only
for a registered recon_iob stream with exact read/MYBUF flags9 and an established
malloc-backed allocation of at least4096 bytes. It performs the current512-byte
read first and publishes the larger capacity only after success. This covers
the reached older C1XX read-only diagnostic catalog following its CRT fseek.
Actual512-byte allocations, user buffers, other flags/owners and EOF/error exits
retain their existing behavior. The explicit510 and Watcom branches are unchanged;
this does not qualify the original/shipped filbuf difference when flag0x400 is set.

RECON_FORCEINLINE is reserved for helpers/construction proved inlined into an
original App vertex. Native always_inline overrides the global no-inline build;
no_instrument_function avoids materializing separate helper addresses solely
for instrumentation. It must not be applied to a real original App function.
Canonical VC5 uses __inline with a separately qualified /Ob1 translation unit.
RECON_RETAIN requests native compiler emission with __attribute__((used)) and
is empty for the original compiler. It preserves a real App symbol without
changing instrumentation, calling convention, inline policy or runtime code.
Any translation-unit optimization/instrumentation exception is separately
qualified against the original graph; the annotation grants no such exception.
The native new.h subset supplies only caller-owned placement construction and
typed cdecl scalar new/delete declarations bound directly to existing
recon_cpp_new/recon_cpp_delete. It adds no wrapper vertex, array forms, exception
policy or libstdc++ dependency.

The native compiler-support __cxa_pure_virtual(void) symbol has C linkage and
never returns: it reports unsupported invocation through the existing exit125
boundary. This permits abstract vtable references without a libstdc++ dependency;
it claims no original purecall diagnostic, success semantics or App body credit.

Watcom target errno is a separate process-local volatile int, exposed through
its typed __get_errno_ptr/errno spellings; only reached ENOENT1/EMFILE11 constants
are supplied. RECON_SHIM_IMPL retains Linux errno. Readonly binary open now
returns-1/targetENOENT1 for a real missing leaf/parent (including an intermediate
non-directory); success preserves the target slot. Preflight restores host errno,
unknown failures remain125, and unmeasured Win32/private DOS errors are unclaimed.
The Watcom-only char __OS alias starts at2, matching the qualified loader AH2
store before App; it does not detect the Linux host or change original naming.

Watcom writable open admits0162 with0666 and0262 with0600/0666 or permission0,
which the reached default-umask0 CRT rule normalizes to0600, for regular
ASCII relative outputs in the owned cwd, with existing-directory components
qualified below. Native O_RDWR/O_CREAT/O_TRUNC/O_TEXT
spellings retain2/20/40/100; shim implementations keep host flags. The exact
tuples translate to existing Microsoft4302/8302 descriptor operations, providing
real create/truncate, initial position0 and text CRLF/binary writes. The caller's
private errno is preserved on success; unknown flags/permissions/paths/failures
deny125. Permission0 is admitted only for0262; readonly0200 never fetches an
absent variadic permission argument. This models the target DOS writable
attribute selection, with no POSIX chmod or file-mode identity claim.

Watcom fpos_t is a typed4-byte long at native source declarations. Its fgetpos
and fsetpos use registered native ShimFILE streams and disjoint aligned4B
position storage, saving an existing ftell value or seeking with SEEK_SET.
Binary positions and fresh unbuffered text saves are supported; unknown stream,
buffered-text save, negative-position and backend-failure domains deny125.
The original fgetpos error path stores-1/returns1, but its unqualified errno
domain is not modeled. No glibc FILE object or private FILE26 overlay is used.

The Watcom-only public utoa(unsigned int,char *,int) boundary admits radix16
through the existing unsigned32 conversion with lowercase digits. Its caller
must own a valid12B destination; NULL/wrapping spans, other radices and profiles
deny125. It returns the supplied start pointer, preserves error/FP state and
writes only up to eight digits plus NUL. WLIB PrintModuleHeading uses this path;
the separate cold decimal CheckLibrarySize branch remains unsupported.

Explicit MSVCRT510 HeapDestroy retires only an exact registered live private
heap from HeapCreate(1,0,0), releasing all outstanding tracked block mappings and
then its owner mapping. It returns TRUE only after successful release, retaining
native errno/LastError/FP and other heap owners. NULL, unknown/dead owners and
other profiles deny125. No general stale-handle generation guarantee after a
later mapping-address reuse is implied. RCDLL cleanup uses this real lifetime
boundary; original private survivor inventory/per-call errors are unmeasured.

Watcom longjmp admits value1 only for the exact context tagged by the existing
52B setjmp adapter. The caller must supply a still-live frame on the same thread
and unwind chain, as in WLINK Spawn/Suicide. It resumes the existing caller-frame
libc context without reading/writing the opaque52B owner or restoring x87 state;
current errors/FP remain current. Generic, unknown and _setjmp3 contexts deny125.
The registry does not prove liveness. Expired frames, Windows SEH/RtlUnwind,
private register serialization and other values remain outside this contract.
Native tracing emits first entries only, so no abandoned trace frames need cleanup.

Watcom writable open also admits ordinary relative output components through
existing directories. Each parent is translated by the existing path backend
and checked as a nonsymlink directory; the final leaf remains new or regular
and nonsymlink. Empty/dot/parent components, absolute/drive/UNC paths and trailing
dot/space components deny before open. No directories are created. Exact
0162/0262 flag and permission tuples, descriptor ownership and failure limits
remain unchanged. This qualifies WLIB out\keep.obj through its fixture-owned out.

The Watcom-only bare itoa spelling aliases the existing signed32 recon_itoa,
matching the SDK int/pointer/radix prototype. WASM uses the decimal path; no
additional numeric conversion implementation or provider state is introduced.

Watcom open also admits exact0621/0666 for an absent ASCII relative leaf:
O_WRONLY1|O_CREAT20|O_BINARY200|O_EXCL400 maps to the existing MS8501 atomic
CREATE_NEW backend. The returned binary write descriptor has position0 and
normal shared ownership/close; the existing0262 tuple can reopen it afterward.
Existing paths/collisions, directory components for this new tuple, other modes
and failures deny125. Native target O_EXCL400 is isolated from host/shim flags;
read-only and prior writable tuples retain their established domains.

Explicit MSVCRT510 VirtualFree admits MEM_DECOMMIT only at the exact base and
full requested length of a tracked VirtualAlloc(NULL,n,COMMIT|RESERVE,READWRITE).
Checked mprotect(PROT_NONE) and madvise(DONTNEED) retain the reservation while
discarding storage; the owner remains tracked until ordinary zero-size
MEM_RELEASE. Native errors/FP are retained on successful decommit. Partial,
foreign, repeated or ineligible decommit and subsequent recommit remain denied.
Other profiles keep the prior unsupported result; existing release is unchanged.
No general VM/recommit model or original per-call error equivalence is claimed.

RECON_QSORT_WATCOM_1994 is a separate opt-in requiring the Watcom CRT profile.
It maps ordinary qsort to the positively identified older midpoint/forward
partition algorithm, including count2 handling and its equal-tail skip. Equal
keys retain that algorithm's observable order rather than a new tie breaker.
The existing byte swap handles unaligned records; pure C bookkeeping leaves
real callback calls/effects intact. Arrays/comparator/width must be valid, with
span/overflow guards. Other profiles and Watcom users without this opt-in keep
the existing recon_qsort. WLIB alone has qualified this variant by actual code
identity and the original/native listing; later shipped hybrid source differs.
No host qsort interposition, App sorting implementation or general sort framework.

The qualified Watcom tmpnam role supplies a genuine absent native relative
RCXXXXXX name in a valid caller-owned13B buffer, returning the caller alias and
retaining untouched tails, independent errors and FP. Host mktemp creates no
file; existing access verifies a real missing leaf before return. The name is
unreserved in the owned, nonconcurrently mutated namespace; the caller performs
its normal create/reopen/unlink lifecycle. Exact0261/0666 maps to MS8301 only for
an absent relative leaf. NULL/static-result, wrapping spans and unknown modes or
filesystem results remain denied. This equivalence applies only to the proved
internal/deleted, non-name-sensitive identity: no historical AAA/A..P sequence,
captured PID, or host-control PID behavior is supplied. Other CRT profiles retain
tmpnam denial, and final exit file inventory remains strict.

Explicit MSVCRT510 _mbscmp has the SDK cdecl unsigned-char string pair signature.
It reuses the default-C/CP0/MB1/gate0 ASCII comparison loop through mismatch or
NUL, returning normalized unsigned -1/0/1 without case folding. Valid readable
string prefixes are caller preconditions; other profiles, NULL, unknown state
and nonASCII compared units deny125. LINK consumes equality for module names.
Original live per-call state and identical provider read schedules are unclaimed.

Explicit MSVCRT510 GetTempPathA admits bound target TMP="." and a verified
canonical Windows cwd, returning its full path with a trailing backslash into
caller-owned260B output. GetTempFileNameA admits that directory, a caller-owned
ASCII prefix with NUL within MAX_PATH, and unique0 with disjoint260B output. The
full prefix including NUL is checked before effects; its first min(3,length)
bytes must be valid leaf characters and name the file, including empty/short
prefixes. A native time-seeded16-bit number follows as %04x.tmp. Existing
CreateFileA CREATE_NEW and checked close
create a genuine empty file, retrying collisions without replacing them.
Returned number/path agree; the same file is available for real write/SELF
response consumption and later DeleteFileA. Each side retains its own actual
name; no original filename/PID or observed success-LastError equivalence is
claimed. Native success preserves independent errors and semantic FP state;
incidental instruction/data-pointer bookkeeping and empty registers are excluded.
The hello prefix passes focused creation/collision/lifecycle/span controls; its
complete ordinary LINK conversion/close lifecycle remains pending. Other profiles, environments,
directories/prefixes/unique values and output domains deny125.

Watcom native direct.h provides the SDK NT packed289B directory/entry shape:
d_attr at21, d_name[256] at30 and d_first at288. Target DIR/dirent and attribute
spellings are profile-only; shared implementation TUs keep host DIR/dirent.
Public opendir/readdir/closedir map to typed no-access denial125 boundaries.
This permits complete WLINK WildCard source to compile while its reached
callback path avoids enumeration. No allocation, filesystem query, returned
entry or close semantics are qualified. Target-local reached CRT adapters must
include shared headers before applying their scoped public-name overrides.

Explicit MSVCRT510 IsValidCodePage accepts the reached MacRoman page10000.
WideCharToMultiByte10000 admits flags0, positive explicit counted ASCII WCHARs,
NULL replacement/used pointers, and either NULL output/capacity0 query or a
disjoint caller-owned output with capacity==count. All units/spans are checked
before writes; output is exactly count identity bytes without an added NUL.
Other forward MacRoman tuples/nonASCII and other profiles deny125. Other
codepages remain unchanged. Native errors/full FP are retained;
original per-call bytes/errors/FP were unmeasured. RCDLL's actual control flow
proves validity and its next expander count; ASCII identity is the public
encoding contract, not a captured original buffer claim.

The same explicit510 page10000 flags0/NULL replacement+used domain also admits
the reached count0/capacity0 with nonNULL aligned input: it returns0 with
ERROR_INVALID_PARAMETER87, without reading input or writing output (including
nonnull output at capacity0). This is invalid-count failure, not empty success.
Independent errno/DOS/FP stay unchanged; original per-call LastError unmeasured.

The explicit510 reverse page10000 arm admits flags1/count-1 and positive
caller-owned aligned WCHAR output capacity up to4177 units, matching the reached
line-buffer domain. It validates default-C/MB1 state, complete terminated ASCII
input within capacity and disjoint nonwrapping spans before copying through NUL.
The return includes that terminator. Queries, other flags/counts, nonpositive
capacities and other profiles retain their previous path; invalid selected domains
deny125. No MacRoman table or normalization is added. Native errors and semantic
FP state remain current; original per-call state is unmeasured. The ordinary
saved-reference adoption restores the exact complete1584-byte prepared RC input.

The cold Microsoft _purecall spelling maps directly to the existing void cdecl
noreturn __cxa_pure_virtual denial symbol. No new body/wrapper or successful
return is introduced. The original VC5 declaration keeps its compiler-known
int return type; the native installed-only denial uses the existing void type.
RPCPROXY.H also has a void dummy definition, not a CRT success specification.
Invocation remains unsupported125; the original import identity and installed
vtable compile without qualifying a purecall execution or returned value.

Explicit MSVCRT510 wsprintfA accepts only the reached exact "%d" format,
consuming one signed32 int through the SDK cdecl varargs ABI. The caller owns
at least12 writable bytes (RCDLL supplies128); NULL/wrapping/format-overlapping
output is denied before effects. Existing recon_itoa supplies decimal output
and NUL; return excludes NUL, errors/full FP remain unchanged. Other formats
(including unvisited "%ws") and profiles remain125. No general formatting or
original per-call bytes/errors/FP claim is introduced.

Explicit MSVCRT510 FormatMessageA FROM_STRING/id0/lang0 additionally admits
registered ANSI message4509/lang409 with exactly one each of inserts1..5:
ASCII strings1/3/4/5 and signed decimal2!d!. Template/strings, computed output
capacity/extents and disjointness are validated before writes; return excludes
NUL and literal CRLF is retained. Other grammars/wide inserts stay denied;
existing two-insert help formatting is unchanged. Original per-call output and
error/FP state were unmeasured; the ordinary warning continuation qualifies use.

Watcom utoa now admits radix10 as well as16, forwarding the validated radix to
existing recon_ultoa. Both need at most12 caller-owned bytes; decimal unsigned32
uses at most10 digits plus NUL. WPP's real CPU macro requests500 with120B left.
Exact decimal output500/max and retained lowercase hex are checked; other
radices/profiles and NULL/wrapping output remain denied. Original per-call
error/FP state is unmeasured.

Explicit MSVCRT510 separately reserved RW arenas track only a page-aligned
contiguous committed prefix. Overlapping/adjacent commits may extend it; holes,
other protection and recommit after decommit remain denied. Exact-base decommit
accepts the nonempty prefix or entire reservation, discards storage and retains
PROT_NONE ownership until zero-size MEM_RELEASE. Empty/partial-prefix/interior,
repeat and foreign requests remain unsupported. Combined whole allocations and
other profiles retain their previous contracts. C1's saved Arena::Release and
ordinary work-buffer cleanup observe these two reservation lifecycles; no broad
VirtualAlloc model or original per-call error/FP values are claimed.

The optional explicit510 process OS-environment input sink validates the
provider immediately and borrows only the genuine launch UTF16 block for
process lifetime. Staging precedes setup/CRT arguments; the first real argument
bind consumes it through the unchanged strict OS binder. Its provider string
need not outlive staging. No input leaves prior behavior unchanged; host control
and target CRT namespaces stay distinct. Native SELF stages its accepted child
OS vector, independently of the root launch block. No mutation is granted here.

Explicit MSVCRT510 SetEnvironmentVariableA now admits only absent
LINK_REPRO_NAME="\deflib.rsp" after genuine initial OS/CRT binding in the
qualified default-C spawn state. One process-owned UTF16 OS block inserts that
assignment before the first greater case-insensitive ASCII name, preserving all
existing entries/order. The borrowed initial block, CRT _environ/__initenv and
getenv snapshot, and Linux host controls remain unchanged. Existing-name,
replacement/removal/repeat/other mutation domains stay125. CP0 spawning accepts
only the original block or this exact validated prefix/assignment/suffix-derived
block. Native errors/full FP remain unchanged; original post-call errors and
block order were unmeasured. Child startup uses its actual accepted OS vector.

Explicit MSVCRT510 legacy swprintf additionally accepts only the exact UTF16
"#pragma code_page %d\r\n" format in default-C/MB1 state. The caller owns at
least64 disjoint writable bytes (RC supplies1024); aligned/nonwrapping format
and output checks precede varargs/output effects. The existing signed32 digit
logic produces at most31 WCHARs plus NUL, returning the count excluding NUL.
Native errors/full FP stay unchanged. Existing #line formats stay supported;
other new grammars/profiles remain denied. Original per-call state unmeasured.

The explicit MSVCRT510 _mbstok spelling has its SDK unsigned-char pair/cdecl
signature. Its reached success domain is fresh nonNULL writable owned ASCII
text shorter4096B, exact space/tab delimiters, default-C/CP0/MB1/gate0/allLC0.
The whole bounded line and delimiter disjointness are checked before each
single ending-delimiter NUL write; leading spaces/tabs are skipped and return
aliases the token or NULL. Qualified fresh input establishes/reset a cursor/end;
NULL continuation checks the exact remaining ASCII extent/terminator and same
state/delimiters, then exhausts toNULL. The caller keeps this line alive on the
same thread until exhausted/reset; unbound/changed-extent/unknown domains stay125.
No general PTD/TLS or lifetime framework is introduced; other profiles keep no
alias. Native errors/full FP are preserved; original per-call state unmeasured.
LINK's unchanged320/325 oracles qualify fresh and live continuation use.

Watcom _rotl maps to a shared unsigned32 helper implementing count modulo32,
with count0 returned unchanged to avoid shift-by32 C undefined behavior. The
actual seven-byte CRT helper uses ROL EAX,CL; no assembly or target flag model
is copied. Other profiles keep no alias and deny direct helper use. WCC's
ordinary hash path retains its real period CALL and qualifies value/count4;
focused wrap-count/signed-bit vectors preserve native errors/full FP. Original
per-call errors/FP were unmeasured.

Explicit MSVCRT510 swprintf also admits the encountered APSTUDIO format
"%c%s%c%s%c%d%c%c%c": E000 start, E002 separators, two ASCII WCHAR strings,
signed32 line and genuine promoted flagFFFFFF80/81 converted to WCHARFF80/81.
Default-C/MB1/gate0 and all locale handles are required. The caller owns2048
WCHARs; format/string/output spans, disjointness and computed length are checked
before writes. Return excludes the NUL, tails are untouched, native errors/full
FP preserved. Other unqualified layouts/marker/flag/string domains stay125. No general
formatter or original per-call state equivalence is claimed; the ordinary
APSTUDIO continuation matches its exact partial output hashes.

The companion APSTUDIO reference swprintf layout is exactly
" %c\"%s%c%s%c%d\"": E001, WCHAR name, E002, WCHAR filename, E002, signed32
line, with literal leading space and quotes. It uses the same bounded/disjoint
ASCII WCHAR and2048-WCHAR caller contract, strings+digits+6 count, and native
error/FP preservation. Its six typed varargs are read without consuming the
first layout's extra flag fields. The original nine-argument layout remains
unchanged; no other pattern is interpreted. Original per-call state unmeasured.

RECON_SPLITPATH_WATCOM_DOS with the explicit Watcom profile opts in only the
identified208B DOS _splitpath variant. Relative ASCII leaves shorter260B use
last-dot splitting, basename8/extension4 truncation including the dot, and empty
drive/directory. Requested outputs require caller-owned1/1/9/5-byte spans;
wrap/input/output overlaps are checked before any writes. Unknown path forms
deny125. Other binaries keep their prior mapping and maxima; no automatic NT
variant change is implied. WASM's ordinary get_module_name path plus a separate
actual-state diagnostic establish ModuleInfo.name="transfor" for transform.asm.

The explicit MSVCRT510 _tempnam branch admits the APSTUDIO internal-file role:
bound target CRT TMP=".", NULL directory, RCX1 through RCX4 prefixes, and default
C locale. It returns an allocated current-directory name using a local mutable
native template and a genuine missing-leaf check, without creating or reserving
a file. Host TMP and RECON_MKTEMP controls do not supply this target namespace.
The caller owns the result and performs real open/close/remove operations;
generated spelling and original success error state are unmeasured. Other
profiles and the existing libc100 variant retain their behavior. Focused native
four-stream lifecycle controls complete cleanup. Subsequent ordinary APSTUDIO
completion also matches the full RES and strict inventory with no temporary-file
leaks. Generated names and original per-call state remain unmeasured.

Registered message4506/lang409 has a separate explicit510 fixed-template branch
for the Mac cursor warning. Its inserts are an ANSI filename, signed32 line and
ASCII WCHAR resource name, under default-C/MB1 state. The complete spans (at most
259 ANSI filename bytes and256 WCHAR name units), output capacity and disjointness
are checked before any write. Output preserves the exact warning/CRLF, count and
NUL; no other wide insert or template grammar is admitted. The ordinary complete
cursor case compares the RSC, both4506/4509 warnings and strict final inventory.
Original per-call output/error/FP state was not observed separately.

Explicit510 sprintf now has a typed cdecl native boundary. Its exact diagnostic
prefix "%ws(%d) : " accepts an aligned ASCII WCHAR filename of at most256 units,
signed32 line and caller-owned at least273-byte output under default-C/MB1 state.
All format/input/output bounds and disjointness checks precede writes. It reuses
the integer backend, returns the character count and appends NUL; the RCDLL caller
uses its own contextual512-byte diagnostic owner. Ordinary narrow formats keep
their previous host forwarding behavior. Other Windows wide string/character
forms, including alternate spellings/modifiers, deny instead of reaching host
WCHAR32 formatting. Other profiles keep their prior unmapped sprintf spelling;
direct use of the new boundary there denies. Original per-call state is unmeasured.

Registered message4067/lang409 has a separate explicit510 fixed-template branch
for the directive/newline warning, with one aligned ASCII WCHAR insert of at
most256 units. Default-C/MB1 and the exact authenticated template are required;
all insert/output spans, capacity and overlaps are checked before output. The
exact text/CRLF/count/NUL are retained. Other messages, wide content and grammars
keep their previous boundaries. Original per-call state remains unmeasured.

Registered2151/lang409 has a separate explicit510 fixed-template branch for the
duplicate-string error. Its five slots are an ANSI filename, signed32 line,
zero-extended WORD ordinal and two ASCII WCHAR strings of at most255 units. The
ordinal's decimal and unpadded uppercase-hex representations are reused twice,
matching the template; the WORD range makes the signed decimal interpretation
equivalent. Default-C/MB1, exact registration and complete prewrite span/capacity/
disjointness checks are required. The ordinary complete case matches its186-byte
diagnostic, exit1 and empty final inventory. Other grammars remain unsupported;
original per-call output/error/FP state was not observed separately.

Watcom's bare strrev maps to a typed shared byte-reversal boundary for the
identified53-byte provider. Caller-owned mutable NUL-terminated strings of at
most255 bytes are supported, matching the byte-sized little_endian domain.
The complete bounded terminator check precedes swaps; the same pointer, NUL and
tails are retained without locale, allocation or I/O effects. These checks do
not establish general memory ownership. NULL/wrap/long/unterminated inputs and
direct other-profile use deny125; other profiles have no new bare alias. WASM's
ordinary first little_endian pair qualifies typing and its width1 bypass only;
strrev execution remains unobserved there. Original per-call state is unmeasured.

Watcom strtod has a finite exact-decimal branch: default-C, endptrNULL, unsigned
plain ASCII with at most17 digits and one optional dot. Integer coefficient
reduction admits only exact dyadic values whose reduced significand fits53 bits;
IEEE binary64 normal/+0 bits are assembled without host parsing or rounding.
Signs, exponents, whitespace, nonexact values and other domains deny125. The
result uses the ordinary native typed-double ABI; no Watcom return-register or
private x87-state equivalence is claimed. Focused exact arithmetic/bit-pattern,
error/control-state and denial checks pass. WASM's ordinary conversion prefix
now executes through the restored original CRT closure and matches the next App
frontiers. The subsequent ordinary step131 retains genuine pre-run source/object/
DSO identities. This pass0 path does not yet compare emitted floating bytes;
original per-call error/FP state remains unmeasured.

Watcom __ZBuf2F and _Scale10V reuse that exact-decimal backend. The former
admits1..17 unsigned ASCII digits and stores an exact coefficient into caller-
owned aligned4B/disjoint double storage. The latter accepts a nonnegative
integral binary64 coefficient with at most17 decimal digits and exponent
-17..0; only exact dyadic results are admitted. Complete input/default-C/span
checks precede the store. Unknown domains/profiles deny125. The native typed
ABI and caller ownership are explicit; no general old-runtime rounding or
private-register equivalence is claimed. Six focused profile/compiler groups
cover exact generic vectors, prewrite denials and independent errors/semantic
FP state. The ordinary WCC conversion pair now matches through CFFree after
the separate formatter exponent-width correction; its source/DSO binding is
contemporaneous post-run, with no invented pre-run hash claim.

The opt-in RECON_WATCOM_SPRINTF_17E maps sprintf only in selected Watcom source
files. Exact %.17e/default-C accepts finite dyadics whose exact decimal
coefficient fits18 digits, with binary exponent reduction bounded below by
-17; negative zero and other domains deny125. Private host formatting therefore
needs no rounding, and the result receives Watcom's minimum3-digit exponent.
Caller-owned output>=26B is an explicit precondition (actual WCC Buffer528B),
with full format/output span and overlap checks before writing. Other formats,
TUs and profiles retain their mappings. Focused output/count/NUL/tails, generic
positive/negative dyadic vectors, denials and independent error/semantic FP
checks pass; the ordinary WCC conversion pair consumes this formatter. No
general old-runtime rounding or original per-call FP/error equivalence is claimed.

Typed native diagnostic units additionally qualify five explicit510 fixed
registered layouts:2104/2175 use ANSI filename,signed line,ASCII WCHAR name;
2135 uses an ANSI third insert;1003 consumes one signed integer;20101 consumes
no inserts. Exact templates/default-C, bounded spans, capacity and disjointness
precede writes; count/NUL/tails and independent error/semantic FP state are
checked. No general message grammar or original per-call state is claimed.
The canonical RCDLL module's actual full219-entry table has no10056 message.
For its exact flags0xa00/lang0/NULL-arguments/4096-byte-output tuple, failure
returns0 with LastError317. Native output is untouched; original failure-buffer
contents remain unmeasured. rc_message consumes only0 and selects literal empty
text, which then initializes diagnostic storage. Unknown IDs/owners stay125.

Watcom ultoa admits unsigned32 decimal/radix10 and lowercase radix31/36 through
existing recon_ultoa. The caller owns at least11 writable bytes for decimal or8
for31/36; the unchanged conservative12B arithmetic nonwrap check neither detects
capacity nor accesses12 bytes. NULL/wrap/other radices/direct other-profile use
deny125. The original pointer is returned, with at most ten decimal or seven
base31/base36 digits plus NUL. Focused protected-page controls prove the written
maxima. WPP prependLen supplies16B; its actual length9 caller prefix matches a
saved original, while focused letter/carry/max vectors qualify base36 outputs.
WPP appendDelta supplies15B at radix31; same-provider alphabet/divide/reverse
semantics and focused4/u/carry/max vectors are bound, ordinary adoption pending.
Prior utoa decimal/
hexadecimal domains remain unchanged. Focused vectors/tails/error slots and
semantic x87 control/status/tag/live values pass; incidental instruction/data
pointers and empty registers are deliberately excluded, not equated bytewise.
Ordinary WLINK MemorySize and FreeAMod continuations compare PE/map output after
the real conversion. The current FreeAMod source/DSO binding is retained; it is
not retroactively attributed to the overwritten MemorySize DSO. Original
per-call private error/FP effects remain unmeasured.

Watcom _expand/_nexpand have the SDK void* cdecl(void*,size_t) surface and
profile-only aliases to one shared failure-only boundary. The caller supplies
the still-live base of a genuine host malloc block; pointer arithmetic does not
establish ownership or lifetime. Positive nonwrapping requests use the existing
public malloc_usable_size backend. Only a real capacity shortfall returnsNULL,
retaining the old allocation, bytes and independent native errors/semantic FP.
Requests within current capacity (success/shrink), zero/invalid spans and other
profiles deny125. No relocating realloc, free, private heap headers or new
allocation registry are used. This is a native capacity policy, not blanket
old-heap equivalence. WLINK's actual16392-byte allocation has16396 usable bytes;
its16408-byte request follows the original NULL fallback then real realloc
shrink. The ordinary joint pair matches325 entries and retains contemporaneous
source/DSO binding. Original per-call private error/FP effects remain unmeasured.

CopyFileA has the SDK BOOL WINAPI(LPCSTR,LPCSTR,BOOL) type. Explicit MSVCRT510
admits FALSE with bounded ASCII mapped paths, a stable normal readable/writable
regular source <=INT32_MAX bytes, and a distinct existing empty writable regular
destination. The caller owns that closed CREATE_NEW reservation and guarantees
stable path/content lifetimes; the API does not discover exclusive ownership.
Path kinds and opened file identities/kinds/size/permission bits are checked
before content changes, including same-inode aliases. Nonempty/readonly,
symlink/device/FIFO, other flags/profiles and unknown domains remain125.
The real16KiB copy loop handles EINTR and short writes, copies basic source
atime/mtime and checks both closes; no extracted/temporary companion is created.
Failures use the existing host-error-to-Win32 mapping. Native CRT errno/DOSerrno
are retained independently, and native success retains prior LastError; original
per-call success/error/buffer state is unmeasured. Security descriptors, alternate
streams, creation times and readonly attribute copying are not implemented.
The actual LINK_REPRO caller reserves/closes the destination and consumes the
copy result and real bytes. Its ordinary progression validates four actual copies
and the caller-owned collision retry; full repro-close remains later App logic.
make copyfile-check runs the isolated focused probe, including injected IO/time/
close failures, prewrite denials and semantic x87 control/status retention.

Canonical native entry foundation
--------------------------------
recon_register_module retains identity-only registration. Ordinary static
bundles then call recon_configure_static_module with native export pointers,
an actual BOOL WINAPI(HINSTANCE,DWORD,LPVOID) entry, and registered dependency
handles. NULL export names permit ordinal-only entries. GetProcAddress returns
the unmodified pointer; callers must restore the real function signature/ABI.
The initial bound remains32 modules,16 exports and16 dependencies per module.
Configuration is startup-only, before concurrent library operations.

LoadLibrary accepts configured names (optional .dll) or exact normalized paths,
uses reference counts, attaches dependencies first, and supports repeat lookup,
load/free and reattachment. Unknown modules/exports return126/127; failed attach
returns1114, sends failed-module detach, and releases newly acquired dependency
references. Entry callbacks run under a recursive process loader mutex; cycles
or reentrant initialization return1114. It is not a PE/TLS loader or a general
Windows loader-lock emulator. Native pthread creation does not synthesize DLL
thread notifications. Existing RECON_MODULE_NO_TLS remains required.

recon_attach_static_module acquires one process-lifetime startup reference
(idempotently) with nonNULL attach-reserved; ordinary loads use NULL. Final
FreeLibrary uses NULL detach-reserved. Process termination detaches in reverse
attachment order with nonNULL reserved, including App exit and wrapper return.
ExitProcess detaches explicitly before its immediate exit. Early host atexit
registration lets App callbacks run first; a final shared-CRT flush covers late
DLL diagnostics. _exit, abort and fatal signals do not run ordinary cleanup.
Static bundles remain physically resident. An optional writable_region in
ReconStaticModuleSpec selects one ReconModuleWritableRegion {address,bytes}.
Configuration copies that descriptor and snapshots the initial relocated native
bytes once into a process-lifetime buffer. Each fresh unloaded-to-attaching
transition restores the span with memcpy before acquiring dependencies and
calling entry. This includes the first attachment and retry after failed attach.
Repeated loads while references remain do not restore it. Final detach/failed
attach cleanup sees mutated state; bytes remain dirty until the next attachment.
NULL preserves the existing resident-state behavior. Dependency counts, loader
metadata, shared CRT/Win32 state and fixture observers remain outside the span.

The caller/linker must select the entire DLL-owned writable data/BSS interval,
including alignment padding, excluding shared runtime and other module storage.
The API checks positive <=INT32_MAX length, nonwrapping nonlow address, and no
overlap with previously configured module regions; it cannot discover ownership,
readability/writability or missing globals. Invalid regions fail87; snapshot
allocation failure returns8. The initial snapshot is native post-relocation
state, not original PE data, a disk reload, COW, or App-specific reset logic.
Native ELF constructor ordering is an explicit adoption prerequisite: registration
must run after relocations and before any DLL initialization (including automatic
constructors). This API does not intercept constructors already run by ELF startup.
The build must arrange that ordering or qualify a module with no such constructors.
No-region bundles must establish their own native state on reattachment.

Fixture observer/lease providers retain all strict inventory/error assertions
and cannot be mixed with ordinary static-module providers in one process.
This makes the policy choice explicit without weakening cached fixture leases.
Win32 LastError is thread-local. Module/handle/resource/mapping state is common
process state. Compile/bind one coherent Win32 shim into the process, exclude
bundle shim duplicates, and preserve each App object's flags. This API neither
chooses CRT ownership nor merges incompatible CRT profiles. Shared CRT imports
must use one compatible runtime; separately linked CRT code/data must retain
its separate owner and bindings. In particular, canonical VC5 RC must select a
compatible common profile with RCDLL; wchar/jmp object flags are independent.
The first canonical VC5 pairings select MSVCRT510 and startup-owned fmode0;
no DLL requires a separate fallback4000 copy. This foundation leaves existing
profile defaults untouched and tests the canonical510 configuration explicitly.

recon_register_embedded_file(path,data,size) snapshots a readable byte span into
a chmod0444 sealed Linux memfd, without extracted or temporary filesystem files.
Each open reopens the backing descriptor read-only through /proc/self/fd for an
independent offset; dup would share offsets. Required Linux memfd/sealing and
procfs facilities must exist: there is no disk extraction fallback. Size is
bounded by INT32_MAX. Registration is startup-only and process-lifetime; duplicate
normalized paths fail183. Keys are ASCII Windows drive-absolute paths (or native
absolute Z: paths), with case-insensitive slash/dot-component normalization.
Relative keys use RECON_CANONICAL_CWD when supplied, otherwise actual host cwd
on Z:. No drive mounts are needed just to resolve registered virtual paths.
UNC, drive-relative and nonASCII registrations are not supported. No basename
redirection, reserved-name or trailing-dot/space alias engine is introduced.

CreateFile/open/fopen reuse existing HANDLE/CRT descriptor/FILE handling, with
normal binary/text reads, seeks, EOF and size queries. access/stat/fstat and
GetFileAttributes include registered assets. Writes/truncation/delete/rename
are rejected; unregistered paths keep existing filesystem behavior and existing
unsupported tuple boundaries. Configured module paths also provide read-only
identity/existence probes (zero-size virtual metadata); no DLL image bytes are
invented. Opening module image contents requires an explicit embedded container.

recon_load_module_resources_span(module,data,size) borrows an immutable span
for the module's process lifetime. It reuses the existing RMR1 identity-bound
parser and its ASCII/language409/kind/count/size scope. It does not copy PE or
resource sections or accept a broader resource grammar. The existing filename
loader also recognizes registered embedded containers before its POSIX open.
The native entry owns resource lifetime and explicit logical-path registrations.

Focused native-only checks: tests/entry-foundation/check.py builds authored
consumers using GCC default/MSVCRT510 and Clang Watcom configurations, exercises
two independently compiled module users, dependency/reference/attach-failure/
termination behavior, TLS LastError, immutable embedded descriptors and existing
resource parsing. Authored linker-selected data/BSS checks cover relocated
pointers/padding, repeated references, final-detach visibility, dependency writes,
failed-attach retry, pinned modules and region-validation rejection. The existing strict fixture module-provider checker is run
separately. These checks do not launch Wine or inspect/rewrite original captures.


Ordinary canonical process bootstrap
-----------------------------------
ReconNativeEntry *recon_native_entry_initialize(argc,argv,envp) prepares actual
native process inputs for a thin root entry.c. It lives in existing crt.c, not
the optional generated-main object. It returns argc, writable owned argv and
environment, plus process-lifetime program_path/current_directory/command_line.
The actual /proc/self/exe and initial getcwd paths become Z: identities. _pgmptr
and _aenvptr are refreshed after constructors; GetCommandLineA owns a lossless
Windows spelling of the supplied direct POSIX arguments. Direct argv is not
retokenized. Current paths/arguments are bounded ASCII; non-ASCII arguments are
rejected, not claimed supported by selecting ACP1252. Environment bytes retain
their exact narrow values with reversible ACP1252 UTF16 OS representation.

This is one fresh ordinary process binding, not a configurable startup or CRT
context framework. It preserves binary-selected fmode/FP and strong CRT roles,
does not call App startup or register primary-module identity, and must not run
at DLL attachment. The caller registers its actual primary path/original resource
identity and embedded assets as appropriate, then invokes its real App entry.
Success initializes fresh target errno/DOSerrno/LastError to zero after host-only
setup. Writable CRT environment and immutable initial OS namespace are independent;
ordinary NULL-env spawning inherits the OS block for LIBC100 and MSVCRT510 alike,
including empty environments and children carrying only MSC_CMD_FLAGS. Fixture
provider-qualified binders and broker controls retain their separate old policy.
Canonical paths use actual Z: mapping; fixture RECON_DRIVE/CANONICAL_CWD settings
are not needed. GetACP has the explicitly selected ordinary native1252 profile.

Owned process inputs stay live until OS process reclamation, including App exit
and final static-DLL detach; no main-registered atexit free precedes DLL cleanup.
Existing CRT/module atexit handling supplies normal cleanup. Ordinary full-DWORD/signal child-status equivalence remains a separate boundary;
basic startup alone does not claim Windows child command-line semantics. The native-only
native-bootstrap-check target covers default/510/Watcom profiles, real minimal/
empty/host environments, direct quoting, independent OS inheritance, actual Z:
identities and final detach input lifetime. No originals or fixture controllers.


Ordinary Windows child command transport (protocol v1)
-----------------------------------------------------
An entry that consumes the shared bootstrap may explicitly opt in with root-only
NATIVE_BIN_ENTRY_CPPFLAGS += -DRECON_NATIVE_ENTRY_PROTOCOL_V1. This emits one
readonly .recon.entry protocol-v1 marker from recon.h. Do not define it on common
support/App objects; a marked root must call initialize before invoking App code.
The initializer remains an ordinary C function. The marker declares a compiled
argument protocol, not original identity/provider proof or a DLL/resource image.
Markerless basic executables still run unchanged and get no transport descriptor.

Only ordinary LIBC100/MSVCRT510 spawning to a marked native ELF32 uses the
protocol. The parent checks bounded section metadata and requires one exact v1
marker (duplicates/wrong version are rejected). The actual old _cenvarg/_dospawn
rule joins argv strings verbatim with one space; it adds no quoting, including
argv0. Quotes supplied by the caller are preserved. The resulting bounded ASCII
line plus NUL is placed in a sealed memfd named recon-windows-command-line.
There is no environment entry, controller, identity proof or runtime state packet.

Descriptor198 is reserved only for this transfer. An occupied parent slot fails
with EBUSY without clobbering it; the slot must be within the native fd limit.
The parent owner is CLOEXEC and closed after fork/error. Only the child duplicates
the sealed input to198 for exec. Bootstrap checks its exact memfd name/type/seals/
size, consumes and closes it before App. It leaves an unrelated direct-launch198
untouched and performs no descriptor scan. Unknown native programs get no extra
fd and keep existing POSIX argv behavior. The declared consumer is responsible
for calling bootstrap; no inference from a filename or adjacent executable occurs.

The child applies the old RCPP CRT argv0/backslash/quote grammar, including its
legacy doubled-quote toggle, and exposes the exact raw GetCommandLineA bytes.
Direct shell argv remains the supplied vector with lossless Windows serialization.
This supports real RC's two-element argv containing an option bag while preserving
ordinary quoted direct invocation. Target OS/CRT environments remain independent
and the child's actual minimal environment is unmodified. Watcom and fixture
spawning retain their prior behavior. Full-DWORD/signal status and general Windows
handle/CRT-private inheritance are not provided by this argument-only protocol.

native-command-check exercises real authored children, old quoting/argv0, empty
arguments, minimal environment, occupied/unrelated198, invalid seals, close-before-
App, repeated parent descriptor ownership and unmarked/native basic compatibility.
Actual RC/RCPP consumes the same slice with strict rich resource output and cleanup;
its old source argument/runtime rules are retained, without an original API probe.

Watcom alloca has a TU-selected caller-frame400-byte native surface under
RECON_WATCOM_ALLOCA_400. Unselected host alloca remains unchanged. The macro
evaluates its size once and calls __builtin_alloca in the actual App caller
only when a shared helper finds sufficient real downward-growing stack space.
Validated pthread stack/guard bounds are cached per thread; first query requires
adequate ordinary call stack, and later stack-limit/guard mutation is unsupported.
Each permission check uses a current in-stack local probe, conservatively below
the caller with native alignment padding. Insufficient space returns NULL;
unknown bounds, other sizes and direct other-profile helper calls deny125.
Returned storage lives only until its caller returns. The actual check_recurse
consumer uses only zero/nonzero, not the pointer's address or bytes. The ordinary
471-entry WLINK continuation to recursive_mangled_name matches (native32.618ms),
qualifying its normal non-NULL path with exact source/DSO binding. This does not claim original private stack layout,
hard overflow protection or original low-stack behavior. Focused authored main
and pthread controls verify400 writable bytes, canaries, one size evaluation,
independent errors/semantic FP and a genuinely low-stack NULL path.

Watcom existing-file RW open additionally admits exact0202 (O_RDWR|O_BINARY)
for a real existing regular relative ASCII path, under the existing bounded
component/lstat checks. It calls the same descriptor backend with8002 and never
fetches a variadic permission argument; no CREATE/TRUNC/exclusive semantics are
introduced. Missing paths and other failures remain unsupported for this tuple,
separately from the prior readonly ENOENT behavior. Success retains target errno
and normal binary bytes, initial cursor0 and independent descriptor ownership.
Native success LastError clearing comes from the existing backend; original
per-call error values are unmeasured. Focused two-argument and unused-third-arg
controls cover raw read/write, independent cursors, close, unchanged tails and
retained readonly/create domains. WLINK's real existing-file native units pass
(three groups19.877ms; direct TempFileOpen19.541ms), with source/DSO binding and
owned close/reopen checks. This is source/static/native-unit support, not
original-runtime equivalence.
