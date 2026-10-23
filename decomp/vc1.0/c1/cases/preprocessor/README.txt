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

These three phases follow the active browser case. Normal fixture execution
will remain native-only, using these immutable references without recapture.
