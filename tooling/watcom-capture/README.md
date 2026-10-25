> Shared implementation: `tooling/recon-tool/watcom/` and `recon-tool original capture`.
> These module/build paths are temporary migration shims. Historical qualification below
> retains its original scope; diagnostic entry-stop is not ordinary CF progress.

# Watcom original capture gate

This is original-reference tooling, never a native fixture dependency. Static
function discovery and graph/ABI audits remain in `tooling/graph-audit/`.

`gate.py` is a deliberately bounded runtime instruction/transfer gate. It accepts
explicit decoded-instruction ownership from an already approved CFG, actual
thread registers, and a checked memory reader. It does not discover a whole
image, infer code sections, emulate application logic, or permit OS free runs.
Unsupported instructions, events and ownership remain HOLDs.

The transport interface is `Gate.plan(actual_context, thread_id)` followed by
`observe` after exactly one real TF step. A `frontier` decision is a pre-transfer
STOP, not callee-entry evidence. To enter that frontier, the transport must
install and verify its target INT3, call `arm_frontier` with actual write/flush
results and hash-bound relocated-image prebytes, execute only the transfer under
TF, then call `observe_frontier` with the actual entry registers. A CALL's return
slot is read from the real stack. The gate stops permanently at that unknown
body; it never fabricates a push or authorizes further execution. Unknown RET
continuations and external services HOLD.

The application image is a location bound, not blanket object ownership. Supply
separate readable/writable spans for proved code, data objects and live stack
regions. TEB stack bounds alone do not prove an arbitrary object capacity. Flat
CS/SS/DS/ES selectors require separate observed-base proof; FS/GS remain closed.

Use `python3 check-gate.py` for offline controls and `python3 check-child.py` for
the authored native i386 child. The latter checks real CPU branches/stack frames
and verifies marker writes occur only after deliberately bypassing a recorded
STOP in that disposable probe. It also checks guarded callee entry, genuine
CALL/RET pushes and an eight-byte RETF with raw selector-slot padding. It invokes
GCC/GDB and reports separately measured build and GDB setup/execution/check time.

No application acceptance is authorized by offline gate controls alone. A
reviewed Win32 debug transport must still prove post-relocation image identity,
trap/TF behavior, thread/event closure and actual owned-child marker negatives.
The detailed proposed protocol is
`decomp/watcom10.0a/wcc386/cases/help/capture-plan.md`.

`ipc.py` supplies the bounded native x86 shared-map channel. Run
`python3 check-ipc.py --output /path/to/receipt.json` for its authored Linux
i386 peer controls: publication, message bounds, stale sequences, controller
death and independent lease/deadline handling. The checked-in
`ipc-independent-review.json` binds the reviewed source and measured costs.
This proves neither Win32 mapping visibility nor debug-event transport; see
`WIN32-TRANSPORT.md` for the gated next step.

`cf_identity.py` independently checks the immutable CF container/header and
complete initialized-byte snapshot after sequential DWORD relocation. Its three
target profiles require the proved trailer relocation stream, entry before that
stream, bounded write sites and an actual separately observed allocation base.
It preserves modulo-32 content additions, including repeated/overlapping sites;
it infers no code ownership. Static CF reading/discovery stays in the common
Rust auditor. Run `python3 check-cf-identity.py --output /path/to/receipt.json`
for pure byte controls. The independent receipt does not establish that a
running Win32 child supplied the snapshot/base; those controls remain required.

Capstone 5.0.7 is the tested decoder (`requirements.txt`). Its indirect FF /3
and FF /5 presentation is explicitly guarded as far-call/jump machine encodings.
