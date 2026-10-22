Original-only CL child-boundary reference captures

These 57 records are new explicit capture evidence for existing completed cases;
they do not replace or refresh the historical case/status/stream/artifact oracles.
The native runner checks retained final products against those frozen originals.

capture-native.py executes only the pinned original CL and children, under a
private Wine prefix outside fixture runs. src/capture-run.c breaks original
__spawnvpe entry/return at its verified code address, records actual argv/env/status,
full before/after case trees, and child slices in separately captured output files.
CL is synchronously blocked during each child, so stream offsets delimit child
output. The capture helper sets Win32 TMP/TEMP explicitly to each Q: case directory;
all generated temporary IR/response files therefore belong to the captured tree.
A separate child-not-found capture reads original errno at 0x2847c on return (2).

The whole immutable original toolchain is pinned by toolchain.json, covering
transitive SDK/library/tool dependencies. Full captured original helper images are
content-addressed blobs with their helper source hashes; native fixtures need no
Win32 build outputs. Each boundary verifies exact invocation, logical environment,
cwd and complete input inventory before applying response file effects/status/raw
streams. Unknown invocations or changed inputs fail. No PE is executed natively.

One original _spawnvpe aggregate ENOENT capture becomes two native _spawnve broker
probes (bare filename and the pinned PATH candidate). Both candidates must remain
absent, arguments/environment/input must match, and the original response must
have no streams or file effects. The native CRT performs its normal PATH search.

blobs/<sha256>.gz contains compressed captured bytes with deterministic gzip mtime.
Before use, decompression validates exact length and SHA256. Only identified
COFF/PE/export/archive/map clock fields are normalized in final artifact checks;
per-child input/output bytes and invocation spelling otherwise match exactly.

Reference capture is explicit and refuses existing records without --force:

    make -j4 build/capture-run.exe
    python3 scripts/capture-native.py --case compile

Native checks never import or run this capture script. The old paired-Wine fixture
runners have been removed. See ../native-all-validation.json for successful native
coverage, split timings, negative checks and precise remaining platform limits.
