# Option path resolver

Accepted: `resolve_option_path`, 422 original bytes at 1335b..1344d and
30a3c..30af0. Original VC5/native i386 builds, exact DLL exports and strict
80-function graph/import/RET/ownership checks pass. Application totals are
78 functions / 6749 code bytes. Separate support remains 451 CRT bytes and
12 import-thunk bytes; total 7212 code + 161 data bytes.

The fresh normal case supplies the complete known `vc50.pdb` string. Its
actual first resolver entry is recorded independently of the next boundary.
The function copies nine defined bytes including NUL into its own 260-byte
stack array, then calls the genuine named `normalize_path` INT3. Original stack
arithmetic and native DWARF both prove the local span and its separation from
the caller's 520-byte output array. The local tail and output contents remain
unclaimed. All target state, locale/tables, namespaces, errors and FP remain
unchanged. No new fields or data were added; 4200 initialized bytes and the
separate 16-byte literal tail remain UNMODELED.

Rooted/UNC/lead-byte/relative-component tests, `_fullpath`, fallback and error
branches have complete source/graph checks only. A qualified static `_mbsstr`
ESI witness is needed for the two genuine cached import sites. Its canonical
156-byte provider body saves incoming ESI and restores it before all four
RET0 exits; removing that sole contract rejects the second call. This evidence
was independently reviewed and gives no provider-body or runtime credit.

Restore remains at its previous resolver-entry boundary with an opaque pointer.
Both current capture runners explicitly reject restore before launching Wine
or GDB. Its old config/raw/ELF stay immutable; no literal was substituted. A
future restore variant requires explicitly initialized inputs and its own
reference.

Fresh pair: original 11.136 s, native 10.041 s; full validation 9.425 s.
Original/native builds 2.169/2.141 s, audit 0.054 s. All 26 focused resolver
controls pass in 1.229 s, and both transport-denial controls pass in about
0.1 s each. Prior broad protocol campaigns were not rerun; the new captured
prefix was fully validated. The completed lifecycle fixture remains separate
from this unfinished minimal compilation. `option-resolver-review.json` binds
ordinary proof files and immutable raw/source/ELF references.
