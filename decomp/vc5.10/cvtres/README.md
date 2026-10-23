# CVTRES 5.00.1668.1

Two cases are complete: `/help` returns 1 with 360 stdout bytes; rich x86 resource
conversion returns 0 with empty streams and an exact 5,080-byte COFF file. Both
match untouched original references without normalization. Fourteen application
functions totaling 5,614 extent bytes are accepted. The current 17-function,
6,840-byte declared inventory is incomplete; no whole-program percentage is
claimed. Eight identified CRT support functions total 424 bytes separately.
Imports and verified data do not count as reconstructed application code.

`make -j4 native` builds i386 Linux. `make -j4 fixtures` checks both cached cases:
status, complete streams/artifacts, immutable inputs and application first-entry
traces. It never launches Wine or a PE helper. Latest two-case time is recorded
in `cases/resources-rich/fixture-times.json`, split into build/setup/execution/
checks. Resource-provider preparation consumes verified cached strings only.
`make -j4 original` explicitly compiles Win32 C/resources with the shipped 5.10
compiler and shared Wine prefix; fixtures do not depend on that target.

Fresh function acceptance uses `scripts/capture.py --candidate NAME --output
NEW_DIR` (original-only), `capture-native.py --original ORIGINAL_DIR --output
NEW_DIR` (Linux/GDB), `audit.py --output NEW_DIR`, then `accept.py`. Evidence binds
actual entered candidates, exact masks/configurations, immutable compiled-source
snapshots, typed state and either an actual ELF INT3 or canonical completion.
Unchanged originals can be reused explicitly with native-only captures after
shared shim changes; old raw receipts remain immutable.

The shared Rust tool checks full candidate CFG calls, RET cleanup, references
and extent ownership/boundaries. Strict imports cover supported direct calls.
`import-residual.py` proves six fixed cached-register live intervals in main,
usage and duplicate-resource diagnostics using exact IAT slots, calls, writes,
bypass branches and rebuilt COFF relocations; it performs no CFG discovery.
Typed debugger checks cover FILE32/buffers/OS flags/physical positions, resource
identifier/header fields, bounded resource-tree pointer aliases and relative
directory contents. Negative checks reject changed fields, aliasing, imports,
and both-sides-wrong completed artifacts/status/streams.

Original captures require the actual loaded provided 277,776-byte MSVCRT.DLL,
SHA256 `e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210`.
The native build opts into its reviewed VC5 CRT profile. `src/messages.rc`, its
cached `.res` and encountered UTF16 entries are cross-checked before shared
resource encoding. The target is a fixed-base i386 EXE at `0x01000000`, entry
`0x01003180`, without TLS/exports. Its 9,694-byte virtual `.text` also contains
loader data/literals and is not a code-coverage denominator.

See `cases/help/` and `cases/resources-rich/` for completion/proof/timing records.
PE/ELF/build artifacts and immutable captures remain in build/shared scratch.
The queued ARM/Windows CE reference uses the same fresh VC5 RC input and
actually converts the bitmap from 24bpp to 2bpp; its reconstruction awaits review
of this completed x86 checkpoint.
