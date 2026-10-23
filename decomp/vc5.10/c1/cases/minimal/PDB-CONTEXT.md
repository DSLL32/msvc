# Persistent PDB context reset

The helper at12676..126a1 reads `c1_reuse_state`, clears six fields only when
it is zero, and returns0 in both branches. The split wrapper1266f registers
an independent cleanup callback at238dc after this helper returns.

The original context lives in plain `.data` at5c048. It is outside BOTH the
initialized-state snapshot5e000..5f994 and BSS clearing50000..5a06c. Its six
encountered fields therefore live in a separate persistent C global rather
than CompilerInitializedState or CompilerBss. Native sizeof is24; original
+0x10..+0x14 remains UNMODELED and has no native placeholder storage.

| Original offset | Field | Evidence |
| ---: | --- | --- |
| 0 | database | PDB::OpenEx output,1593f..15941 |
| 4 | names | NameMap::open output,192dd..192e6 |
| 8 | types | first argument of TypesQueryTiForCVRecordEx,23fb6..23fc2 |
| 0xc | minimal_rebuild | MREngine::FOpen output,1935a..1936d |
| 0x14 | rebuild_state | opaque target-owned object passed as this to4dcc8 through226c3 |
| 0x18 | read_mode | 1588b initializes0;31844 stores1 when strchr(mode,'r') succeeds |

Provider objects remain opaque pointers. No provider layout, internal state,
method implementation or broader repeated-compilation semantics is claimed.
Any future unmodeled consumer must still stop at its named trap.

The normal case reaches callback126af with nine registered cleanup identities.
Two explicitly labeled auxiliary code probes create an owned snapshot allocation,
seed five distinct interior-pointer sentinels and read_mode37 BEFORE the repeat-
pass initializer, and verify their preservation through snapshot restoration,
BSS clearing, every context/buffer callback, numeric copy and signature callback.
No sentinel is dereferenced or passed into a provider. At the real helper entry,
reuse0 proves clearing and reuse1 proves retention. Every other typed field,
module/error/x87 state and storage identity is checked. Only the expected cleanup
count/slot changes afterward. The original-only full plain-.data comparison also
proves the helper left its unmodeled gap untouched; this makes no native-gap
representation or equivalence claim.

`evidence/pdb-context-{normal,clear-aux,retain-aux}/` retains the candidate-inclusive
Rust audit, immutable original/native receipts, actual ELF/source contents,
typed field map, measured capture/check times and mutation controls. Large raw
original state remains private and is bound by external artifact hashes. Fixtures
are cached native Linux runs and never invoke Wine.
