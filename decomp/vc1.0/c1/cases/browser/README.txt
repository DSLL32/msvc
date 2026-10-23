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

The reconstruction completes with657 total accepted source functions and763
accepted entries. Ordered masked original/native evidence proves all browser
application frontiers and final raw files/streams. Cleanup additionally proves
native remove("test.sbt") returns0 and deletes the481-byte first-pass file.
The original __unlink/remove/DeleteFileA chain is retained; shared CRT internal
imports are excluded from application traces. The normal native suite now
includes this case and all ten cases pass. See validation.json for timings.
