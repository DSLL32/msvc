Fixed-drive original CL reference captures

All 57 records were explicitly recaptured with the shared win32 Wine prefix and
its immutable R: repository and S: scratch drives. Original case work is
S:\vc1.0\cl\<case>; sibling .capture outputs and per-case locks isolate work.
TMP/TEMP are relative ".". No private prefix clone or custom drive exists.

Each original run captures app entries with the same accepted prefix excluded,
actual _spawnvpe invocation/environment, complete before/after input/output trees,
child status and raw child streams. Whole-toolchain hashes pin transitive SDK and
library inputs. Original helper images are captured and source-pinned; native
fixtures do not require Win32 helper build outputs. A failed child capture reads
actual errno from the original. The debugger validates original instruction bytes
and restores original instructions/EIP after each one-shot application entry.

The native application entry cache pins each new original proof. Fixtures verify
invocation, exact mapped cwd and every input byte before replaying captured file
effects/status/streams. Broker cwd is host transport; captured DOS cwd must equal
the fixed S: name and map to that exact host directory. Unknown dependencies fail.
The missing-child aggregate is checked as two absent native PATH probes, without
executing a child. Native fixtures execute no Wine or PE program.

blobs/<sha256>.gz contains captured bytes; decompression checks size and SHA256.
Only identified COFF/PE/export/archive/map clock fields are normalized for final
products. No path normalization or old-capture relabeling is used. Old transport
bundles and unreferenced blobs were removed; independent semantic analysis remains.

Explicit original-only operations, outside fixture runs:

    make -j4 build/capture-run.exe
    python3 scripts/capture-native.py --case compile --force
    python3 scripts/native-traces.py --write

Normal recapture checks existing streams and normalized artifacts. An explicitly
staged path migration may use --migrate-paths --output-store <S:path>; registering
it requires independent original entry/exit/state proofs before replacing data.
See ../path-migration.json and ../native-all-validation.json for changes and checks.
