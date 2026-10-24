# recon-tool

Shared native i386 case runner. Build from this directory with `cargo build -q`;
`.cargo/config.toml` defaults to `i686-unknown-linux-gnu`. Run the resulting binary
directly, rather than `cargo run` for each case. Requires the Rust i686 standard
library and working `gcc -m32`/glibc development files. C++ applications also need
a matching 32-bit C++ toolchain.

```sh
target/i686-unknown-linux-gnu/debug/recon-tool run ../../decomp/PROJECT/BINARY/cases/0000-example
target/i686-unknown-linux-gnu/debug/recon-tool test ../../decomp/PROJECT/BINARY/cases/0000-example
target/i686-unknown-linux-gnu/debug/recon-tool io pack ~/.ai-tmp/case.json /path/to/case/io.bin
target/i686-unknown-linux-gnu/debug/recon-tool io unpack /path/to/case/io.bin
```

`run`/`test` accept multiple case directories or `case.so` paths. `--io` overrides
the inferred fixture for a single case. `--report` writes timing/results outside
the ephemeral workspace; keep authoring JSON and reports outside case folders.
`inspect CASE.so` prints descriptors. `registry-export CASE --image-base 65536
--policy policy-data.json --output .build/functions.json` projects the binary's
single registry for the existing graph auditor. Policy data must not duplicate
`target`, `accepted`, or `driver`. Original-compiler builds and audits remain
separate. Native execution never invokes Wine.

Include `include/recon-tool.h` for the versioned C ABI. `binary.mk` compiles the
reconstruction, `binary.c`, and uninstrumented shared support once into PIC
`libbinary.so`. `case.mk` links only case glue/tests against it. Root builds should
finish `recon-native` before parallel case submakes, passing
`RECON_SKIP_BINARY_BUILD=1`; standalone case builds default to building the common
library incrementally. Optional application-interface dependency headers should
be normal make prerequisites. The common rules generate dependency files.

The executor loads the library, applies fixture environment and cwd, and runs
optional `setup` once. Setup must remain single-threaded and create no children.
Application first-entry tracing begins only for the IO/direct execution, after
setup. A case's explicit `first_entry_seen` RVAs are omitted afresh on each run;
they preserve a captured suffix, not evidence of a complete historical trace.
The current terminal is exit or a named unimplemented App frontier. Implementation
status is bookkeeping, not a claim that the tool has accepted a function.

`test` starts a separate execution. Each forked test starts from the initialized
setup checkpoint and gets a copy of the setup directory. Unforked tests must be
pure. A fork does not isolate inherited open-file offsets or external resources;
tests needing those must arrange them explicitly. Test calls do not enter the
case trace. The supervisor reaps all descendants, including orphaned grandchildren,
before checking outputs and deleting its owned workspace. Timeouts also kill
descendants that moved to another process group. SIGKILL of the supervisor itself
cannot guarantee cleanup.

## io.bin version 1

Eight-byte `RECONIO\0`, little-endian u32 version, little-endian u64 payload
length, then uncompressed CBOR. No trailing bytes are accepted. The payload's
JSON authoring form uses `text:...` or `hex:...` for byte strings; CBOR stores raw
bytes. Unpack uses hex, preserving arbitrary bytes exactly. Missing/null and empty
byte strings/files remain distinct.

```json
{
  "args": ["example.exe", "input.c"],
  "stdin": "text:",
  "stdout": "text:hello\n",
  "stderr": "hex:",
  "exit_code": 0,
  "input_files": [{"path":"input.c", "data":"text:int main(void){return 0;}\n"}],
  "output_files": [{"path":"output.obj", "sha256":"...64 hexadecimal digits..."}],
  "env": {"EXAMPLE":"value", "UNSET_ME":null},
  "reject_unexpected_files": true,
  "timeout_ms": 30000
}
```

Optional input metadata: `mode` (Unix permission bits),
`modified_unix_seconds`. Output entries may assert `absent:true`, exact `data`,
`sha256`, or explicitly identified nondeterministic `zero_ranges:[[start,end]]`.
Ranges are half-open and zeroed before byte/hash comparison. Unexpected-file
checking allows input files and explicitly listed present outputs. Symlinks and
special output files are rejected. Relative fixture paths cannot contain `..`.

`logical_cwd` may be a stable Windows drive-absolute path. `logical_drive` chooses
its private drive root (for example S); T always maps to this run's private root.
The physical cwd uses the canonical path suffix. Explicit `readonly_dependencies`
contain host absolute `path` and `sha256`, verified before and after execution;
these are inputs, not undisclosed implicit references. `output_env` checks values
inside an adapter that returns; a target calling `exit` cannot supply this
observation automatically.

Events are ordered. Each has optional `after_ms`, optional `stdout_contains`
(both conditions must hold if present), and an `action` object:
`{"kind":"stdin","data":"text:..."}`, `{"kind":"close_stdin"}`, or
`{"kind":"signal","signal":2}`. Close waits for queued bytes to be written.
Signals go to the executor process group. These are Linux signals, not yet a
claim of arbitrary Win32 console-event equivalence.

Top-level expected outputs describe successful completion. Optional `partial`
has its own stdout/stderr/output_files/output_env/reject_unexpected_files and
applies at an expected current frontier. Final expectations are retained intact.

Run `python3 tests/smoke.py` for the focused native integration tests. Explicit
original Wine capture and debugger transport are the next implementation stage;
they are not provided by this native checkpoint.
