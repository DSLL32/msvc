Browser definitions and references
=================================

Untouched CL accepts /nologo /c /FR test.c. Its captured C1 child has no
command-line arguments and a single MSC_CMD_FLAGS environment entry. The
manifest retains that original entry, portable toolchain substitutions and
relative temporary prefix ir. Inputs include the local header and source;
C1.ERR is the only external file dependency. C2 capture is reference only.

Two independent original C1 runs at N:\browser reproduce all four raw IR
files and the236-byte test.sbr exactly, without normalization. Each isolated
original/native run maps its own directory to the same canonical N: namespace.
The SBR header retains N:\browser verbatim; paths are never rewritten in output.

The current reconstruction is an intermediate checkpoint. Ordered fresh masked
original/native observations and pending FILE buffers are in evidence/, with
acceptance order and the next named trap in manifest.json. validation.json
records the checked extent, original-toolchain audits and old-case regressions.
The normal native suite will register this case when its full output passes.
