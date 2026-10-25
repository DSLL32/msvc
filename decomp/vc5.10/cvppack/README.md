CVPACK 5.10 reconstruction
========================

`make -j4 check unit` runs the two cached cases and narrow native tests through
the shared runner. `make -j4 win32 audit` checks original-compiler compatibility
and the function graph/ABIs. Fixtures never invoke Wine.

The accepted runnable baseline is 117 application functions totaling 40,111 original code
bytes. This is 64.01% of the 62,662-byte whole-text extent;
that denominator also contains CRT/support, unknown code and padding. A complete
non-CRT denominator is not established. CRT/import selection earns no App credit.

0000-help completes with status 1, 193 stdout bytes and empty stderr.
0001-hello-debug-dll invokes `/nologo hello.dll` on the original embedded-CodeView
DLL. Its current frontier is the named `end_buffered_output` stub (RVA 0x10240). Its capture distinguishes the
actual partial artifact from the final NB11 master. The active case uses a fresh env={} original reference; prior capture context remains
in Git history. Native module-path setup is explicit host metadata; no equality
with historical or future original staging paths is asserted. A future real
module-path consumer requires fresh input qualification.

`src/` contains application reconstruction; `harness/` contains the native host,
strict dependency adapter and its ABI glue. `crt/crt-imports.h` declares the
original direct `_iob` import to avoid introducing the SDK accessor call. The
shipped MSVCRT supplies its implementation; this header earns no body credit.

The native MSPDB adapter is a strict 21-lifetime typed response provider. Only
the captured factory/Query/Emit/Release sequence and complete inputs are allowed;
unknown methods or inputs fail. It writes observed output prefixes and preserves
each native buffer's own unwritten capacity. It is outside the App graph.

`binary.c` is the sole function/extent/status registry. `policy-data.json` holds
live finite graph/ABI/table rules; `abi-policy.json` records their original/provider/SDK
premises. The exact opaque 8-byte Release tail has its reviewed ABI residual;
every other accepted body retains strict cleanup checks. Historical acceptance
proofs and superseded observers remain in Git history rather than fixture data.

Migration timing (2026-10-25): combined parallel native/Win32 build 9.088s;
12 native boundary tests 17.637ms. Current case replay measurements:

| Case | Setup | Execution | Checks | Total |
| --- | ---: | ---: | ---: | ---: |
| help | 3.957ms | 2.666ms | 0.587ms | 7.210ms |
| hello-debug-dll | 3.722ms | 7.264ms | 21.791ms | 32.778ms |

These replay checks cover first-entry order, terminal status, streams and actual file
artifacts. Historical per-function GDB/state proof campaigns are not rerun.

Final canonical graph/ABI audit: 0.198s (113 App bodies, 112 scoped RET checks).

Layout cleanup validation: native build 1.926s, original-toolchain build 7.918s;
parallel cached replay, 12 units and strict graph/ABI audit passed in 0.365s.
No source bodies, registry entries or captured case bytes changed.

Function114 validation: reused the genuine qualified original capture (1.319s),
then native rebuild, both replays,12 units and strict audit passed in0.845s.
The partial debug replay took29.932ms (5.093 setup/8.072 execution/16.767checks).

Functions115–117 reconstruct reference symbols, global-type output and destructive
type-pointer-to-offset conversion. Fresh original/native pairs reach output
flush; the thirteen narrow tests include a 48K type-page boundary check.
