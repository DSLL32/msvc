# Continuous historical replay

`fast_replay.py` prepares one masked executable and restore plan per side and
runs independent debugger walkers concurrently. Target adapters own case setup,
the acceptance prefix and expected sequence, terminal identity, and concrete
output validation. This is a first-encounter regression, not branch coverage.

The walker is compiled with the original MSVC 1.0 NT toolchain. Run from this
directory:

```
make -j2
make check
```

Builds use a private prefix under `~/.ai-tmp/vc1.0-fast-replay/wine`. Actual target
runs use the adapter's Wine environment and namespace. `make check` exercises
real Win32 processes: ordinary/prefix/zero-step/terminal-stop/paired success,
wrong entry, early exit, exception, unsupported thread and child, timeout, wrong
exit, corrupt restore, changed frozen plan, and wrong final output rejection.

## Adapter contract

Call `prepare_side` with audited PE/index objects, the full accepted list,
absolute `first`/`last` indices, and an explicit terminal (`exit:N` or a logical
unaccepted entry). Supply the existing mask callback so whole-code masking and
shared-text ownership remain controlled by the target's audited indexes.

Supply each target's exact command line, cwd, environment and canonical executable
path where observable. An environment dictionary becomes an exact Windows
environment block; `None` inherits the Wine launch environment. Fixed-name
side namespaces may use `image_path` and `application`. Closed stdin is provided.
Only single-process/main-thread ordinary cases are supported; other modes fail.

Call `run_pair` with the two sides, then run the adapter's existing status,
stream and output-file validators. A completed trace alone does not validate
products. Explicit fresh oracles and specialized runners remain available.

The plan restores complete function/shared ranges and all same-name entry
aliases, checking writes/readback/protections/cache flush before rewinding EIP.
Raw restore data requires the preferred image base. A declared terminal trap
remains masked. Early exits and unexpected events fail without accepting code.

## Evidence and debugging

Per-side evidence includes the frozen plan/image hashes, symbol ranges, raw
JSON-lines events and normalized events, register context, host logs and a final
receipt. Normalized indices are absolute acceptance indices; `local_index`
retains the raw side-relative index. Failures record expected and actual logical
sites where indexed, otherwise the unknown address. Both launched sides are
stopped on failure; the native debugger kills/reaps its target and host timeout
cleanup terminates the launch process group.

Use the target adapter's continuous stop-at option to replay up to a selected
frontier, or its focused fresh fallback to reproduce that stop from startup.
Restart after source/config changes; never continue a process after relinking.
Raw evidence is retained even if output validation fails.

## Cadence

Ordinary histories default to this fast pass and retain final input/output
checks. Keep the fresh paired new-function check. Run convoluted histories at
substantial reconstruction checkpoints/case completion, or when directly
affected, using the adapter's explicit specialized/fresh command. Do not add
console/signal machinery to the common walker merely to accelerate one rare
frontier. See repository `AGENTS.md` and `FAST-REPLAY.md` for the agreed policy.

Adapters record measured fixture wall times and separate setup, pair execution,
fresh references and output oracles where available; unmeasured splits remain
explicit. Prefix consolidation is a separate, deferred change.

See [validation-accounting.json](validation-accounting.json) for adoption timings,
including repeated runs, aborted attempts and unmeasured work. Phase timings are
subsets of their enclosing runs; overlapping worker elapsed times are not CPU
time or session duration.
