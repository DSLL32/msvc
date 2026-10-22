# Fast historical frontier replay

Implemented for the unfinished vc1.0 targets **C1, C1XX, C2 and LINK**, using the
shared infrastructure in `tooling/replay/`. Their ordinary historical replay
commands default to continuous replay; explicit fresh fallbacks remain available.
CL, RC and RCPP are complete under the project's scope and retain their existing
runners. Remaining unused CRT code does not make those targets unfinished.

All seven binary folders use `src/` for source and `scripts/` for utilities.
Makefiles remain at binary roots. The historical benchmark below explains the
motivation; committed target validation records cover the implemented adapters.

## Purpose and scope

Avoid restarting Wine processes for every historical frontier. Start each side
once per case segment, then restore accepted functions progressively as their
entry traps are encountered.

Design for the common case: ordinary inputs with concrete expected outputs.
Most cases should follow this simple path. At the end of a fast run, reuse the
case's existing validators to compare exit status, streams, and output files,
with only its already established normalization rules. These output checks do
not require a restart for each frontier or live state comparisons at every stop.

This is a regression check of the recorded **first-encounter sequence of logical
functions**, including the expected terminal event. It does not record every
repeated call, branch, or loop iteration, and is not branch coverage or a proof
of complete control-flow equivalence.

Keep the existing fresh paired reconstruction loop for newly reconstructed
functions. Existing specialized state/buffer comparisons, ABI and function-graph
audits, and independent fresh/untouched-original checks remain in their
appropriate validation paths. The fast historical pass need not duplicate those
checks at every frontier. Ordinary final input/output validation belongs at the
end of the fast case run.

## Simple cases and complex replay cadence

Fast replay should serve the majority of cases with captured inputs and concrete
outputs. A specialized setup such as creating a console, arranging a blocked
read, and delivering a real Ctrl+C is an exception, not a requirement for the
common walker.

Keep such complex cases on their existing specialized runners initially. Batch
their historical replays after substantial reconstruction progress for a case,
at meaningful checkpoints and case completion, rather than replaying the whole
complex history after each small addition. Changes that directly affect the
special behavior can justify an earlier targeted rerun. No fixed batch size was
chosen.

This batching applies to historical regression work. It does not defer the
focused paired check needed to accept a newly reconstructed function or to
investigate an actual divergence. `AGENTS.md` records this working policy; each
adapter documents its fast, fresh and specialized-history commands.

## Agreed execution model

1. Freeze the original, reconstruction, maps, function/runtime indexes, harness,
   restore plans, and case inputs. Record hashes and reject changes during a run.
2. Build a plan for each case's acceptance segment. Initially restore that case's
   previously accepted prefix; then expect only the frontiers introduced by that
   case, in their recorded order. Different input/argument cases start fresh
   processes. Cases that introduced no functions still retain their existing
   end-to-end checks.
3. Launch one debugger walker per side, concurrently. Preserve each harness's
   arguments, original executable identity where observable, environment, working
   directory, temporary paths, and input/dependency setup. Use separate Wine
   prefixes where canonical DOS paths must name isolated files.
4. Each walker independently checks the same frozen expected transcript. At each
   expected entry trap, restore the complete function and its recorded shared
   `.text` ranges, rewind the actual trapping thread's EIP to the exception
   address, and continue. Preserve other registers and the stack.
5. Compare the two receipts/transcripts and require the declared terminal event:
   either the expected exit status or the next, still-unaccepted function entry.
   Leave that terminal frontier masked. Nonzero exits can be valid case outcomes.

There is **no requirement for a host barrier at every stop** for this trace-only
check. Independent walkers can each assert the expected sequence. A paired
barrier would be needed if comparing live state before either side resumes;
that is outside the initial fast-pass scope.

PCH creation and use remain separate processes. Each consumer must receive its
corresponding producer's actual generated PCH, after the existing producer
validation; a single uninterrupted session cannot replace that dependency.

## Restoration and failure requirements

- Derive restore ranges and entry aliases from the existing audited indexes.
  Include shared text data and import thunks as recorded; reject interior traps
  and incomplete or inconsistent plans.
- Check memory operations, byte counts, and restored bytes; flush instruction
  caches and restore page protections. Distinguish loader and observation
  breakpoints from frontier traps, preserving any required observation overlay.
- Raw file bytes can contain absolute addresses. Initially assert each image
  loaded at its preferred base. Relocated images need relocation-aware restore
  data or the existing runner; adding the load base to a raw byte-copy address
  alone does not relocate its contents.
- Support only declared process/thread modes. Reject unsupported execution or
  route that case to the existing runner. Preserve ordinary foreign helper
  execution where the current case requires it.
- Fail on unexpected entries, faults, early exits, timeouts, or restoration
  failures. Never skip a mismatch or advance acceptance metadata to get past it.
- Retain the frozen run identity, case and frontier index, expected and actual
  logical sites, process/thread identities, image bases, exception addresses,
  register context, and available caller information. Terminate and reap the
  affected run and any owned child processes on failure.
- Provide an explicit fresh-history fallback and a rerun stopping at the divergent
  frontier. A continuous stop-at-frontier rerun is also useful when a divergence
  does not reproduce fresh. Restart from clean case state after a failure or
  rebuild; do not resume an old process against newly linked code.

## Initial adoption boundaries

| Target | Initial fast-pass scope | Retain existing runners initially |
| --- | --- | --- |
| C1 | Ordinary per-case segments, with canonical `N:` paths and separate prefixes; preserve producer/consumer dependencies | Unsupported special modes; existing full observations and final validations stay separate |
| C1XX | Ordinary per-case segments, including incomplete cases ending at a named frontier | Existing 14-field, pending-buffer, caller, and artifact checks stay in their validation path |
| C2 | Completed minimal case and optimized hello segment; hello starts after its accepted prefix and ends at its current unaccepted frontier | Existing state captures, artifact checks, and fresh reconstruction loop |
| LINK | Help, ordinary archive operations, dump/edit, and ordinary console/non-debug linking | Import-library/DLL cases that self-spawn LINK, full-debug cases invoking CVPACK, and signal-handler segments |

CL, RC and RCPP received the source/scripts reorganization and regression
verification only. Their prior adapter assessments were exploratory and do not
authorize additional CRT reconstruction or a new replay implementation.

LINK's deferred cases require deliberate child-image/process handling, rather
than a generic assumption that one main EXE owns every trap. GUI/DLL loading and
painting checks remain existing post-link validations. RC/RCPP interrupt cases
depend on real console delivery, callback threads, and blocking/return barriers;
they are not prerequisites for accelerating ordinary historical segments.

## Validation before changing a default

For each migrated segment, compare the complete fresh and continuous transcripts
on the same frozen build, including its initial prefix and terminal event. Keep
the existing full validation suite. Verify rejection of a wrong entry, a bad
restore, an unexpected exception, and an early exit/timeout; verify retained
failure context and the debugging fallback. Measure the migrated adapter rather
than assuming the C2 prototype's speedup applies to every target.

## Implemented commands and accounting

Run these commands from the corresponding binary folder. Target READMEs and
validation receipts describe segment selection and debugging options.

| Target | Ordinary history | Fresh history |
| --- | --- | --- |
| C1 | `make replay-all` | `make replay-fresh CASE=CASE` |
| C1XX | `python3 scripts/replay-frontiers.py` | `python3 scripts/replay-frontiers.py --fresh` |
| C2 | `python3 scripts/replay-frontiers.py --profile minimal` (or `hello`) | Add `--method fresh` |
| LINK | `python3 scripts/replay-frontiers.py` | Add `--fresh`; `--include-specialized` includes deferred cases |

Shared walker rejection and success checks run with `make -C tooling/replay check`.
The four adapters were compared against fresh histories; the shared suite also
exercised wrong entries, restore failures, faults, early exits, unsupported
threads/children, timeouts, frozen-input changes and output mismatches. Existing
target state, ABI, graph and artifact checks remain available.

The adoption time accounting is in
[`tooling/replay/validation-accounting.json`](tooling/replay/validation-accounting.json).
It includes per-case timings, repeated adoption runs, aborted attempts and
unmeasured work. It is an inventory of available measurements, not an exact
session total. Runs overlapped across workers; their elapsed times must not be
summed to infer session duration or CPU time. No additional fixtures were run
just to collect timing data.

Final ordinary fast runs include setup and output validation. C1's six cases
took 10.039 seconds versus 665.139 seconds for its fresh history; C1XX took
3.921 versus 133.849 seconds. C2 took 4.194 versus 64.305 seconds for minimal,
and 2.718 versus 62.326 seconds for hello. LINK's 16 ordinary cases took 58.081
seconds versus 535.918 seconds fresh; its two specialized histories took another
182.605 seconds on their existing fresh runners. These are observed warm-prefix runs
under concurrent load, excluding target builds and initial prefix creation.
Fresh histories can perform more state checks than fast histories; these are
measurements of the retained validation paths, not identical-work benchmarks.

The roughly one-second paired walk is only part of the total for some cases.
LINK's ordinary post-link validators may launch additional processes or exercise
a generated GUI program. C2 retains a fresh final oracle, explicitly accounted
separately from its continuous walk. Original fixture capture, clean-build and
early smoke-test times were not consistently instrumented and remain marked
unmeasured. The standalone C1/C1XX and C2 validation records contain detailed
splits; LINK's final accounting includes its specialized fresh cases.

No function reconstruction is part of this implementation change, and no
reconstruction trial follows its commit. Existing pending C2 scheduler work is
preserved separately from the implementation commit.

## Measured prototype

The scratch prototype used C2's completed minimal-C case and all 352 historical
frontiers on one frozen reconstruction. Wine was Debian Wine 10.0 with a private,
warm prefix. Other reconstruction workers continued running.

| Method | Measured seconds |
| --- | --- |
| Fresh starts, four concurrent original/reconstruction pairs | 61.141, 63.197 |
| Continuous live unmasking | 2.314, 2.494, 2.233 |

Median comparison: **62.169 seconds versus 2.314 seconds**, approximately
**26.9 times faster and 96.3% less time**. Timings include masking/plan setup and
process execution, excluding build and Wine-prefix initialization.

Both methods checked all 352 expected entries and final successful exits, empty
stdout/stderr, and the captured object bytes with only the existing COFF
timestamp exception. A deliberately incorrect expected entry at frontier 100
was rejected. Frozen reconstruction SHA-256:
`d7b311afaa248780e36121947b373a6e897b3e63c15708a2008ad7156a7bf165`.

The prototype used one Windows debugger controlling both sides; the agreed
general design uses independent side walkers where isolation requires it. The
benchmark did not validate all target-specific environments, partial cases,
child processes, callback threads, or internal-state observations.

Local experiment files are under
`/root/.ai-tmp/vc1.0-replay-benchmark/`: `walk.c`, `benchmark.py`,
`check-rejection.py`, and `results-final.json`, with frozen inputs and raw run
logs. These are scratch artifacts, not committed harness implementations.
