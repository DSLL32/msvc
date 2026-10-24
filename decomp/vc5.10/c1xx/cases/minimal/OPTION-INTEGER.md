# Numeric option leaf (accepted)

The complete 135-byte function comprises `39779..397db` and cold code
`72671..72696`. It skips public `isspace` characters, requires `isdigit`, converts
with signed 32-bit `atol`, accepts 0 through 65534, and otherwise reports diagnostic
49 while preserving the parsed value. The full body remains readable Win32 C and
compiles with the original compiler; the Win32 strict audit includes both chunks,
imports, references, ownership, RET cleanup and exact extents.

This case actually converts the borrowed `Invoke[6]` string `1`. Both captures
observe its true entry, all three original/native public entries and returns,
physical argument slots, numeric return 1, nonvolatile registers, stack cleanup,
full target/snapshot state, locale tables and owners, private CRT/Invoke vectors,
errors and architectural FP. Original complete 112-byte FP is preserved; native
inactive x87 payload/IP differences are not claimed equivalent. Original isdigit
returns 4 and Linux libc returns 2048; only the actual TEST/nonzero consumption is
compared. `atol` is compared exactly. No public CRT body receives APP credit.

The original cached-isspace EDI witness is restricted to the proven finite
unsigned-byte/SBCS1 path. Both real callsites resolve under the strict auditor;
removing that sole contract rejects the cold loop site. Native public addresses
are tied to mapped Linux libc, its retained ELF symbol offsets and the actual
observed 16-byte entry bytes; this is a host dependency identity, not a substitute
original oracle or fabricated target DWARF.

The same observations continue through warning-level store 1, action return, two
additional genuine signature calls (`1`, then `-W`), a fourth iterator result
`-Ze`, and the real pending suffix helper. Its three actual inputs are the rule29
literal interior `*`, borrowed argument interior `e`, and scalar action 35. The
normal case has five signature calls, restore four. Earlier complete action
prefix observations are checked at the real numeric entry, not relabelled as a
later frontier. The final original target state and FP are directly observed;
provider/vector continuity after the fourth matcher entry is derived from its
accepted no-import path, not labelled an additional original API observation.

No fields were introduced. All 6031 unknown initialized bytes remain unknown;
complete original neighbor guards and existing typed nonzero reset/restore tests
remain enforced. Normal/restore each pass 39 new affected semantic controls.
Prior unchanged action controls are retained explicitly as historical provenance.
Whitespace/error/range paths are source and graph coverage, not runtime coverage.

`integer-trial.json` pins both raw receipts, compiled content, fresh strict audits,
current checker hashes, independent observer-source approval and the receipt-only
frontier-field correction. Existing raw and earlier receipts were not rewritten.
The new observer cache entries change no locale observations; removing exactly
those three entries reproduces the old authenticated schema source byte-for-byte.

Read-only validation: `python3 scripts/check-integer-history.py` (Linux only).
Per-command Win32 compile/link: 0.919/0.713 s; native compile/link: 0.123/1.604 s
(parallel build wall time not inferred). Original captures: 71.920/67.147 s;
native GDB observations: 11.620/11.647 s. Full offline pair checks: 12.486/11.920 s;
new normal/restore controls: 6.924/6.612 s. The unchanged cached lifecycle's prior
13.760 ms fixture/2.007 ms execution remains a separate measurement.

The pre-promotion `integer-trial.json` remains unchanged as reviewed history.
`integer-acceptance.json` records the actual successful94th-boundary promotion
and independent review;90 APP functions/6861B, historical CRT451B/import12B
separate, total7324/751216 whole code bytes. Minimal compilation is incomplete.
