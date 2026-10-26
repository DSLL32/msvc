# Graph audit

Standalone Rust audit with compatibility profiles for the eight `vc1.0` reconstructions
and explicit PE/Watcom CF input support. Migrated reconstructions use the shared
registry and harness while preserving each project's required checks and explicit
reference provenance. The executable reads files;
it never invokes Python, Wine, a compiler, or a reconstructed/original executable.

Build with Rust 1.88+ and a host C compiler (for Capstone):

```sh
cargo build --release --locked --manifest-path tooling/graph-audit/Cargo.toml
tooling/graph-audit/target/release/graph-audit --help
```

Dependencies are Clap, Serde/JSON, SHA-256, Regex, Anyhow and Capstone's established
Rust bindings. Only the x86 Capstone backend is built. `Cargo.lock` is tracked.

`--profile` selects existing metadata/index conventions, not a new acceptance
policy. `--original`, `--rebuilt`, `--map`, `--config`, `--runtime` and
`--rebuilt-runtime` override individual paths. Paths resolve from the invoking
working directory; normal build paths otherwise resolve under `--project`.
Output is deterministic JSON on stdout; `--report PATH` additionally saves it.
`-q`/`--quiet` suppresses successful stdout output while preserving report writes.
Errors go to stderr with nonzero status. Existing audit reports are never changed.

## Shared build interface

From a binary-root Makefile:

```make
include ../../../tooling/graph-audit/graph-audit.mk

audit: $(GRAPH_AUDIT_BIN)
	@"$(GRAPH_AUDIT_BIN)" --profile c2 --project . --repo ../../.. --discovery cfg
```

The include exports `GRAPH_AUDIT_BIN`, defines `GRAPH_AUDIT_BUILD_SCRIPT` and the
convenience target `graph-audit-tool`, and preserves the caller's default goal.
Its binary prerequisite tracks Rust source, embedded profile metadata and Cargo
inputs. A successful no-op Cargo build also refreshes the binary's Make timestamp.

`build-quiet.sh` works from any cwd and runs Cargo with `--quiet --release --locked`.
Both Cargo streams go to a unique retained log under `target/build-logs/`; successful
builds are silent. Failures print both captured streams and retain Cargo's exact
nonzero exit status. `GRAPH_AUDIT_BUILD_LOG` optionally selects a log, relative to
the caller's cwd; `CARGO` optionally selects the Cargo executable. The tool target
directory is fixed to this package's `target/` for consistent include paths.
Invoke the resulting binary directly so its actual errors/output remain visible.

## What the graph means

The comparison is a **function call graph**: each function's distinct callee
identities and the presence of indirect transfers. It does not compare internal
basic-block layout, instruction counts, call multiplicity or call order. Internal
CFG traversal discovers calls without treating switch data as instructions.
Aliases are matched bijectively by address; sharing one CRT name cannot collapse
two different physical vertices into one.

Default `--discovery cfg` uses one instruction-level CFG engine for all profiles:
conditional branches, direct/indirect calls, direct tail transfers, inferred or
declared switch tables and additional SEH roots. A fallthrough at the declared
endpoint is an edge only if that endpoint is another identified function entry.
A conditional transfer outside the extent is also a graph edge; CVPACK compatibility retains its explicitly bounded-root policy. An unresolved
indirect tail transfer remains an indirect edge; it is never discarded or guessed
into a switch. Declared text data is excluded from decoding.

Ordinary software interrupts can return, so `int n` falls through in common discovery; the old CVPACK compatibility rule is explicit.

Valid x86 can have overlapping alternative instruction streams. Ordinary graph
discovery permits them and records instruction starts independently. Instructions
crossing the **declared function endpoint** are always rejected. Explicit strict
CVPACK compatibility retains its existing stronger non-overlap policy.

## Optional checks

Default discovery compares the graph vertices declared by each project: `driver`
for the six compiler/linker/RC profiles, `functions` for RCPP, and non-stub
application functions for CVPACK. RCPP's list explicitly includes library/support
routines. This is not an implicit full CRT or native-ELF graph audit. Extra checks
are opt-in:

| Flag | Check |
| --- | --- |
| `--resolve-register-imports` | With `--check-imports`, conservatively resolve cached EBX/ESI/EDI/EBP import calls using the existing CFG and explicit, hash-bound callee-preservation witnesses. |
| `--check-imports` | Compare verified DLL+name/ordinal import transfers, normalizing direct IAT calls and proven import thunks while retaining declared source/runtime vertices. Standalone profiles only. |
| `--ret-checks PATH` | Assert explicit expected cleanup sets for named accepted functions; all graph/ownership/extent checks retain their full selection. Standalone CFG profiles only. |
| `--check-ret` | Compare RET cleanup sets, inheriting identified direct tail callees; unknown indirect tail ABIs and cycles fail. CVPACK also checks declared cleanup. |
| `--check-references` | Compare immediate references to indexed function entries, with alias bijection. Available for the seven standalone profiles. |
| `--check-callbacks` | Compare ordered, non-null callback table entries and acceptance requirements from metadata. RC has supplied table metadata; other profiles require `--metadata`. |
| `--check-ownership` | Require original application ranges to be disjoint, file-backed code. CVPACK additionally checks application/text-data/runtime/contextual-data ownership and explicit runtime aliases. |
| `--check-extents` | Check whole extent tail boundaries beyond reached instructions, excluding discovered switch data. CVPACK uses its complete rooted-body/padding checks. |
| `--include-evidenced-runtime` | Also compare accepted indexed runtime vertices with explicit evidence; names already in the driver list are not duplicated. |
| `--check-accepted-extents` | Check every accepted source/runtime/import index extent on both sides, independently of graph-vertex selection. Indexed standalone profiles only; CVPACK retains its explicit support policy. |
| `--check-instruction-overlap` | Explicitly assume one instruction owner per byte within a function. This is a reconstruction constraint, **not a universal x86 rule**. |

```sh
tooling/graph-audit/target/release/graph-audit \
  --repo . --project decomp/vc1.0/rc --profile rc \
  --check-ret --check-references --check-callbacks --check-ownership
```

`--ret-checks` reads binary policy, not per-case evidence. For example:

```json
{"schema":1,"functions":{"@helper@16":{"expected_cleanup":[8]}}}
```

Each named function must be selected, accepted, present unambiguously in both
indexes, and have a determined extent. Expected cleanup sets must be nonempty,
unique unsigned 16-bit values. Both original and rebuilt cleanup sets must equal
the expected set; unknown tails still fail. This option never disables global
`--check-ret`, changes graph scope, or supplies a missing ABI. Its report adds
`ret_cleanup_checks` with policy SHA-256 and each original/rebuilt/expected set,
and marks `selected_ret_cleanup`. Without this option reports stay unchanged.

`checked_accepted_extents` reports raw accepted index-record counts, and
`checked_unique_accepted_extents` reports distinct `(name,start,end)` checks.
Identical duplicate records share one decode; no distinct extent is omitted.
`original_accepted_graphs` exposes raw numeric call targets and non-branch immediate
references for the accepted original records, unioned by name. These are unfiltered
addresses for inventory consumers, separate from mapped graph-equivalence checks.
Extent-only passes allow a complete final instruction at a library fragment's
endpoint, but never an instruction operand clipped by that endpoint.

Standalone `driver[].symbol` (RCPP: `functions[].symbol`) optionally names the
actual MAP public for a function whose `name` remains its historical logical ID.
Absent `symbol`, the actual public is `name`. Explicit mappings require a nonempty
existing real MAP key, no logical/public-name conflict, unique logical names and
injective symbol keys. They resolve directly against immutable raw MAP publics;
alias chains are rejected. `symbol == name` is a valid identity mapping, and
different real MAP keys may retain the same physical address. No wrapper function,
MAP rewriting or archived-trace relabeling is needed.

Input identity and existing profile-specific index validation still run: original
SHA-256, symbol mapping, declared duplicate/extent checks and relevant existing
CRT defaults. These establish the input being audited. Runtime identification is
not reimplemented by this graph tool.

CVPACK `alignment_padding` metadata permits only explicitly declared five-byte
`ADD EAX,0` intervals with exact original bytes `05 00 00 00 00`. Each interval
must lie inside its function, be disjoint from text-data and other declarations,
and remain unreachable in the final CFG including derived compiler roots. The
original image hash binds these bytes. This is an extent-policy declaration, not
a general permission to omit unrooted code; it is checked with `--check-extents`
or the legacy CVPACK preset.

`--check-imports` reads the standard PE32 import directory with bounded, file-backed
lookup/IAT/string validation. Exact unprefixed absolute `FF 15` calls and `FF 25`
jumps resolve to structured `{dll,name}` or `{dll,ordinal}` identities. Direct
transfers through proven six-byte import thunks use the same identities. Named
source/runtime graph vertices remain function edges even if their bodies begin
with a thunk. DLL basename and ASCII case are normalized; export name case and
ordinals are preserved. Unresolved register/indexed/segment-relative transfers
remain indirect, and malformed import tables fail. This opt-in check leaves the
default and legacy reports unchanged.

`--resolve-register-imports` adds a must-value pass over those same discovered
instructions and control-flow edges. It tracks only EBX/ESI/EDI/EBP, seeds full-width
MOVs from verified absolute IAT slots, and copies full-width register MOVs.
Other writes (including partial and implicit writes) clear the affected fact.
BP/BPL aliases and frame operations such as ENTER/LEAVE invalidate EBP; saved
register values are not recovered from the stack. Existing contracts listing
only EBX/ESI/EDI do not authorize EBP survival across a call.
Every entry/SEH root starts unknown; joins retain only identities agreed by all
reachable predecessors, and loops run to a fixed point. Segmented/indexed loads,
stack values, arbitrary pointers, register tail jumps and external RET cleanup
are not inferred. Unknown calls clear all facts. Across a call, a fact survives
only under an explicit contract for that actual named callee or qualified import.
A calling-convention annotation on the caller is insufficient.

Add contracts to schema-1 metadata, and enable strict resolution on functions
whose register calls must all be imports:

```json
{
  "schema": 1,
  "register_imports": {
    "preservation_contracts": [{
      "target": {"import": {"dll": "msvcrt.dll", "name": "_stricmp"}},
      "preserves": ["ebx", "esi", "edi", "ebp"],
      "evidence": {"path": "cases/register-preservation.json", "sha256": "<64 lowercase hex digits>"}
    }]
  },
  "functions": {
    "_configure_link_options@12": {
      "register_imports": {"require_all_register_calls_resolved": true}
    }
  }
}
```

To enable resolution on selected owners while retaining **one full graph/import
audit**, omit the global resolver flag and set the owner policy explicitly:

```json
{
  "functions": {
    "@read_initializer_records@4": {
      "register_imports": {"enabled": true, "require_all_register_calls_resolved": true}
    }
  }
}
```

`--check-imports` remains required, and all selected functions retain the same
call-graph/import/ABI/extent checks. Only the named owners get the additional
must-value pass. An enabled owner must be selected/accepted and present in both
verified indexes. `enabled` accepts only `true`; `false`, null and malformed
policies fail. Policy sections must be objects with recognized keys; a typo
cannot silently disable resolution. Effective resolution is the global CLI flag **or** explicit owner
enablement, so this metadata cannot disable global resolution. It grants no
register facts, seeds, preservation claims or exemptions. Existing hash-bound
contracts still apply, as do strict unresolved-call rejection and independent
semantic proof. New scoped reports bind the deterministic effective owner list
in `register_import_inputs.enabled_functions`; old global/default reports are
unchanged. This mode is CFG-only and supports the standalone profiles.

For a source/runtime callee use `target: {"function": "<logical index name>"}`.
Targets must exist in both verified indexes/catalogs; no name/address guesses or
blanket ABI default apply. Contract evidence is a project-relative regular file:
the canonical path must stay inside `--project`, and its SHA256 must match.
The witness packet's semantic proof and binding to actual callee bodies/providers
remain the target checker's responsibility; the graph tool validates the packet
bytes rather than interpreting arbitrary proof formats. Unknown/custom ABI calls
need a separately justified preservation proof; otherwise they are barriers.

A separately proven, closed absolute callback slot may have an exact site contract:

```json
{
  "register_imports": {
    "callback_preservation_contracts": [{
      "owner": "?FullLink@LinkContext@@QAEHPAE@Z",
      "original": {"site": 4474773, "slot": 4714192},
      "rebuilt": {"site": 4238927, "slot": 4252440},
      "preserves": ["edi"],
      "evidence": {"path": "cases/callback-preservation.json", "sha256": "<64 lowercase hex digits>"}
    }]
  }
}
```

This only lets **existing** must-values survive that call. It never seeds a
value, resolves the callback, or changes its graph edge: the call stays opaque
in `other_unresolved_calls`. Both sides must identify an actual discovered,
exact unprefixed `FF 15` absolute32 memory CALL in the named audited owner. Each
physical site may have only one contract. The complete four-byte slot must lie
in a mapped readable PE section (zero-filled BSS is valid), cannot overflow the
32-bit address space, and cannot overlap a verified IAT slot. Register-relative,
indexed, segmented, prefixed and unresolved register calls are unsupported here.
The contained hash-bound witness must independently prove slot closure and the
claimed preservation for every possible actual target; the common tool checks
its bytes and site binding, not its arbitrary semantic proof format. Unknown
callbacks remain barriers unless such a contract is present. Per-side
`callback_preservation` report maps bind each applied site to slot, register mask
and evidence digest; the top-level input binding includes both side declarations.
These fields are absent when no callback contracts are supplied.

Per-function `register_imports` reports both sides' resolved and unresolved
register callsites, other unresolved calls, direct import callsites and CFG roots.
Top-level `register_import_inputs` binds PE and metadata hashes plus the verified
contracts. Strict mode rejects any unresolved register call, even when another
call already supplies the same import or an indirect topology bit. Genuine
callbacks stay indirect in non-strict functions. For such mixed functions, the
target checker must assert the hash-bound inventory of resolved cached-import
sites and genuine opaque sites from this report. A lost cached-import fact can
otherwise hide behind an existing indirect bit and an import supplied elsewhere.
This inventory check consumes the report; it does not discover another CFG.
Graph equality still compares
target sets, not instruction ordering or callsite correspondence. The flag is
optional, CFG-only, standalone-profile-only; flag-off reports are unchanged.

Per-function `imports` fields and the `import_catalogs` slot maps record the
verified identities separately from function edges. No loader is invoked. Import
data references, delay imports and external DLL function ABIs are not inferred;
retain applicable separate policy. `--check-ret` still requires an actual or
separately proven return convention, so imported tail thunks may need a scoped
support ABI check. The PE structures follow Microsoft's
[PE format specification](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#the-idata-section).

Fragmented functions retain their primary `start`/`end` and may declare additional
`code_chunks: [[start,end], ...]` in standalone driver/runtime index records.
Driver `rebuilt_code_chunks` supplies additional counterpart ranges; otherwise the
rebuilt range remains the normal MAP-derived interval. Addresses are preferred
absolute VAs. These intervals must be disjoint, file-backed code, preserve known
function ownership and contain complete instructions. Exact physical aliases
remain aliases; a cold chunk cannot claim another function's body or shared text
data. No gap belongs to the function. `shared` still denotes text data.

Branches and separately evidenced roots can enter any owned interval, and all
calls/returns contribute to one logical function. Chunk starts are **not** added
as roots. Extent checks cover each declared interval independently. Legacy mode
rejects fragmented records rather than ignoring them; CVPACK's distinct ownership
profile currently rejects them explicitly. Fragmented audits report exact ranges
under `owned_code_chunks.{original,rebuilt}.functions`, with logical-function and
interval counts, declared bytes, and unique physical code bytes. Consumers must
check this report and pin the auditor hash when requiring chunk support; older
binaries do not produce it.

Per-side `functions[name].{original,rebuilt}.text_data` declares exact data
intervals for discovery and extent checks. Interior data must fit one owned code
interval; external restored `.text` data must additionally be owned by that
record's `shared` ranges. Data must be file-backed, disjoint from other function
bodies and executable instructions, and cannot straddle a chunk boundary.
Declarations are exact uint32 pairs and cannot overlap one another. Inferred
switch tables and explicit data are skipped only over those intervals: every
remaining code gap and suffix still receives instruction-boundary checks. A data
declaration cannot replace a missing terminal instruction. These ownership checks
do not prove the table's contents; retain the target's byte/shape/target proofs.

Graph metadata is JSON, supplied through `--metadata`; RC's callback table layout
is in `profiles/rc.json`. Example function metadata:

```json
{
  "schema": 1,
  "functions": {
    "logical_name": {
      "original": {"roots": [4198448], "text_data": [[4198464,4198472]], "switches": {"4198420": [4198432,4198448]}},
      "rebuilt": {"roots": [], "text_data": [], "switches": {}}
    }
  },
  "callback_tables": []
}
```

Addresses are preferred-image VAs, matching existing reconstruction indexes.
Discovery does not pretend that same-named functions across separate DLL modules
have a shared identity. Multi-module IDs, DLL export ordinal/ABI catalogs and loader
entry/TLS contracts are future work, outside this initial verification.

## Exact legacy compatibility

`--discovery legacy` is an explicit transitional parity mode. It preserves existing
Python behavior, including differences and blind spots; it is not the consolidated
engine's policy. Extra flags/metadata with this mode are rejected rather than
silently ignored. Its compatibility walkers can be retired when callers migrate.

| Profile | Existing Python behavior preserved |
| --- | --- |
| CL | Linear decode; direct call targets and indirect-call presence only. |
| C1 | Linear decode excluding discovered switch tables; external JMP tails; external original text tables must be explicitly restored. |
| C1XX | Similar switch-skipping scan and external JMP tails; target/index checks differ from C1. |
| C2 | Reachable CFG plus SEH entries and switch tables; external branch/tail edges; accepted CRT/import extents also checked. |
| LINK | Table-aware linear decode, known-vertex JMP tails; additional evidenced runtime graphs; all accepted original extents checked. |
| RC | Reachable calls, alias bijection, function references, inherited RET cleanup, indirect topology, callback tables. Its existing FILE/locale/Kanji layout checks are retained only for exact compatibility. |
| RCPP | Complete CFG with bounded switches; separate linear startup SEH scan; accepted function/import tail boundaries. |
| CVPACK | Strict rooted whole-function graph, declared switch/data ownership, compiler roots/tables, RET cleanup and restored-byte ownership subset. |

Legitimate differences include SEH/switch metadata and alias conventions. Other
variation is historical: CL does not check tail edges or instruction endpoints;
RC embeds data/CRT checks alongside its graph. These are not silently unified
into the default optional-check policy. Legacy C2/RCPP branch-group tests also
do not follow `LOOP` (Capstone classifies it as `branch_relative`, not `jump`);
the common engine has a focused test for the missing-edge scenario. Parity is
verified for the pinned current corpus, not arbitrary future x86 images. The
Python and vendored Rust Capstone engines are 5.0.7 and 5.0.6 respectively.

Concrete consolidated rules are regression-tested from observed layouts:

* C1 `_finish_extended_arithmetic` falls through and branches to the separately
  indexed `_store_extended` at `0x4c8f1`.
* C2 `_normalize_extended` has the same form ending at `_store_extended` `0x71681`.
* C2 `_finish_debugging` ends in `jmp dword ptr [0x7b92c]`, an indirect tail edge.

## CVPACK registry inputs and scope

Migrated CVPACK uses the shared registry export's `function_registry`, `driver`
and graph metadata, together with the flat shared rebuilt-runtime identifier
output supplied through `--rebuilt-runtime`. Its immutable original runtime
catalog and finite `runtime-policy.json` identity qualifications remain explicit
inputs. The normal binary-root `make -j4 audit` supplies the generated paths and
runs the separate contextual/source checks.

Runtime classification does not grant graph-body availability. Reviewed nonexact
original CRT variants retain their exact canonical extent and body-hash checks;
the graph adapter treats them as identity-only anchors, excluding accepted bodies
and restored support. The legacy CVPACK input/cache format remains supported for
historical inputs, without a live Python cache-generation helper.

## Verification and recorded migration timings

Run current Rust checks directly:

```sh
cargo test --locked --manifest-path tooling/graph-audit/Cargo.toml
cargo clippy --locked --manifest-path tooling/graph-audit/Cargo.toml --all-targets -- -D warnings
```

The focused static CLI controls below use synthetic inputs or explicit frozen
corpora. The small fixtures in this package's `fixtures/` directory remain Rust
test inputs; they are independent of the retired shared reconstruction fixtures.

`validation.json` and the later validation receipts preserve historical input
hashes, test results and timings. The migration-only Python parity, snapshot and
benchmark scripts were retired after the reconstructions moved to the shared
harness. Their source and original baselines remain in Git history and the
retirement archive; they are not current audit commands. No baseline or timing
receipt was regenerated during that cleanup.

The following measurements are historical migration results. They used frozen
scratch copies, warmed each CLI, and excluded builds, fixture execution, Wine and
explicit identity-cache preparation. The CVPACK baseline was an extracted
**graph-only** reference with the same frozen identity index, rather than its full
integrated audit. These results do not certify later source or tooling changes.

Measured medians in seconds (seven samples; ranges and full provenance are in
`validation.json`). These compare retained Python baselines with later samples of
the delivered Rust binary; the main table is **not an interleaved paired run**.
RC and C1XX also have paired refreshes recorded in the receipt.

| Tool | Python | Rust legacy | Rust CFG | Legacy speedup |
| --- | ---: | ---: | ---: | ---: |
| CL | 0.214 | 0.029 | 0.042 | 7.3× |
| C1 | 2.160 | 0.369 | 0.329 | 5.9× |
| C1XX | 2.575 | 0.417 | 0.375 | 6.2× |
| C2 | 4.319 | 0.435 | 0.487 | 9.9× |
| LINK | 11.469 | 0.247 | 0.313 | 46.5× |
| RC | 5.124 | 0.053 | 0.073 | 97.3× |
| RCPP | 1.341 | 0.056 | 0.061 | 24.0× |
| CVPACK | 0.367 | 0.044 | 0.041 | 8.4× |

Summed medians: **27.569s → 1.649s**, saving **25.920s (16.7×)** for legacy-compatible checks. The common CFG pass sums to 1.721s with its different optional-check scope. CVPACK is the graph subset, not its entire audit.

For automation, build once with `cargo build --quiet --release --locked` (with
`--manifest-path` as above), capture Cargo stderr to a log, and show that log only
on build failure. Then invoke the binary directly with its stdout/stderr intact.
Do not use `cargo run` per audit or discard actual tool diagnostics.

Real split-function discovery and CLI policy controls (no target execution):

```sh
cargo test --quiet --locked --manifest-path tooling/graph-audit/Cargo.toml
python3 tooling/graph-audit/check-chunks.py \
  --scratch ~/.ai-tmp/graph-audit/chunk-cli \
  --report ~/.ai-tmp/graph-audit/chunk-cli-validation.json
```

The real PGO fixtures pin the repository's VC5.10 C1XX/C2 original image hashes;
these tests require those originals to be present. The CLI fixture audits an
original against itself to verify discovery/policy, not reconstruction acceptance.

Synthetic import-identity controls (provider/name/ordinal, thunk equivalence,
ordinary indirection, malformed bounds and declared-vertex protection):

```sh
python3 tooling/graph-audit/check-imports.py \
  --scratch ~/.ai-tmp/graph-audit/import-cli \
  --report ~/.ai-tmp/graph-audit/import-cli-validation.json
```

For a frozen real C2 switch/classmap snapshot prepared under `~/.ai-tmp`:

```sh
python3 tooling/graph-audit/check-text-data.py \
  --snapshot ~/.ai-tmp/graph-audit/c2-data-fixture \
  --report ~/.ai-tmp/graph-audit/text-data-validation.json
```

This control runner changes only the scratch snapshot and restores every input.
It deliberately demonstrates that changed declared data bytes require a separate
content proof; graph/extent acceptance alone is insufficient for those bytes.

`text-data-validation.json` records this change's real C2 controls and exact VC1
report parity. Its historical C1 support index contains a zero-length `__stricmp`
record rejected by both versions with optional `--check-accepted-extents`; C1 did
not adopt that flag. The source/legacy policies are preserved without weakening
that rejection.

Logical-to-MAP symbol controls:

```sh
python3 tooling/graph-audit/check-symbols.py \
  --scratch ~/.ai-tmp/graph-audit/symbol-cli \
  --report ~/.ai-tmp/graph-audit/symbol-cli-validation.json
```

Cached-register import controls (no compiler, target execution or Wine):

```sh
python3 tooling/graph-audit/check-register-imports.py \
  --binary tooling/graph-audit/target/release/graph-audit \
  --scratch ~/.ai-tmp/graph-audit/register-import-cli \
  --report ~/.ai-tmp/graph-audit/register-import-cli-validation.json
```

## Watcom CF and PE MAP inputs

`--map-format watcom10` reads the original Watcom 10 PE linker MAP without
rewriting it. Group/segment offsets are anchored to matching sections of the
actual rebuilt PE, with mapped bounds checked per public. A DGROUP offset may
reach a separate BSS section beyond DGROUP's raw bytes. Names, case and genuine
same-address aliases are preserved. Conflicting public definitions, ambiguous
anchors and absolute symbols used as functions reject. This format is explicit;
the default remains the existing MSVC MAP parser.

For the Watcom `binb` CF containers, use standalone `--profile cl --discovery cfg`
(the profile selects the `driver` metadata convention). Declare an image view:

```json
{
  "target": {
    "path": "toolchains/watcom10.0a/binb/WASM.EXE",
    "sha256": "<whole original file SHA256>",
    "image": {
      "format": "watcom-cf",
      "coordinates": "image-offset",
      "analysis_base": 0,
      "initialized_sha256": "<exact initialized image SHA256>",
      "relocation_sha256": "<exact relocation stream SHA256>",
      "loader": {
        "path": "toolchains/watcom10.0a/binnt/WASM.EXE",
        "sha256": "<whole actual loader SHA256>"
      },
      "layout_evidence": {
        "path": "cases/cf-layout.json",
        "sha256": "<reviewed layout witness SHA256>"
      }
    }
  }
}
```

The loader path is contained under `--repo`; the layout witness is contained
under `--project`. Both files are hashed before use. The witness's semantic
claims must be verified by the target's checker/reviewer; the common tool binds
the packet rather than interpreting a second provenance language.

CF function/data addresses are **image offsets**, not PE RVAs or observed process
VAs. The actual six-DWORD header is found through MZ `e_cparhdr`; initialized
file bounds, entry/allocation/relocation layout, grouped relocation writes and
exact stream termination are checked. Fixup *writes* must be disjoint and fit
initialized pre-stream bytes. Fixup contents are 32-bit addends, which may be
negative: the real WPP386 indexed displacement is valid and wraps on base
addition. No PE sections or import table are fabricated.

Code comes only from explicit index records/chunks. External switch data needs
exact `shared` ownership, and inferred tables must not intersect executable
instructions. There is no whole-image executable classification. SEH roots must
be explicit target metadata; a recognizable scope target without its declared
root rejects instead of silently expanding traversal. Allocation tail contents
are not inferred. The normal CFG, callee identity, RET cleanup, ownership and
extent checks run over the same reader interface as PE.

`image_inputs` binds the actual file/image/stream, loader/witness hashes, header,
relocation counts/groups, explicit code ranges and MAP/PE identities. It is absent
from unchanged PE/MSVC reports. A pending named callee entry pin is not a whole
function extent, source reconstruction or accepted code credit.

CF currently rejects legacy mode, other metadata profiles, PE-IAT import
checks and `--check-callbacks`. At base zero, a relocated pointer with addend zero
and an unrelocated NULL have the same raw value; callback checking needs a real
fixup-aware fixture before it can distinguish them. Function-reference comparison
remains the existing immediate/address heuristic, not typed pointer or ABI proof. Its rebuilt input must be a real PE. `--check-ret` still compares cleanup
sets; original Watcom register/formal/near-far ABIs need the target's explicit
proof. The reader does not normalize far service gateways or supply execution
masking. Actual load-base, before/after relocation, complete loaded-byte and
mask/frontier evidence remain capture-controller obligations. Never mask the
on-disk fixup bytes and assume loader relocation preserves those traps.

`check-cf.py --binary BIN --fixture FROZEN_PROJECT --report RESULT.json` runs
scratch-only mutations against the real WASM `trademark` / original-Watcom-build
fixture. This is a static auditor fixture, with no application acceptance or
runtime equivalence claim. It checks input bindings, layout/fixup failures,
code/data ownership, changed callees, extent splits and MAP/unsupported modes;
unit tests also cover the actual three CF files and base-invariant decoding at
zero and WASM's observed unaligned `0x008e0020` base. No compiler, Wine or target
executable is launched by these checks.

## Decoded instruction export

`--export-instructions` adds `decoded_instructions` to a **successful paired
standalone CFG audit**. The ordinary function selection and checks stay unchanged;
this option exposes the existing CFG's reached instruction starts and exact raw
bytes. It does not linearly export an extent, decode pending callees, generate
assembly/source, or authorize execution. Legacy and integrated CVPACK profiles
reject it. A selected `pending_extent` owner rejects rather than disappearing
from an incomplete export.

The version-1 object binds both source image identities and coordinate systems,
the complete parsed configuration, effective indexes, accepted-name set, public
symbol map and graph metadata. These SHA256 bindings use canonical consumed JSON
(ordered object keys, compact UTF-8), distinct from C/header source hashes or
irrelevant JSON whitespace. Existing Watcom provenance also retains its raw MAP,
loader, layout and relocation-stream hashes. The target's build/acceptance receipt
must separately bind the auditor executable, report and reconstructed source/build.

`functions[logical_name].original` and `.rebuilt` each contain:

- `record`, `record_sha256`: the exact indexed owner and its semantic binding,
  including entry/end, additional chunks, shared data and evidence declarations.
- `effective_roots`, `declared_roots`, `declared_text_data`, `inferred_tables`:
  actual discovery roots and explicit/discovered data exclusions.
- `instructions`: sorted `{address, bytes_hex}` rows, each exactly 1–15 source
  bytes inside an owned code chunk. Unreached padding/data and unknown child
  bodies remain absent. Logical aliases and valid overlapping streams remain
  distinct facts; a consumer may reject ambiguities it cannot execute safely.
- `local_ret_cleanup_set`, `checked_ret_cleanup_set`: decoded local cleanup facts
  and the existing inherited cleanup comparison result, respectively. The latter
  is `null` unless `--check-ret` ran. Neither proves register/formal/near-far ABIs.

The owner keys exactly match the ordinary checked function keys. Membership in an
input's accepted list may describe a prospective trial; it is not independent
acceptance evidence. A runtime controller must apply its own approved prefix,
candidate/support, ABI, memory, stack, thread and service-boundary policies.

CF instruction bytes remain canonical **raw base-zero** bytes. The controller
must first verify the immutable source and complete initialized image, apply every
actual DWORD fixup at the observed base, then slice and compare actual stopped-child
instruction bytes. It must not guess relocation by subtracting arbitrary immediates,
turn declared bounds into whole-image execution/data permission, or freely run
unknown calls/returns/callbacks. Unclassified destinations remain frontiers.
This paired option does not add original-only support facts or runtime transport.

`check-instruction-export.py --binary BIN --scratch ~/.ai-tmp/CASE --report RESULT`
exercises the optional field, bindings, pending boundaries, gaps/data, overlapping
streams and rejection-without-partial-output controls using static synthetic inputs.
The real Watcom fixtures additionally check raw operand fixups and omission of an
unreachable extra RET. These checks never launch Wine or target executables.

### Original-only CRT support facts

Repeat `--export-original-support-instructions NAME` to request a finite selection
of canonical CRT records in a standalone CFG audit. Each selected name must have
one exact nonpending original index record, matching registry CRT classification
(`definition:3`, `implementation:0`), canonical code/shared intervals, and a
project-relative `evidence:{path,sha256}` witness whose contained file hash is
verified. Unknown/App owners, ambiguous records, fabricated bounds and overlapping
ownership reject. The witness's semantic qualification remains independently
reviewed policy; its digest alone is not a library-match or execution proof.

The separate `decoded_support_instructions` object binds the actual original
image/coordinate/relocation provenance, configuration, original index, metadata
and selected records. Each row includes its witness, exact range-byte hashes,
`original` instruction/record/root/data facts, direct tails and unresolved-tail
status. It has no rebuilt counterpart or paired-equivalence claim. Local RET
sets are structural facts under the existing calls-may-return CFG model;
`checked_ret_cleanup_set` is always null for this component, including when the
ordinary application audit uses `--check-ret`. A no-local-RET tail does not imply
cleanup zero or prove noreturn behavior.

This selection never changes accepted/restored sets, application graph checks,
credit or execution permission. In particular, differing OSI and NT startup
bodies need no fabricated pair. A controller still needs independently reviewed
support/ABI/admission policy, exact observed relocation and memory ownership,
and fail-closed treatment of unknown/far/service transfers. The ordinary report
and paired export are unchanged when this option is absent.

### Exact callback and import-site policy

Callback tables optionally set `stride` and `pointer_offset` (defaults 4 and 0).
Ranges/counts describe whole rows; only each row's four-byte function pointer is
read. `expected_functions` optionally lists every logical function name or null
in row order, preserving null positions. Scalar tags remain target data policy.

A resolver-enabled function's `register_imports.expected_sites` can contain exact
`original` and `rebuilt` inventories. Both sides require `resolved_register_calls`
and `direct_import_calls` address-to-qualified-import maps, plus
`unresolved_register_calls` and `other_unresolved_calls` address arrays. These
assert existing computed report facts; they neither seed identities nor permit
unknown calls. Mixed real callbacks stay opaque and must match their declared
sites. The owner must be selected, accepted, unambiguous and resolver-enabled.
Without these optional fields, reports and checks retain their existing behavior.

### Canonical registry projections

Standalone CFG profiles accept the shared registry export's `driver` and optional
`function_registry`. RCPP uses `driver` when present, otherwise its legacy
`functions` input. Registry App/CRT entries and actual MAP symbols add endpoint
names only: an entry without a verified body never creates a one-byte extent,
accepted graph body, RET evidence, or execution permission. Physical aliases
retain one-to-one vertex comparisons. Named preservation contracts may bind a
unique endpoint with their existing hash-bound ABI evidence.

With `--check-imports`, entries classified ImportName/ImportOrdinal normalize only
verified PE import thunks. Supplied DLL/name/ordinal facts must match the actual
PE catalog. App/CRT entries that happen to begin with FF25 remain function
vertices. A logical runtime name can select an already identified rebuilt body
at its unique mapped entry; missing or conflicting real bounds fail.

The CVPACK profile additionally accepts canonical `driver`/`function_registry`
configurations with explicit `--metadata`, the frozen original runtime index,
and flat shared `--rebuilt-runtime` identifier output. It retains its rooted
whole-function, RET, alignment, switch, SEH and ownership checks. All App entries,
including stubs omitted from `driver`, still constrain ownership. The finite
`restored_runtime_names` selection preserves support ownership. Identity-only
variants must be Unknown canonical registry intervals, match the frozen original
anchor and `runtime-policy.json` body hash, and remain `accepted:false` and
`unmask:false`. This path needs no legacy graph-identity cache or anchor authoring
file. The legacy CVPACK format and cache policy remain supported unchanged.

`--check-runtime-data` opts the RC CFG profile into its existing FILE, locale and
Kanji predicates. It does not change graph-body selection and is rejected for
other profiles. Runtime/import structural extents and graph-body acceptance are
separate policies; naming an endpoint grants neither.


For a correlated external endpoint whose whole body/classification is still unknown,
`identity_only_entries` may explicitly select an existing unique Unknown registry
row. Each selector is `{ "name": "write", "evidence": { "path": "owned.json",
"sha256": "..." } }`. The row must remain unimplemented, with empty code/data,
no native symbol, no driver/acceptance, and an explicit actual rebuilt MAP
`original_symbol`. The existing contained witness SHA check binds the correlation
packet; it does not interpret that packet's semantic proof. Both entry bytes must
be initialized (executable for PE); absolute/missing MAP symbols, interior body
entries and conflicting physical identities fail. This adds endpoint names only,
with a separate report binding. It grants no extent, CRT classification, ABI fact,
body availability or runtime permission. Without this policy, Unknown entries
remain unmapped. The paired graph still checks every edge normally.


`--check-registry-support-extents` separately walks every nonempty reviewed
CRT/import interval in `function_registry`, using the same instruction-boundary
and code/data validation as accepted extents. It skips Unknown records,
unimplemented App entries and entry-only support declarations. Original bounds
come directly from the registry. Rebuilt checks require unique existing real
Record, shared identifier or verified import-thunk bounds, including physical
aliases; the flag never invents a bound from the next MAP public. Missing rebuilt
structural bounds are listed explicitly with counts and names. Accepted graph
bodies still require their rebuilt counterparts independently. This flag neither
selects graph/RET bodies nor grants acceptance, restoration or execution credit.

Watcom zero-sized DATA tables may name a boundary exactly one past an actual PE
section. Such otherwise-unmapped markers are recorded as `boundary_symbols` in
MAP provenance only. They are never callable publics, body extents or mapped
bytes; nonzero/CODE or arbitrary outside symbols still reject.

An object-proven rebuilt body end can be supplied by the shared exporter as
`driver[].rebuilt_extent`: exact end, `padding_hex:"00"`, current PE/MAP SHA256,
plus a mandatory contained `{path,sha256}` binding for the compiled object
and an optional documentary witness. This scope accepts exactly 1..15 declared zero alignment bytes between the
object-proven body end and the unchanged next MAP public. The bytes remain explicitly checked,
not text-data or executable ownership. CFG roots/edges into it, overlapping code,
stale inputs and malformed contracts reject. The common tool verifies hashes and
boundaries, not the witness's object-format semantics. Default MAP-derived
extents remain unchanged when no descriptor is present.

Finite paired compatibility CALL boundaries can be declared in optional metadata
`compatibility_calls`: each row has `owner`, a unique `name`,
`original: {site, target}`, `rebuilt: {site_offset, import: {dll, name}}`,
and contained SHA-bound `evidence: {path, sha256}`. The original must be an exact
unprefixed direct CALL; the rebuilt owner-relative instruction must CALL the
named standard PE import, directly through its IAT or its verified thunk.
Both sites must be discovered instruction starts in the selected nonpending
paired owner. The report retains their raw bytes/targets and bound evidence.
Only these two CALL edges share the declared boundary role; unrelated calls,
references, indirect topology, RET, ownership and extent checks remain active.
The declaration supplies no ABI, register preservation, provider equivalence,
function extent, acceptance or execution permission. Boundary semantics and
behavior remain the target's explicit responsibility. Without this metadata,
existing discovery and reports are unchanged.

A canonical registry may explicitly declare `shared_return_entries` as
`[{owner, entry, evidence: {path, sha256}}]`. Both names must denote implemented,
nonpending application records with canonical extents. The entry must consist
of exactly one unprefixed `C3` byte at the owner's end, and the existing CFG
must discover that RET as an instruction start in both functions. Only this
exact original owner/entry pair may overlap; all other ownership checks remain.
The report records the shared executable byte and zero additional unique code
bytes. These remain distinct graph identities; no artificial owner-to-entry
CALL is added. Rebuilt bodies may be separate. This declaration provides no
runtime admission or entry-trace observation; those require their own explicit
case treatment. Without a declaration, overlap rejection is unchanged.

Opaque documentary `evidence` is optional on `identity_only_entries`,
`compatibility_calls`, `shared_return_entries` and `rebuilt_extent`. The canonical
registry and declarative site/target/ownership fields remain mandatory and undergo
all existing structural checks. A supplied evidence field, including null, must
still pass the contained file/SHA verifier. Diagnostics may stay outside the
checkout without becoming audit prerequisites. Actual rebuilt-extent compiled
object, image and MAP bindings remain mandatory; this does not make preservation
contracts or semantically consumed ABI policy optional.

`shared_terminal_epilogues` can declare `[{owners: [name1, name2], original:
{start, bytes_hex}}]`, with optional documentary evidence. The suffix must be
1..32 exact bytes, with contiguous fallthrough and no control transfers before
its final unprefixed near RET (`C3` or `C2 imm16`). Both canonical App
records must own the entire declared overlap, terminating at both code-chunk
ends, and both existing CFGs must discover every complete suffix instruction and each fallthrough edge.
This preserves two function identities and their original control paths; it
creates no suffix function, added edge, code permission or ABI inference.

`shared_call_return_tails` is a separate exact two-owner rule for a shared terminal
sequence containing one explicit direct call. Each row uses
`{owners: [name1,name2], original: {start,bytes_hex}, call: {offset,target}}`, with
optional documentary evidence. The target is a unique implemented canonical App
entry. Both original CFGs must discover the complete overlap through the exact
unprefixed `E8` call at the declared offset, retain its raw callee edge, and
fall through to the terminal near RET. Every other control transfer rejects.
All existing chunk-end, ownership, graph and ABI checks remain; no call is
normalized, removed, or granted runtime permission. The branchless
`shared_terminal_epilogues` contract is unchanged.

`shared_jump_tails` uses the same exact two-owner range and chunk-end checks with
`{owners:[name1,name2],original:{start,bytes_hex},jump:{offset,target}}` and optional
documentary evidence. Both existing CFGs must discover contiguous branchless
instructions through the one final unprefixed direct `E9`, preserving both raw
tail/callee edges to the unique canonical App endpoint. The endpoint may remain
pending and bodyless: this declaration neither discovers its body nor asserts
cleanup, nonreturn, acceptance or execution permission. Existing per-owner RET
checks and any separately reviewed binary-specific nonlocal-tail checks remain
necessary. Published returning-tail declarations retain their previous meaning.

`register_imports.virtual_preservation_contracts` can qualify one explicitly
selected owner's typed no-stack-argument `thiscall` virtual call for EDI
preservation on normal return. Each contract supplies `owner`, `abi:"thiscall"`,
`stack_argument_bytes:0`, `preserves:["edi"]`, a hash-bound ABI `evidence` file,
and `original`/`rebuilt` locations. A location contains `site`, byte `slot`,
`receiver_global`, `receiver_load_site`, `this_load_site`, `vptr_load_site` and
finite `vtables:[{address,target}]` (logical function names). Both sides must
name the same finite target set. Each target also needs an existing named
normal-return preservation contract that qualifies EDI; pending targets gain no
body availability or acceptance.

The actual CFG must contain the receiver-global and ECX loads, the vptr load
and exact indirect memory call. The bounded receiver chain must fall through
without a bypass or unqualified clobber; other intervening stores may only
write disjoint writable absolute globals, without touching the receiver, declared
vtable cells or IAT. Actual readable vtable cells must equal the
separately qualified target entries. Only an existing EDI must-value is retained;
no target/import/value is seeded and the virtual call remains opaque. Existing
absolute `FF15` callback contracts and flag-off report behavior are unchanged.

`shared_jump_tails` may additionally declare one `call: {offset, target}` role before
its terminal jump. The exact direct CALL must be discovered at that offset in
both owners, target a unique implemented canonical App body, and retain both raw
caller edges. All intervening instructions must have contiguous normal
successors; no other control transfer is admitted. Both roles must be consumed,
and the complete exact shared range must still end both owning chunks. The
terminal endpoint can remain pending; neither role supplies cleanup, nonreturn
proof, body availability, or runtime permission. Without `call`, the existing
branchless shared-JMP rule and report are unchanged.

`shared_terminal_regions` describes an exact physically shared terminal subgraph
for two implemented canonical App owners, including disconnected chunks:

```json
{"shared_terminal_regions":[{"owners":["template_arg","name"],"original":{
  "regions":[{"start":4281981,"bytes_hex":"31c0eb19"},
             {"start":4282010,"bytes_hex":"c95f5e595bc3"}],
  "branches":[{"site":4281983,"target":4282010}]
}}]}
```

The sorted exact-byte regions (1–64 bytes total) must equal the complete pairwise
code intersection and end both owning chunks. Both existing owner CFGs must
discover identical complete instruction coverage and successors. Every branch is
explicitly inventoried; all normal successors stay within the union and form an
acyclic graph terminating at actual unprefixed near RET instructions. No new
roots or function identities are created.

Optional `original.calls` inventories exact existing near CALL sites. Each row
has `site` and either `target` (using the existing implemented canonical App
qualification) or `indirect_operand` (the exact existing decoder operand, e.g.
`"dword ptr [esi]"`). Both owners must retain the actual raw direct or opaque
indirect call fact and the normal post-CALL successor inside the union. Every
CALL must be consumed once; absent inventory admits none. Indirect/external
jumps, interrupts, cycles and other transfers reject. Raw callee edges and all
ordinary paired graph/ABI/RET checks remain unchanged. This is physical ownership
metadata, with zero unique-byte credit; it supplies no callback target, register
value, preservation, cleanup, body availability or runtime permission.

`terminating_calls` is an opt-in finite semantic assertion for an independently
reviewed process-exit boundary, not a global name-based noreturn inference:

```json
{"terminating_calls":[{"owner":"ReportSyntaxError","side":"rebuilt",
  "site_offset":143,"target":"exit_","role":"process_exit"}]}
```

The owner must be one selected implemented canonical App body. The target must
be one unique already-classified CRT identity with its actual mapped executable
endpoint; classification or spelling alone never creates this assertion. The
checked owner-relative site must decode as the exact final unprefixed direct E8
CALL to that target, ending the genuine owning code chunk with no contiguous
owner continuation. The existing owner CFG must actually discover and consume
it. The raw callee/instruction stays; only this CALL's false continuation into
the next function stops. No code bytes, neighboring ownership, roots or RETs are
changed. The same validated map applies to RET traversal without supplying any
cleanup convention. Selected scope declarations must name selected owners.

The separate binary-local exit role/status residual remains required; this
mechanism does not verify a provider body or introduce callback, ABI, body
availability, execution permission or acceptance claims. With the field absent,
ordinary CALL/fallthrough discovery and reports are unchanged.

RET traversal distinguishes an unknown empty cleanup set from a validated
no-return endpoint. Only an explicitly declared terminal-CALL owner can earn the
latter: its existing normal CFG must close acyclically to the consumed CALL sinks,
with no RET, other tail or unresolved path. A direct tail caller keeps its raw
edge and contributes no RET value for that endpoint, while retaining its own
actual RET set. Unknown/ambiguous endpoints and cycles still reject. This tag is
not inferred for unannotated tail-only wrappers or arbitrary providers, and never
supplies a fake RET0 convention.
