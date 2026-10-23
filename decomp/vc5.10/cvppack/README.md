# CVPACK 5.00.7113

The target is `toolchains/5.10/bin/CVPACK.EXE`; the requested project-folder
spelling `cvppack` is retained. The first `/help` case is complete:
native and masked-original runs produce exit 1, exactly 193 stdout bytes and
empty stderr, with the same six application entries. The second, genuine
embedded-debug GUI DLL case is now authorized and in progress; its current
matching frontier is `initialize_module_type_storage`. Fifty application functions are accepted
overall, covering 13,832 extent bytes (22.07% of the same whole-text denominator).
The type-table initialization and complete PE/COFF debug-reader body have
also been recovered; the PE and CodeView directory reads, complete subsection sort and owned
segment-map/library loads now match. Module-node creation, scratch setup and the per-module dispatcher are
also reconstructed, including the first owned module payload load. The cache prefix
now includes the actual mapped-file registry and strict
file-handle identity, input-content, access-mode, size and position checks.

The completed help case accounts for six functions and 1,269 extent bytes, plus
48 bytes of parser switch data. This is 2.03% of the executable's 62,662-byte
`.text` extent, which includes CRT, unknown code and padding; it is not a
non-CRT coverage percentage. Three reached CRT routines and two import thunks
are tracked separately. CRT ranges are identified against shipped MSVCRT.LIB,
including relocated call/import/global bindings, rather than blanket unmasking.

Run from this folder:

```
make -j4 native check final-check
make -j4 audit-native
```

These commands use Linux i386 code and cached original evidence only. `check`
compares the native application trace, status and complete streams. `final-check`
also checks the last immutable candidate trial against a fresh native capture,
the current Rust graph audit and 24 rejection tests. `audit-native` requires the
previously built Win32 EXE/MAP and never launches an original. `make -j4 all`
explicitly checks compilation with the original 5.10 CL/RC/LINK through the shared
Wine prefix. Native code uses `-O0 -g`, shared shims and entry instrumentation.

Each accepted function has immutable original/native raw evidence under
`cases/{help,hello-debug-dll}/evidence/`, in acceptance order. Candidate-inclusive graph audits,
actual candidate entry, typed parameters/globals and real trap/completion events
are required before promotion. Captures verify the actual loaded MSVCRT file
against the canonical 277,776-byte provider hash. Compiled-source contents are
retained in `cases/source-content/`, independently of later shared-library edits.
Unchanged original trials can be reused for native-only rebuilding and static
reauditing; references are never silently recaptured by fixtures.

`cases/help/completion.json` records the result, extent qualification and final
validation paths. The retained final native fixture took 12.31 ms internally: 2.60 ms
preparation, 4.85 ms execution and 1.71 ms checks. The last function's explicit
original build took 3.46 s, native build 1.42 s and original capture 1.40 s;
the native capture command took 0.108 s. Full final native pair/audit/rejection
commands took approximately 1.39 s, including 0.97 s for the 24 negative checks.

The three encountered language-1033 resource strings are verified against target
UTF-16 bytes and the contextual `src/cvpack.rc`, then prepared using shared
resource support. No runtime PE helper is involved. Distinct original `argv[0]`
and loaded-module paths are preserved. USER32's executed LoadStringA boundary is
supported; unencountered resources fail closed.

This version imports eleven MSPDB50 PDB/type APIs. `/help` does not reach them,
and no speculative PDB semantics have been implemented. The new case was compiled and linked by the actual 5.10 tools with embedded
CodeView and no PDB file, then successfully packed repeatedly to an identical NB11
master. Its encountered opaque WidenTi interface is documented under
`cases/hello-debug-dll/DEPENDENCY.md`; its strict typed response provider has passed concrete contract and genuine
i386 `thiscall` ABI review. It checks the complete frozen DLL and provider
identity, all 21 object lifetimes and full initialized inputs, preserves each
native allocation’s own untouched tail, and rejects unknown calls. The actual
MSPDB implementation remains outside reconstructed application coverage.

Native-only boundary checks are `scripts/check-widen-contract.py`,
`scripts/check-widen-provider.py`, `scripts/check-state-graphs.py` and
`scripts/check-file-state.py` and `scripts/check-mapping-state.py`. The latest function’s explicit original build took
2.35 s, native build 1.66 s, original capture 1.88 s, native capture 0.29 s and
pair check 0.18 s; both native regressions together took 0.45 s. The independent
C++ provider test runs 24 success/rejection executions in 66 ms after a 1.56 s
build. No native test launches Wine.

The current mapped-view state includes the entire 2 MB region and its actual
VirtualQuery/protection metadata, backing-file identity, offset and mapping/view
ownership. Fourteen semantic mutations fail. Original 33,280 input bytes plus
2,063,872 zero-extension bytes match the live native view completely. All eleven
initialized type-table allocations and the nested empty type record are compared.

Twenty-three completed reads now compare their actual caller/local or owned-heap
identity, capacity, count, logical positions, LastError and initialized output
bytes. Seventeen read mutations and eight initialized-storage mutations fail.
Uninitialized allocation tails are excluded from initialized input claims; the
70-entry directory compares its full initialized 840 bytes and separately proves
the actual 912-byte allocation. Original and native sorting results match exactly.

The audit runner and acceptance checker honor an explicit `GRAPH_AUDIT_BIN`
for an approved immutable auditor. Function 34 additionally has a static sidecar
revalidation with the published pre-edit auditor hash, retaining its unchanged
original/native raw captures. No reference was rerun for that tool-version check.

The current prefix verifies 99 object identities, 23 completed reads and seven
actual allocator requests. Six work buffers have checked capacities and aliases;
their uninitialized bytes are not treated as defined inputs. Twenty further
module/allocation mutation controls fail, including payload contents, capacities,
module/directory/global aliases and resident-pointer relocation. The partial-case native fixture takes
about 0.70 s including GDB capture; the direct instrumented execution is about
7 ms. Shared-source updates are handled with native-only recapture and a fresh
static sidecar audit against the unchanged original trial.

The rebuilt per-module dispatcher owns exactly 39 bytes of compiler switch data.
Its six pointer relocations and 15-byte class map are checked against the original
and linked output before passing their exact intervals to the shared Rust audit.
Instruction, gap and terminal checks remain enabled.

The first public-symbol reader and the complete symbol-widening dispatcher now
match. This first subsection has signature 2 and therefore bypasses WidenTi;
its next boundary checks the full live 116-byte symbol buffer, directory-element
identity and bytes. The earlier empty pack-symbol parameter schema was tightened
and explicitly recaptured before proceeding; both historical observations remain.

The legacy type compactor, primitive conversion, queued recursive-type cleanup
and public-symbol insertion are reconstructed. The next callback boundary checks
the caller-owned record, four distinct output locals and their widths, the two
initialized zero inputs and the bucket divisor. Twelve ABI mutation controls pass.
Primitive conversion has a separate 517-input native check against the original
semantic tables, taking 1.4 ms after a 215 ms standalone build.

At the 50-function review checkpoint, 13,832 application extent bytes and 353
associated switch-data bytes are reconstructed. These are accepted extents, not
executed-branch coverage. Four CRT bodies (400 bytes) and four import thunks
(24 bytes) are separate support. The current boundary precedes type-table
initialization; the complete streamed-type reader is reconstructed but has not
yet proceeded beyond that child boundary. No WidenTi factory or method has been
executed by this reconstructed prefix.

Checkpoint validation freshly ran eleven semantic-control suites: 131 distinct
rejections, 517 primitive inputs and the 21-lifetime provider positive test.
The parallel batch took 2.81 s; adding an exact public-padding mutation required
a supplemental 0.94 s public-state run. The historical help completion also
passed 24 protocol rejection checks (1.11 s), native capture (0.11 s), audit
(0.25 s) and pair comparison (0.12 s). The help fixture took 13.6 ms and the
partial debug fixture about 0.72 s, including GDB. No original was rerun for
these checkpoint regressions. Hashes, exact reports and fresh/reused accounting
are in `cases/hello-debug-dll/checkpoint50-controls.json`.

Historical source-status auditing distinguishes old masked stubs from bodies
accepted later: those later bodies remain excluded from the historical mask and
checked-function set. Current genuine stubs must still contain INT3. The new
status note identifies the later acceptance proofs explicitly.

Qualified import auditing now includes cached EBX/ESI/EDI/EBP calls. The 50-function
review correction preserves the direct imported `_iob` array and binds fourteen
narrow ordinary-return ABI contracts to actual provider/SDK/code witnesses.
All fourteen contracts and twenty individual facts are necessary; main also has
an exact five-free-call inventory so aggregate graph equality cannot hide unknown
cached calls behind its genuine checksum callback. Original observations remain
immutable; current native/static sidecars bind both generations and regenerate
current COFF/import metadata. See `cases/hello-debug-dll/REVIEW-50.md` and
`import-audit-review/0003-qualified-stable/review.json` beneath that case.

The first module type-storage initializer is accepted (51 application functions,
13,893 extent bytes: 22.17% of the qualified whole-text denominator). Fresh raw
checks all 384,000 initialized table bytes, 336 record entries and their payload
offsets, fifteen cleared allocations, and the complete 12,324-byte type read in
an owned 12,332-byte segment. Its eight uninitialized tail bytes remain native
owned: changing them is an explicit allowed control. Thirteen focused mutations
and the affected seventeen read/eight initialized-storage controls pass. The
next frontier is the original 0x40c760 routine. This first-module table contract
fails closed on a reused table until its new lifetime/state is observed.
See `cases/hello-debug-dll/evidence/0051-initialize-module-type-storage/function-review.json`
for exact fresh/reused provenance and split timings.

Routine 0x40c760 is now `reset_type_name_cache`: it frees counted names and
clears record/index associations while retaining each bucket chain. Its former
index-builder name was an analysis hypothesis; historical raw remains unchanged.
The genuine first-module case currently traverses an empty cache. Its complete
original/rebuilt import graph still requires the actual EBP-cached free call;
a new narrowly scoped provider/SDK witness and single-fact removal control
prove that requirement. There are now 52 accepted application functions and
13,958 extent bytes (22.28% with the same qualification); the next frontier is
`read_module_lines`. No reconstructed MSPDB call has been traversed.
