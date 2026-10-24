# Multibyte scanner initializer

The complete72-byte `initialize_multibyte_scanner` is accepted:54 hot bytes at
12f08..12f3e and18 cold bytes at307c0..307d2. It retains `_setmbcp(-4)` and the
unsigned-byte `_mbctype[c+1]&4` test before ORing scanner flags80 and assigning
lead-byte class35. The VC5 DLL build explicitly selects the canonical direct
imported `_mbctype` array instead of the SDK's optional `__p__mbctype` accessor,
preserving the real function graph. Strict Rust audit passes72 source vertices,
5272 code bytes and16 switch-data bytes, with graph/import/RET/ownership/extent
checks. Accepted ownership is70 application functions/4809 original code bytes;
CRT support remains separate and receives no application credit.

Two contextual256-byte initialized lexer tables use readable ASCII dispatch
classes and scanner flags. All512 C-compiled defaults and the independent
semantic field map equal the complete canonical tables. The typed initialized
projection is2348 known/4200 UNMODELED bytes of the original6548-byte snapshot.
The tables participate in snapshot/restore, never BSS clearing. Distinct nonzero
live/saved patterns prove genuine restoration and complete512-byte identity.
The actual case is SBCS1252 and has no lead bytes; the cold source branch is
preserved/audited, without claiming DBCS execution coverage.

Own original normal/restore runs reach the real next named stub12f3e. The
canonical CRT observer records setlocale entry/return, actual `_setmbcp(-4)`
entry/return, and next-helper state. All14 exports, caller bytes/formals/RET,
complete257-byte MB/257-WORD ctype arrays including EOF, retained old/new table
and returned-string ownership/lifetime, separate private ordered host-CRT and
Invoke vectors, target-thread errors/full112-byte FP and complete original C1
regions are bound to actual loaded-hFile hashes. Mapped regions do not imply
original malloc object extents. Private vector values stay outside Git.

`setlocale(2, "")` changes LCcodepage to1252 and CTYPE handle409, returns
`English_United States.1252` and supplies a new ctype table; stored MBcodepage
and flag remain0. The subsequent `_setmbcp(-4)` returns0 and changes only
stored MBcodepage to1252. Flag0/MBmax1/static MB table/ctype/result/errors/FP
remain unchanged. Reviewed shared support b9863419d reproduces these effects
only after the host binds the own authenticated capture; target calls perform
the transitions. Native builds explicitly enable the510 CRT profile. Both
complete current original/native pairs match, including physical scanner/API
entries and returns, next INT3, all modeled state and pointer aliases.

The two stale component policy labels advance12f08→12f3e through an explicit,
independently reviewed validator-only derivation. Original candidate/field map,
mask/source/raw/audits stay immutable; no field/layout/default/alias/code/ABI or
execution-input changes are permitted. The4200 unknown bytes remain guarded.
See `scanner-policy-refinement.json` and `evidence/scanner-policy/`.

| Phase | Normal | Restore |
| --- | ---: | ---: |
| Original capture |4.525498s|4.775630s|
| Native dual-decoder capture |15.633216s|16.770448s|
| Grouped projection, same stops |2.561532s|2.772845s|
| Legacy projection, same stops |10.327321s|10.800902s|
| Equal complete checkpoints |219|242|

Recorded native builds took2.093229s and2.049848s during preparation, then
2.074843s for the actual published510-profile support/host build. Strict audit
is0.039451s. Fresh affected checks are27 native component controls25.653938s,
22 original-API controls14.071715s,22 policy controls0.363s, six static layout
controls0.035461s, and18 complete core controls71.324641s. The core campaign is
a one-time affected checkpoint for the changed transport/current512-byte schema,
whole API transition and policy binding. Ordinary later functions reuse unchanged
hash-bound core/API control proofs and run only affected/new checks. The27
component controls reuse their immediately validated older prefix with every
unchanged input hash verified. Both final full current positives also pass.

The grouped reader resolves mutable object addresses and reads known owned spans
fresh at every existing checkpoint. Only a field-map-validated immutable DWARF
layout is cached; padding has no semantic credit. Its219/242 same-stop results
exactly equal legacy projection and raw/schema hashes are retained. Independent
review permits it as the normal C1 reader, with `--grouped-equivalence` retained
for diagnosis. Measured projection improvement is4.03x/3.90x; dual capture totals
include both decoders. The next naturally required grouped-only capture will
supply end-to-end timing; no profiling-only replay was run.

Originals are retained in `~/.ai-tmp/msvc2/vc5.10/c1/minimal-multibyte-{001,restore-001}`;
natives in `native-minimal-multibyte-001` and `native-minimal-multibyte-restore-002`.
The first restore pilot stopped before initialization because host setup
redefined its wrapper recursively; its failure is retained and fixed natively,
with unchanged original/ELF. Earlier preprofile builds are preparation only.
Current source/native/reference receipts, controller snapshots, strict audits,
external paths/hashes and controls are under `evidence/multibyte-*`.
Minimal C compilation remains in progress at12f3e.
