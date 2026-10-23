browser C1 case
===============

This case is complete. manifest.json records source/header dependencies,
exact arguments/environment, status, streams and complete artifact oracles.
Fresh original first-entry captures use the shared repository Wine prefix,
with fixed R: repository and S: scratch paths. Native fixtures never run Wine.
The accepted prefix and expected entry sequence are checked independently on
the original; current native traces/status/streams/artifacts match. See
../shared-prefix-validation.json and ../shared-prefix-evidence/.
