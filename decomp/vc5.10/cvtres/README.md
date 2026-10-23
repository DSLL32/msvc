# CVTRES groundwork

Target: `toolchains/5.10/bin/CVTRES.EXE`, version 5.00.1668.1. No functions
have been reconstructed or accepted. Work is held pending the orchestrator's DLL
workflow review and Rust graph-audit readiness.

`make inspect` inventories the PE without running it. Build, audit and fixture
targets deliberately fail until reconstruction is released. The future builds
must support `make -j4`, original Win32 compilation and native i386 testing.

The image is a fixed-base i386 console EXE, base `0x01000000`, entry
`0x01003180`, with no exports or TLS. Its `.text` virtual size is 9,694 bytes;
that includes loader metadata and data, not just instructions. The IAT starts at
RVA `0x1000`; imports and names also occupy `.text`. Masking must distinguish
these proven loader-owned ranges from executable instructions. See
[cases/pe-inventory.json](cases/pe-inventory.json).

The only static dependencies are MSVCRT.dll and KERNEL32.dll. The latter supplies
LoadLibraryA/GetProcAddress. A full-body analysis of the routine at `0x0100310a`
identifies a cached USER32.DLL!LoadStringA lookup and a 1,024-byte destination
buffer for this executable's own resource strings. This is a future shared shim
question, already raised with `/root/recon_lib_maintenance`; no compatibility
implementation has been added. VC1's CRT FILE layout is not assumed valid here.
The shipped compiler is being verified by the CL worker; PE linker version 6.0
alone does not establish the exact compiler that produced this target.

Initial cases are `/help` and resource conversion. Fresh original-only help
probes found `/help` and `/?` both return 1 with the same 360-byte stdout and
empty stderr. No arguments print identical help but return 0. Raw bytes and
receipts are in `cases/help/probe/`; these are observations, not acceptance
oracles or traces. Options advertised are `/MACHINE`, `/NOLOGO`, `/OUT`,
`/READONLY`, `/VERBOSE` and `/WINDOWSCE`. The RC worker owns fresh resource
compilation; its menus/dialogs/strings/icon/raw-data source is the seed for the
conversion case. No VC1 reference output is reused.

Explicit original-only probes can be captured using
`python3 scripts/capture-probe.py help --output NEW_DIRECTORY`, or
`python3 scripts/capture-probe.py convert --res ORIGINAL_RC_OUTPUT.res --output NEW_DIRECTORY`.
These commands use the shared Wine prefix and isolated scratch under
`S:\vc5.10\cvtres`; they are never part of fixture execution. Future native
fixtures consume complete captured `.res` input and compare complete COFF output,
streams, status and application-entry traces. Any timestamp normalization must
first be identified in repeated original outputs. LINK's resource invocation is
being investigated by its worker; native LINK fixtures must replay a captured
CVTRES child or use the native reconstructed child, never invoke Wine.

Current probe blocker: resource compilation and conversion are paused under the
root shared-Wine health hold. RC's resource probes and a separate CVTRES loader
diagnostic stalled before producing output. The successful help observations
predate that incident. No timeout is represented as a successful resource case;
no further Wine launch is allowed until the hold is released. Resource source
and assets are copied locally under `cases/resources-rich/source` with hashes
in `cases/plan.json`.

LINK's worker identified static child invocation at `0x0041031b`: `_spawnv`
with a sibling CVTRES path, falling back at `0x00410339` to `_spawnvp` searching
`cvtres.exe`. Arguments include a machine buffer, `/nologo`, `/readonly`,
conditional Windows CE switches and a quoted resource filename. This is
call-site evidence only; an actual executed child capture remains pending.
