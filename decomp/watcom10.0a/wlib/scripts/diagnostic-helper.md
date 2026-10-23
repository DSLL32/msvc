# Original-only diagnostic helper

`diagnose-startup.c` is a bounded public Win32 debugger, not reconstructed WLIB
logic, a function-acceptance controller, or a native fixture dependency. It uses
`CreateProcessA(DEBUG_ONLY_THIS_PROCESS)` and records actual process/thread/module
events, contexts and configured one-shot breakpoints. It does not mask code,
inject callbacks or apply the proposed GS patch.

Arguments after the helper executable:

```
TARGET RAW_ARGUMENTS CHILD_CWD STDOUT STDERR EVENTS SITES TIMEOUT_MS STDIN_MODE READS CF_CONFIG
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

A site is one-shot, including a site after a store. Shared joins can execute
before that store on another branch; inspect actual event order before making
any paired-state claim. This helper does not implement persistent all-call
selector guards or function-graph acceptance.

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
