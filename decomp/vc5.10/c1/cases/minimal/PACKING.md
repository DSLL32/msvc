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

The separate allocator is now accepted:43 hot bytes134c9..134f4 and15 cold
bytes30b02..30b11. It advances arena0 by the unsigned four-byte-rounded request,
calls the existing commit member only if necessary, and returns the old cursor.
Actual request8 fits the prior64KiB commit. All8 newly owned bytes are observed
unchanged zero on entry, return and next member entry; the allocator itself does
not clear memory. The caller publishes that exact pointer and reaches the real
`CompilerPacking::Push` with ECX receiver and no stack arguments.

The full original/native pair takes13.545/10.238s, full validation9.826s,
original/native builds1.495/2.143s and audit0.053s.40 focused allocation/return/
receiver controls pass2.343s. Prior unchanged protocol controls are retained
with historical hashes and are not claimed as rerun. No global data fields are
added. Totals are81 application functions/6949 code bytes, separate451 CRT,
12 import and161 text-data bytes. Evidence: `packing-allocation-review.json`.

`CompilerPacking::Push` is accepted:67 hot bytesd458..d49b plus21 cold bytes
2d999..2d9ae. Actual ECX/this and8-byte object layout are independently observed.
The member advances arena0 by12, links the owned frame with previousNULL,
publishes active and returns that frame in EAX with RET0/nonvolatiles intact.
The initializer writes alignment8/tagNULL before the genuine
`push_packing(0,NULL)` frontier. The12-byte frame has previous/alignment/opaque
tag at0/4/8. No new global fields or initialized unknown bytes are introduced.
The next normal call will exercise a naturally nonzero predecessor; the current
free-list and commit-overflow branches have source/graph checks only.

The full composed check caught two earlier native memory artifacts sampled
at the final trap. These had remained zero before the packing member's new
nonzero active link. Pool memory is now read at physical parse_command_line
entry; alias memory at physical initialize_checksums entry. Both reads have
write-once files and exact PC/caller/owner/base/extent metadata. The earlier
receipts and observation limitation remain unchanged and explicitly documented.
20 new phase/owner/single-read controls reject wrong boundaries and replacing
earlier zero bytes with actual later nonzero object bytes even with a fresh hash.

One original capture took13.759s. Native attempts took10.221s (missing module
context),10.350s (detected wrong snapshot phase), then10.340s (complete corrected
proof); both failures are retained, and original/ELF bytes were reused unchanged.
The full successful check took10.218s,43 member controls1.871s and20 prefix-phase
controls0.081s. Original/native builds5.123/2.166s; strict84-source audit0.057s.
Totals:82 application functions/7037 code bytes,451 separate CRT support,12
imports and161 data bytes. `packing-frame-review.json` is the immutable proof
packet; `packing-frame-acceptance.json` records promotion.
