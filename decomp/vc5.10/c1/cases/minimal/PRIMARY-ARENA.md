# Primary arena selection

initialize_primary_arenas owns61 hot bytes129db..12a18 and15 cold bytes30510..3051f.
It selects descriptors with flags bit0 and calls their genuine reservation member,
reporting compiler_fatal(60) on failure. Current actual descriptors select only
arena0. Both fresh normal and restore cases stop at that member's named INT3.
No VirtualAlloc or arena-field write executes in this step.

Original ECX identifies arena0; stack slots identify descriptor0 and NULL, with
actual caller return12a0f. Native hardware ECX and the three raw entry stack words
are independently recorded before the compiler prologue. They agree with actual
GDB this/descriptor/address formals and exact types at the trap. The member's
original RET8 is ABI analysis for a pending function, not body credit. Its native
signature expresses thiscall directly with the shared annotation; no wrapper or
assembly transport is introduced.

An independent preprologue primary entry separates the accepted wrapper proof
from the new loop. All complete original state regions and all currently modeled
native state stay unchanged across selection; the descriptor table and zeroed
arena objects retain their previously proved identities/defaults. No new fields
or data bytes are introduced.

Both complete paired positive proofs, strict60-source/3019-code+16-text-data-byte
audit, exports and cached native lifecycle pass. Fresh30 targeted ABI/boundary
controls exercise the actual primary-entry/member validators in2.41s. They reject
entry omission/identity/formals/caller, raw ECX/stack/return, argument/type/count
mismatches, state mutation and changed original call/RET8 evidence. Independent
restore raw positives pass; unchanged state/protocol guards and the same targeted
normal guard result are explicitly reused with hashes, not reported as rerun.

Original/native builds2.756/1.580s. Original/native captures2.321/2.044s normal,
2.572/2.688s restore; audit0.031s. Native lifecycle takes4.69ms excluding build.
Minimal compilation remains incomplete. Evidence is retained in
`evidence/primary-arena-{normal,clear-aux}`.
