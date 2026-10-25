CVPACK 5.10 reconstruction
========================

`make -j4 check unit` runs the cached cases and eighteen narrow native tests.
`make -j4 win32 audit` checks original-compiler compatibility and the function
graph/ABIs. Fixtures never invoke Wine.

141 accepted application functions total 43,446 original code bytes: 69.33% of the
62,662-byte whole-text extent. That denominator includes CRT/support, unknown
code and padding; a complete non-CRT denominator is not established. CRT/import
selection and the external MSPDB provider earn no application reconstruction credit.

Completed cases:

- 0000-help: status 1, 193 stdout bytes, empty stderr.
- 0001-hello-debug-dll: `/nologo hello.dll`, status 0, empty streams and exact NB11
  output SHA256 `935d7828c7003e4f951e79cb4a709c747eac8006405ac996299dea4e64dd0d40`.
- 0002-cpp-scene-growth: `/nologo growth.dll`, status 0, empty streams and exact
  285,168-byte output SHA256 `1cd8da46f548a48e42b79f8bb8a7c1191a323842f14c7675f0ab5e2dea3dc6cd`.
- 0003-precompiled-catalog: genuine `/Yc`/`/Yu` objects, `/nologo /min /v`,
  status 1 and exact 94-byte CK1017 diagnostic. Minimum packing suppresses the
  module-name records needed by the later precompiled-object lookup.
- 0004-catalog-bases: CRT-free `/nologo /v catalog.dll`, status 0, verbose
  statistics and a 151,600-byte packed DLL. Only the two proven unwritten
  SST_FILEINDEX alignment bytes at offsets 151370..151372 are masked; native
  output must explicitly contain zeros there.

- 0005-optimized-telemetry: optimized CRT-free three-TU telemetry readers,
  exported global data and an ignored unknown option; status 0, exact warning
  and verbose statistics, and a 17,808-byte packed DLL. Only the three proven
  unwritten file-index alignment bytes at 17577..17580 are masked; native
  output must contain zeros there. The warning and two DWORD-type symbol
  callbacks were reconstructed. No Widen dependency calls occur.
- 0006-release-checksum: genuine LINK `/RELEASE` telemetry DLL, status 0,
  verbose statistics and an 18,320-byte packed image. Each side's raw PE
  checksum is independently correct before comparison. Only the proven
  unwritten file-index tail at 18089..18092 and its dependent checksum DWORD
  at 264..268 are masked; native padding must be zero. The file-size and
  mapped-range helpers were reconstructed; shared IMAGEHLP supplies the
  standard public checksum operation.

- 0007-old-index-archive: genuine VC1.0 `/Z7` two-TU archive readers linked
  into an NB05 DLL, exercising narrow C7 indices, recursive types, bitfields,
  function pointers and x87 code. Status 0, exact verbose statistics and a
  5,216-byte packed image, SHA256
  `5583fdbb41f8f2645ed6d96816181c06cda40ad8098c6f37408d3f1041abcf2e`.
  The genuine Record wrapper was reconstructed. This is C7 widening, not the
  older C6 conversion path; that reader remains a trap.

0002-cpp-scene-growth is a new three-module C++ workload with 240 telemetry
schemas, shared recursive types, virtual inheritance and large arrays. Original
packed CodeView is 238,240 bytes. Its source inputs remain capture assets. Numeric sizing, derived-base tracking,
temporary type-buffer allocation and buffered output were reached and reconstructed.
No Widen calls occur, and no oracle from the older DLL is installed for this input.

The DLL cases use a genuinely fresh reference with no explicit target environment
overrides from historical harness isolation. Prior captures remain in Git history.
Module-path setup is explicit host metadata, not target environment. Its retained
observed path is not asserted equal to historical or future staging paths; a
future actual application path consumer requires new input qualification.

The native MSPDB adapter has two separate, input-bound response formats: the
original 21-lifetime hello case and the four-lifetime C7 archive case. The latter
contains 35 actual Record calls and two Query/Emit pairs. Only captured ordered
requests, complete inputs, scalar sizes and owner aliases are allowed; unknown
calls fail. It writes observed consumed prefixes and preserves each native
buffer's own unwritten capacity. The archive's provider tag is the expected
shipped identity, with staged-file identity recorded separately; it does not
claim an observed loaded-hFile hash. Dependency responses earn no App credit.

`src/` contains application code, `harness/` the native host/provider and ABI glue.
`crt/crt-imports.h` only declares the original direct `_iob` import, avoiding an
extra SDK accessor. Shipped MSVCRT supplies its implementation; no CRT body was
reconstructed. `binary.c` is the sole function/extent/status registry. Finite
rules remain in `policy-data.json`/`abi-policy.json`; the exact 8-byte Release tail
retains its reviewed residual, with strict cleanup checks on all other accepted bodies.

Latest archive verification (2026-10-25): bounded dependency observation 1.150s,
ending at the final actual Release return without a process-exit claim. The
separate fresh ordinary original capture completed in 1.333s; matching native
replay took 10.433ms (setup 4.755/execution 4.468/checks 1.210ms). Necessary
native/original builds took 8.545s. After case completion, all eight cached cases,
eighteen focused units, original build and strict graph/ABI audit passed in
8.548s including an original rebuild; the cached archive replay took 10.070ms.
The five added provider tests include unchanged own unwritten bytes and denial
of changed inputs, capacity, method or lifetime order.

Full build/capture accounting stays in scratch. Historical per-function
state/proof campaigns are not rerun. Unvisited application stubs remain traps.
