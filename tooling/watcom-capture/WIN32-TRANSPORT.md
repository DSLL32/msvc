# Proposed Win32 transport

This is a source/probe design, not a passing Win32 capture protocol. Application
acceptance stays closed until the real Win32-child negatives pass. The gate's
native-child tests do not prove Wine debug-event behavior.

## Scope and connection

Keep `gate.py` as the runtime policy/decoder. A small original-compiled Win32
helper supplies only public debug-event, process-memory and thread-context
operations. Reuse reviewed patterns from WLIB's diagnostic helper, without
extending or silently relabeling that diagnostic's scope. No CPU emulation,
whole-image Python CFG walker, arbitrary register/data seeding, or OS free-run
service list is introduced.

Prefer a fixed-size scratch-file mapping for one outstanding command/response.
Python and the helper map the same local file through their normal APIs. This
avoids blocking inherited RPC pipes and keeps command handling under explicit
deadlines. Use aligned little-endian sequence/length/status DWORDs and bounded
payloads; reject stale, skipped, duplicate or oversized commands. Publish a reply
only after its complete payload, using InterlockedExchange on the Win32 side.
The parent waits for exactly its expected sequence before consuming a reply.
Record mapping format/hash, command sequence, budgets and measured costs.

Both publication words must be aligned actual atomic DWORD accesses. Python
uses `ctypes.c_uint32.from_buffer(mapping, aligned_offset).value` for sequence
loads/stores, never struct.pack_into/unpack for synchronization. Check actual
alignment and restrict this protocol to native x86 hosts. Win32 reads a volatile
aligned DWORD and publishes through InterlockedExchange. Payload writes precede
publication; the consumer reads only after the exact expected sequence, verifies
stable sequence/length/status, and acknowledges before reuse. Reject sequence
wrap rather than using modular ambiguity. x86 shared MAP coherence/store order
is a protocol assumption to test, not a claim of cross-architecture portability.

The proposed version-1 map is 16384 bytes: magic/version/map-size at offsets
0/4/8; command/reply/owner-lease publication DWORDs at 12/16/20; command opcode
and length at 24/28; response status and length at 32/36. The shared payload is
at offset 256, limited to 4096 bytes. No other fields are currently defined.
Sequences start at zero, advance by exactly one, and stop before wrap (limit
1000000). The initial heartbeat is one. An unchanged current sequence means no
new message; it cannot distinguish a redundant store of the same DWORD. An
older or skipped published sequence is rejected. A successful next command
acknowledges the prior response; the peer never rewrites a published response
until that command arrives. The sender copies its request before publication;
the peer copies it before overwriting the shared payload with its response.

`ipc.py` and `check-ipc.py` currently implement/probe only this native channel.
The authored `tests/ipc-peer.c` is an i386 Linux peer using GCC atomic accesses;
it has PING/EXIT probes and deliberate malformed-message modes. It is not a
Win32 helper, and its probe opcodes are not a production debug-command schema.
Native controls cover publication/coherence, partial unpublished replies,
replays, bounds/alignment, controller death, heartbeat loss and independent
deadlines. Win32 mapping visibility and debug transport remain unproved.

Mapping and log handles are non-inheritable. Child stdin/stdout/stderr are three
explicit case files. The helper closes its unused inherited standard handles
before child creation; it does not expose its control channel to the target.
The mapping lives outside the target's working directory. No extra target
environment variables carry controller state.

## Operations and phases

Expose bounded operations: wait for an event; read at most 512 bytes; read the
pending thread's context/selector descriptors; install a checked one-byte INT3;
restore only a previously saved trap byte; execute exactly one TF step; perform
an explicitly approved loader-prefix continuation; stop/terminate and drain.
There is no generic WriteProcessMemory or SetThreadContext RPC. Context edits
are limited to TF and the checked EIP adjustment when consuming an owned trap.
Actual returned register/stack values remain observations.

Track phases `created`, `loader-prefix`, `CF-stopped`, `gated`, `frontier-stopped`,
`draining`, `exited`. Enforce phase-specific operations in the helper, not just
in a caller comment. `DEBUG_PROCESS` catches child processes before continuation;
an additional process/thread is a HOLD, not an ignored event. Exceptions, SEH,
APCs, callbacks and unknown services are also closed. The first implementation
has one permitted target thread and no general OS-operation free runs.

Each command/event loop checks a global monotonic deadline, event count and
step count; each wait uses a bounded quantum. Parent disappearance, a failed
operation or a malformed sequence stops the case and tears down its owned child.
Teardown tracks pending debug events, requests termination, continues only to
drain owned exit events, and records actual EXIT_PROCESS plus helper exit.
It must never stop or remap a shared Wine server.

Parent liveness is an independently aligned atomic heartbeat/lease sequence.
The helper measures time since the last changed lease with its own GetTickCount;
it does not reinterpret a Linux Python PID as a Win32 OpenProcess PID or compare
unrelated clock epochs. A bounded owner lease and an independent global deadline
both lead to teardown. A heartbeat continuing while its caller is stuck cannot
extend the global deadline. A native outer watchdog handles a hung broker and
tracks only its owned processes; successful cases still require real child exit
evidence rather than assuming a killed broker also killed its target.

## CF launch protocol

The initial target is the hash-bound 7704-byte binnt stub. From CREATE_PROCESS,
derive its actual PE base, verify and trap its entry and launch call at RVA
`0x10763`. The only proposed bulk prefix is this positively identified loader,
with the CF launch trapped before its first payload instruction; no arbitrary
payload CRT/startup waiver follows from it. Unexpected events/threads HOLD.

At launch, observe allocation globals and ESI, read the complete initialized CF
image in bounded pieces, and compare all bytes against the exact immutable file
plus every DWORD base relocation modulo 2^32. Preserve relocation-cell ownership
and hashes, including non-fixup bytes. Do not infer code/data from allocation
pages or the unrelated LX loader objects. Bind actual CF base/entry, parameter
block, stack frame, selector descriptors and command/environment inputs.

Install and verify the dynamic CF entry trap after relocation. Restore only the
owned launch breakpoint, execute the exact verified `_InvokePgm` sequence, and
stop before the first original payload instruction. Its contract is AH=2,
ECX=0, BX=CS, EDX=gateway, EDI=parameter block; entry EBX is not the allocation
base. Preserve the actual eight-byte launch far frame and raw upper selector
padding. Subsequent startup instructions require positive compatibility
classification or remain frontiers, like other unclassified payload code.

The Python gate then supplies only one-instruction decisions. A frontier call
requires actual INT3 prebyte/write/flush proof and an actual TF arrival with the
real pushed return slot; a pre-CALL context is not callee-entry evidence.
Keep its target trapped and stop. No unknown body executes while its source is
still being reconstructed.

ARM must SetThreadContext with the actual pending context plus the owned TF
change, then read back TF=1 before ContinueDebugEvent. Preserve and record every
other flag bit and control register. WLIB's authored Win32 child demonstrated
that the same thread's real EXCEPTION_SINGLE_STEP arrives at the correct next
PC with TF consumed to zero in GetThreadContext. Do not require TF to remain set
at the stop merely because native GDB presents it differently. Any consumed-TF
profile needs the successful ARM readback, owned outstanding step, exact event,
first-chance status, thread and next PC/SP/CS; retain raw before/armed/stopped
EFlags and full GP/segment contexts. ARM must preserve every unrelated value.
For the proved PUSH profile, all non-TF flag bits (including RF), other GP
registers and all CS/SS/ES/FS/DS/GS remain exact. General instructions need their
own decoded write effects; that PUSH proof does not authorize ignoring their
unrelated state. This grants no general flag normalization and no permission
for unexpected events.

## Probe gates before real cases

First compile the helper and an authored disposable Win32 child, with source,
command, MAP and PE hashes. Submit the exact build/run plan before any new Wine
probe. The child deliberately prepares test data/branches; it is never an
original tool or source of application equivalence evidence.

Repeat the native marker controls through real WaitForDebugEvent/CONTEXT calls,
including genuine CALL/RET/RETF and target INT3 plus TF ordering. Add missed
target/failed trap write/failed flush, foreign return or cleanup, stale sequence,
wrong thread, exception/callback/child-process, deadline/parent-disappearance,
unowned data-write and full relocation/non-fixup image corruption negatives.
Every failure must stop before the relevant marker changes, retain its cause,
and demonstrate owned-child termination. No original Watcom case acceptance
precedes that review.
