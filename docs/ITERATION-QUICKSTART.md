# Iteration quickstart

The support is opt-in; existing binaries have not been migrated. See
[ITERATION.md](ITERATION.md) for the design and the
[tool README](../tooling/recon-tool/README.md#opt-in-iteration-orchestration) for configuration details.

## Setup

Build the tool from the repository root, then work from the binary's root:

```sh
tooling/recon-tool/build-quiet.sh
recon_tool="$PWD/tooling/recon-tool/target/i686-unknown-linux-gnu/release/recon-tool"
cd decomp/PROJECT/BINARY
iteration_case=cases/0001-example
```

In that binary's Makefile, enable iteration alongside its existing build/audit
configuration:

```make
RECON_ITERATION_ENABLED := 1
# Default input: $(RECON_ITERATION_CASE)/capture.bin
```

Author input JSON in a new scratch folder and pack the selected case's
`capture.bin` with `recon-tool capture pack INPUTS.json CASE/capture.bin`.
For an existing case, preserve its oracle and set `RECON_ITERATION_INPUT` to a
separate authored input capture if necessary. Declare any existing DLL host,
DIRECT helper, CF profile, metadata prerequisites and residual checks needed by
the case; the iterator does not infer them. Do not use a prior observed launch
as new input unless its exact launch metadata is intentionally required.

## Walk a frontier

First establish this case journal's checked static baseline, preferably at an
already planned audit boundary:

```sh
"$recon_tool" original iterate "$iteration_case" --all-restored --check-only
```

This builds and checks all currently restored App owners, including a period
build if needed. It does not execute the original target or run all cases.
Omit `--check-only` to discover and compare the first frontier as well.

Reconstruct the pending function, then request one new original/native pair:

```sh
"$recon_tool" original iterate "$iteration_case" --owner 'logical_function_name'
```

Use names from `binary.c`; repeat `--owner` for affected callers/data owners.
The command builds, checks the selection, captures original progress once and
compares native execution. A matching frontier is progress, not case completion.

For a native mismatch, fix the source and retry the saved original:

```sh
"$recon_tool" iterate replay "$iteration_case" --reference latest
```

Replay builds only native prerequisites and never launches Wine. `latest` can
select a captured original whose native comparison failed. Changed original
restoration or invocation inputs require a new pair. Use
`original iterate "$iteration_case" --owner NAME --check-only` for a required period-build/scoped check without
another original target run.

## Inspect and finish

```sh
"$recon_tool" iterate status "$iteration_case"
"$recon_tool" iterate latest "$iteration_case"
"$recon_tool" iterate export "$iteration_case" --reference STEP --output "$HOME/.ai-tmp/NEW-CASE-FOLDER/update.json"
```

Results show reference IDs, timings and failures; detailed logs stay in scratch.
Export prepares a reviewable expectation update, not an automatic source edit
or acceptance. At case completion, publish reviewed expectations, run the full
regressions/audit, update the README and commit. Unit-only helpers keep using
existing `test`/DIRECT tests plus scoped checks; do not fabricate an original pair.
