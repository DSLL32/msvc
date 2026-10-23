automatic-pch C1 case
=====================

Automatic-PCH producer/reuser/changed-macro reconstruction is active.
All other cases are complete. manifest.json records source/header dependencies,
exact arguments/environment, status, streams and complete artifact oracles.
Fresh original first-entry captures use the shared repository Wine prefix,
with fixed R: repository and S: scratch paths. Native fixtures never run Wine.
The accepted prefix and expected entry sequence are checked independently on
the original; current native traces/status/streams/artifacts match. See
../shared-prefix-validation.json and ../shared-prefix-evidence/.

Header modification time is preserved as input metadata. The original
pch_file_mtime reads st_mtime at+28 of the36-byte CRT status record; shared
layout/field provenance is recorded in lib/recon/stat-validation.json. The
current named frontiers are finish_pch_capture, compare_work_stack and
pch_replay_mismatch for producer/reuser/changed phases respectively.
