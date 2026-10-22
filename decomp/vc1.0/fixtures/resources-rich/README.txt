VC1.0 resource compiler and preprocessor starting case
====================================================

This case was compiled and independently replayed using the original 1.0-nt
RC.EXE and RCPP.EXE. Both phases return 0 with empty stdout and stderr. The
compiled resources.res has 3,684 bytes and 19 nonempty Win32 resource records.
The captured RCPP output has 336,686 bytes. All comparisons are exact bytes:
there are no ignored fields, path replacements or timestamp exclusions.

Run from any directory:

    python3 /root/msvc2/decomp/vc1.0/fixtures/resources-rich/capture-original.py

Use rc or rcpp as the positional argument to verify one phase. --prefix selects
a private Wine prefix; the default is ~/.ai-tmp/vc1.0-resource-capture/wine.
The verifier maps T: to toolchains/1.0-nt so header names in #line directives
remain stable when the repository moves. Working files and verification.json
are outside the repository under ~/.ai-tmp/vc1.0-resource-capture.

The original top-level command is:

    RC.EXE /dCASE_FROM_COMMAND=7 /Iinclude /foresources.res resources.rc

manifest.json records original binary/header hashes, source hashes, exact
streams, resource record hashes and the captured child command. RC launches
RCPP with real argv and an inherited environment; this differs from the
MSC_CMD_FLAGS transport used by CL's compiler passes. No MSC_CMD_FLAGS is set.
The launch passes a null current-directory parameter, inheriting the case cwd.

RCPP's input is the original RC-prepared temporary text, not the unmodified
resources.rc. Its captured input/output basenames (RC000360 and RD000360) are
retained because the input name appears in the first emitted #line directive.
Both files are under rcpp/. Headers and all asset files retain their relative
layout under source/. The verifier replays the captured child independently.
RC's case invokes the original RCPP child normally; reconstructing RCPP is a
separate task. RC does not import RCDLL.DLL on this case.

Native reconstruction checks:

    make -C /root/msvc2/decomp/vc1.0/rc -j4 check
    make -C /root/msvc2/decomp/vc1.0/rcpp -j4 check

Normal fixtures run native i386 reconstructions against cached original
references. They do not invoke Wine or refresh the original captures. The
capture-original.py command above is an explicit original-only verification
operation, outside normal regression execution.

composed-validation.json retains historical evidence from the former Win32
composed RC/RCPP experiment. Its Wine runner has been removed.

Features exercised
------------------

Preprocessing:
  windows.h and its transitive includes (32 toolchain headers total); quoted and angle
  includes; nested user headers; repeated includes and guards; object-like and
  function-like macros; argument expansion, stringizing and token pasting;
  command-line definition; #undef; #ifdef/#ifndef; #if/#elif/#else/#endif;
  defined(), arithmetic, shifts, bitwise and logical expressions; continuation
  lines and comments. Inactive branches contain a missing include and #error
  to verify that those directives are not acted upon.

Resources:
  Nested menus with separators, checked/disabled items and tab text;
  one numeric dialog with font/menu/style and standard/generic controls;
  one named dialog; English and French string tables across multiple ID blocks;
  string escape sequences and doubled quote syntax; ASCII/virtual-key
  accelerators with modifiers; a two-image icon and generated group;
  a bitmap; a cursor and generated group; inline RCDATA with word/dword/ANSI/
  wide data; numeric and named custom types read from a binary payload;
  DLGINCLUDE; a message table; nested VERSIONINFO string/translation blocks;
  LANGUAGE and resource memory flags.

The original 3.10 compiler preserves surrounding quotes in the recorded named
resource identifiers. That behavior is retained in the oracle. Its STRINGTABLE
syntax accepts doubled quotes, but not adjacent C string literals. Its RCDATA
parser requires an inline BEGIN block, so external binary input is exercised
with user type 256 and named CUSTOMTYPE instead. These are observed target
behaviors, not modern-resource-compiler assumptions.

make-assets.py constructs the small icon, cursor, bitmap, message table and
payload from documented structures/explicit fixture content. It copies no data
from an executable. Regenerating these files is deterministic. The font used
by the dialog is referenced by name; no font binary is embedded.

This is a broad starting case, not a claim of complete function or branch
coverage. Font compilation, alternate encodings, error paths, other switches,
and any additional legacy formats need separate measured cases as the
reconstruction proceeds. MENUEX/DIALOGEX and other later syntax are not assumed
to exist in this version.

Capture evidence
----------------

capture-rc.c is an analysis-only debugger helper, built with the original
toolchain. It breaks at RC's CreateProcessA import thunk (RVA 0xebc0), records
the application/command/cwd/environment mode, and snapshots RC's temporary
files at DeleteFileA (RVA 0x12680) before deletion. It never modifies the
executable on disk. It only records MSC_CMD_FLAGS if explicitly supplied;
unrelated environment values are never dumped. The capture stdout and metadata
are retained under ~/.ai-tmp/vc1.0-resource-capture/final-capture*.

Both target binaries have early PE section headers with VirtualSize=0.
Reconstruction tooling must handle this explicitly, use the backed raw size
for code coverage/masking, and treat uninitialized data sections as zero-filled
rather than reading their non-backed file offset as original bytes.
