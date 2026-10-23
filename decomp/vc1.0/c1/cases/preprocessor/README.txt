Preprocessor output modes
=========================

Untouched CL independently accepts /nologo /E test.c, /nologo /EP test.c and
/nologo /P test.c. Each captured C1 child has no command-line arguments and
only MSC_CMD_FLAGS in its environment. Portable flags, all local header/source
inputs, C1.ERR dependency and exact stdout/files are in manifest.json.

Independent original-only runs reproduce every captured byte: /E writes381
stdout bytes, /EP writes292, /P writes381 bytes in test.i and no stdout. All
exit0 with empty stderr and no IR files. There is no normalization. /EP omits
generated file/line markers but preserves the source's explicit #line directive.

All three phases now pass at776 accepted entries, including665 reconstructed
source functions. The normal native suite registers preprocessor-E/EP/P and
all thirteen cases pass traces, status, streams and complete artifact sets.
Eight meaningful oracle mutations are rejected. Original/default/alias/ABI and
both complete grammar audits pass; see validation.json and evidence/.

The /P CRT seam is additionally observed in native GDB: _dup(1) returns3 with
identical descriptor flags, freopen("test.i","wt+",stdout) retains its FILE
identity and descriptor1, then the final381-byte text file matches. The original
internal GetCurrentProcess/DuplicateHandle/SetStdHandle entries are captured
in order and excluded from application traces. No shared shim was added for
these internal calls. Normal fixture execution is native-only and never
recaptures references. The assigned queue is complete and paused.
