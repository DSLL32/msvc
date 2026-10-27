# Common reconstruction iteration

Shared opt-in command support is implemented; binary workflows remain unadopted. This
extends `recon-tool`, its shared Make rules and `graph-audit`; it does not replace
the capture, tracing, debugging or audit engines. `STANDARDIZATION.md` remains
the case format contract and `ENTRY.md` governs canonical executable startup.
Iteration continues to execute cases through their case libraries, not through
the new standalone executables.

## What workers do today

The five previously active workers reported the same basic loop: reconstruct
an owner, build, export the registry, check affected owners, capture masked
original progress, and compare native execution to that specific reference.
Most remaining duplication is command orchestration and bookkeeping in scratch
Python scripts, rather than separate execution engines.

| Worker | Repeated scratch machinery | Target-specific support to retain |
| --- | --- | --- |
| VC5.10 C1 | `browserPair` / `browserRetry`, selected-owner audit projection, manual reference selection | Typed DLL host and lifecycle, PE/MAP switch layout, parser/assembler semantic generators |
| VC5.10 C1XX | `pair.py`, selected-owner projection, manual trace/frontier updates | Typed DLL host, C++ ABI/layout contracts, semantic generators and finite residual checks |
| VC5.10 C2 | `c2step` / `c2audit` / saved-reference retries, descriptor rebuilding | Genuine compiler IL inputs, machine-table relations, x87/ABI/layout checks |
| VC5.10 LINK | `step.py`, qualification notes, selected-owner projection, descriptor rebuilding | EH metadata/residual checks, real SELF librarian invocation and input validation, artifact comparison policies |
| Watcom WCC | `advance.py` / `record.py`, selected-owner projection and promotion bookkeeping | CF profile/loader and raw command tail, native startup, reviewed OMF/layout and nonlocal-tail checks |

The shared operations already exist as `registry-export`, `original capture
--progress`, and `run --capture ... --original-report ...`. Registry export also
has `--accepted-scope NAME`: it selects a finite graph comparison view while
retaining the full registry and driver identities. It does not automatically
scope every independent RET or target-specific assertion.

Recent reported examples, not averages:

| Worker | Original capture | Native replay, total | Scoped checks |
| --- | --- | --- | --- |
| C1 | 1.1–2.1 s | 19–22 ms | Separate graph/layout checks |
| C1XX | 2.329 s | 29.437 ms | Graph 75–82 ms |
| C2 | 1.8–5.6 s | 43–48 ms | Graph 130–260 ms |
| LINK | 1.6–3.1 s | 25–53 ms | Graph 130–150 ms, plus about 400 ms for projections/EH |
| WCC | 6.010 s | 51.947 ms | Scoped graph 4.712 s |

Builds add variable time. These workers do not currently have reliable aggregate
launch counts or an accounting of analysis versus shared-support waiting time.
Function counts and first-entry counts are not original-launch counts.

## Shared interface

Supported commands (opt-in Make configuration is documented in tooling/recon-tool/README.md):

```sh
# Incremental preparation, preflight, one fresh original reference, native pair.
recon-tool original iterate CASE --owner FUNCTION

# Fix native code and retry a selected reference without launching the original.
recon-tool iterate replay CASE --reference latest

# Show the last result/frontier, or export a reviewable case expectation update.
recon-tool iterate status CASE
recon-tool iterate export CASE --reference STEP
```

First use requires an explicitly requested `--all-restored --check-only` static
baseline, or `--all-restored` on the attempt. A narrow first request fails before
launch instead of triggering an implicit full audit. No binary is migrated by
providing these commands.

`original iterate` explicitly permits the original invocation. It performs one
discovery attempt, not an unattended reconstruction loop or automatic retry
campaign. `iterate replay` is native-only. Existing `run` and `test` retain their
Wine-free meaning. The underlying commands remain available for debugging.

The common Make include supplies incremental preparation and scoped-check
targets, using existing source lists, toolchain settings, case discovery,
projection inputs and audit flags. Default to `make -j4` and preserve its
jobserver. Bind the requested case directly; do not use an unrelated help case
just to obtain the binary registry.

Use existing binary policy and format profiles for persistent configuration.
Support existing named scopes and a transient list of changed owners using the
same validated selection logic, rather than modifying generated JSON in scratch
or appending a permanent policy scope after every function. Include affected
callers and data/table owners when their contracts change. Keep the full identity
catalog and applicable global ownership checks; a smaller scope cannot waive
an unresolved edge, bad extent or required ABI check.
Retain generic preservation, table and callback facts when selecting
owner-specific assertions and RET contracts. `--owner` may be repeated; data-only
restoration corrections are valid steps and add no reconstructed-code credit.
Allow explicit supporting owners/witnesses where a contract requires them;
consistently select their owner-specific assertions without inventing an
automatic dependency-reconstruction engine.
Every newly added or changed App restoration in the planned mask must be
covered by the checked scope or unchanged, previously checked artifacts. An
incidental implementation-status edit outside `--owner` must not silently
unmask an unchecked body.

Target-owned Make prerequisites/hooks may refresh layout or EH metadata and run
finite residual checks. Capture configuration supplies existing DLL-host,
DIRECT, CF, provider and SELF options. Do not add another case manifest or allow
a hook to bypass a failed common gate. Binary-specific semantic reconstruction
and decisions about unsupported behavior remain the worker's responsibility.

## One iteration

1. The worker reconstructs readable source and updates the canonical registry,
   extents, encountered data and necessary contracts. Unresolved callees remain
   named traps. The tool does not infer function bodies, ABIs or classifications.
2. Incrementally build the native reconstruction and selected native case DSO,
   and separately the reconstruction PE/MAP with the original compiler. Refresh
   necessary metadata after that link from the actual PE/MAP or OMF artifacts.
   Fixtures remain native-only. A failed command stops the attempt; never run a
   stale success artifact left by an earlier build.
3. Export the complete registry and selected audit view. Run the applicable
   extent, ownership, graph, import, callback and RET checks **before** launching
   a changed-mask original. Keep target-specific residuals explicit. Do not run
   the full regression suite or repeat unchanged checks after the pair.
4. Record the actual consumed build, registry, policy, original-image and tool
   identities. Run existing `original capture --progress` once, retaining its
   capture, report and raw artifacts under a unique scratch attempt directory.
   Masking remains derived from the registry and existing format backend/profile.
5. Run native execution with **both** that capture and that original report.
   Compare actual first-entry order, terminal/frontier, streams and applicable
   artifacts. A final result must also satisfy the final output/status oracle;
   matching a partial prefix does not finish the case.
6. Report the next frontier or a specific failure. Record which candidate owners
   were actually exercised by the passing pair. Implementation availability in
   `binary.c` is distinct from runtime qualification. Do not promote untouched
   siblings, hidden branches or unobserved table entries merely because they
   were included in a source edit or selected audit scope.
   Reaching the next matching stop need not mean that the owner returned or
   that every descendant was implemented.

An unknown masked PC is a discovery/extent issue to investigate, not a fabricated
successful named frontier. A native divergence stops the step for debugging;
the tool does not change inputs, comparisons or expectations to make it pass.

First-entry order is a useful regression signal, not a complete call sequence,
instruction trace or proof that arguments and internal objects are equivalent.
Retain necessary focused tests and static contracts. Explicit per-case seen
sets apply symmetrically; a filtered entry does not prove nonexecution. Do not
silently discard the accepted prefix. WCC
currently executes and records that prefix, excluding only seven approved
static-empty entries. Shared child seen-state is used only where the case
actually has supervised children.

## Reusing references and avoiding stale state

Native-only fixes normally retry the same immutable original capture/report.
Rebuild the native artifacts, including the descriptor DSO, and record the new
attempt; do not launch Wine
just because a native source file, link address or commit changed.
Verify changed Win32 reconstruction source with a separate explicit incremental
original-compiler build; host-only glue does not need that build. This does not
require another original reference, and native-only replay stays Wine-free.

`latest` selects the latest compatible, successfully captured original reference,
even if its native comparison failed. Display its identity before execution.
A failed prelaunch attempt creates no reference; never select it, require the
latest native pass, or change native stubs to recreate an obsolete frontier.

A changed original mask, restored code/data extent, actual invocation/input or
dependency premise requires a new original reference. Validate that distinction
from the original-side execution plan, not a hash of the entire working tree.
An extent correction that changes restored bytes requires fresh discovery;
an unrelated native relink does not. Never overwrite or relabel a prior receipt.
The reconstruction's rebuilt PE/MAP is an audit input, not the original target.
An auditor or metadata/witness correction alone does not require recapture when
the original restored bytes, host and invocation remain unchanged.

For a new invocation, use the authored required inputs. Do not accidentally
freeze the previous run's full inherited OS environment: LINK lost five
prelaunch attempts to this unnecessary requirement. Unless exact preservation
is intentional, capture the new invocation's actual OS block and stage that
same block natively. Keep required environment settings, cwd, files, timestamps,
raw command tails and dependency inputs explicit. Fixture controls are not
target environment inputs.
Full OS-block observation/staging remains optional and consumer-driven: it is
needed by LINK's reached SELF environment path, not a universal DLL requirement.
C1 currently declares consumed environment facts and forwarded call arguments.

Current original receipts bind capture bytes and logical execution identity;
they do **not** prove that current source, DSO, PE/MAP, policy and tools are the
ones built or checked. The new orchestration must track the artifacts each
phase actually consumed and reject changes between preparation and execution.
It must still permit an explicit native-only retry with newly built native code.
Record original-side and native-side dependencies separately.
Fresh iteration acceptance requires the existing capture/identity bindings.
The current parser also supports legacy unbound reports: disclose such reuse
explicitly rather than treating it as a newly bound qualification. Existing
cached replay compatibility is unaffected.

Existing root progress execution permits the prior cached frontier to have
become implemented. Iteration must wire that mode into registry export and
SELF child setup too; their current ordinary loaders can still reject the stale
frontier. Retain selected-reference validation and strict ordinary cached replay.
Avoid rewriting `case.c` solely to make intermediate inspection or SELF setup
accept an obsolete frontier.
Carry that explicit progress context to necessary focused test/inspection setup
as well; do not weaken ordinary regression validation.

For DLL and SELF cases, reuse existing typed hosts, child policy, genuine child
execution and fresh state. Pending native child traps still fail. CF retains
its existing loader/allocation/relocation checks and raw-tail transport. Its
conservative CODE mask envelope and registry code/text-data restoration holes
are also original-side plan inputs; that envelope can contain mixed data.
There is no new interpreter or instruction-by-instruction admission system.

## Results, publication and cadence

Keep iteration state and reports outside committed case folders, under owned
`~/.ai-tmp` scratch. Store a small journal pointing to immutable references,
their hashes, selected owners, commands, outcomes and timings; do not duplicate
full registries, instruction dumps or source trees as per-function evidence.
Hashes and paths document what was consumed; they do not promise historical
reproducibility after files have been overwritten. No mandatory source archive.
Preserve failed-attempt logs/reports and available explicit raw exports for
diagnosis. Native workspaces are currently deleted and have no durable artifact
export option; some original failures happen before export. Universal raw-file
retention is not an initial implementation prerequisite. Workspace cleanup
remains supervised by the existing executor.

Console output should be concise: build/check/original/native durations with
units, actual original-launch count, matching prefix length, next frontier, and
the first useful divergence. Separate prelaunch failures and native-only retries
from actual original target/loader launches, rather than counting capture CLI
invocations, helper builds or prelaunch failures as launches. Where a failed
attempt does not establish whether launch occurred, report "unknown". Retain
setup/execution/check timing splits where available;
do not estimate analysis time from command durations.

Intermediate references override historical descriptor expectations through
`--original-report`. The journal is execution history, not a second function
registry. `iterate export` prepares a reviewable first-entry/frontier/capture
update for the existing descriptor; it must not rewrite the whole C file,
silently accept a comparison exception or mark a case finished.

At case completion, publish the reviewed expectations, rebuild the descriptor,
run the existing full native regression/unit suite and full audit, update the
README and commit. During reconstruction, use the current pair and necessary
focused checks. Explicit user-requested checkpoints remain allowed.

## Initial implementation scope

Start with command composition, progress-mode registry export, validated owner
selection, artifact freshness checks, immutable reference selection and compact
results. Reuse `registry-export --accepted-scope`, `original capture`, `run` and
the existing graph auditor. Migrate scratch orchestration only after the common
path works for the DLL, EXE/SELF and CF variants above.

Do not make a new debugger, masking engine, proof framework or acceptance ledger
format a prerequisite. Broader audit caching or native artifact-export helpers
can be separate changes if measurements or concrete debugging needs justify them.
