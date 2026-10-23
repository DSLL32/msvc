Original-backed DEF directives

Direct LINK -lib mode reads directives.def against the unmodified original
minimal-C test.obj. HEAPSIZE131072,8192 and STACKSIZE262144,16384 become the
original hexadecimal directives. SECTIONS .text EXECUTE READ and SEGMENTS .data
READ WRITE SHARED become -SECTION:.text,ER and -SECTION:.data,RWS. These strings
are checked as part of both complete captured outputs, not synthesized or patched.
The original emits warning LNK4053 for IMPORTS and ignores the statement. This
case covers its syntax consumption, not imported-symbol generation. It does not
claim that every recovered attribute or invalid-input branch was executed.

Two independent untouched-original captures exit0 with identical255-byte stdout
and empty stderr. The full2MB .lib and .exp match after only structural archive,
COFF and export-directory timestamps. The .exp and archive retain preallocation;
no trimming, padding, ordering or path normalization is permitted. The source
object comes from fixtures/minimal-c/link/test.obj; hashes and exact environment
are in manifest.json. LINK.ERR is explicitly frozen beside the native logical
program path to preserve the unsupported-statement diagnostic.

Four readable C functions close the case: size parsing, section parsing, shared
attribute lookup and ignored IMPORTS parsing. Fresh masked-original/native-GDB
stops identify each function before replacement. The final fresh comparison
exits0 and matches both full artifacts and all streams. Native fixture runs
launch only ELF against these captured references, never Wine or PE helpers.

All24 native trace/output/artifact cases pass, with570 application first entries.
The measured loop is1.508s, trace preparation0.100s; external process wall was
not separately measured. Combined original-compiler/audits/native-trace build
is16.786s. See validation.json for component times and scratch evidence hashes.
Coverage is218415/281634=77.5528%,744 accepted frontiers. The contextual CFG
inventory separates genuine instruction bytes, inline tables and alignment.
