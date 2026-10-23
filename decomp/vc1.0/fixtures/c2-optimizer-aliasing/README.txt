C2 optimizer stress: pointer aliasing and structure copies

Original CL /nologo /c /O2 /G5 aliasing.c compiles source/aliasing.c with
original toolchain1.0-nt. A24-byte structure copy surrounds interleaved integer
pointer loads/stores; a loop aliases indexed integer and byte accesses. No source
headers or reconstructed frontend are used. Actual CL child launches, explicit
environment blocks, working directories and C1 IR are observed at the original
CreateProcessA thunk RVA11000 by pinned ../minimal-c/capture-cl.c.

manifest.json pins source, original executables/error files, launches and four
IR inputs. Portable standalone original C2 independently exits0 with empty
streams and matches the600-byte pipeline object after only timestamp4..7:
44bf756a158394dab84fbb60cb4ad9e686cf34ced9377cd666f1307a03f744ae.
Copy inputs into an isolated cwd, no argv, explicit MSC_CMD_FLAGS-only env,
relative -il ir and trailing space retained. Data/metadata only; fixtures run
native and never Wine. Explicit original capture is ../../c2/scripts/capture-stress.py
with --fixture here and --stem aliasing, only before registration.
Capture6.237s: helpercompile1.798/link.813,pipeline2.317,standalone1.194.
See capture-validation.json; do not overwrite the registered manifest.
