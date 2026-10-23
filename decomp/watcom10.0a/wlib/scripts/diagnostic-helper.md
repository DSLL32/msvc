# Original-only diagnostic helper

`diagnose-startup.c` is a bounded public Win32 debugger, not reconstructed WLIB
logic, a function-acceptance controller, or a native fixture dependency. It uses
`CreateProcessA(DEBUG_ONLY_THIS_PROCESS)` and records actual process/thread/module
events, contexts and configured one-shot breakpoints. It does not mask code,
inject callbacks or apply the proposed GS patch.

Arguments after the helper executable:

```
TARGET RAW_ARGUMENTS CHILD_CWD STDOUT STDERR EVENTS SITES TIMEOUT_MS STDIN_MODE READS CF_CONFIG [GUARD_CONFIG]
```

`STDIN_MODE` is a decimal count of newline bytes (maximum 64) followed by pipe
EOF, or `@S:\path\stdin.bin` for an existing inheritable regular input file.
Output files must not exist. `SITES` has `hexPErva name [hexExpectedFirstByte]`
rows. `cf:hexPayloadRva` uses the actual verified CF base and requires an expected
first byte. `READS` is a list of hexadecimal PE-relative DWORD locations, or `-`.
`CF_CONFIG` is `-` or a header containing hexadecimal base-slot RVA, entry-slot
RVA, expected entry offset and image limit, then decimal relocation count,
followed by exactly that many `hexRva hexUnrelocatedDWORD` rows.

At a PE site named `cf_launch`, the helper reads actual base/entry slots, checks
entry equals base plus expected offset and actual ESI, then verifies every
listed relocated DWORD before arming CF sites. This does not establish a general
CF control-flow or allocation-ownership proof.

`owner_*` sites dump 128 bytes at actual EDX; `owner_stack*` uses actual ESP.
These names request observations, not ownership assertions: the caller's real
instructions and object lifetime must establish ownership. `resource_*` sites
assume the independently established GetString frame is active and record its
output offset at EBP+0x14, selector WORD at +0x18, capacity at +0x1c and caller
return at +0x10, plus raw bounded output bytes. Mapping metadata is descriptive
and does not prove object size. Other names log raw context only.

Without `GUARD_CONFIG`, a site is one-shot, including a site after a store. Shared joins can execute
before that store on another branch; inspect actual event order before making
any paired-state claim. Persistent guard mode is described below; neither mode establishes function-graph
acceptance.

The deadline is checked every debug loop iteration. Unidentified app INT3 traps
fail closed; the one initial loader INT3 must be a real CC opcode within the
observed ntdll image before PE entry. Error/timeout cleanup releases a pending
debug event, terminates only the owned child and drains its real exit event.

The reviewed v3 build is in shared scratch
`watcom10.0a/wlib/startup-diagnostic/helper-build-v3/`, with immutable source,
compile/link commands, logs and binary hashes in `receipt.json`. It was built
using shipped VC5.10 CL `/c /W3 /Od /MT` and LINK `/subsystem:console` under the
shared prefix; helper compilation does not replace WLIB's required Watcom build.

The initial v1 compile recorded source hash but did not snapshot that source
before later edits. Its old binary and events are retained; v2/v3 retain exact
source snapshots. Only v3 is the corrected buffer/selector diagnostic evidence.


The optional `GUARD_CONFIG` enables a reviewed all-reader-call selector and stack
owner guard. Its header is `pe|cf ownerRva readerRva readerReturnRva returnByte
maxCalls`, followed by an owner count and `callerReturnRva ebp|esp displacement
actualObjectCapacity returnByte` rows. RVAs/bytes are hexadecimal; counts,
displacements and capacities are decimal. Named `guard_owner_begin` and
`guard_reader_entry` sites must be original one-byte PUSH-register prologues.
Actual caller stack objects, argument tuples, selector equal to actual DS,
initial PE DS/GS, thread identity and RET12 returns are checked on every call.
Unknown owners, inadequate objects, new threads and unexpected traps hold.

Persistent breakpoints restore one original PUSH, request TF, verify actual ARM
readback (TF1), then require an owned first-chance single-step at the exact
successor (TF0 in the observed Wine transport). Every other flags bit including
RF, all six selector WORDs and unrelated registers remain exact; no flags mask
waives a discrepancy. The original byte is checked before rearming. Raw contexts
are logged; only the owned TF bit is cleared. Temporary return breakpoints
expire actual owner/reader lifetimes.

`probe-resource-guard.c` runs the same policy natively: 198 checks cover missing
or duplicate ARM, wrong thread/context, every flags bit/register/selector and
owner/return/capacity failures. `probe-resource-guard-win32.py` explicitly runs
only frozen authored Windows probes, outside native fixtures. Its reviewed
13-case set passed: repeated guarded calls with four real TF/rearm cycles,
six pre-entry failures, and six unguarded marker controls. Authored assembly is
a narrow debugger ABI probe, not reconstructed application implementation.
Receipts and raw evidence are in `cases/help/guard-probes/authored-v4/`.
Patched WLIB/WASM execution requires separate review; these probes grant no
application-function acceptance.


Subsequent separately approved guarded patched-copy captures are retained in
`cases/patched-minus-question/` (diagnostic exit 1) and `cases/patched-help/`
(bare `?`, help exit 0), with two identical stream/status results per input.
The frozen helper observed real exit debug events but did not check the normal
ContinueDebugEvent result or a signaled handle; those receipts claim only the
observed events. Any future cleanup hardening is versioned separately.
`capture-reviewed-help.py` and `repeat-reviewed-case.py` are explicit original-only
reference tools, never fixture dependencies or authorization to recapture.


The subsequent normal-exit cleanup checks ContinueDebugEvent success, then waits
at most 1,000 ms for the owned process handle and verifies GetExitCodeProcess
matches the exit event before logging normal completion. Authored positive and
wrong-selector controls passed; ordinary receipts and compressed raw logs are
in `cases/help/guard-probes/normal-exit-cleanup/`. Earlier references remain
bound to their old frozen helper and their narrower observed-exit claims.


For a CF case with one `observe_CreateFileA` site, the verified CF launch rearms
that same already-hit/restored PE observer once. The labels `loader-open` and
`first-postlaunch-open` include actual caller, arguments and pathname; the latter
is not assumed to serve resources. Byte/phase mismatches hold. The unchanged
resource guard remains active. `cases/help/guard-probes/cf-file-observer-proof.json`
binds the actual helper06 WASM first-capture proof in the WASM worker's owned
receipt and compressed log; evidence and binaries are not rebundled here.
