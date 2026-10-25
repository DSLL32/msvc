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

LocalAlloc/LocalFree support the encountered fixed LMEM_ZEROINIT positive-size
allocation and same-owner successful free. Every requested byte is zeroed;
native_local_memory records base, requested size and flags for typed inspection.
Successful free returns NULL and both calls preserve observed errno/LastError.
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
Fixed-base or partial-view requests remain explicitly unsupported.

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

C++ import allocation ownership
------------------------------
Distinct operator-new/delete imports use host-only recon_cpp_allocations records
(base, requested bytes, rounded malloc request, generation, live state). Successful
delete and ordinary free retire a live record; successful realloc retires old-new
ownership, failure retains it. Tracked realloc(size0), NULL/foreign/interior/dead
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
surfaces to native target translation units. _heapchk, _nheapchk, signal, raise,
_setjmp/setjmp and longjmp all deny with status125; no heap result, registration,
delivery or context-save/restore behavior is claimed. SIGNAL.H numbers1..12 and
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
support normalreturn0 only. The macro invokes real libc _setjmp directly in
the live application frame; the existing registry keeps an opaque host context
out of line and marks it normal-only. Windows'48-byte serialized register/FS
context and native host context are private differing CRT representations.
No captured Windows context is copied into native storage: all52 target bytes,
including finalDWORD/adjacent guards, remain their own native bytes. The actual
reached application uses only the owner address and result, not context fields.
Indirect/bare typed calls, pointer-sized/truncated/NULL owners, every Watcom
longjmp and generic recon_longjmp under this profile remain unsupported. This
adds no SEH/unwind or resumed-return semantics. Watcom application reconstruction
is paused after this bounded plumbing proof for user review and assigned cases.

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
The native new.h subset supplies only caller-owned placement construction and
a typed cdecl allocating-new declaration bound to existing recon_cpp_new. It
adds no allocator wrapper, delete form, exception policy or libstdc++ dependency.

The native compiler-support __cxa_pure_virtual(void) symbol has C linkage and
never returns: it reports unsupported invocation through the existing exit125
boundary. This permits abstract vtable references without a libstdc++ dependency;
it claims no original purecall diagnostic, success semantics or App body credit.
