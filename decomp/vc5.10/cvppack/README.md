CVPACK 5.10 reconstruction
========================

`make -j4 check unit` runs the cached cases and twenty-three narrow native tests.
`make -j4 win32 audit` checks original-compiler compatibility and the function
graph/ABIs. Fixtures never invoke Wine.

159 accepted application functions total 47,835 original code bytes: 76.34% of the
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

- 0008-type-server-archive: genuine VC5 `/Zi` two-TU archive with a shared
  53,248-byte compiler PDB and real LF_TYPESERVER records. Status 0, exact
  verbose output and a 5,208-byte packed DLL, SHA256
  `38425100d452a54db119174283be8258a3cd70fdf983f48367075753b278321c`.
  The PDB stays byte-for-byte unchanged. Type-server selection, object-directory
  lookup and record import were reconstructed. The absolute compiler PDB path
  is reproduced through the case's stable T: cwd in a private native workspace.

- 0009-stream-file-cache: internal API DIRECT from the genuine main entry,
  followed by the unmapped initializer. Two real pages and descriptor slots copy
  and read back 65,573 defined bytes in 997-byte fragments, cross page boundaries,
  reuse cached descriptors, evict LRU pages/closed entries, and shrink the copied
  file to 12,033 bytes. All three files match their complete deterministic recipe.
  This proves the internal workload, not ordinary CLI reach or natural process
  completion. Fifteen stream/cache functions were reconstructed; mapped growth
  and page-table growth remain named traps.

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

The native Widen adapter has two separate, input-bound response formats: the
original 21-lifetime hello case and the four-lifetime C7 archive case. The latter
contains 35 actual Record calls and two Query/Emit pairs. Only captured ordered
requests, complete inputs, scalar sizes and owner aliases are allowed; unknown
calls fail. It writes observed consumed prefixes and preserves each native
buffer's own unwritten capacity. The archive's provider tag is the expected
shipped identity, with staged-file identity recorded separately; it does not
claim an observed loaded-hFile hash. Dependency responses earn no App credit. The type-server case separately replays its
observed named C exports, with one PDB/TPI lifetime and 19 mutable record spans
(532 bytes). These buffers retain the App's mutations until TypesClose; only
actual opaque owner identities, ordered calls and input-bound responses are
available. Successful Open's unused error output has no initialization claim.
The bounded support observation ends at the final dependency return; the
separate ordinary CLI pair establishes case completion.

`src/` contains application code, `harness/` the native host/provider and ABI glue.
`crt/crt-imports.h` only declares the original direct `_iob` import, avoiding an
extra SDK accessor. Shipped MSVCRT supplies its implementation; no CRT body was
reconstructed. `binary.c` is the sole function/extent/status registry. Finite
rules remain in `policy-data.json`/`abi-policy.json`; the exact 8-byte Release tail
retains its reviewed residual, with strict cleanup checks on all other accepted bodies.

Latest stream/cache verification (2026-10-25): the final fresh original DIRECT
capture completed in 1.041s. Matching native replay took 15.099ms (setup6.837 /
execution6.214 / checks2.048ms). The case-end `make -j4 check unit win32 audit`
passed all ten cached cases, twenty-three units, original-compiler compatibility
and the full159-body graph/import/data/extent audit, with strict RET checks on
158 bodies and the existing Release residual. The incremental batch took0.605s;
its stream/cache fixture took12.906ms. During reconstruction only current-case
pairs and selected changed-function audits ran. The final small capture stores
invocation and file hashes; the deterministic source recipe avoids pattern blobs.

Full build/capture accounting stays in scratch. Historical per-function
state/proof campaigns are not rerun. Unvisited application stubs remain traps.
