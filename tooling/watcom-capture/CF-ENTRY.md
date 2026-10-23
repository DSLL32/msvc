# Actual WCC CF entry handoff

This extends the stopped-loader milestone b711f8590; it is plumbing, not an
application case or function reconstruction. Application acceptance remains0.
The default stopped-loader plan still stops before Invoke. A separately reviewed
`entry_handoff:true` plan may perform the following handoff only.

After the same complete observed CF allocation/relocation verification, RPC22
checks actual globals and launch registers, requires entry=load+2fe00 inside the
requested91000-byte allocation and committed memory, installs the checked
88→CC entry trap, then consumes only the owned loader-launch trap. Existing
ARM/STEP transport executes exactly these actual loader instructions:

| PC relative to observed PE | Bytes | Expected effect |
| --- | --- | --- |
| 10763 | e8adf8ffff | Real near CALL10015; push10768 |
| 10015 | 0e | PUSH CS,4-byte stack slot; only low16 is formal |
| 10016 | 668ccb | MOV BX,CS; preserve upper16 EBX |
| 10019 | b402 | MOV AH,2; preserve other EAX bits |
| 1001b | 2bc9 | ECX=0; exact SUB flags |
| 1001d | ffd6 | Real CALL ESI, only verified trapped CF entry; push1001f |

The broker independently checks each PC, exact bytes, nextPC/SP/CS and final ESI.
The controller additionally compares full actual context after each instruction,
including raw flags under the proved ARM/DB profile; proves live writable TEB
stack bounds and absence of code overlap; checks real pushed returns/CS.
No registers or stack frame are fabricated. Six actual first-chance DB events
must reach CF entry with TF consumed and the entryCC intact. The actual frame is
return1001f, CS low16 with unowned upper padding, return10768.

RPC23 admits only that installed CC: same full actual DB context, exact entry,
TFclear, correct real return, trap stillCC. Require an actual owned first-chance
BREAKPOINT at CFentry, EIPentry+1, same complete context except EIP. This confirms
the masked entry trap itself, not only a single-step event before it. All later
mutating/continuation RPCs remain denied; inspection and owned teardown only.
No original CF instruction has executed. Wrong bytes/PC/destination/state,
unexpected thread/process/exception and foreign events fail closed. Preserve any
failed run, stop with owned cleanup and do not retry implicitly.

The concrete runtime packet binds broker source/PE/MAP, controller, handoff
module, immutable originals, original CF verifier and documentation. One original
launch only, same55s/2s/60s deadline/lease/channel budgets, binary target/launcher
streams and private environment as the preceding capture. Build-only helper
checks may use MSVC5; it is external capture tooling, not reconstructed source.

## What this does not establish

This handoff is not yet the ordinary CF reconstruction loop. The existing
`gate.py` runtime instruction gate has native proof and four basic authored
Win32 transport controls. The following concrete adapter/transport pieces must
be implemented and proved before claiming that loop ready:

- Admit only explicitly selected, byte-bound Rust instruction descriptors for a
  candidate/accepted prefix, applying the complete verified relocated image
  before slicing instruction bytes. A report's existence grants no permission.
- Register the observed CF arena for checked trap installation/consumption,
  preserve exact pre-mask bytes and typed code/data/stack/frame ownership.
- Drive existing plan→ARM→STEP→observe for each owned instruction. At an unknown
  near direct/indirect CALL/JMP, install its verified destinationCC before the
  one transfer, verify actual DB target/frame, then observe actual targetINT3.
- Prove that same path on a small authored owned arena/prefix, including progress,
  next unknown frontier, near returns and forged-return rejection. A target
  byte/PC mismatch and unexpected thread must stop before unknown execution.

Unsupported instructions, indirect far/service calls, unproved callbacks/threads,
foreign RET/RETF/cleanup, SEH/APC and FS/GS operations remain HOLD. No CRT startup
free-run is authorized. These are explicit unsupported classes, not graph/audit
exemptions. Needed basic adapter work reuses existing modules; no new static CFG
walker or generic framework is justified.
