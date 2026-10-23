# Callback 18: token sentinel value copy (pending review)

Original C1XX.DLL SHA-256:
`e27df3bb9a37058c85dad3fb7d1ed30b9e52d38fb6c0f66c8fdba0a11111d3d9`.
RVA `3891e..3893b` sets a token-kind byte to 1, then copies ten DWORDs.
No target body or data projection for this callback is accepted yet.

| Storage | Original range | Proposed known fields |
| --- | --- | --- |
| Current token value | `104ba8c8..104ba8f0` | First 8 bytes, semantic union prefix |
| Sentinel token kind | `104c0b80` | One unsigned byte |
| Sentinel token value | `104c0b88..104c0bb0` | First 8 bytes, same union prefix |

The source ends immediately before the separately modeled context at `104ba8f0`;
source, destination and kind do not overlap. The destination's two preceding
DWORDs are not payload. The original seven bytes between kind and payload are
unmodeled; no raw native placeholder will represent them.

Own writer/consumer evidence:

- RVA `08bc8..08bd8` sign-extends EBP into EDX:EAX and stores the low/high words
  at `104ba8c8/+4`. RVA `08bfa..08c0a` copies the full 40-byte value into a
  48-byte token record at offset 8, with token kind `0x2d`.
- RVA `b507d..b50c1` stores an allocated object pointer at `104ba8c8`, then
  dereferences it to initialize object members. Other variants therefore exist;
  this is not an exclusively signed-integer field.
- RVA `497cb` and `49893` compare token pointers against `104c0b80`.
  RVA `4989e` reads a token's byte kind. RVA `49819..4982e` copies its 40-byte
  value at offset 8 onto parser value storage. The initializer's byte is a
  sentinel token kind, not a Boolean availability field.

Proposed source uses an 8-byte union prefix with `__int64 integer` and
`void *pointer`, a separate current value, and a sentinel with kind and known
value prefix. Only reached variants are declared. The 32-byte tail of each
40-byte payload remains unmodeled. No native full-payload equivalence is claimed.
Native/Win32 natural member padding will be measured and recorded separately.

Before promotion: preserve complete original raw source/cache payloads and kind
at entry/default, BSS-reset phase, callback entry and final frontier. A natural
pair plus explicit positive/negative 64-bit auxiliary seeds must check distinct
old cache, nonzero kind, exact copied numeric prefix, source preservation, both
restore modes, adjacent known fields and ordered cleanup callbacks. Original
40-byte source-to-destination equality is checked only against original raw bytes.
Native checks cover 17 semantic bytes and unchanged own adjacent storage; opaque
tails are neither seeded into native objects nor silently declared equivalent.
Any semantic read, write, escaping pointer, serialization or wider comparison of
unknown bytes must stop at a named stub until that state is reconstructed.
