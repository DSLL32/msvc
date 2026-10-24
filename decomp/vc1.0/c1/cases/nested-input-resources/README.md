This one case combines a guarded four-header include chain, records, callbacks
and a traversal function. Both actual public phases are frozen: the valid phase
exits zero and produces the browser file plus all five IR files; the deliberate
missing include exits one with the exact C1083 diagnostic at resource4.h(12),
removes the staged browser file and leaves all five partial IR outputs. Native
status, streams, complete output bytes and fresh masked first-entry projections
match. Fixed input timestamps avoid browser timestamp variation.

Four separate direct phases stop at that real report_fatal(83, "include",
"absent-resource.h") entry. Its actual depth-four input stack has five live
SourceFile owners, initialized descriptors/buffers and genuine ENOENT. The
external launcher supplies ordinary call arguments on its existing bounded
stack; it does not seed compiler globals or parser/token state. These functions
are explicitly direct support; neither public phase claims to produce them.

| Function | Original extent | Bytes | Direct input/effect |
| --- | --- | ---: | --- |
| retry_malloc | 4a6b0..4a6df | 47 | Ordinary successful allocation of 96 bytes |
| release_input_handle | 1b5d0..1b620 | 80 | Close the genuine depth-two ancestor descriptor |
| report_error | 4aa40..4aa83 | 67 | Actual code83/kind/filename, current header and line12 |
| compiler_signal | 43ea0..43eb3 | 19 | Ordinary handler argument2; cleanup and normal exit32768 |

Each body has a fresh original/native pre-prologue entry pair, whole-body result
and a fresh accepted-prefix first-entry projection. Returning calls retain stack
cleanup, nonvolatile register, x87 and canary proofs. Allocation contents are not
read while uninitialized; no OOM is injected or failure retry coverage claimed.
Release changes exactly one owned descriptor/handle field and ends its direct
phase immediately. Its original integer result corrects the provisional void
source declaration. All other owner fields, links and buffers remain unchanged.

report_error has five fixed cdecl arguments, correcting the provisional varargs
declaration. Its actual six forwarded fatal arguments include the CRT error
reason followed by the real header/line. Those slots are observed in both
processes even though diagnostic83 consumes only its three string fields. No
failed-write scenario is invented. The actual fatal cleanup, status, streams,
pending data and complete owned files match.

A corrected planning error matters for compiler_signal: it calls original53530,
COFF `_exit`, which is C `exit` with quick=0. C `_exit` is the distinct COFF
`__exit` at53550, quick=1. The initial nonflushing/no-onexit assumption was wrong.
Shipped STDLIB.H/PROCESS.H declarations and the matched LIBC.LIB CRT0DAT.OBJ
provider establish both identities. The source preserves normal `exit(0x8000)`;
no shared API or CRT body is added. Existing recon_exit and TerminalStatus retain
full Win32 DWORD32768 beside native POSIX status0. Actual browser deletion,
pending IR at the real exit entry and fully flushed final files match. This
terminating phase makes no return/canary claim. Existing shared onexit/status
canonical proof is bound in validation.json; no console automation is involved.

check-resource-tests.py executes four instrumented native units against frozen
original hashes, invocation, inputs, entry order, defined owner state, pending
streams, full terminal status and all owned files. It never invokes Wine, GDB,
the original program or a reference refresh. GDB is used only for the separate
fresh acceptance ABI observations. Native runs take milliseconds each.

Generated masked EXEs are omitted from Git. The path/hash bindings and compact
mask recipe regenerate all fifteen images exactly, including the prelaunch
metadata-plan collision artifact; that failed transport attempt launched no
target process. Raw failures and successful observations remain distinguishable.
The missing native observer include was also fixed before entry observations.
No completed original phase was repeated merely for packaging.

Four application functions add213 bytes. There are964 accepted application
functions,227565 application bytes and250303/277234 total accepted extent bytes
(90.28582%, including CRT/imports). Public accepted extents remain245156; explicit
direct support accounts for5147. Extent coverage is not branch coverage or a
complete non-CRT denominator. The eleven historical source CRT bodies are
unchanged. Self-based token validation and unlisted ownership-ambiguous/candidate
bodies remain unresolved, not classified as CRT. Final timings/catalogs are in
validation.json.
