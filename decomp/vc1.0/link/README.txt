Visual C++ 1.0 for NT: LINK.EXE reconstruction

Target: toolchains/1.0-nt/bin/LINK.EXE. Reconstruction source is readable,
period Win32 C preserving individual graph vertices and calling conventions.
Pending functions are named INT3 stubs. There are no assembly ABI exceptions,
lifted implementation blocks or copied original data sections.

Current checkpoint: 801 accepted frontiers, 229573/281634 accepted .text bytes
(81.5147%). This union includes contextual inline data and alignment; it is not
branch coverage. The partial checked inventory distinguishes166234 accepted
application instruction bytes,27047 CRT instructions,198 import-thunk bytes,
1784 inline-data bytes,2670 alignment bytes and31640 accepted unclassified bytes.
Unclassified bytes are not all padding. cases/omf-lidata-exestr/components.json
records the partial accounting; it is not a full application-body denominator.

The39 completed native cases cover help; minimal console/GUI/DLL linking;
archive creation/list/extract/remove; COFF/archive/PE/FPO/symbol-map dumps; PE
editing; ordinary OMF conversion/linking; old C++ undecoration; x86 disassembly;
Macintosh linking/relocations/debug/swapper/static/resource/pcode paths; DEF
handlers; and absolute/weak externals. Exact phase arguments and environment,
immutable inputs, streams, outputs and clock rules are in cases/shared-prefix/.
Only these fresh shared-prefix references are active. Earlier accepted-body,
ABI/state, CFG and rejected semantic-defect evidence remains historical analysis.
Both selected Win16-resource and ordinary OMF LIDATA/EXESTR families are complete.
Root approved the next single combined ROM/local-import/duplicate-warning/MAP/
LINK_REPRO case for original validation. No reachability is inferred from percentage.

Native build, fixtures and GDB

Run from this binary root:
    make -j4 native native-trace
    make -j4 check
    make -j4 replay
    python3 scripts/check-native.py --trace --case m68k-debug-swapper
    python3 scripts/check-native-boundary.py
    python3 scripts/check-native-rejections.py
    gdb --args build/native/LINK.EXE /?

check/replay execute the instrumented i386 ELF against all39 cached originals.
They never invoke Wine, original executables, PE helpers or automatic capture.
Clang flags are -m32 -std=gnu89 -fms-extensions -fasm-blocks -O0 -g.
The same src/ reconstruction files build both ELF and Win32; shared lib/recon C
sources compile directly at final link, without a static archive. Only app
objects are instrumented. src/runtime.c, CRT/import/shim functions are excluded.
recon.h is last in each source/header include list and is empty for Win32.

scripts/native-traces.py validates every native application definition, including
unreconstructed stubs, against functions.json. It projects the original accepted
history, excludes CRT/imports, seeds each case's accepted application prefix and
binds each sequence to fresh original capture receipts/events. All39 cases check
625 new application entries; zero-new-entry cases reject unexpected first entries.
Synchronous native LINK selfchildren share seen-state and retain original argv.
Missing/stale references, undefined paths and unexpected child invocations fail.

cases/reference-cases.json selects LINK-owned active references. Cases use fixed
canonical S:\vc1.0\link\<case> cwd and R:\toolchains\1.0-nt\bin\LINK.EXE identity.
The native fixture exposes the repository as virtual R: and its private temporary
case namespace as virtual S:. These are native filesystem translations, not Wine
drive changes. Input and reference hashes are checked before execution. Streams
and status are exact. Only structurally proved timestamp fields are ignored;
where required the PE checksum is validated and derived from normalized clocks.
No path, padding, ordering or arbitrary-byte normalization is permitted.
Native receipts split setup, execution, output checks and trace comparison.

DLL children execute the actual native ELF. The external CVPACK response is a
strict captured boundary: invocation/environment and the entire normalized
prepack input must match before the frozen packed response is applied. Nonempty
captured child streams are rejected. See cases/native-boundaries/README.txt.
GUI execution, DLL load/unload, SEH fault delivery and arbitrary process trees
are outside this ELF fixture suite. Actual GUI/console behavior was checked in
the original capture provenance; generated PE programs are never run by fixtures.

Explicit original-only compilation and captures

These commands use Wine outside fixture execution:
    make -j4 all audit
    python3 scripts/measure-coverage.py
    python3 scripts/measure-components.py
    make -j4 capture-helper
    python3 scripts/capture-shared.py --case help --output-root <new-reference-folder>

The original compiler/linker build remains the default target. Static checks
use the shared Rust CFG engine for652 source/runtime graphs, including10
evidenced runtime vertices, with return-cleanup, function-reference, application-
ownership and whole-extent checks. All807original/804rebuilt accepted extent
records (unique802/801) receive boundary checks. Retained Python checks cover
642callback sets,454contextual data definitions and41ECX-member return cleanups.
The quiet incremental Cargo helper builds once; audit invokes the executable
directly, preserving its JSON output/errors. No cargo run or Wine is introduced
by this host tool. See cases/graph-audit-validation.json and the explicit
Python baseline target audit-graph-python. Toolchain files remain
untouched. scripts/capture-shared.py is an explicit original-only analysis tool,
not a fixture dependency. src/capture-original.c is its debugger helper and is
not linked into the reconstruction. Capture plans mask the entire original
.text, restore accepted extents/contextual code data, and trap application first
entries while excluding CRT/imports. Masks are written only into stopped original
process memory. The original disk EXE and actual R: _pgmptr remain unchanged.
The debugger follows LINK selfchildren and snapshots the CVPACK boundary.

All original tools use the existing shared ~/.ai-tmp/vc1.0-capture/wine prefix:
R: is /root/msvc2; S: is ~/.ai-tmp/msvc2. These mappings are asserted, never
created/remapped by LINK. Scratch reservations are S:\vc1.0\link\<case> and
S:\vc1.0\link\capture.lock. The shared server is never stopped. LINK's obsolete
private prefix/server and custom drive configuration were retired after ownership
and idle-service inventory. Raw earlier analysis was not relabeled.

The shared link-debug-dll/link-omf/link-undecorate capture tools are explicit
original-only diagnostics. OMF and undecoration retain exact shared frozen
outputs after migration. The old shared debug-DLL reference embeds old tool paths
in verbose stdout/debug content; shared-prefix invocation cannot match it. Its
shared manifest/oracles remain untouched. LINK's freshly captured active debug
DLL reference replaces only LINK's own former transport/oracle. CVPACK owns
independent byte-identical copies of its earlier input/output, so its current
case has no dependency on LINK's active boundary directory.

Migration results and reconstruction limits

cases/shared-prefix-validation.json records all35 original captures and repeat
runs, native checks/negatives, static build/audits, timings, changed path-dependent
artifacts, shared capture-tool limitations and exact private-prefix retirement.
Mac debug embeds its new absolute S: output path byte-for-byte. Full-debug DLL
stdout and DLL bytes also change with real paths; its packed output is71756bytes
rather than the earlier71800. These are fresh original references, never patched
or additionally normalized. Old own output caches/transport manifests were removed.

Target defects remain explicit: /absolutesfrom may write an External union before
removing an undefined node; a forced unresolved DIR32 case leaks a process pointer
into its output. Rejected original/native artifacts and state remain under
cases/absolute-weak/capture/. No pointer normalization or false acceptance was
introduced. The accepted /include no-relocation companion preserves the original
LNK4016 warning. The56-byte External layout separates archive/Mac/fallback fields;
all prior Mac artifacts remain regressions. Redirected stderr buffering is modeled
in shared CRT from stopped-original FILE state, without partial-stream exclusions.

The approved OMF and large-symbol/wildcard families are recorded in
cases/omf-symbols-wildcards-queue.json. OMF is complete (ten functions/1936extent
bytes,1864instructions and72alignment); its source/alias/provenance/validation
are in cases/omf-records. The second family uses4096ordinary external definitions
compiled by original CL and a relative unit?.obj operand. No further families
are authorized. Known-body estimates are opportunities, not branch-coverage or
99% promises. SOURCE-NOTES.txt retains historical checkpoints; current metadata
supersedes them.
