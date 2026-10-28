# C2.EXE

Native reconstruction and cases use `tooling/recon-tool`: `make -j4 check unit`,
`make -j4 recon-original`, and `make -j4 audit`. Cached replay is Linux-only and
never refreshes originals. Application code is in `src/`, justified target CRT
support in `crt/`, and fixture adapters and finite ABI checks in `harness/`.
The canonical native executable uses root `entry.c` and the shared runtime.

The registry contains 1,301 qualified application functions / 389,855 code bytes,
68.02% of the 573,183-byte whole text extent. That denominator includes CRT,
text data and padding; no complete non-CRT denominator is known. CRT/import
support and contextual data receive no App credit.

- `0000-unsupported-help`: completed `/?` diagnostic, exit 1 and 51 stderr bytes.
- `0001-numeric-control-flow`: completed original CL `/O2 /G5` IL covering signed
  and unsigned arithmetic, loops, calls, pointer aliasing, aggregate copies and
  x87 conversions. Original and native exit 0 with matching streams, first-entry
  order and final 2,347-byte COFF object. Only the identified four-byte COFF
  timestamp at offsets 4..8 is normalized. The exact NUL stdin and frozen target
  environment are retained. All 166 focused units accompany the workload.
- `0002-no-arguments`: completed missing-intermediate diagnostic, full DWORD
  exit 1, empty stdout and exactly 94 stderr bytes including original CRLFs.
  The isolated cwd has no `in` file, stdin is NUL and target environment is empty.
  Its unit also executes the actual coordinator-installed
  `../build/native/bin/C2.EXE`, comparing its streams and POSIX status separately.
  Root `entry.c` has no argument special case.

The numeric case reaches the complete optimizer, allocation, scheduling,
peephole, instruction-emission and COFF-completion path exercised by this input.
It does not qualify pending child bodies or unvisited semantic domains. Partial
machine/register/grammar data retains its explicit guards and ownership limits;
reserved native storage does not establish original defaults or payload meaning.
Known original defects remain documented at their actual consumers. Native-only
traps cover specific unsupported host-layout/address-dependent domains.

The complete graph/import/reference/callback/ownership/extent audit retains every
accepted owner. Live `policy-data.json` cleanup contracts enforce the actually
checked RET sets for every selected owner except `c2_compile_unit`.
`harness/check-unit-finish.py` closes only that original twelve-byte absolute-slot
CALL/JMP dispatcher's valid normal-return ABI. Its exact initializers, callable
alternatives and emitted counterpart are checked; NULL remains non-callable.
The independently qualified completion callback receives ordinary strict checks,
while the unresolved callback remains a genuine named INT3 with zero credit.
Canonical `c2_set_argv` callback aliases share one physical CRT identity.

Raw numeric captures, failed diagnostics, superseded source snapshots and closing
logs remain outside Git under
`~/.ai-tmp/msvc2/vc5.10/c2/numeric-resume-20261028/` (completed reference1302).
The original raw COFF SHA256 is
`9106007fc74dfd7a182529def834808e0813ca412e62b7b2a779836fb8076f8a`.
Earlier numeric research remains under `numeric-resume-20261025/`; migration and
no-arguments evidence remain under `migration-20261025/` and `noargs-20261027/`.
No historical captures were relabeled or recaptured for closing bookkeeping.
