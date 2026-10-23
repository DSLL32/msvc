# First debugger transport probe

The four initial authored-child controls pass in 3.010s. This does not authorize
original CF execution or application acceptance; the remaining gates below stay
unproved. Source, plans, actual contexts and raw logs are retained under
`probes/win32-gate/`, including the two failed runtime attempts.

The first child is `tests/win32-gate-child.c`, ordinary authored C with a
custom PE entry and no CRT. Compile it with the original WCC `-bt=nt -3r -od
-d2 -s -zl`; link without a CRT library. It calls
one leaf that writes a declared DWORD marker, returns, updates the marker and
loops. An unused public anchor bounds the final function. The actual MAP and
disassembly confirm those claims; unexpected imports/helpers, entry,
instruction effects or function boundaries stop the probe before execution.
The broker is external tooling built with the original compiler and NT headers,
CLIB3R/NT libraries. Its own ABI is not application equivalence evidence.
Authored capture tools may use another compiler; original-toolchain verification
is required for reconstructed targets.

Old WLINK rejects `option start` for PE; the first failed link is retained.
The corrected link emits a no-import/no-TLS child and defaults entry to the
first function (`probe_leaf_`). A separate authored test copy changes only its
AddressOfEntryPoint field from RVA10000 to the MAP/body-verified `probe_entry_`
RVA1001f. Only file byte152 differs; all sections/code remain unchanged. The
reviewed receipt is scratch `broker-build2/entry-patch.json`, SHA256
`dd630a4daa4f0d2a617e8a1d7c808578a92b18d9be88ba7cef3cca66a1fca469`.
This is test-entry metadata, not an original-app or reconstruction patch.

Each attempt binds the complete source, actual argv/environment, child/helper
PE and MAP hashes, immutable input bytes, raw binary stdout/stderr files and
measured compile/link/setup/execution/check costs. Full environment values stay
in a private 0600 scratch file. Use the shared repo prefix and S: scratch files;
never create/remap a drive or stop a server. Retain failed attempts separately.

## Consumed state

The Python controller validates the exact PE file, `probe_entry_`, `probe_leaf_`,
`probe_end_` and `_probe_marker` through its actual MAP, entry RVA and bytes.
There is no Python whole-image CFG walker: this tiny authored probe uses explicit
reviewed instruction ownership. Production CF ownership/discovery remains in
the common Rust auditor, including its Watcom MAP reader.

At CREATE_PROCESS, derive the actual image base and entry, require the bound
entry RVA, install and read back its expected-byte INT3, then permit only the
initial OS-loader sequence. DLL events are returned. At most one positively
identified initial ntdll first-chance loader breakpoint may continue. A target
TLS directory or unexpected event is unsupported. No target instruction before
the entry trap and no payload/CRT prefix is approved by this startup phase.

At the owned entry breakpoint retain actual exception address, first-chance
status, process/thread IDs and full raw GP/segment/EFlags context. Restore only
the saved trap byte and rewind only its checked EIP+1, then read back the exact
context with that single EIP change. Observe CS/SS/DS/ES descriptors and prove
their flat base/limits. FS/GS remain unsupported by the gate. Read actual TEB
stack bounds and committed writable memory-region attributes before declaring
implicit stack-word ownership. The marker is exactly its declared DWORD;
allocation/image bounds do not imply arbitrary object capacity.

## First controls

1. Step the actual entry PUSH, with exact ARM readback TF=1 and all unrelated
   raw context unchanged. Require the same thread's first-chance DB, predicted
   next PC/SP/CS, real pushed value and consumed TF=0. All other flags, GP and
   segment values must remain exact for that PUSH. Marker remains zero.
2. With only caller instructions owned, stop BEFORE the unknown leaf CALL.
   Check and install its target trap, execute only the real CALL under TF,
   prove the actual target/SP/CS and pushed return word, and retain target CC.
   Marker remains zero. This is actual callee entry, never a fabricated frame.
3. In a fresh authored child, deliberately add the separately reviewed leaf
   instructions to ownership and follow the SAME path. Require marker one,
   the genuine RET and shadow-frame cleanup, then caller continuation. This
   bypass demonstrates that the stopped path really would have changed the
   marker; it grants no application credit.
4. Supply a deliberately wrong expected trap prebyte. The broker must reject
   it before any target application instruction and terminate/drain its child.

This initial set does not satisfy the later indirect/return/callback/thread,
missed-trap/flush, relocation or full-image corruption negative requirements.
Those remain closed before CF application capture/acceptance. No generic OS
service or callback free-run is added to make these controls pass.

## Execution and teardown

The broker accepts bounded event/context/selector/region/read operations,
checked one-byte traps/restores, owned-trap EIP rewind, TF-only ARM/readback,
exactly one instruction step, initial-loader continuation and owned teardown.
No arbitrary register/data write RPC exists. Instructions, branches and CALLs
remain under `gate.py`; it supplies predicted transitions before execution.
The broker returns raw contexts, never ABI-normalized registers or flags.

Use one outstanding command, the proved shared-map sequence/lease channel,
global helper deadline, event/step caps, per-wait quantum and an independent
outer timeout. Unknown exceptions, second threads/processes, callbacks, foreign
returns or malformed commands HOLD. An unexpected debug child remains stopped
and is terminated before its creation event may continue during drain.

Before stopping, read the marker while the child is held. Terminate only owned
processes; drain actual EXIT_PROCESS events, require signaled process handles,
then collect actual broker exit. A killed broker without owned child-exit proof
is failed evidence. Binary target streams remain raw. This first probe neither
launches an original Watcom application nor counts reconstructed functions.
