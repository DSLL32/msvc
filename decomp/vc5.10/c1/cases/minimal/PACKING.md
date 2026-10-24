# Packing stack initialization

Accepted normal-only: `initialize_packing`, 70 original bytes at
1348d..134c9 and 30af8..30b02. Its actual first call reaches the genuine
`allocate_packing_memory(8)` INT3 before any target-state write or new object
access. The physical initializer entry independently anchors all preceding
normalizer/parser continuation state.

The full initializer is readable period source. Its original member call uses
a small C++ translation unit with the actual implicit thiscall convention; no
ABI wrapper, constructor, RTTI or exception machinery is introduced. The
8-byte packing object contains active/free-frame heads. The encountered frame
links its predecessor and holds alignment plus an opaque tag identity. Pending
member/allocation bodies remain named traps and receive no reconstruction credit.

Both builds and strict82-source graph/import/RET/ownership audit pass. The fresh
original/native pair takes11.542/10.165s; native recorded build2.152s and audit
0.052s. Whole original data/BSS/snapshot/outside state and locale/vectors/errors/FP
are unchanged; all native modeled state matches. Full prefix validation passes.
24 fresh boundary controls pass0.931s and44 refreshed affected prefix controls
pass4.563s. Original build/full-validation elapsed times were not separately
measured; earlier broad protocol campaigns were retained rather than repeated.

Totals:80 application functions/6891 code bytes,451 separate CRT support bytes,
12 import bytes and161 text-data bytes. No data fields were added;4200 initialized
bytes and16 literal-tail bytes remain UNMODELED. The old restore input remains
held before its unknown resolver read. `packing-initializer-review.json` binds
the ordinary proof files and retained raw/helper/source/ELF identities.
