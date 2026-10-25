CVPACK 5.10 reconstruction
========================

`make -j4 check unit` runs the two cached cases and narrow native tests through
the shared runner. `make -j4 win32 audit` checks original-compiler compatibility
and the function graph/ABIs. Fixtures never invoke Wine.

The accepted runnable baseline is 113 application functions totaling 37,019 original code
bytes, unchanged from 8f832c69. This is 59.08% of the 62,662-byte whole-text extent;
that denominator also contains CRT/support, unknown code and padding. A complete
non-CRT denominator is not established. CRT/import selection earns no App credit.

0000-help completes with status 1, 193 stdout bytes and empty stderr.
0001-hello-debug-dll invokes `/nologo hello.dll` on the original embedded-CodeView
DLL. Its current frontier is the named `write_globals` stub (RVA 0xd9a0); the
948-byte prospective body remains analysis only. Its capture distinguishes the
actual partial artifact from the final NB11 master. No new references were run
for this migration.

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
