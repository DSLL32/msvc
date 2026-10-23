C2 optimizer stress: nested loops

Original CL /nologo /c /O2 /G5 loops.c compiles source/loops.c using toolchain
1.0-nt. No reconstructed frontend is involved and the source includes no headers.
CL's CreateProcessA thunk at RVA11000 is observed by the original-toolchain-built
../minimal-c/capture-cl.c debugger. capture/ retains actual application, command,
cwd and explicit environment blocks for C1/C2. manifest.json pins source, original
executables/error files, launch metadata and actual four intermediate inputs.

The backend portable replay copies c2/ir{ex,sy,in,gl} to an isolated directory,
passes no arguments and an explicit MSC_CMD_FLAGS-only environment. Substitute
{bin}, retain relative -il ir and the trailing space. Original standalone C2
independently exits0 with empty streams and matches the620-byte pipeline object
except the four-byte COFF timestamp at offset4. Expected normalized SHA256:
cafbb33726695b60d2e92d4173682de0f4ae47c401ecfcbe3b8ce38a54568f87.

This directory contains captured data and metadata only. Native fixture runs
never invoke Wine. Explicit reference capture lives in ../../c2/scripts/:
capture-stress.py captures an unregistered approved source with original CL/C1/C2;
capture-original.py captures the currently masked original backend separately.
Do not overwrite an existing fixture manifest. Capture timing is recorded in
capture-validation.json; pipeline/helper phase splits were unmeasured for this
first case, while the independent standalone phase was measured.
