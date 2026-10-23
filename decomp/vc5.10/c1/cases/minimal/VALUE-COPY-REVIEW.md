# Held value-copy trial12646 — concrete review packet

The callback at RVA12646..12663 sets BYTE55100 to1 and copies24 original
BSS bytes from518a0..518b8 to55108..55120. The buffer queue and empty callback12645 are accepted. The readable trial body
is compiled in both native and original targets, but remains unaccepted pending
this concrete review. Durable functions.json still contains30 source functions
and1788 original bytes; the trial has31 source functions and1817 bytes.

Writer evidence from the canonical C1.DLL:

- RVA03120..03127 sign-extends EBP with CDQ and stores EAX/EDX at518a0/518a4.
  RVA0314f..0315f then copies this24-byte operand payload into a32-byte record
  at offset8 and writes record kind0x2d. This proves a real signed64-bit value
  alternative; the reconstruction name `integer` describes that alternative.
- RVA05d2c..05d59 instead copies an object’s fields at offsets14/18 into the
  first8 bytes and fields04/24/2c into518a8/518ac/518b0. These are a different
  operand alternative. Their eventual types and semantics are NOT reconstructed.
- RVA0b3d5..0b3f6 writes metadata pointers/callbacks at518ac/518b0 and later
  copies the same24-byte payload into a record of kind0x24 (RVA0b42c..0b43c).
- No direct disassembled reference to518b4 was found. That is insufficient to
  label it padding; all16 bytes beyond the known integer remain UNMODELED.

The reconstructed trial type contains only `__int64 integer`, with source
and cached instances in typed CompilerBss plus the actual availability BYTE.
The callback’s body preserves flag-before-copy order:

```c
c1_bss.saved_literal_available = 1;
c1_bss.saved_literal = c1_bss.literal;
```

Original field map and measured native layout (GDB, actual ELF):

| Field | Original RVA | Bytes | Default proof |
| --- | ---: | ---: | --- |
| literal.integer | 518a0 | 8 | loaded first BSS raw snapshot; native offset520 |
| saved_literal.integer | 55108 | 8 | loaded first BSS raw snapshot; native offset528 |
| saved_literal_available | 55100 | 1 | loaded first BSS raw snapshot; native offset536 |

This is a17-byte semantic refinement, not equivalence of either24-byte object.
Original source/destination24-byte records are retained in the complete private
BSS capture. There is no native raw array, placeholder gap or guessed default.
Other operand alternatives require a later union/type refinement before use.

The next callback12663 remains a named trap. Subsequent unaccepted compiler
functions (including main at127a7 and the record-producing writers above) stop
before interpreting the unmodeled words. None of the accepted prefix functions
consumes them; CompilerState restore only clears/copies its currently typed
fields. An unknown consumer, serialization, hash, comparison or external escape
would require modeling it before that function can be unmasked.

The actual native value type is8 bytes; CompilerBss is540 bytes. Its513
modeled original bytes plus27 natural C padding bytes account for that size;
no original object gap appears as native storage.

Fresh paired evidence lives in `evidence/value-copy-normal/`,
`evidence/value-copy-positive-aux/` and `evidence/value-copy-negative-aux/`.
All three pairs reach named INT3 callback12663. Rust audits the exact immutable
31-function trial (1817 original owned bytes), including the candidate body,
using published graph-audit90f7ef6d (SHAfc5c81f2...). Actual target/provider
loaded-file hashes, effective masks, original relocated snapshots, actual ELF,
compiled-source contents and both complete original24-byte records at every
boundary are bound in receipts and external artifact manifests.

The normal case copies zero numeric prefixes and sets flag1. The explicitly
labeled auxiliary probes independently seed source0x1122334455667788 and
source-0x112233445566778 at the real callback entry, with different cached
integer0x5566778811223344 and flag165. Genuine original/native callback bodies
produce the matching signed cached value and flag1, leave source unchanged,
and preserve every adjacent modeled field, pointer identity and registered
cleanup callback. Before repeat-pass clearing both probes seed another negative
source and nonzero cached value/flag, then prove that all three modeled fields
clear to zero while the saved allocation is retained. Every intervening context
and buffer-constructor observation also verifies these value fields stay zero.
The original helper writes only the known8+8+1 bytes; its16-byte tails are never
seeded or sent to native execution. Full original-to-original24-byte copying is
checked separately, without any native tail-equivalence claim.

To reproduce validation without Wine, from this binary folder run:

```
python3 scripts/check-trial.py ~/.ai-tmp/msvc2/vc5.10/c1/minimal-value-normal-001 ~/.ai-tmp/msvc2/vc5.10/c1/native-minimal-value-normal-001
python3 scripts/check-trial.py ~/.ai-tmp/msvc2/vc5.10/c1/minimal-value-positive-aux-001 ~/.ai-tmp/msvc2/vc5.10/c1/native-minimal-value-positive-aux-001
python3 scripts/check-trial.py ~/.ai-tmp/msvc2/vc5.10/c1/minimal-value-negative-aux-001 ~/.ai-tmp/msvc2/vc5.10/c1/native-minimal-value-negative-aux-001
```

Each evidence folder includes generic and focused semantic rejection results.
The focused controls alter signed high/low words, source/destination/flag,
width/alias/defaults, actual original copy, seed inputs/order, callback PC,
neighbor state and the explicit unmodeled-tail/hard-frontier metadata. Raw
artifacts are rehashed when testing semantics, so rejection is not merely a
file-hash mismatch. Native regressions remain cached and Wine-free.

Canonical target SHA256:
7f12a4a889c5a0277f12c391ca462657dc81ef7be769faf629331bd117983d5d.
Analysis source is the original binary and the private objdump listing under
~/.ai-tmp/msvc2/vc5.10/c1/analysis; no assembly is part of reconstruction source.
