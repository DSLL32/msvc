CVPACK 5.10 reconstruction
========================

`make -j4 check unit` runs the cached cases and twenty-four narrow native tests.
`make -j4 win32 audit` checks original-compiler compatibility and the function
graph/ABIs. Fixtures never invoke Wine.

209 accepted application functions total 55,562 original code bytes: 88.67% of the
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
  and page-table growth are exercised separately below.

- 0010-file-growth: internal API DIRECT with genuine mapped and stream owners.
  Both receive and read back 4MiB+8193 recipe bytes, crossing two mapping expansions
  and the stream page-table threshold. Both shrink to2MiB+37 and close with
  identical complete output SHA256
  `b8346f545c8c31e76ec5bccad7b77da62c975c0c4d192e94d82ea3dd797c8299`.
  The two growth helpers were reconstructed. Only final hashes are retained;
  generated pattern files are not embedded fixture inputs.

- 0011-library-archive: actual original LIB/LINK archive members with `/Zi` type
  servers and nonzero SST_LIBRARIES indices. Status 0, exact verbose output and
  packed DLL SHA256 `b52cc2729fd24157dfa05a1166061e9c4a0d224139de00a25d2f9b321bc81eb4`; the canonical compiler PDB remains byte-for-byte unchanged.
  Counted library-name selection was reconstructed. Its dependency transcript
  is a labelled derivation of 0008: only the whole App-DLL input binding changes,
  while all PDB bytes, response bytes and strict request/owner/stage checks remain
  unchanged. No second provider observation or loaded-provider claim is made.

- 0012-fieldlist-continuations: format-authored PE32/NB05 with two module
  contributions and genuine signature2/DWORD-wide C7 types and symbols. Three
  structures have 2, 20 and 100 members split across acyclic LF_INDEX chains;
  repeated field lists exercise interning and small/medium/large payload
  ownership. Ordinary CLI status 0, exact verbose output and 4096-byte packed DLL
  SHA256 `897748ea974f612ed4ec570ceaa73bb5e3c0cbcfce815c0ef81f88104cb4eaeb`.
  Six continuation/payload-pool functions were reconstructed. The input is
  explicitly authored, with structural validation in the binary-specific
  generator; no historical producer claim or old output oracle is reused.

- 0013-c6-symbols: format-authored PE32/NB05 with genuine C6 marker/length/leaf
  types and balanced 16/32-bit local-symbol scopes. Seven referenced type leaves,
  procedures, blocks, registers, BPREL/data/labels, counted constants, UDT and
  control records complete through the ordinary CLI. VC5's actual wide BPREL,
  DATA and UDT index layouts are encoded independently of the VC1 layouts.
  Status 0, exact 223-byte verbose output and packed DLL SHA256
  `7e14054c0a5c88f35ba77b1f6baf75a3a995b8a7cf135fddef0bc671b64478c8`.
  Forty-one application functions were reconstructed. Opaque COBOL, structure,
  pointer/BARRAY and unvisited register-relative conversions remain traps.
  The existing coalesced one-byte RET identity stays CRT-classified, with a
  native denial at its unvisited callback slot and no new application credit.
  Input validation checks actual consumed type domains, name/record endpoints
  and scope balance. Earlier width-assumption variants remain scratch history;
  the final input has a fresh original master and matching completion pair.

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

Latest C6 verification: final genuine original CLI capture 949.635ms; matching
native replay 12.825ms (setup6.159/execution5.376/checks1.289). All fourteen cached
cases, twenty-four units and original-compiler compatibility passed. The full
209-body graph/import/data/extent/callback checks and strict RET on 208 bodies
passed with the existing Release residual. An omitted rebuilt 44-byte switch table
was identified by the first extent audit, then bound to its exact symbol-relative
interval and independently checked against COFF relocations/classmap; only the
static audit was repeated. The first native batch took 0.968s including that rejected first audit; its C6 replay took 9.894ms.
The final affected build/static audit took 9.320s after EOF-only cleanup, without
new original execution. During each case only current pairs and selected changed-function audits run;
completed cases receive one commit.

Full build/capture accounting stays in scratch. Historical per-function
state/proof campaigns are not rerun. Unvisited application stubs remain traps.
