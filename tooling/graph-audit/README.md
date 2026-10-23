# Experimental graph audit

Standalone Rust audit for the eight `vc1.0` reconstructions. Integration is
coordinated per worker; preserve each project's required checks and explicit
reference provenance. The executable reads files;
it never invokes Python, Wine, a compiler, or a reconstructed/original executable.

Build with Rust 1.88+ and a host C compiler (for Capstone):

```sh
cargo build --release --locked --manifest-path tooling/graph-audit/Cargo.toml
tooling/graph-audit/target/release/graph-audit \
  --repo . --project decomp/vc1.0/c2 --profile c2
```

Dependencies are Clap, Serde/JSON, SHA-256, Regex, Anyhow and Capstone's established
Rust bindings. Only the x86 Capstone backend is built. `Cargo.lock` is tracked.

`--profile` selects existing metadata/index conventions, not a new acceptance
policy. `--original`, `--rebuilt`, `--map`, `--config`, `--runtime` and
`--rebuilt-runtime` override individual paths. Paths resolve from the invoking
working directory; normal build paths otherwise resolve under `--project`.
Output is deterministic JSON on stdout; `--report PATH` additionally saves it.
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

## CVPACK identity cache and scope

CVPACK's Python `audit.py` combines graph checks with strict archive relocation
identification, source/stub checks, native symbol checks, contextual data and CRT
seam approval. This tool **does not replace that full audit**. Its graph subset
still checks every graph edge; ownership is not skipped merely because runtime
identities are cached.

For comparison, explicitly prepare a frozen identity index with its existing
strict verifier, then give that file to Rust:

```sh
python3 tooling/graph-audit/cvpack-reference.py \
  --project /path/to/snapshot/decomp/vc1.0/cvpack \
  --cache /path/to/snapshot/decomp/vc1.0/cvpack/graph-identities.json --prepare

tooling/graph-audit/target/release/graph-audit --profile cvpack \
  --repo /path/to/snapshot --project /path/to/snapshot/decomp/vc1.0/cvpack \
  --rebuilt-runtime /path/to/snapshot/decomp/vc1.0/cvpack/graph-identities.json \
  --check-ret --check-ownership --check-extents
```

The cache binds exact original/rebuilt PE bytes, LIBC archive, configuration, MAP
and runtime anchor metadata. Original runtime records must equal the current
recorded index. A changed input fails; no implicit cache refresh occurs. Cache
creation is explicit Python tooling, outside executable operation and timed graph
comparisons. It does not invoke Wine.

## Verification and timings

`validation.json` records the tested snapshot hashes, tests and measured results.
The seven standalone Python audits are unchanged baselines, executed in scratch
copies so their report writes never touch workers. CVPACK uses a newly extracted
**graph-only** Python reference with the same frozen identity index; its full
integrated audit is separately checked and timed, not used for a misleading
like-for-unlike speedup.

```sh
cargo test --locked --manifest-path tooling/graph-audit/Cargo.toml
cargo clippy --locked --manifest-path tooling/graph-audit/Cargo.toml --all-targets -- -D warnings
python3 tooling/graph-audit/verify.py --snapshot ~/.ai-tmp/graph-audit/snapshot \
  --snapshot-only --report ~/.ai-tmp/graph-audit/benchmarks.json
# Prepare the CVPACK cache explicitly after making a new snapshot, then:
python3 tooling/graph-audit/verify.py --snapshot ~/.ai-tmp/graph-audit/snapshot \
  --reuse --report ~/.ai-tmp/graph-audit/benchmarks.json
python3 tooling/graph-audit/negative-parity.py --snapshot ~/.ai-tmp/graph-audit/snapshot \
  --report ~/.ai-tmp/graph-audit/negatives.json
python3 tooling/graph-audit/check-flags.py --snapshot ~/.ai-tmp/graph-audit/snapshot \
  --report ~/.ai-tmp/graph-audit/flags.json
python3 tooling/graph-audit/benchmark-cfg.py --snapshot ~/.ai-tmp/graph-audit/snapshot \
  --report ~/.ai-tmp/graph-audit/cfg-benchmarks.json
```

Snapshots use a hash/recheck loop and are read-only with respect to worker files.
Do not run mutation tests and benchmarks concurrently on the same snapshot.
Benchmarks warm each CLI, alternate Python/Rust ordering, include startup/input
loading/output, and report seven samples with medians and ranges. They exclude
builds, fixture execution, Wine and explicit cache preparation. Current workers
may advance after the pinned checkpoint; these results do not certify later
unbuilt source changes.

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
