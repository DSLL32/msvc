FPO and symbol-map image output

Input debug.dll is the exact trusted original pre-CVPACK image already frozen in
cases/native-boundaries/hello-debug-dll/before.dll. Two untouched-original-only
-dump /fpo /map /out:dump.txt captures agree:56 FPO records and17790 output bytes,
exact streams/status, no normalization. The three whole C bodies are dump_fpo,
locate_fpo_data and dump_symbol_map. Fresh masked-original/native GDB entry and
terminal evidence is under capture/frontiers; partial dump bytes also matched.

FPO_DATA is the original WINNT.H16-byte record, supplied by the shared native
header. Record locals remain DWORD units and parameters are multiplied4 only for
display. Image symbol lookup deliberately scans every18-byte record, while the
symbol-map walker skips auxiliaries and retains adjacent-record size estimates.
The original locator's uninitialized local for zero-entry directory is preserved;
that invalid/empty branch is not dynamically claimed. Both image and ordinary
object .debug$F routes are covered by this and dump-fpo-object.

The initial stderr mismatch is preserved under capture/stderr-buffering. Native
stderr was forcibly unbuffered while original redirected FILE flags2 became0xa
with24 pending LF bytes/count4072/4096 buffer. Explicit original debug state and
original CRT getbuf/stbuf evidence drove shared fix17b8e62. All subsequent partial
and complete streams match, with no exclusion. Captured helper source is analysis
only, never compiled/launched by a fixture.

validation.json records the native case; cases/absolute-weak/family-validation.json
records all35 regressions, audits and actual timing samples. The accepted extent
union is separate from the checked original CFG instruction/data/alignment union.
