# Initializer and real allocator trial — promotion held

Durable accepted ownership remains 664 original bytes, four source functions
(two application/two CRT) and one import thunk. This **two-candidate trial** adds
`initialize_compiler_pass` (318 bytes) and `allocate_compiler_memory` (44 bytes).
Neither is accepted. The candidate-inclusive Rust audit checks six source
functions and 1,026 owned original bytes with imports, RET, ownership and whole
hot/cold extents. These are real bodies on both sides; no allocator stub return
is used. Root/orchestrator review must settle staged versus joint acceptance.

Source: `src/pass.c`, `src/compiler-state.h`, `src/compiler-state.c`, and all
33 named INT3 consumers in `src/initializers.c`. Metadata: `candidate.json`,
`trial.json`, `field-map.json`. All 34 actual `.initfe` slots, including the NULL
first slot, are compared with actual native ELF pointers and captured original
loaded pointers. No unknown callback executes.

Only currently recovered fields are projected: initialized `first_call`, BSS
saved pointer/cleanup count/32 callback pointers, and the outside-BSS reuse and
pass-mode flags. The initialized region has 6,548 historical bytes; only four
are modeled. The historical BSS range has 41,068 bytes; 136 are modeled. Other
bytes remain **UNMODELED**, never assumed zero, undefined or equivalent. The
native allocation request remains exactly 6,548; the source copies the complete
currently modeled typed initialized state. Every newly recovered field must join
the typed snapshot/restore/default map before any consumer can execute.

All original snapshots, including complete initialized/BSS/allocation bytes,
remain in private scratch and are hash-listed in evidence manifests. They are
not compiled, loaded into the native program, or used as native input. The
complete original environment also remains private; only its identity is kept.

From the binary root, current checks are:

```
python3 scripts/check-trial.py /root/.ai-tmp/msvc2/vc5.10/c1/minimal-init-callback-001 /root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-init-callback-005
python3 scripts/check-trial.py /root/.ai-tmp/msvc2/vc5.10/c1/minimal-restore-001 /root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-restore-003
```

Run `scripts/check-state-negative.py` with either same pair: 18 normal and 25
restore corruption controls pass. These cover allocation size, live/snapshot
aliasing, field-map overlap/offsets, defaults, omitted/swapped callbacks, nonzero
restore input, retained allocation identity and actual post-restore PC/state.

The preserved earlier natural initializer-to-allocator frontier is under
`evidence/initializer-allocation/`; it proves the 6,548-byte call before copying.
`evidence/initializer-real-allocation/` now proves the genuine successful allocator
and first copy: live first-call flag0, saved flag1, correct module/CRT/heap state,
then the actual first callback's named trap at RVA0x12365. The malloc failure
path retains a named `compiler_fatal(60)` trap and is not exercised.

`evidence/initializer-restore/` is a separate explicit debugger-seeded code probe.
The original receives an owned remote allocation seeded with its own complete
original data and a mapped first-call value0x13579bdf. The native receives only
mapped typed inputs. Both start with live first-call0, cleanup count7 and two
actual callback identities. The original observer at RVA0x303d5 and the native
source breakpoint independently show nonzero typed restoration, cleared modeled
BSS and preserved saved-pointer identity. The original's complete runtime copy
is also checked against its own original seed; no cross-platform unknown-byte
comparison is claimed. Both then reach the same unknown callback and trap.
The observer lies inside the audited candidate's cold chunk; no extra code is
unmasked outside the exact trial ownership.

Measured runs: normal original 0.867s/native GDB 0.155s, restore original 0.867s/native
GDB 0.176s, candidate audit 0.014s. Build times for this particular trial were not
separately measured. The existing lifecycle fixture still runs only Linux code
and checks cached streams/status/application trace; it passes in about 4ms.

This probe does not establish exception dispatch, repeated compilation completion,
unknown data equivalence or compiler output equivalence. The untouched original
minimal output reference remains separate from reconstruction acceptance.
