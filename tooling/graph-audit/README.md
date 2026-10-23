# Experimental graph audit

Standalone Rust audit for the eight `vc1.0` reconstructions. **Not integrated into
any reconstruction Makefile or acceptance workflow.** The executable reads files;
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
| `--check-ret` | Compare RET cleanup sets, inheriting identified direct tail callees; unknown indirect tail ABIs and cycles fail. CVPACK also checks declared cleanup. |
| `--check-references` | Compare immediate references to indexed function entries, with alias bijection. Available for the seven standalone profiles. |
| `--check-callbacks` | Compare ordered, non-null callback table entries and acceptance requirements from metadata. RC has supplied table metadata; other profiles require `--metadata`. |
| `--check-ownership` | Require original application ranges to be disjoint, file-backed code. CVPACK additionally checks application/text-data/runtime/contextual-data ownership and explicit runtime aliases. |
| `--check-extents` | Check whole extent tail boundaries beyond reached instructions, excluding discovered switch data. CVPACK uses its complete rooted-body/padding checks. |
| `--check-instruction-overlap` | Explicitly assume one instruction owner per byte within a function. This is a reconstruction constraint, **not a universal x86 rule**. |

```sh
tooling/graph-audit/target/release/graph-audit \
  --repo . --project decomp/vc1.0/rc --profile rc \
  --check-ret --check-references --check-callbacks --check-ownership
```

Input identity and existing profile-specific index validation still run: original
SHA-256, symbol mapping, declared duplicate/extent checks and relevant existing
CRT defaults. These establish the input being audited. Runtime identification is
not reimplemented by this graph tool.

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
