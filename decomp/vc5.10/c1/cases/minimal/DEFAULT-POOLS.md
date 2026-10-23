# Default pool binding wrapper

`initialize_default_pools` owns109 bytes12ac3..12b30. It calls the genuine
thiscall `CompilerPool::SetArena(0)` in pool order8,7,0,1,2,3,4,5,6. Both fresh
pairs stop at the first member, with pool8 in ECX and arena index0 on the stack.
All nine original call sites and their order are checked; only the first call
executes before the named stub. No pending member body or result is credited.

An independent actual wrapper-entry observation verifies the completed six-arena
prefix. Whole known state, initialized snapshot, original regions, real member
formals, caller return address and ABI match at the new frontier. Strict graph
and ownership checks cover64 source functions:3311 code bytes plus16 switch-data
bytes. This includes62 application functions/2848 code bytes; CRT/import support
is counted separately. Initialized state remains64 known/6484 UNMODELED bytes.

Both complete paired positives and25 focused mutation controls pass. Earlier
unchanged Reset/namespace/protocol controls are reused explicitly with their
frozen input and checker hashes. Original build took4.023s, native build1.931s,
and audit0.033s. Original/native captures took3.072/7.496s normal and3.121/7.918s
restore; focused controls took5.071s. Native lifecycle fixtures pass in4.85ms
excluding build. Ordinary frozen evidence is in
`evidence/default-pools-{normal,clear-aux}`; private raw host vectors and exact
debugging artifacts remain retained at their recorded owned scratch paths.

The next function is the25-byte SetArena member. Minimal compilation is ongoing.
