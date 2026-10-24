# Argument cursor

`next_argument` is the complete 34-byte cdecl/no-argument leaf at
`39428..3944a`. It decrements the signed remaining argument count, advances the
borrowed argument-vector pointer by one element when positive and returns that
string; otherwise it returns NULL. Its two BSS fields were already modeled,
reset and included in the existing nonzero state probes. No new data was added.

The fresh acceptance execution is **restore-nonzero only**: its existing
`option_c9il` value skips the unsupported signature import. The real Invoke
argument vector has 26 elements, separate from the original host CRT vector.
The candidate returns the owned vector's borrowed slot 1 (`-il` including NUL),
changes count 26 to 25 and advances its cursor by four bytes. All other target,
initialized/saved, locale/CRT vector/error and architectural FP state remains
unchanged. Actual entry, RET0, caller instruction and nonvolatile registers are
observed independently on both sides.

The real next named INT3 frontier is `match_option` at `3944a`, cdecl with four
parameters: validated grammar table, borrowed argument string, the real
`next_argument` callback and integer `copy=0`. All four physical arguments and
their original/native owners are checked. The original caller's stack slots
belonging to the pending matcher are not invented iterator parameters.

The vector checker uses the directly captured iterator entry as its completed
prefix checkpoint. The iterator checker owns its subsequent return and genuine
matcher frontier. This is a new whole candidate/mask/audit and fresh paired run;
it does not relabel an earlier vector capture. The prior normal reference remains
historical at the genuine unsupported MSPDB entry. No normal iterator execution,
provider result or runtime exercise of the exhausted-argument branch is claimed.
The full leaf source and function graph cover both branches.

Evidence is `argument-trial.json`, with ordinary immutable raw files referenced
by path/hash and 24 fresh affected negative controls. Unchanged prefix/transport
controls retain their earlier vector packet provenance. Revalidate without
executing either binary:

```
python3 scripts/check-argument-history.py
```

The original capture took 36.281 seconds; native GDB observation took 11.309
seconds, including the existing full reconstruction prefix. Original/native
compilation took 0.789/0.136 seconds, linking 0.726/1.589 seconds, and the strict
Rust audit 0.125 seconds. These are reconstruction observations, not standalone
fixture timings. The native ELF uses the prior published c11 shared sources and
skips the signature API; later shared additions do not retroactively change it.

The prepared standalone CPP signature probe was never executed. Root selected a
small shared integer CRC implementation instead. `signature-reference-plan.json`
is retained as withdrawn historical preparation; its compiled helper/source
manifest is under the scratch path recorded in `SIGNATURE-PREPARATION.md`.
Actual CPP call/result evidence belongs to the next necessary ordinary matcher
candidate run.
