# Path normalizer

Accepted normal-only: `normalize_path`, 72 original code bytes at
1344d..1348d and 30af0..30af8. The original VC5 and native i386 builds and
strict 81-source graph/import/RET/ownership audit pass. Totals are 79
application functions / 6821 code bytes; separate support is 451 CRT bytes
and 12 import bytes, with 161 text-data bytes.

Both actual leaf calls, the authenticated canonical `_fullpath` entry/return,
the resolver return and the real `initialize_packing` frontier are observed.
The API receives capacity 520 and produces 57 defined bytes for this case.
Returned pointer aliases, complete defined strings, each buffer's own untouched
tail, namespaces, errors, FP, locale owners/tables and all modeled target state
match. Five real parser continuation writes are checked separately.

The native process uses an owned private S: tree mirroring the original logical
cwd, with the source copied and hash-bound. It never executes in or writes the
original reference directory. Existing shared drive/cwd controls suffice.

Original leaf code reuses the low byte of its incoming argument slot; original
`_fullpath` reuses its third incoming slot. Raw before/after words are retained.
Exact caller cleanup proves those discarded slots have no later read or escape;
no cross-platform equality is claimed for those private representations. The
real by-value APIs, meaningful arguments and consumed results remain strict.

No new data fields are introduced. The 4200 unknown initialized bytes and
separate 16-byte literal tail remain UNMODELED. The prior restore case is held
before its opaque resolver input; it does not inherit normal-only acceptance.

Original/native builds took 1.332/2.209 s, captures 10.585/9.837 s, static audit
0.052 s and full validation 9.934 s. All 44 focused corruption controls pass
in 4.378 s. Parent independently repeated the full check in 10.467 s and reviewed
the retained controls without rerunning them. Prior broad campaigns were reused,
not repeated. These observer costs are separate from the cached lifecycle fixture.

`path-normalizer-review.json` stays as the immutable pre-promotion packet;
`path-normalizer-acceptance.json` records its reviewed promotion and parent proof.
