CVPACK 5.10 reconstruction
========================

`make -j4 check unit` runs the cached cases and thirteen narrow native tests.
`make -j4 win32 audit` checks original-compiler compatibility and the function
graph/ABIs. Fixtures never invoke Wine.

124 application functions total 40,643 original code bytes: 64.86% of the
62,662-byte whole-text extent. That denominator includes CRT/support, unknown
code and padding; a complete non-CRT denominator is not established. CRT/import
selection and the external MSPDB provider earn no application reconstruction credit.

Completed cases:

- 0000-help: status 1, 193 stdout bytes, empty stderr.
- 0001-hello-debug-dll: `/nologo hello.dll`, status 0, empty streams and exact NB11
  output SHA256 `935d7828c7003e4f951e79cb4a709c747eac8006405ac996299dea4e64dd0d40`.

0002-cpp-scene-growth is a new three-module C++ workload with 240 telemetry
schemas, shared recursive types, virtual inheritance and large arrays. Original
packed CodeView is 238,240 bytes. Its current original/native frontier is
`numeric_leaf_size` (RVA 0xfcb0); source inputs remain capture assets. No Widen
oracle from the older DLL is installed for this different input.

The DLL cases use a genuinely fresh reference with no explicit target environment
overrides from historical harness isolation. Prior captures remain in Git history.
Module-path setup is explicit host metadata, not target environment. Its retained
observed path is not asserted equal to historical or future staging paths; a
future actual application path consumer requires new input qualification.

The native MSPDB adapter remains a strict 21-lifetime typed response provider.
Only captured factory/Query/Emit/Release requests and complete inputs are allowed;
unknown calls fail. It writes observed prefixes and preserves each native buffer's
own unwritten capacity. It remains outside the application graph.

`src/` contains application code, `harness/` the native host/provider and ABI glue.
`crt/crt-imports.h` only declares the original direct `_iob` import, avoiding an
extra SDK accessor. Shipped MSVCRT supplies its implementation; no CRT body was
reconstructed. `binary.c` is the sole function/extent/status registry. Finite
rules remain in `policy-data.json`/`abi-policy.json`; the exact 8-byte Release tail
retains its reviewed residual, with strict cleanup checks on all 123 other bodies.

Latest completion verification (2026-10-25): original capture 1.045s; native DLL
replay 14.615ms (setup 5.339/execution 7.852/checks 1.424ms). Final native build
0.201s, incremental Win32 build 0.043s, and combined cached replays, thirteen
units and strict audit 0.287s. Full build/capture/check accounting stays in
scratch. Historical per-function state/proof campaigns are not rerun. The growth case is under reconstruction; unvisited application stubs remain
explicit traps.
