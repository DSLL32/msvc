automatic-pch C1 case
=====================

The /YX producer, reuser and changed-macro rebuild all complete with raw exact
status, streams, four intermediate files and PCH contents. The producer writes
38021 bytes; reuse retains those bytes; changing FLAVOR rebuilds38022 bytes.
Selecting a dependent native case runs its producer first and checks the actual
generated PCH against the consumer input before launch. Header mtimes are frozen.

manifest.json records exact arguments/environment, source/header/PCH dependencies,
complete output sets, actual fd1/fd2 alias and ordered acceptance ranges.
validation.json records fresh masked-original/native frontier comparisons,
original first-entry plans, canonical compiler audits, all18 native regressions,
rejection checks, coverage and measured time including repeated captures.
Evidence is under evidence/. Full untouched-original captures were repeated
before reconstruction; expected bytes are never silently recaptured by tests.

Original reference operations use the shared repository Wine prefix with fixed
R: repository and S: scratch paths. Native fixtures never invoke Wine. The
stat modification-time field is at+28 in the36-byte legacy CRT layout; see
lib/recon/stat-validation.json. Replaying a candidate header keeps its typed
state separate from live compiler PCH fields until dependency replay succeeds.
There is no normalization of PCH paths, pointers or payload data.
