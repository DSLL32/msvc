# PCH relocation context

The callback1279c loads ECX=58f68 and jumps to11e2a. The separate18-byte member
zeroes17 DWORDs and returns its receiver with RET0. The original tail jump keeps
the initializer loop return12348 on the stack; the unoptimized native caller
has its own frame. The frontier adapter must observe that explicit difference,
not pretend the original pushed a member-call return.

The context's encountered68 bytes contain image count/root/image pointer array,
a stored file position, path and starting offset, file handle, six mapping
handles, loaded image count, scratch arena pointer and original/mapped page
lookup tables. `field-map.json` records the exact offsets and individual writer/
consumer references. ReadFile/SetFilePointer establish file/count/position
roles; the cold cleanup closes every mapping handle; pointer-field traversal
and remapping index the two lookup tables with the high16 address bits.
Pointed image/arena layouts and any original context tail remain unknown.
The original defaults and nonzero BSS reset are checked for all68 known bytes.
The former wrapper-only trial hit a strict pending-tail RET inference limit.
Root approved an ordered bounded two-candidate chain: retain an explicitly
prospective, non-promotable wrapper-to-member input observation, implement the
real18-byte member, then run a fresh strict joint trial to the next named trap.
The source now expresses the genuine member as typed assignments and a six-
handle loop; neither a synthetic ABI wrapper nor a RET waiver is used.

The normal and nonzero joint pairs both reach callback127a6. The member proof
records ECX, its actual implicit formal, original inherited return12348 versus
the actual native CALL return, ESP+4, EAX=this and preserved EDI. Whole original
BSS/data/snapshot and native BSS before/after checks limit the writes to the
encountered68 bytes. Nonzero pointer/handle values are labeled owned snapshot
interior sentinels; the actual member clears them before dereference or API use.

`evidence/pch-joint-{normal,clear-aux}/joint-acceptance.json` explicitly labels
the JOINT two-function closure and retains the distinct prospective receipts.
It binds51 source functions/2158 owned original bytes, all five strict graph/
import/RET/ownership/extent checks, exact source/controller contents, canonical
original raw observations, retained native ELF and rebuilt DLL/MAP. New PCH
negative controls34/43 and core47/100 controls pass. Existing unchanged pool
negative results are reused explicitly; all nine current pool entries/returns
are still validated on both fresh raw pairs.

Positive review from the binary root:

```
python3 scripts/check-trial.py /root/.ai-tmp/msvc2/vc5.10/c1/minimal-pch-joint-002 /root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-pch-joint-002
python3 scripts/check-trial.py /root/.ai-tmp/msvc2/vc5.10/c1/minimal-pch-joint-clear-aux-001 /root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-pch-joint-clear-aux-001
```

The earlier prospective checker is deliberately bounded to the old49-function
prefix plus pending-member wrapper. Its frozen controller/input proof remains
historical after this joint trial; ordinary acceptance rejects that evidence
kind. No prospective audit/RET/ownership pass is claimed.

Joint timing: strict audit0.020s; original normal/AUX captures1.820/2.170s;
native normal/AUX captures0.238/1.039s. Native cached lifecycle fixture4.36ms
(setup1.57/execution2.04/checks0.29). Core negative controls17.36/62.27s; new PCH
controls12.69/23.56s. Both original/native joint builds passed, but their initial
elapsed time was not measured. No rebuild was repeated solely for accounting.


The orchestrator independently reran both strict joint positive checks and
reviewed actual source,68-byte field evidence and ABI. Promotion is explicitly
JOINT for the two functions, with no further original capture for review.
