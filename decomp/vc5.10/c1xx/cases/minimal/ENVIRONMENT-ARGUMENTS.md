The environment-argument parser is one338-byte cdecl function: hot39413..39428
and cold72436..72573. Its readable source in `src/options.c` retains the full
in-place space/quote/backslash/multibyte parser. The actual minimal case supplies
NULL, returns without touching target state, and its caller applies ten warning
defaults before reaching `new_option_vector(40, 1)` at682f. The cold branch is
compiled and graph-checked, but has no runtime coverage from this case. Its
append-slot helper is a genuine named INT3 stub, not a simulated allocator.

Both fresh original/native pairs observe the real parser entry/formal, caller,
return PC, RET0 stack effect and nonvolatile registers. They compare complete
original BSS/initialized/snapshot bytes before/after this NULL call and complete
native modeled state, padding and saved state. The caller's only subsequent
writes are the ten independently recovered warning bytes; the allocator traps
before producing a result or assigning any vector pointer. Both its actual
source formals and values40/1 are checked. Provider-table/result owners, private
CRT/Invoke vectors, errors0/0/203 and architectural x87 state retain their prior
identities and values. No semantic parser EAX return is claimed.

The restore auxiliary retains the existing declared nonzero typed data seeds;
it does not replace a code body, argument, API return or allocation. It proves
these older fields still restore/reset and that the ten live warning writes do
not mutate the saved copy. No new fields were introduced. Original6031 unknown
initialized bytes and future opaque consumers remain unmodeled.

`env-arguments-trial.json` pins the separate raw receipts, source/build identities,
strict audits and validation results. The initially denied native setup retained
a stale whole rebuilt-DLL hash in older scanner-default evidence; the correction
authenticates the actual frozen trial DLL and still verifies its exact512-byte
initializer. It requires no original replay or historical relabeling. Earlier
controller-negative campaigns remain referenced in their original packets;
16 fresh affected ABI/state/owner/vector/next-frontier mutants reject.

Original reference runs took31.920/33.698s; native GDB captures10.145/10.772s.
Those captures retain the rich earlier prefix and256/255 public case-map calls;
they are not standalone parser timings. The focused controls take0.784s plus an
8.673s full positive. The ordinary cached native lifecycle fixture takes14.990ms
(build5.619/setup0.951/execution3.368/checks4.250ms), with zero Wine invocations.
The measured original object/link steps take1.105/0.725s; native object/link
0.136/1.662s; strict graph audit0.121s. These are individual steps, not summed
parallel wall time.

The checkpoint is85 application functions/5773code bytes, historical CRT451B
and generated imports12B separately:6236/751216 whole code bytes, not non-CRT
coverage. The real C++ compilation remains incomplete.

Revalidate both immutable pairs without execution or promotion:
`python3 scripts/check-env-arguments-history.py`.
