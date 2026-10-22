Visual C++ 1.0 for NT: LINK.EXE reconstruction

Target: toolchains/1.0-nt/bin/LINK.EXE. Build with its original compiler,
linker, headers and libraries through Wine. Program logic is readable period C;
all 70 formerly lifted implementations and their generators were removed. Only
INT3 statements in pending named stubs remain. No assembly ABI exception exists.
Data is contextual and typed; no original section snapshots are retained.

The fifteen prior cases are complete:
  /? (exit 3, exact 884-byte stdout, empty stderr)
  minimal-c, hello-win32, hello-dll
  library-create, hello-debug-dll
  library-list, library-extract, library-remove, library-list-trimmed
  library-dump-library, library-dump-object, library-dump-dll
  library-edit, library-dump-edited

The user selected both remaining-code candidates. Frozen original-backed fixtures
link-omf and link-undecorate are in commit86a7f87. Current OMF reconstruction stops
at matching omf_record_segments:545 frontiers,164825/281634 bytes=58.5245%.
All17 additions since full528 replay, all15 prior cases and400 graph/391 callback/
358 data/551 boundary audits pass. The newly assigned families remain incomplete.
cases/omf-undecorate-queue.json records order and provenance. No lifted assembly,
bulk data or ABI exception was introduced. The original compiler rejects template
syntax; the C++ fixture covers supported name kinds without a template claim.

The expanded scenarios cover buffered C++ archive creation and member lifecycle,
COMDAT selection, full COFF/CodeView/FPO debug DLL linking, native CVPACK,
resources, DEF/order parsing, map/verbose output, archive/COFF/PE dump formatting,
and editing PE stack/heap reserve/commit values. Generated console programs exit 0;
original/reconstructed/baseline GUI executables and DLLs pass load/call, 300 painted
pixels, WM_CLOSE, exit 0 and DLL unload checks where applicable. Streams and exit
status match the frozen fixtures and untouched original.

Previous completed-family verification (933267c): a fresh original-toolchain
make -j2, full 528-frontier replay,
all 15 completed-case regressions, 384 direct/indirect/tail-call graph audits,
375 callback audits,306 contextual-data initializer audits and 534 accepted-record
instruction-boundary checks pass. cases/coverage-after-library-tools.json records
the accepted .text union: 160229/281634 bytes=56.8926%, up from 110787/281634=39.3372%
at the 400-frontier baseline. This is whole-function extent coverage, including
accepted inline tables; it is not dynamic instruction or branch coverage.
Verification log hashes and case provenance are in cases/library-dumps.json and
cases/library-edit.json. SOURCE-NOTES.txt preserves implementation milestones.

Normalization remains limited to structurally proved clocks and the independently
validated PE checksum derivation where a fixture requires them. No path, padding,
output-order or arbitrary byte normalization is used. The editor's 71800-byte
edited.dll is raw-exact with no ignored fields. Its stack reserve/commit are
0x180000/0x2000; heap reserve/commit are 0x140000/0x2000. Native /release sets the
checksum field to 1 before the buffered backend emits LNK4058, reporting checksum
calculation unsupported on this OS; the actual resulting bytes and warning are
preserved. No checksum-success behavior is claimed for that path.

The debug DLL uses default mapped IO. Library setup/lifecycle uses LINK32_DB=27,
the original's buffered backend: under Wine the original mapped library creator
otherwise leaves its 2 MB allocation and subsequent listing calls it corrupt.
Inputs are not trimmed or patched. Extracted alpha is byte-exact and the trimmed
archive contains no beta member. The native DEF limitations are retained:
OrdinalEntry stays named despite NONAME, and ordinal 11 has no populated forwarder
entry. These inputs prove parsing, not effective ordinal-only/forwarder emission.

Build and check help:
    make -j2 check
Check one fixture (the harness sets its original environment):
    python3 compare.py --case hello-debug-dll --require-exit
    python3 compare.py --case library-edit --require-exit
Replay every recorded frontier and finish with edited headers:
    python3 compare.py --replay --case library-dump-edited --require-exit
Measure accepted original code extents:
    python3 measure-coverage.py

The harness masks the entire original code section and restores only accepted
function extents and encountered inline data. Unaccepted reconstruction functions
and its entry are masked too. Each pair runs concurrently and must stop at the
same logical function. Per-run Wine namespaces expose the matching masked tool
copy at its canonical original DOS path; other toolchain files resolve to
untouched originals. argv[0], recursive verbose command text and self-spawned
LINK children therefore remain exact. The debugger follows the full child tree
with per-process image bases and loader breaks. The external original CVPACK
helper is retained as a dependency. Original toolchain files are never modified.

Runtime members come from relocation-target-verified original LIBC objects.
Older differences, including text-mode ftell/fseek discovered by the editor's
external LINK.ERR catalog, are readable C in runtime.c. Catalog path/offset/line
buffers and the genuinely indexed shared code table belong to diagnostics.c;
message sentences stay in the original external catalog.

cases/remaining-gaps.json ranks analysis-only follow-ups using actual decoded
body ranges and native callers, excluding CRT aliases, padding and placeholder
extents: about 21.3 KB in OMF conversion and 19.0 KB in C++ undecoration. These are
static opportunities, not promised coverage from a single input. Those two families are now authorized and active. Remaining Macintosh/m68k, ROM, bind/rebase, disassembly,
diagnostic/error and dormant CRT paths mean 99% whole-binary coverage is not
established. Shared fixtures remain immutable. Scratch/evidence/private Wine
prefixes live under ~/.ai-tmp/vc1.0-link.
