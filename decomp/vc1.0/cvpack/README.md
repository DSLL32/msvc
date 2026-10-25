CVPACK 4.25 (VC1.0 / 1.0-nt) is unfinished at **187 accepted application
functions, 56,759 original code bytes, and 20 named trapping stubs**.
`binary.c` is the canonical function registry. This harness migration accepts
no new application functions and grants no CRT reconstruction credit.
The last accepted function is `type_server_close` (sequence 187,
`src/pdb-close.c`, commit `a00fd6a90`). There were no candidate bodies or
uncommitted source changes at migration start.

Run `make -j4 native cases`, `make -j4 check unit`, and `make -j4 win32 audit`
from this directory. Shared `recon-tool.mk` builds `.build/libbinary.so` once,
automatically discovers the ten numbered `case.c` descriptors, and runs their
cached trace/status/stream/artifact checks in independent native workspaces.
Fixtures never invoke Wine or recapture. `win32` explicitly uses the original
compiler through the shared Wine prefix. `audit` compares full original/compiler
application CFGs, RET cleanup, extent ownership and instruction overlap;
`check-context.py` additionally checks 206 initial data objects, diagnostic
strings, typed callback identities and source/stub inventory. Target-specific
switch/SEH/compiler annotations live in `policy-data.json`.

All ten dispatched cases have complete original-backed expectations and explicit
unseeded first-entry order. The PDB cases materialize owned PDB bytes under
private native Z: namespaces while preserving the original embedded Windows
filenames. Existing references retain their historical Z: program path; they
were packed without relabeling or rerunning the original. Future path-dependent
captures must use the fixed shared drives and fresh original evidence.

The original `build_source_file_index` left trailing filename alignment bytes
uninitialized. At the user's direction, this reconstruction now writes zeroes
to that padding in both native and Win32 builds, with the original defect
documented at the write. This explicit behavior correction replaces the old
allocator launch qualifications. Comparisons mask only the two established
filename padding tails: hello `[0x110ae,0x110b0)` and embedded C++
`[0xab91,0xab94)`. Case callbacks require actual zero padding. The hello callback
also independently validates the raw PE checksum before its derived field
`[0xd8,0xdc)` is excluded from the comparison hash; the mapped case retains its
original zero checksum. All other bytes remain exact. Raw original oracles are
preserved in the snapshot and Git; no allocator tunables or libc profiles remain.

`cases/0009-buffered-strip-cache/capture.bin` preserves the queued invocation,
input and untouched original final oracle. It deliberately has no descriptor:
the prior worker never established a masked/native frontier and explicitly
excluded it from fixtures. To resume reconstruction, inspect its saved invocation
with `recon-tool capture unpack`, establish a genuine masked/native frontier,
then author the descriptor and reconstruct functions under the normal policy.
Do not treat the saved final oracle as current native completion. The superseded
unaligned field-list draft serialized nondeterministic original padding and
remains excluded; its raw input/oracle/reason stay in Git and the snapshot.

`runtime.json` is the unchanged frozen original library/import identity index.
`runtime-policy.json` preserves identity-only anchors and the narrowly qualified
case/phase/input/mode/share CRT boundaries. They grant neither unconditional
restoration nor application credit. Shared original progress capture does not yet
implement those conditional guards; resume that facility before using it on
these cases. The native source still traps on unsupported all-pinned/empty LRU
selection and NULL PDB-close arguments; no equivalence is claimed there.

The complete pre-migration tree, including ignored builds and diagnostics, plus
scoped Git diff/status, is preserved at
`/root/.ai-tmp/msvc2/vc1.0/cvpack/standardization/pre-migration-20261025T085002Z/`.
Historical acceptance/proof packets remain in Git through `faceaf25b`.
Migration command logs and authoring data are under that snapshot's parent.
Do not rerun the historical proof/negative campaigns; the local `AGENTS.md`
retains the explicit user override.
