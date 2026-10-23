# Empty metadata prototype

The sole pending candidate is callback36, RVA38ab7..38ad8,33 bytes,
cdecl void(void). It reads no incoming formal/register/stack value, clears
all20 bytes at c64a0..c64b4 (and repeats the first DWORD store), then returns.
The next genuine named frontier is callback37 at38ad8; its initial instruction
loads its own fixed receiver, so no incoming hidden/register input is required.
The tailcalled member's ABI will be addressed when that wrapper is reconstructed.

This is a named20-byte whole prototype value, not five guessed fields or an
opaque slice of an unknown larger object. Own C1XX instructions establish
all referenced bytes and the value's complete size:

- RVA94a4f..94a71 copies exactly20 bytes into a larger object's +f4 member.
- RVA9695e..96984 and96f00..96f20 copy the same20-byte prototype into local state.
- RVA97252 and991d2 provide two more whole20-byte copies.
- RVA97002,9728e,97320,974cc,97584,97d91,97dd1,97e86 each compare exactly20 bytes
  with the prototype to determine whether such member state is empty.
- Direct addresses c64a4/c64a8/c64ac/c64b0 occur only in the reset stores.
  No interior field interpretation or larger-record layout is asserted.

The source therefore uses unsigned char empty_metadata_prototype[20], with a
readable byte-clear loop. The shared initialized data projection remains89
semantic bytes and8431 original unknown bytes; no initialized image was added.
The BSS field map gains exactly20 semantic bytes. Native BSS is912 bytes:
883 semantic bytes and29 natural padding bytes. The existing two trailing
padding bytes move after the newly encountered byte value; they have no
original semantic counterpart. The original four-byte gap after this value
is unmodeled and has no native placeholder.

Four immutable paired captures are recorded in prototype-trials.json:

1. Natural execution: real accepted prefix and sole33-byte candidate, with
   original/native zero defaults and genuine callback37 frontier.
2. Nonzero compiler-mode restore: value seeded165 in every byte before general
   BSS reset; explicit phase observation proves all20 bytes are cleared.
3. Zero compiler-mode restore: the same field/state proof on the other mode.
4. Explicit callback-entry data auxiliary: after general BSS reset and all
   previous initializers, actual callback36 entry is seeded with bytes1..20.
   The real original and native function bodies clear these bytes. No function
   stub return, code patch beyond normal mask restoration, allocator mock,
   table substitution or additional target body is involved.

The auxiliary seed is declared in the immutable trial, source/controller and
raw event stream, and the promotion checker rejects auxiliary acceptance.
Original captures retain the complete BSS range b9000..c869c at the actual
callback36 entry and immediate callback37 entry. Original-to-original comparison
allows only the known20 bytes to change; every other original BSS byte must be
identical. This is not a native equivalence claim for unmodeled BSS bytes.
Native immediate entry/result observations compare every previously modeled
neighbor, all typed storage identities, source/snapshot state, last error and
x87 context. These checkpoint observations remain separate from later state.
All earlier output/PDB/pool/token/rank proofs continue to be checked in each
fresh pair.

Source compiles with the original compiler; the Rust audit checks the exact
current sole candidate plus prefix, including all33 original bytes, function
references, imports and cdecl cleanup. The whole original code mask is verified.
Promotion is held for concrete projection review.
