# CVPACK testing override

Explicit user instruction, superseding earlier checkpoint/cadence notes:
never run the heavyweight historical proof/negative pipeline again. Do not
run `scripts/check-rejections.py`, periodic/final all-prefix proof replays,
or the bespoke 97/98 completion pipelines. Preserve historical evidence.

Use the original-compiler build, static graph/ABI/data audits and ordinary
cached native trace/status/artifact fixtures. Add only small targeted checks
for concrete changes or mismatches. No full-suite checkpoint exception.
