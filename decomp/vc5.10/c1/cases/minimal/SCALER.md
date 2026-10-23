# Reservation-size calculation

The sole application candidate owns57 bytes: hot12a7a..12aac and cold30524..3052b.
Its signed64 C cast preserves the original QWORD conversion, followed by the
actually consumed low32-bit commit-unit rounding. Both original5.10 and native
builds compile the same source in `src/arenas.cpp`. The existing6-byte `_ftol`
import thunk is public CRT compiler support, with zero application credit; its
canonical MSVCRT export ordinal239/RVA4dc1/full39-byte body is independently bound.
No CRT implementation was added to the target source.

Both fresh original cases reach the real `CompilerArena::Reset` frontier.
After shared support was reviewed, only the native side was rebuilt and captured
again. All equivalent x87 checkpoints, signed QWORD, rounded return, restored
control word, status, tags and remaining empty stack match. The native compiler's
two additional temporary stack values are explicitly checked, including exact
operands, depths, status and ownership of otherwise empty registers. The normal
QWORD/result are -52428800/52428800; restore uses -43515904/43515904.

The native host applies the captured architectural input once immediately before
the first DLL lifecycle call: CW027f, SW0020, TAGffff, no live registers. Its
reference is the exact original receipt. Shared `c59ca5fb` preserves unrelated
native FP fields and errors; no scaler/API reseeding or raw-context loading occurs.
Shared `24be87d5` supplies the reached NULL RESERVE|TOP_DOWN READWRITE subset as
a real private, inaccessible,64KiB-aligned reservation. The proofs compare exact
size, errors, callee cleanup/preserved registers and owned returned identity.
They claim neither reservation contents nor highest-address allocation order.

Both complete candidate-inclusive62-function audits and paired state checks pass:
3185 code bytes plus16 owned switch-data bytes. At Reset, only arena0's descriptor,
reserve size and base have changed; every other modeled field and complete
original data/BSS/snapshot region is checked. Reset's sole real `this` formal,
preprologue ECX/stack and caller return are recorded independently. Its body is
still an INT3 stub. Initialized state remains64 known bytes plus6484 UNMODELED;
no full snapshot or reserved-memory equivalence is claimed.

Fresh47 semantic mutations and11 malformed-ELF controls pass in8.53s; unchanged
core controls are explicitly reused with pinned historical identities. Original
build1.617s, native build1.889s, audit0.032s. Original/native captures are
2.822/2.748s normal and2.521/3.714s restore. The native-only lifecycle fixture
passes in4.47ms excluding build. Evidence is in `evidence/scaler-{normal,clear-aux}`.
The independent review passed; the57-byte scaler and6-byte import support are
accepted. The review packets remain unchanged history; acceptance proofs pin
the final review. Minimal compilation is still in progress.
