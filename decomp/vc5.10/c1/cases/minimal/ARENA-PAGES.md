# Arena page allocation

The73-byte `CompilerArena::AllocatePages` memberaacb..ab14 calculates the
available byte count from requested pages, page size and current offset, writes
it through the caller's actual unsigned-count pointer, then advances cursor.
It calls the genuine CommitExtent member when cursor exceeds committed_limit,
and returns the previous cursor. The return has not executed in this trial.

Both fresh pairs stop at CommitExtent9883. Actual allocator entry records prove
arena0, one page and the owned pool8.remaining output address. Exactly two
original BSS writes occur: remaining becomes4096 and arena cursor becomes
base+4096. Complete original regions, native modeled state, neighbor fields,
real formal inventories and caller/receiver/stack identities match. Limit remains
base and committed bytes remain0; reservation contents are still opaque. The
pending helper reloads cursor itself and has only the ECX receiver, with RET0.

Strict graph/ABI/ownership checks pass67 source functions/3463 code bytes plus16
switch-data bytes. Application accounting is65 functions/3000 code bytes. Both
paired positives and26 fresh affected controls pass; unchanged prefix campaigns
are explicitly reused. Original/native builds took3.556/1.990s, audit0.032s,
original/native captures2.370/7.472s normal and2.470/8.136s restore, controls7.807s,
and native lifecycle fixture4.03ms excluding build. Frozen proofs and retained
external artifact identities are in `evidence/arena-pages-*`.

Minimal compilation continues at CommitExtent. Initialized state remains64 known
and6484 UNMODELED bytes; no broader array, reserved-byte or address-order claim
is made.
