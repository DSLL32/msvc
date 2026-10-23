C2 optimizer stress: x87 arithmetic and conversions

Original CL /nologo /c /O2 /G5 floating.c compiles source/floating.c with
original toolchain 1.0-nt. A loop converts float loads, performs double
arithmetic and comparisons, stores doubles, calls an external double function,
and converts a final double through signed long. No headers or reconstructed
frontend are used. Actual CL child launches, explicit environment blocks, cwd
and C1 IR are captured at original CreateProcessA thunk RVA11000 by pinned
../minimal-c/capture-cl.c.

manifest.json pins source, original binaries/error files, launches and four IR
inputs. Portable standalone original C2 exits0 with empty streams and matches
the979-byte pipeline object after only COFF timestamp4..7 normalization:
b3d040e58e811e7414c45bf1b8473c1a7a66fd79092ba599ad88abaf1bf1761e.
Copy inputs into isolated cwd, no argv, explicit MSC_CMD_FLAGS-only environment,
relative -il ir and trailing space retained. This folder is captured data only;
fixtures run native and never Wine. Explicit capture is ../../c2/scripts/capture-stress.py
with --fixture here --stem floating, only before registration. Capture measured
6.083s: helper compile1.943/link0.798, pipeline2.331, standalone0.909.
See capture-validation.json; never overwrite the registered manifest.
