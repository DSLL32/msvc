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

Native receipts under ~/.ai-tmp/msvc2/vc1.0/cl/native-check contain per-case setup,
execution, oracle checks and trace checks. Build and explicit capture timings
are recorded separately. No automatic reference refresh exists. Fixed drives R: (repository) and S:
(shared scratch) are never remapped. Original case work is S:\vc1.0\cl\<case>;
sibling .capture directories and per-case locks isolate concurrent work.
TMP/TEMP are relative ".". Native runs map their own isolated S: namespace and
validate its canonical cwd; the broker separately checks the exact host cwd.
The shared Wine prefix is root-managed; workers never stop its server.

Original-only reference operations remain explicit, outside fixture execution:

    make -j4 reference-build audit
    make -j4 build/capture-run.exe
    python3 scripts/capture-native.py --case compile
    make probe

Capture requires an explicit case selection and refuses existing captures unless
--force is supplied. It runs only the original CL and original children, checks
the current original stream/status and normalized artifact oracles, and records inputs/outputs. The
original toolchain build uses the shared ~/.ai-tmp/vc1.0-capture/wine prefix. Static
The shared tooling/graph-audit Rust CFG engine audits all 110 source functions
with --check-ret --check-ownership --check-extents. It compares call/tail edges,
RET cleanup, code-range ownership and whole function boundaries. This replaces
the former Python graph walker and includes the prior return-cleanup evidence.
No legacy discovery mode or duplicate Python graph/ABI check remains.

Make builds the host tool once through the shared quiet Cargo helper, then runs
the binary directly with its stdout/stderr preserved. Native fixture targets
never invoke Cargo or Wine. Existing canonical files can be audited without any
compiler execution by marking their metadata prerequisites as existing:

    make -o build/runtime.json -o runtime.json audit

GRAPH_AUDIT_INPUTS accepts the tool's --rebuilt/--map/--rebuilt-runtime overrides
for a coherent saved canonical snapshot. Runtime identification remains separate:
scripts/identify-runtime.py checks exact LIBC/SETARGV sections and their relocation
destinations, distinguishing library aliases. The Rust graph engine consumes
these identities; it does not identify CRT archive code. Python PE/index helpers
also remain available for independent analysis and shared-tool mutation tests.
The native first-entry filter/object-definition audit, process-boundary input
checks and fixture negatives cover different contracts and are retained.

src/cl.c contains main, src/driver.c argument/option processing and invocation,
src/remaining.c diagnostics, response files, IDE callbacks, timer and OMF writer,
src/data.c encountered option grammar/profiles, and src/cl.h the original layouts.
src/runtime.c preserves the six reached CRT differences: setvbuf,_freebuf,_sopen,
_filbuf,__tzset,_isindst. Remaining CRT behavior is supplied by shared native shims
in Linux and the original LIBC.LIB/SETARGV.OBJ in the canonical Win32 build.

cases/remaining/semantic-coverage.json and coverage.json retain historical
semantic evidence without retired transport data. Current transport oracles come
only from fresh fixed-drive original captures. The former paired-Wine fixture runners have been removed.

Shared graph-tool integration evidence is in cases/graph-audit-validation.json.
Seven alternating samples measured the former Python linear graph audit at 0.216s,
Rust CFG graph-only at 0.044s, and the enabled CFG+RET/ownership/extents at 0.092s.
These are CLI/input/output timings, excluding builds and fixtures; the enabled
pass is stronger than the previous audit. Six scratch mutation checks reject
wrong/missing callees, missing public mapping, a split RET operand, changed RET
cleanup and overlapping original ownership. The final three were accepted by the
former Python graph-only pass. All 57 native traced/output cases and the six
existing native rejection checks still pass. No check was dropped, and no references were recaptured.
