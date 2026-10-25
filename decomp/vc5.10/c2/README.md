# C2.EXE

Native reconstruction and cases use `tooling/recon-tool`: `make -j4 check unit`,
`make -j4 recon-original`, and `make -j4 audit`. Cases contain saved invocation
inputs/oracles; replay is Linux-only and does not refresh originals.
Both invocations retain the original NUL stdin and frozen target environment.
Application code is in `src/`; existing target CRT boundaries and their rationale
are in `crt/`. The common native case adapter is in `harness/`.

The registry preserves 124 accepted application functions / 23,651 code bytes.
Five historical CRT helpers (438 bytes), four import thunks (24 bytes), and
897 bytes of contextual text data are separate. The full text extent is
573,183 bytes including CRT/data/padding; no complete non-CRT denominator is known.

- `0000-unsupported-help`: completed `/?` diagnostic, exit 1 and 51 stderr bytes.
- `0001-numeric-control-flow`: original CL `/O2 /G5` IL and final COFF oracle;
  current accepted frontier is `insert_flow_block` at original RVA 0x28ee.
  Fifteen narrow units retain real-conversion and register-list behavior checks.

The 74-byte `add_register_to_set` (0x40189f..0x4018e9) is now accepted after a
fresh original/native pair at the genuine node-constructor trap. Its prior
uncommitted source, patch, and status were saved before migration in
`~/.ai-tmp/msvc2/vc5.10/c2/migration-20261025/owned-worktree/` and
`uncommitted.patch`. The 126-byte node constructor is also accepted. Its two pool1 free-list pointer
slots have proven NULL defaults; other slots remain unmodeled and guarded in
native execution. Function-entry reset clears 49 size-indexed free-list pointers;
their node-specific payload remains incomplete. The small-node size table currently
models only kind24 = 24 bytes; native guards reject unmodeled kind rows before reading. Old proof packages remain in Git history.

Migration also corrected 19 address stores in 17 callback slots to the existing
physical `c2_set_argv` function and removed the duplicate trapping placeholder.
Sixteen unresolved slot views have no reconstructed consumers; the sole typed
directive consumer is reached after assignment of its real writer. New consumers
require their actual ABI before use. This source identity correction adds no
function or coverage credit. The source inventory made before correction is saved
in the same migration scratch directory as `noop-consumer-inventory.json`.

The flow allocator additionally models only the proven 12-byte pool12 slot;
slots8–11 remain unmodeled backing and are guarded before lookup. The new unit
checks the allocated zeroed span and preserves an unrelated nonzero canary.
