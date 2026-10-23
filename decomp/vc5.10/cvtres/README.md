# CVTRES 5.00.1668.1

Five cases match untouched original status, complete streams/artifacts and
application first-entry traces without normalization:

| Case | Status | Stdout bytes | COFF bytes |
| --- | ---: | ---: | ---: |
| `/help` | 1 | 360 | — |
| rich x86 resources | 0 | 0 | 5,080 |
| rich ARM/Windows CE bitmap conversion | 0 | 40 | 4,912 |
| default-machine warning 4001 | 0 | 68 | 5,084 |
| ARM without `/windowsce`, error 1109 | 0 | 74 | 5,080 |

The last case really returns 0 and writes an object despite its “fatal error”
message. The CE case converts the same input bitmap from 24bpp to 2bpp.
All 22 identified application functions, totaling 7,445 bytes, are accepted.
`cases/code-extent-inventory.json` partitions all 9,694 virtual `.text` bytes:
7,445 application code; 400 exact shipped CRT code; 24 import thunks; 756 encountered
application data; 851 loader data; 12 CRT scope data; 84 stale debug records;
107 zero padding;15 INT3 padding. The 34-byte raw alignment tail is zero.
Orphaned debug records remain masked; the report records their exact fields,
stale/unbacked payload offsets and constant-reference scope.

This is extent coverage, not branch/input/API/resource completeness. Only seven
encountered message IDs are provided; unknown IDs fail closed. Valid bitmap
1/2/4/8/24bpp paths are recovered, but the cases exercise 24bpp input and 2bpp
output. The original has no defined C result for unsupported depths; that path,
other resource formats and unexercised option/error branches remain unvalidated.

`make -j4 native` builds i386 Linux; `make -j4 fixtures` checks all five cached
cases without Wine or PE helpers. Latest warm suite: 0.321s including 4.72ms
incremental build check and 13.23ms combined ELF execution. Per-case setup,
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
