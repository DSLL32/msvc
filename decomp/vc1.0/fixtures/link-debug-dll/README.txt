Original-validated debug DLL fixture for LINK.EXE 1.0-nt

Sources/catalog derive verbatim from link-coverage commit 68789b5. This is the
queued debug DLL after the shared buffered library-create setup. Source C/C++
programs were compiled by untouched CL /nologo /c /O2 /G5 /Zi /Gy /MT. Compiler
products and their original CodeView source/PDB paths are frozen inputs, not
reconstruction source. Capture commands/cwd/tool hashes are in manifest.json.
The resource input is the exact resources-rich/rc/resources.res fixture.

LINK arguments are -link @debug-link.rsp with default mapped I/O, LIB pointing
to the original toolchain and CL/_CL_/LINK/_LINK_/MSC_CMD_FLAGS/LINK32_DB unset.
Two independent original captures and a third run in a different directory pass
exact streams and structural artifact comparison. Paths, padding and ordering
are never normalized. capture-validation.json records differences between runs.

Timestamp fields are located by COFF, archive, PE export/debug directory and
recursive resource-directory structures. The map permits only the eight-digit
PE timestamp and its 24-character ctime rendering to vary. Its spacing stays
exact. The raw /release PE checksum must equal the checksum calculated from its
complete actual bytes. After clearing only identified timestamps, the checksum
is recomputed from those normalized bytes; no arbitrary checksum is ignored.
Original outputs contain nonempty export/resource directories and COFF, CV and
FPO debug entries. Other embedded input timestamps and CodeView bytes remain
unchanged. The buffered lifecycle fixture references a frozen debug DLL.

Run the untouched original independently:
  python3 capture-original.py --phase link
Run the shared archive/tools fixture:
  python3 capture-original.py --fixture ../link-library-tools

The original reconstruction cases remain separate. These files are immutable
inputs/expected products; acceptance and replay evidence belongs under LINK.

Capture-tool migration note: explicit original diagnostics use only the existing
shared Wine prefix and fixed R:/S: mappings. Scratch is below
~/.ai-tmp/msvc2/vc1.0/link/shared-capture-tools, protected by a capture lease.
The frozen debug-link stdout/artifact embeds old tool paths: a shared-prefix
run cannot match that old reference. This is reported as a comparison failure;
no extra normalization or shared manifest/oracle replacement is permitted.
Current LINK-owned active references are under link/cases/shared-prefix/.
This tool is never called by a native fixture.
