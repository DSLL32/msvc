Proposed single case: nested include/error/resource ownership

Read-only assessment after 6e34c724e. No capture, build, reconstruction source or
shared implementation change was made. All concrete statuses below still need
untouched original observation before becoming oracles.

Source: test.c includes resource1.h -> resource2.h -> resource3.h -> resource4.h.
Each guarded header contributes a real record/type/macro; resource4.h defines a
small callback-driven record traversal. Its RESOURCE_MISSING conditional includes
"absent-resource.h", which is deliberately absent. test.c uses the resulting
record/callback declarations in an ordinary valid function. Use genuine existing
frontend flags, -Ze -Zp8 -Gs -W3 and -Fr browser output, retaining all five IR
artifacts, stdout/stderr, and browser file state. Prefer relative case paths.

Two public phases: RESOURCE_MISSING=0 completes normally; RESOURCE_MISSING=1
reaches the actual report_fatal(83,message_text(343),failed_filename) after the
natural _sopen failure. Freeze status/streams/complete outputs from the original.
This error phase does NOT publicly reach any of the four proposed new bodies.

Use that real report_fatal entry as the one direct-phase marker. It is unique to
this deliberate missing include, so no conditional breakpoint/stepping scheme is
needed. Its actual five-node active_input chain has include_depth=4, genuine
live descriptors, and errno=ENOENT from the failed _sopen. Check exact owner
records/next links/filenames/descriptor relations and input_kind before calls.
The error wrapper's own stack supplies code/kind/failed filename; source line
and current header filename come from existing parsed state. Unknown state fails.

Four independent fixed direct phases of the same case:

1. retry_malloc(96): ordinary successful allocation, one stdcall argument and
RET4. Capture returned allocation ownership and unchanged compiler state; the
native observer may write/read its own returned allocation, but do not compare
uninitialized allocator bytes. Return/nonvol/x87/canary proof; external unit ends
99 as existing direct units do. No OOM/fault injection or retry-failure claim.

2. release_input_handle(): no arguments, cdecl, int result. With depth4 and all
ancestor descriptors live, it walks to depth2, closes exactly that ancestor's
actual descriptor, sets its handle=-1, leaves all other nodes/descriptors and
buffers unchanged, and reports close success. Retain actual pre/post close
result/errors plus all chain records. No EMFILE injection or public producer
claim. No need to resume the parent parser after this explicitly direct effect.
The existing source declaration void is provisional and must become int only
when fresh actual entry/body proof accepts the whole 80-byte function.

3. report_error(code,kind,filename,source,line): five fixed cdecl arguments,
not varargs. Two possible explicit invocations for root's preference:
(a) preserve actual code83/kind/filename, supply real active header name and line;
the code83 diagnostic consumes only its three string fields but the wrapper's
five input slots and six forwarded fatal slots can all be observed;
(b) request code85 explicitly with those same real values so formatting consumes
source/line too. This is an explicitly requested direct write-error diagnostic,
NOT evidence of an actual failed write. Do not claim either is a public call.
Preferred minimal proof is (a), plus direct forwarding-slot observation; avoid
inventing a write failure or forcing FILE flags. Fatal cleanup/exit makes it
nonreturning in this actual phase. Freeze actual original status and outputs.
Optional kind/filename NULL fallback is ordinary API input but not essential.

4. compiler_signal(2): direct C handler call with ordinary SIGINT numeric argument,
without sending an OS signal/console Ctrl-C. It calls accepted
cleanup_failed_compilation (cleanup_precompiled_files then conditional
browser discard) and C exit(0x8000). Observe actual browser cleanup/removal and
pending IR/stdio flushing, complete files/streams and full terminal DWORD.
No canary/return claim for a process that terminates; actual entry/argument,
cleanup edge, exit argument and terminal receipt are the meaningful proof.

Critical corrected planning error: the handler calls original0x53530 COFF `_exit`,
which is source C `exit`; original0x53550 COFF `__exit` is source C `_exit`.
53530 pushes quick=0 into doexit53590;53550 pushes quick=1. doexit quick0 executes
the actual onexit list and C termination range, then common final termination;
quick1 bypasses those. STDLIB.H159/195 and PROCESS.H76/77 declare both distinct
cdecl void(int) functions. LIBC.LIB startup\\obj\\i386\\CRT0DAT.OBJ matches are
recorded in runtime.json. The earlier nonflushing/no-onexit assertion was wrong.
Keep source exit(0x8000), existing recon_exit and ordinary normal-flush semantics.
No new shared _exit API or target source substitution is needed.

Reuse lib/recon/status.py TerminalStatus with RECON_STATUS_FD/pass_fds for native
full DWORD32768, check POSIX returncode0 and receipt code0x8000. The current
original debugger already records EXIT_PROCESS_DEBUG_EVENT's DWORD. Keep the
host descriptor outside target environment/input namespace as existing support
does. Reuse existing shared onexit-probe.c/onexit-original.json/check-onexit.py
and status-target/check-status.py as canonical boundary proof; no new standalone
process/IPC framework or additional CRT reconstruction. Actual compiler pending
streams and cleanup effects must also be compared in the handler phase.

Static target extents: release1b5d0..1b62080B; retry4a6b0..4a6df47B;
report4aa40..4aa8367B; signal43ea0..43eb319B. At most four whole application
bodies213B if fresh evidence proves them. No acceptance/coverage credit yet.
Existing bounded direct launcher already has seven argument slots, sufficient
for five cdecl arguments; add only fixed context/snapshot selection and ordinary
noreturn terminal handling. No general stepper/marshaller. Cache native-only
fixtures, normal original-compiler -j4 check and graph/ABI audit after acceptance.

Based-pointer validation131B stays outside this proposal: needs a genuine parsed
self-based token producer; no token/global seeding. Unlisted ownership ambiguities
and unknown bodies remain unresolved, not CRT.
