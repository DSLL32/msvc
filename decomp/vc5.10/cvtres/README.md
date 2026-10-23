# CVTRES 5.00.1668.1

Nine cases match untouched original status, complete streams/artifacts and
application first-entry traces without normalization:

| Case | Status | Stdout bytes | Object bytes |
| --- | ---: | ---: | ---: |
| `/help` | 1 | 360 | — |
| rich x86 resources | 0 | 0 | 5,080 |
| rich ARM/Windows CE bitmap conversion | 0 | 40 | 4,912 |
| default-machine warning 4001 | 0 | 68 | 5,084 |
| ARM without `/windowsce`, error 1109 | 0 | 74 | 5,080 |
| 4/8bpp CE palettes, verbose | 0 | 476 | 1,164 |
| duplicate BITMAP | 1 | 566 | 0 |
| duplicate named type/name | 1 | 593 | 0 |
| duplicate unknown ordinal type | 1 | 559 | 0 |

The ARM-without-WindowsCE case returns 0 and writes an object despite its “fatal error”
message. The CE case converts the same input bitmap from 24bpp to 2bpp.
All 22 identified application functions, totaling 7,445 bytes, are accepted.
`cases/code-extent-inventory.json` partitions all 9,694 virtual `.text` bytes:
7,445 application code; 400 exact shipped CRT code; 24 import thunks; 756 encountered
application data; 851 loader data; 12 CRT scope data; 84 stale debug records;
107 zero padding; 15 INT3 padding. The 34-byte raw alignment tail is zero.
Orphaned debug records remain masked; the report records their exact fields,
stale/unbacked payload offsets and constant-reference scope.

This is extent coverage, not branch/input/API/resource completeness. Only 18
encountered message IDs are provided; unknown IDs fail closed. Valid bitmap
1/2/4/8/24bpp paths are recovered; cases exercise 4/8/24bpp input and 2bpp
output. For unsupported depths the original falls through with AX derived from
bits and residual high EAX. That result remains unvalidated; native execution
traps instead of inventing a return. One direct debugger rejection checks the
actual ELF INT3, without claiming a public 16bpp case. Original-compiler pixel
code/data and relocations are byte-identical before/after this native guard.
Other resource formats and unexercised option/error branches remain unvalidated.

`make -j4 native` builds i386 Linux; `make -j4 fixtures` checks all nine cached
cases without Wine or PE helpers. Latest warm suite: 0.610s including 4.73ms
incremental build check and 25.59ms combined ELF execution. Per-case setup,
execution and checks are in `cases/fixture-times.json`.
`make -j4 original` explicitly compiles Win32 C/resources with the shipped 5.10
compiler and shared Wine prefix; fixtures do not depend on that target.

Fresh acceptance uses `scripts/capture.py --candidate NAME --output NEW_DIR`
(original only), `capture-native.py --original ORIGINAL_DIR --output NEW_DIR`
(Linux/GDB), `audit.py --output NEW_DIR`, then `accept.py`. It binds actual
candidate entry, exact mask/config, immutable compiled-source/binary snapshots,
typed state and either a real ELF INT3 or canonical completion. Shared shim or
validator changes do not force unchanged original captures to run again.
Static refresh uses `audit.py --trial ORIGINAL_DIR/configuration.json
--artifacts PRIOR_AUDIT_DIR --output NEW_AUDIT_DIR`; old packets stay immutable.

The shared Rust tool checks full candidate CFG calls, RET cleanup, references
and extent ownership/boundaries. Strict imports cover supported direct calls.
`import-residual.py` proves ten fixed cached-register intervals by exact IAT
slots, callsites, writes, bypass branches and rebuilt COFF relocations; it does
no CFG discovery. Typed checks include FILE32/buffers/physical positions,
resource identifier/tree aliases, bitmap buffers, full 1024-byte LoadStringA
callbacks and cdecl va_list/vprintf state. Rejection controls mutate these fields,
switch/classmap data, imports and both-sides-wrong completed outputs.

Original captures verify the actual loaded 277,776-byte provided MSVCRT.DLL,
SHA256 `e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210`.
Native builds use its reviewed VC5 CRT profile. Encountered `.rc` entries,
cached `.res` bytes and source hashes are checked before shared resource encoding.
See each `cases/*/completion.json` and `time-accounting.json` for frozen proofs
and measured phases. PE/ELF/build artifacts remain in build/shared scratch.

The bounded final group is complete in `cases/final-validation-group/completion.json`.
Its named-resource output exposed a shared `%S`/WCHAR16 formatting gap, fixed in
`lib/recon` commit `4ec9223d`. Five actual calls now match full terminated arguments,
following arguments, return counts and FILE32/pending stdout. The supported slice
is default-C ASCII with the reached plain `%S`, `%c` and lowercase `%.4x` grammar;
unknown wide formats/locales fail closed. Fifteen new-state controls ran; 52
unchanged protocol controls are reused with explicit prior-report/validator
hashes. Four cases add validation and encountered message data, with no new
application-function credit. CVTRES is idle; there is no further queued case.
