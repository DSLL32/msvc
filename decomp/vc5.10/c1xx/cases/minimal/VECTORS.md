This116-byte allocator is accepted after the orchestrator independently reviewed
both frozen pairs and current strict audits. The allocator owns104 hot bytes at682f..6897 and12 cold bytes
at532af..532bb. `src/vectors.cpp` retains both genuine CommitExtent calls; no
new wrapper, allocation mock or import implementation is introduced.

`CompilerVector` is12 bytes: signed short count/capacity, int arena index and a
pointer to pointer storage. Each actual call receives40/1, reserves a12-byte
header and160 bytes of items, returns the real header address, and preserves
nonvolatile registers with cdecl RET0. The first header crosses arena1's current
commit limit, causing the already-reconstructed thiscall CommitExtent to request
exactly32768 bytes via VirtualAlloc. Both sides observe all four API arguments,
real result/RET16, zero committed bytes, precise mapping/protection and reserved
tail. All three full32768-byte snapshots contain only their exact headers and
zero pointer arrays/tail. Other target live/saved/unknown bytes are unchanged.
Zero/negative-low16 capacities and the second expansion branch are source/graph
coverage only; these cases do not execute them.

Earlier accepted prefix decoders use a genuine structured first-vector-entry
checkpoint and the actual original prefix, before any vector body executes.
There is no invented earlier frontier or relabeled acceptance. The distinct
vector checker owns every later allocation, caller pointer store and true final
event; the current immutable mask and strict audit include the entire candidate.
Its first locale/vector owners are tied to the independently checked preceding
entry so a consistent owner replacement cannot manufacture agreement.

Normal execution creates all three vectors then stops at actual named imported
MSPDB50!SigForPbCb, ordinal127/RVAf22d. The original debugger compares the actual
DLLLOAD hFile to the canonical provider, authenticates all53 entry bytes and
IAT/export identity, and traps before its first instruction. The native debugger
stops at the genuine shared adapter entry. Both observe read-only `CPP`+NUL,
signed long length3 and unsigned signatureffffffff, all raw ESP slots and real
caller, with unchanged errors/full architectural FP/provider owners/target state.
The provider does **not** execute, return or earn application credit. The first
native generic collector's incorrect pre-prologue DWARF values are retained;
final evidence uses raw ESP and independently enumerated source formals.

The explicit nonzero restore auxiliary has a real nonzero C9IL flag and instead
reaches the genuine `next_argument` INT3 stub with no source arguments. Original
final target/FP/caller/error records are direct. Provider-table/vector continuity
from the last vector return is derived from the exact decoded taken JNE path:
only forced_includes is stored before caller stack operations and the final
stub call. This is not an extra original provider-table observation, and no
provider result or fake hash is supplied.

`vector-trial.json` pins both raw pairs, compiled sources/helpers, checkers and
24 normal plus5 restore controls. Originals take36.830/35.148s and native GDB
10.521/10.625s, retaining the rich earlier prefix. Controls take1.669/0.615s plus
9.011/9.034s full positives. The cached native lifecycle fixture passes in13.349ms
(build5.574/setup0.929/execution2.076/checks3.986ms), with zero Wine invocations.
Individual new-vector compile steps are0.716s original and0.131s native; link
steps0.710/1.589s. They are not summed parallel wall time. Shared crt390ea919 was
captured as coherent WIP and subsequently published unchanged as READYc11fa6763.

No new global data bytes are modeled. Original6031 initialized bytes remain
unmodeled; no unknown consumer is licensed. Durable credit is90 boundaries:86 application functions/5889B, historical CRT451B
and generated imports12B separately (6352/751216 whole code bytes, not non-CRT
coverage). Read-only revalidation, with fresh strict
audits and no execution: `python3 scripts/check-vector-history.py`.
