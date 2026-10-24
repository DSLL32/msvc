# Include-alias root initialization

The accepted 53-byte initializer owns hot `12df6..12e1c` and cold
`307b1..307c0`. It takes the old arena-0 cursor, advances that cursor by four,
conditionally commits through the existing member, and publishes the old cursor
as the include-alias root. The reached case fits in the existing committed span;
the cold branch is reconstructed and audited but unvisited. The four-byte root
contains a null head because the earlier 64 KiB commit supplied zero bytes;
this helper does not clear it. Node layout remains unknown.

Both normal and nonzero snapshot/restore auxiliary pairs reach the genuine
no-argument `initialize_checksums` entry at `12e1c`, caller return `12b66`.
The checks bind actual helper/next-entry registers, empty formal inventories,
caller arguments, preserved registers, errors, x87 state, complete modeled state,
original whole-region deltas, and the full unchanged committed span. Thirty-one
fresh affected corruption controls pass; eighteen core controls were refreshed
because the transport and observers changed. Prior parser grammar/reset negative
proofs are referenced with hashes and were not rerun. Fresh positives recheck the
entire preceding state proof.

Original builds took 3.562 s, native build 1.945 s, and strict 70-function audit
0.040 s. Successful original captures took 3.472/3.272 s and native captures
10.991/11.490 s (normal/restore). Focused controls including the complete restore
positive took 9.533 s; core controls were not timed separately. The native-only
lifecycle fixture took 4.687 ms, split into 1.566 ms setup, 2.496 ms execution,
and 0.241 ms checks. Export names, ordinals, and cleanup checks pass.

The first two native observations are retained as rejected attempts: their
observer compared physical helper-entry registers with the next stub after its
prologue. A separate physical next-entry observer fixes that host-only error;
the unchanged original captures and compiled binaries were reused. An affected
return-address control also found an unused raw entry-return field; the checker
now validates it. No original execution was repeated for either checker change.

Evidence is under `evidence/include-alias-{normal,clear-aux}`. Large immutable
raw regions, exact helpers, compiled ELF and private host vectors stay at the
hash-pinned external paths; no raw vector contents are published.

# Original debugger file identity

The earlier concurrent originals, both named `*-include-alias-001`, failed
before C1 loaded. Their actual KERNEL32 hFile reads had interleaved offsets and
truncated outputs, despite an unchanged canonical provider. Those failures stay
unchanged in scratch and are referenced from `evidence/hfile-offset`.

The reviewed correction reads the same actual debugger hFile at explicit
`OVERLAPPED.Offset` values, using a fresh structure for each synchronous read,
positive exact bounded byte counts, complete length/content comparison and hash
verification. The actual loaded file is never reopened or replaced. A dedicated
transport-only probe deliberately disagreed with the current pointer and proved
five real offset/tail/EOF responses plus the full 2,008,819-byte canonical hash,
before any target DLL load. It took 0.404 s. Its synchronous pointer **does**
update; no pointer-immutability guarantee is claimed. The failed first probe
setup omitted the adjacent canonical CRT and remains a separate failed attempt.

During the fix, original debugger/reference subprocesses used the temporary
common capture lock; builds and native work stayed outside it. Successful helper
adoption used the ordinary fresh serialized include-alias captures above.
After all seven corrected default helpers were reviewed, root released this
temporary serialization. New captures use the corrected reader concurrently;
old cached helpers with cursor-dependent readers must not run on the shared
prefix. Historical wait/failure/probe evidence remains unchanged.
