# VC5.10 CVPACK

Target: `toolchains/5.10/bin/CVPACK.EXE`; the requested folder spelling is
`cvppack`. Reconstructed source is readable Win32 C with a minimal genuine C++ DLL interface, compiled with the provided
5.10 toolchain and also as native i386 Linux code through `lib/recon`.

`/help` is complete: status 1, 193 stdout bytes, no stderr, six application
entries. The genuine embedded-CodeView `hello-debug-dll` case is in progress at
the arena-fill child (`fill_type_arena`),
after all 21 Widen lifetimes. There are 108 accepted application functions /
36,069 extent bytes, plus 784 associated switch-data
bytes. This is 57.56% of the 62,662-byte whole-text denominator, including CRT/unknown/padding;
it is neither non-CRT nor executed-branch coverage. Four CRT bodies (400 bytes)
and five import thunks (30 bytes) remain separate support.

## Build and validation

```
make -j4 all native
make -j4 check audit-native
```

`all` uses the shared repository Wine prefix for the original compiler only.
Fixtures, GDB, dependency responses and regression checks run natively. Never
rebuild concurrently with an immutable pair check: build once, freeze its input
content and PE/MAP/ELF hashes, then run validators. Shared-source changes may
require native-only rebuild/capture. Reuse original raw only when its target,
mask, effective candidate, configuration and execution inputs are unchanged;
strict current sidecars bind both generations and regenerate current metadata.

The source retains standard `main`; its documented native target-TU alias
`cvpack_main` separates it from the host driver. `src/crt-imports.h` preserves the
original direct `_iob` data import instead of adding the SDK accessor.

The shared Rust audit checks candidate-inclusive CFG/RET, qualified imports,
function references, ownership and complete extents. Encountered switch tables
have exact COFF/relocation/linked-byte proofs. Narrow provider/SDK preservation
witnesses resolve cached EBX/ESI/EDI/EBP imports. Main additionally requires all
five original cached free calls and only the genuine checksum callback as
unresolved. Actual GDB DWARF formals independently check argument count/names,
integer/pointer kind, width and known pointee size before typed state comparison.

## Cases and evidence

Case definitions and immutable original inputs/masters live under `cases/`.
`hello-debug-dll` was prepared by the actual 5.10 compiler/linker with `/Z7`,
`/DEBUG /DEBUGTYPE:CV /PDB:NONE /NOPACK /INCREMENTAL:NO`. Its NB05 input and NB11
master are specific to this toolchain; no VC1 output is reused.

The current prefix compares 336 type records and all 384,000 initialized table
bytes, buffer/record aliases, actual reads and allocations, complete mapped-file
contents, defined symbol/source-module buffers, and 87 owned counted type-name
cache nodes with their bucket/segment aliases, plus initialized arena prefixes,
global pointer slots and direct/pool interned-node ownership. Lookup outputs use actual DWORD caller storage; the recursive-reference helper shares its saved DWORD/WORD local through an explicit union. Earlier byte-schema scaffolds were rejected before the callee executed. Local/global symbol hash heads
retain their actual shared public-arena nodes, full initialized records and owned padding where defined
and separate byte/count counters. Three new reference nodes bind module/source offsets and initialized14B payloads while retaining each native allocation's own two unwritten alignment bytes. The source-line flag is one original WORD shared by pack_module and main; its old statistics_enabled label is metadata-only. The default dispatch row and
next global index share their original storage, with actual DWARF offset proof. Uninitialized allocation tails
remain native owned; explicit positive controls permit changing them. Extended reference arrays retain their actual cleared allocation identities and
full initialized capacities, including zero spare bytes. Recursive hash nodes
retain their actual 12-byte pool ownership and per-bucket counts. Arena-record
arguments bind initialized bytes to local records and global index slots. The first
module's type-table contract fails closed on reuse until that lifetime is observed.

Known original callee extents may supply tail RET closure while their source
remains INT3, masked, unaccepted and excluded from coverage. The allocator and
both compactors use one original `discard_types` flag, with a nonzero native
identity control.

The pointer compactor preserves an original unwritten-offset path for unsupported
embedded symbol kinds. Native execution traps before that unvalidated read; a
focused native control verifies it. Recognized offsets and current pointers run
normally. This limitation has no unsupported-path equivalence claim.

Module source-line copies bind the complete original256B input to one allocation shared by its directory and ModuleInfo. Rewrite allocation provenance retains actual call-time operands across module-global resets and input-buffer reuse.

The reviewed MSPDB WidenTi case provider is outside application coverage and has
now supplied all 21 complete factory/query/emit/release lifetimes. It enforces 21 actual lifetimes,
complete inputs/aliases/sequences/capacities and actual thiscall interfaces,
writing only observed prefixes and preserving each native allocation's own tail.
The exact8B Release tail uses a reviewed local slot0/RET0 proof alongside full graph checks and common strict return checks on every other function. The unreached Record slot fails closed. See `cases/hello-debug-dll/DEPENDENCY.md`.

Main cleanup records initialized inputs before each real CRT free, then retains
only ended lifetimes and pointer identities. Current-state observations exclude
freed payloads; a finite no-consumer continuation is checked before the next stub.
Scratch cleanup uses the meaningful source name `free_widen_buffers`; the historical logical ID `write_debug_directory` remains explicit in evidence. Seven real frees cover 21,934 initialized bytes, including maximum written prefixes from the same scratch allocation generation; later untouched capacity remains native owned. Type-table cleanup adds five ended allocations: 39,854 byte-compared inputs plus 416,768 typed pre-free table bytes, 456,622 initialized bytes total. Released roots stay identity-only in current state; live child nodes are still distinct allocations. See `evidence/0099-finalize-derived-types/table-lifetime-review.json` and its independent parent review.

`cases/hello-debug-dll/REVIEW-50.md` records the independent source/graph/ABI review.
Each accepted function's `evidence/*/function-review.json` or `validation.json`
records fresh/reused controls and separate build/capture/check/fixture times.
Historical raw is never relabeled; incomplete and superseded trials retain their
status. Guarded `TypesClose`/`PDBClose` imports have actual named-export/caller ABI witnesses; native host adapters deny125 until those paths are observed. No PDB behavior is credited. The source-file index builder now uses its meaningful source name `build_source_file_index`; historical `print_statistics` is metadata-only. Its first live module header has an explicit post-free allocation generation, separately observed even when its native address reuses an old table. Old roots remain historical and generic retired-range reads still fail. Current native help runs in roughly 16 ms; the partial debug fixture is
about 7.66 s including all 375 sort and 58 copy calls in GDB. Completion will use native entry traces, exact streams,
status and the cached NB11 master without launching Wine.

The packed-directory sort now matches all 375 canonical callbacks, complete inputs/results/ABI, errors and all 840 initialized directory bytes. Its x87 comparison permits each side's own unused payload only for this integer-only path with control `027f`, status zero and all eight tags empty throughout; each full own raw baseline remains strictly preserved. This does not establish a live-FP or general CRT/CL rule.

The output initializer now establishes a separate 65,512-byte allocation generation and compares its eight initialized header bytes. Original allocation reuses a retired type-table address; that historical table remains identity-only and its payload is never restored as current memory. The remaining 65,504 buffer bytes are own unwritten storage. Later stale-root consumers remain held.

All 58 subsection calls now copy 5,002 initialized source bytes and 46 alignment bytes into the new buffer, producing its 5,056-byte initialized prefix. Sorted input pointers remain a historical snapshot; all 58 current rows carry checked relative offsets. The 144-byte initialized directory tail is retained, with 72 spare bytes unobserved.

The public-symbol writer now reaches its real page-padding child with the existing bucket table and reserve four. The actual 16-byte header and four-byte padding local are zero; both 24-byte hash streams remain own uninitialized storage. Its 920-byte whole-body graph is checked, while dynamic proof stops at that first helper. The source-snapshot drift attempt is retained separately from the reviewed native-only refresh.

The public page-padding helper preserves all 64 existing initialized nodes and adds one 24-byte end marker from the same arena. Its actual eight-byte contribution updates the public stream length to 2,116. All six name-hash arguments now have checked callback/local-field/stream/bucket identities. This input never crosses an internal page boundary; that future path stays held for a new case contract.

The name-hash builder now derives six buckets and two overflow nodes from all 64 public records. Five actual new allocation generations retain separate ownership even when either heap reuses retired addresses; complete initialized bucket/overflow capacities are checked. Canonical arena initialization and fastcall copy helpers are reused with fresh contextual ABI evidence. The next fill call receives the actual names arena, zero and size two. Only its first two payload bytes are initialized; the remaining 4,094 bytes and second 24-byte stream remain unclaimed. Later serialization and temporary frees need the next candidate’s fresh proof.
