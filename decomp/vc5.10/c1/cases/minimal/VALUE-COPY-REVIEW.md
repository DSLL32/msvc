# Proposed limited projection for callback12646 — not accepted

The callback at RVA12646..12663 sets BYTE55100 to1 and copies24 original
BSS bytes from518a0..518b8 to55108..55120. The current buffer queue precedes
this function; it remains a named INT3 stub. No value-copy source is implemented.

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

The proposed reconstructed type contains only `__int64 integer`, with source
and cached instances in typed CompilerBss plus the actual availability BYTE.
The callback’s proposed body preserves flag-before-copy order:

```c
c1_bss.saved_literal_available = 1;
c1_bss.saved_literal = c1_bss.literal;
```

Proposed original field map (native offsets/sizeof verified from the real builds
when a trial is authorized, never guessed):

| Field | Original RVA | Bytes | Default proof |
| --- | ---: | ---: | --- |
| literal.integer | 518a0 | 8 | loaded first BSS raw snapshot |
| saved_literal.integer | 55108 | 8 | loaded first BSS raw snapshot |
| saved_literal_available | 55100 | 1 | loaded first BSS raw snapshot |

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

Proposed independent auxiliary proof: seed nonzero signed64 source and a
different nonzero destination plus a nonzero flag before repeat-pass clearing;
verify all three fields become zero while the saved allocation is retained.
At the real callback entry, seed source0x1122334455667788 and a distinct cached
integer, then execute actual original/native callback bodies and compare cached
integer, unchanged source and flag1 at the next named trap. Original-only full
24-byte copying may be observed, but no16-byte native equivalence is claimed.
Field nonoverlap, object identity, width, signedness, actual copy order, snapshot
reset/restore and explicit unknown-consumer rejection remain required.

Canonical target SHA256:
7f12a4a889c5a0277f12c391ca462657dc81ef7be769faf629331bd117983d5d.
Analysis source is the original binary and the private objdump listing under
~/.ai-tmp/msvc2/vc5.10/c1/analysis; no assembly is part of reconstruction source.
