# Path folding initializer

The trial is the complete83-byte `initialize_path_fold_table`: hot12f3e..12f87
and cold307d2..307dc. It fills a byte-indexed256-WORD BSS table at55928,
calls the genuine public `_mbctolower(unsigned int)` for non-lead bytes,
usesffff for scanner class35 and forces `/` to `\`. Consumer1344d..1348d
uses the low byte for in-place path normalization and skips a multibyte pair
forffff. The following storage at55b28 is unrelated and is not included.

The new512B belongs to BSS clearing, outside the6548B initialized snapshot.
All actual original entry defaults are zero. A restore auxiliary seeds the
known table before BSS clearing, verifies that it clears, then supplies a
separate labeled nonzero input at the actual helper entry to check complete
replacement, lead-byte skipping and the slash override. Unknown initialized
bytes remain4200; no larger buffer or snapshot contents are inferred.

Fresh original-only captures `minimal-path-fold-001` and
`minimal-path-fold-restore-001` reach `read_env_arguments`12f87 with a genuine
NULL argument. They take6.177840s and5.977631s and observe256/255 actual
`_mbctolower` calls respectively. Every entry and return retains the full
unsigned scalar result, cdecl stack/return address, four nonvolatile registers,
three errors,112B original floating state, complete locale/MB scalars and
current/retained tables, provider case map, result-string ownership and the
partially written target table. Only the512B target output changes across the
whole helper; initialized data/snapshot remain untouched. These are original
observations, not native API success or accepted application credit.

The shared import surface is READYa6e94f33f and deliberately fails125.
`native-path-fold-entry-analysis-001` stops at the true public native entry
before that body; its receipt is explicitly analysis-only and ordinary
candidate acceptance rejects the role. The true DWARF formal is
`character: unsigned int`; the raw cdecl input is0. It takes5.305410s with
221 fresh grouped-only modeled checkpoints,2.584033s decoder time, and no
legacy reads. This is a prefix measurement, not a complete helper fixture.

Recorded native compilation is2.235573s for50 source inputs. Strict common
Rust audit checks73 source vertices/5355 code bytes plus16 separate data
bytes in0.043051s, with every cached EBP import call resolved under the
finite canonical provider contract. No EBP/RET/ownership checks are waived.
Six affected static DWARF/layout guard rejections pass0.044150s without
executing the target. Full native helper adoption and promotion are held
until the shared API contract and own actual complete pair pass.
