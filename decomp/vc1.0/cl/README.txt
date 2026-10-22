Visual C++ 1.0 for NT: CL.EXE reconstruction

The completed driver implements all 104 application functions in the original
[00017010,0001b081) region, plus six differing CRT routines. Historical paired
acceptance recorded 233 frontiers across 57 cases. Unvisited CRT is out of scope.
functions.json retains full original extents and individual ABI identities.

Sources live in src/, scripts in scripts/, and original case evidence in cases/.
Native i386 Linux regressions (32-bit libc development files, Clang, Python 3
and GDB required):

    make -j4 native native-trace
    make -j4 check
    make replay
    make native-reject-check
    python3 scripts/check-native.py --trace --case compile

The native compiler uses -O0 -g. Application source objects are instrumented only
in native-trace; CRT overrides and shared compatibility/tracing sources are not.
The shared lib/recon C sources are compiled directly at final link, without an
archive. recon.h is last in source/header include lists and empty on Win32.

check/replay never invoke Wine or execute a PE child. The application entry cache
projects the 233 original accepted frontiers onto 104 application functions,
explicitly excluding CRT/imports on both sides. Every native application definition
must map to an original identity. Each case seeds the preceding accepted prefix
and compares its complete first-entry segment, status, raw streams and artifacts.
Cases with no new application entries still check their complete outputs.

CL's compiler/linker children are explicit original-backed process boundaries.
cases/native-boundaries stores captured invocation/environment, complete case
input/output trees, child status/streams, and content-addressed compressed files.
The complete toolchain inventory pins transitive SDK headers/libraries and tool
images. Before any response, the broker checks exact argv/program/environment,
cwd and every input file; unknown or changed dependencies fail. The missing-child case checks each absent
PATH candidate against the original aggregate _spawnvpe failure; it never executes
a missing dependency. It then applies
captured file changes and streams synchronously. No compiler/linker implementation
is substituted into CL, and no PE is launched by a fixture.

/Bt cases retain the original controlled local calendar sequence; internal OMF
and IDE cases retain explicit, documented GDB state injection. Interrupt adapters
exercise the registered handler with native signaling in the corresponding idle
and child-running state. These do not claim Windows console event delivery.
The original SEH exception handler remains outside the native platform scope.
Generated Windows GUI/console products are compared as bytes, not executed.
Only identified COFF/PE/export/archive/map timestamp fields are normalized.

Native receipts under ~/.ai-tmp/vc1.0-cl/native-check contain per-case setup,
execution, oracle checks and trace checks. Build and explicit capture timings
are recorded separately. No automatic reference refresh exists.

Original-only reference operations remain explicit, outside fixture execution:

    make -j4 reference-build audit
    make -j4 build/capture-run.exe
    python3 scripts/capture-native.py --case compile
    make probe

Capture requires an explicit case selection and refuses existing captures unless
--force is supplied. It runs only the original CL and original children, checks
the frozen historical stream/status oracles, and records inputs/outputs. The
original toolchain build uses the private ~/.ai-tmp/vc1.0-cl/wine prefix. Static
Win32 call-target auditing checks all 110 reconstructed source functions.

src/cl.c contains main, src/driver.c argument/option processing and invocation,
src/remaining.c diagnostics, response files, IDE callbacks, timer and OMF writer,
src/data.c encountered option grammar/profiles, and src/cl.h the original layouts.
src/runtime.c preserves the six reached CRT differences: setvbuf,_freebuf,_sopen,
_filbuf,__tzset,_isindst. Remaining CRT behavior is supplied by shared native shims
in Linux and the original LIBC.LIB/SETARGV.OBJ in the canonical Win32 build.

The historical cases/remaining/validation.json and coverage.json are retained as
original evidence. The former paired-Wine fixture runners have been removed.
