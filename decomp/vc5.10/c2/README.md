# C2.EXE

Native reconstruction and cases use `tooling/recon-tool`: `make -j4 check unit`,
`make -j4 recon-original`, and `make -j4 audit`. Cases contain saved invocation
inputs/oracles; replay is Linux-only and does not refresh originals.
Both invocations retain the original NUL stdin and frozen target environment.

The registry preserves 106 accepted application functions / 21,801 code bytes.
Five historical CRT helpers (438 bytes), four import thunks (24 bytes), and
853 bytes of contextual text data are separate. The full text extent is
573,183 bytes including CRT/data/padding; no complete non-CRT denominator is known.

- `0000-unsupported-help`: completed `/?` diagnostic, exit 1 and 51 stderr bytes.
- `0001-numeric-control-flow`: original CL `/O2 /G5` IL and final COFF oracle;
  current accepted frontier is `add_register_to_set` at original RVA 0x189f.
  Three narrow units retain real-conversion and register-list behavior checks.

The unaccepted 74-byte `add_register_to_set` candidate (0x40189f..0x4018e9)
remains research, with a trial frontier at `new_register_set_node` 0x4018e9.
Its exact uncommitted source, patch, and status were saved before migration in
`~/.ai-tmp/msvc2/vc5.10/c2/migration-20261025/owned-worktree/` and
`uncommitted.patch`. Ordinary source restores the actual prior named INT3 stub;
no candidate was accepted. Reuse the saved numeric inputs when reconstruction
is separately resumed. Old proof packages remain in Git history.

Migration also corrected 19 address stores in 17 callback slots to the existing
physical `c2_set_argv` function and removed the duplicate trapping placeholder.
Sixteen unresolved slot views have no reconstructed consumers; the sole typed
directive consumer is reached after assignment of its real writer. New consumers
require their actual ABI before use. This source identity correction adds no
function or coverage credit. The source inventory made before correction is saved
in the same migration scratch directory as `noop-consumer-inventory.json`.
