# Scanner initializer work in progress

The next candidate is the complete72-byte `initialize_multibyte_scanner`:
54 hot bytes at12f08..12f3e and18 cold bytes at307c0..307d2. Its source
retains `_setmbcp(_MB_CP_LOCALE)` and the unsigned-byte `_mbctype[c+1]&_M1`
test before ORing scanner flags80 and assigning lead-byte class35. The original
VC5 DLL build uses the actual direct imported `_mbctype` array; its optional
SDK `__p__mbctype` macro is explicitly disabled only for that compiler setup.
The strict candidate-inclusive Rust audit passes72 source vertices/5272 code
bytes plus16 switch-data bytes, with all graph/import/RET/ownership checks.
This is a trial audit, not acceptance of the new function.

Two contextual256-byte initialized lexer tables are represented by readable
ASCII dispatch-class and flag initializers. All512 C-compiled defaults and the
independent semantic field map equal C1's complete canonical tables. They extend
the pending typed initialized projection1836→2348 bytes, leaving4200 of the
original6548 bytes UNMODELED. Both belong to snapshot/restore, not BSS clearing.
The required original restore auxiliary proves distinct nonzero live/saved
patterns and genuine restoration before the real helper executes.

The actual next-helper captures are retained at
`~/.ai-tmp/msvc2/vc5.10/c1/minimal-multibyte-001` and
`minimal-multibyte-restore-001`. Both reach the genuine12f3e trap. Their isolated
CRT observer records setlocale entry/return, subsequent `_setmbcp(-4)`
entry/return, and next-helper state, with actual canonical hFile identity,
14 exports/instruction/caller witnesses, complete257-byte MB and257-WORD ctype
arrays, retained old/new table and result-string mapped ownership, complete
private ordered host-CRT/Invoke vectors, target-thread errors/full112-byte FP
state and complete unchanged C1 regions. Private vector contents remain outside
Git; receipts pin their retained paths/hashes. No original heap-object extent
is inferred from a mapped region.

The own case has errno/doserrno/LastError0/0/0. Isolated `setlocale(2, "")`
changes LCcodepage to1252 and CTYPE handle409, allocates a new ctype table and
returns `English_United States.1252`; stored MBcodepage/flag remain0. The actual
following `_setmbcp(-4)` returns0 and changes only stored MBcodepage to1252;
flag0/MBmax1/staticMB table/ctype/result/errors/FP remain unchanged. These are
original-only observations until reviewed shared support and own native pairs
match; they never authorize a combined CL-locale preseed or CRT credit.

Measured original captures are4.525498/4.775630s. Read-only validation authenticates
1459/1478 artifacts in0.314700/0.320785s.22 fresh semantic/provenance mutations
are rejected in14.071715s. Original and native source builds pass; recorded
native builds took2.093229s and2.049848s. The latter honestly contains a frozen
then-unpublished isolated-locale candidate, does not yet enable the510 profile,
and is not a successful locale/MB runtime proof. A new explicitly510-profile
native build and complete state proof are pending shared READY.

An optional native observer pilot, `--grouped-equivalence`, reads fresh known
owned objects once per existing checkpoint, caches only validated immutable
DWARF layout, and compares its entire decoded projection with legacy GDB scalar
reads at the same stopped state. It retains exact raw objects/schema/timings,
ignores padding semantics, and never caches mutable addresses/values. Static
ELF/DWARF field-map validation and six malformed-layout/path controls pass
without executing the target; runtime equivalence/performance remains untested.
Accepted ownership remains69 application functions/4737 code bytes.
