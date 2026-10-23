C2 optimizer stress: register pressure

Original CL /nologo /c /O2 /G5 pressure.c compiles source/pressure.c with
original toolchain1.0-nt. Twelve loaded integer values remain live across two
opaque external calls, then feed weighted arithmetic and two branch results.
No reconstructed frontend or source headers are involved. Actual CL child
launches, explicit environment blocks, working directories and C1 intermediates
are observed at the original CreateProcessA thunk RVA11000, using the pinned
original-toolchain-built ../minimal-c/capture-cl.c debugger.

manifest.json pins source, original CL/C1/C2/error dependencies, actual launches
and four IR files. The standalone backend receives no arguments and an explicit
MSC_CMD_FLAGS-only environment, with portable -il ir and trailing space retained.
Its879-byte output independently matches the original pipeline after only COFF
timestamp4..7 normalization. Expected normalized SHA256:
692d3dbb7de8c5e16843d6f9c658bbcc9e1753cc111e281cc4c0b3ffa016a6e3.

Data/metadata only: native fixtures never invoke Wine. Explicit capture is in
../../c2/scripts/capture-stress.py; run with --fixture pointing here and --stem
pressure only before registration. Do not overwrite an existing manifest.
Measured capture helper compilation.873s, link.534s, pipeline1.513s and independent
standalone1.027s, total4.047s. See capture-validation.json.
