This case creates and reuses a debug/browser PCH through genuine -Z7/-Yd,
-Yc/-Yu and -FR flags. Its record, qualifier, callback and traversal source
produces exact frozen PCH/browser/five-IR files. The native consumer consumes
the actual native producer PCH. Two public phases add no new functions.

Ten separate direct phases stop at the real completed metadata_sink call.
Its three ArgumentNode owners come from accepted constructors in one actual
16384-byte arena; initialized fields and allocation/used extents are checked.
Every visited node slot lies outside every current permanent/PCH page. No
compiler globals, tokens, synthetic FILE objects or parser continuation are
introduced. These interfaces are explicitly direct support, without a public
producer claim.

| Function | Original extent | Bytes | Genuine owner/effect |
| --- | --- | ---: | --- |
| mark_pch_ir_node | 44320..443d3 | 179 | Completed argument chain, no relocation/recursion |
| encode_pch_vector | 446a0..446e6 | 70 | Restored six-element saved-defines vector |
| decode_pch_vector | 449a0..449f8 | 88 | Constructor vector and actual encoded references |
| copy_permanent_bytes | 44c00..44c36 | 54 | Actual 24-byte vector storage; capacities32 and10 |
| smaller_size | 44b80..44b91 | 17 | Nested unsigned capacity/source comparison |
| copy_pascal_symbol_name | 4a360..4a3a5 | 69 | Parsed metadata_sink and owned256-byte output |
| symbol_has_qualifiers | 2c420..2c447 | 39 | Real qualified and unqualified scalar declarations |
| suppress_browser_output | 270e0..27122 | 66 | Live FILE/template; one0e packet, second call no-op |
| format_initializer_text | 2a1b0..2a222 | 114 | Independent constructed WorkingStack,15-byte text |

Each new body has fresh original/native pre-prologue entries, whole-body result,
individual ABI/return and fresh accepted-prefix entry-order proof. Existing
constructors establish the vector and formatter owners; their returns, bounds,
cleanup, nonvolatiles and canaries are checked. The formatter's four cdecl slots
are buffer, "%s:%d", actual parsed name and count6; finish/reset/read returns
metadata_sink:6 without replacing the compiler's existing stack. Mutating direct
phases terminate rather than resume parsing.

Both vector interfaces compute values but never store them: the original
missing-store bugs are preserved. Copy tests compare the defined copied prefix
against each side's actual source and compare untouched storage only within
that side. Uninitialized new allocation tails are never compared across sides.
The Pascal result contract is AX only; void helpers have no invented EAX result.
All seven pending streams and every owned input/partial file are compared.

The raw initial cache discrepancy is retained: reference0/baseNULL with page
original65535 versus native0. Every actual nonNULL walker slot has unsigned
offset above0xeffc, forcing accepted lookup to write the page before consumption;
NULL slots return without reading it. Vector lookup initializes the cache, with
strict full post-state afterward. Other direct methods do not consume it and
preserve their complete respective raw states. The orchestrator reviewed this
exact qualification; no cache value is initialized or repaired to hide it.

check-metadata-tests.py validates all frozen oracle hashes/invocation/input and
entry-prefix bindings, then runs ten instrumented native units. It invokes no
Wine, GDB, originals or recapture. Fresh GDB observations remain separate raw
acceptance evidence. The first full ten-unit cached run took0.330 seconds.
Thirty generated masked images are outside Git; exact recipes reproduce their
hashes. Fixtures require only committed owned inputs and oracles. Retained
transport failures are distinguished from successful observations in the proof
catalog; packaging caused no new original invocation.

Nine application functions add696 bytes:973 accepted application functions and
228261 application bytes; total accepted extents250999/277234 (90.5369%, including
CRT/imports). This is extent coverage, not branch coverage or a complete non-CRT
denominator. Eleven historical source CRT bodies are unchanged. The self-based
token validator, one-byte RET ownership ambiguities, unattributed byte copier
and other unlisted/mixed candidates remain unresolved. Final checks and elapsed
timings are recorded in validation.json.
