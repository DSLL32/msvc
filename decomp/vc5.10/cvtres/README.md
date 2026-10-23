# CVTRES 5.00.1668.1

First case `/help` is complete; reconstruction is paused before resource
conversion. It returns 1, writes exactly 360 stdout bytes and no stderr, matching
the untouched canonical original. Three application functions are accepted:
`main` (1,447 bytes), `usage` (69), and `load_message` (107).

Use `make -j4 native` and `make -j4 fixtures` for native i386 Linux builds/tests.
Fixtures compare status, complete streams and filtered first-entry traces with
cached originals. They prepare two verified cached resource strings through
`lib/recon/resources.py`; they never launch Wine or parse/run a PE helper.
`make -j4 original` explicitly compiles Win32 C/resources using the shipped 5.10
toolchain and shared repository prefix. `make -j4 audit` includes that explicit
original build and the common Rust graph audit; it is outside fixture execution.

Fresh acceptance uses `scripts/capture.py --candidate NAME --output NEW_DIR`
(original-only), `scripts/capture-native.py --original ORIGINAL_DIR --output
NEW_DIR` (GDB/Linux), and `scripts/audit.py --output NEW_DIR`. `accept.py` accepts
only an exact candidate entered in both raw traces, matching a real INT3 frontier
or canonical completion, typed state and a bound exact-trial audit. The native
observer binds an INT3 PC to the actual ELF symbol and byte. Immutable build
snapshots preserve compiled source contents. For unchanged original evidence,
`capture-native.py --configuration ORIGINAL_DIR/configuration.json` and
`audit.py --trial ORIGINAL_DIR/configuration.json` support explicit native-only
refresh after shared shim changes; the accepted ledger stays untouched.

The full three-function Rust audit checks CFG calls, RET cleanup, references and
extent ownership/boundaries. Its strict-import subset checks `load_message`'s
KERNEL32 imports. Main and usage cache CRT function pointers in callee-saved
registers, beyond that import check's resolver; `import-residual.py` verifies
five explicit live regions, all writes/bypass branches/callsites and the rebuilt
COFF relocations against qualified IAT slots. It performs no CFG discovery.

Target `toolchains/5.10/bin/CVTRES.EXE` is a fixed-base i386 EXE at `0x01000000`,
entry `0x01003180`, without TLS or exports. The 9,694-byte virtual `.text`
contains loader metadata and literals, so it is not an application-code coverage
denominator. Accepted application extents total 1,623 bytes; the seven currently
declared application extents total 4,446 bytes, an incomplete inventory.
Separately, eight precisely library-identified CRT support extents total 424
bytes. Imports and verified data are excluded from application acceptance.
Masking preserves only parsed loader data and explicitly proven code/data ranges.

Original captures require the actually loaded provided MSVCRT.DLL, size 277,776,
SHA256 `e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210`.
Both resource calls and returns are observed: IDs 8003/8004, NULL module,
1,024-byte buffer, dynamic USER32 binding, complete buffer state and pending CRT
output. `src/messages.rc`, its cached `.res` and original encountered UTF16
entries are cross-checked before the shared native provider is encoded.

See `cases/help/` for canonical outputs, acceptance proofs, negative checks and
timings. PE/ELF/build artifacts and full capture snapshots remain in build/shared
scratch. The source-only `cases/resources-rich/` seed is not an executed case.
