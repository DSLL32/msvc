CVPACK 4.25 (VC1.0 / 1.0-nt) is unfinished at **187 accepted application
functions, 56,759 original code bytes, and 20 named trapping stubs**.
`binary.c` is the canonical function registry. This harness migration accepts
no new application functions and grants no CRT reconstruction credit.
The last accepted function is `type_server_close` (sequence 187,
`src/pdb-close.c`, commit `a00fd6a90`). There were no candidate bodies or
uncommitted source changes at migration start.

`src/` contains application reconstruction only. Shared fixture checks live in
`harness/`; this binary has no private native host or binary-specific CRT
implementation. Shared CRT compatibility remains in `lib/recon/`.

Run `make -j4 native cases`, `make -j4 check unit`, and `make -j4 win32 audit`
from this directory. Shared `recon-tool.mk` builds `.build/libbinary.so` once,
automatically discovers the eleven numbered `case.c` descriptors, and runs their
cached trace/status/stream/artifact checks in independent native workspaces.
Fixtures never invoke Wine or recapture. `win32` explicitly uses the original
compiler through the shared Wine prefix. `audit` compares full original/compiler
application CFGs, RET cleanup, extent ownership and instruction overlap;
`check-context.py` additionally checks 206 initial data objects, diagnostic
strings, typed callback identities and source/stub inventory. Target-specific
switch/SEH/compiler annotations live in `policy-data.json`.

All eleven dispatched cases have complete original-backed expectations and explicit
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

`cases/0009-buffered-strip-cache` now has a fresh shared-prefix masked-original
completion at the 187-App baseline: exit 0, 34 first entries, exact stripped
image and extracted debug artifact hashes, matched natively. This qualifies the
previously queued invocation without accepting an application function.
The superseded
unaligned field-list draft serialized nondeterministic original padding and
remains excluded; its raw input/oracle/reason stay in Git and the snapshot.

`runtime.json` is the unchanged frozen original library/import identity index.
`runtime-policy.json` qualifies the original `__sopen`, `__freebuf` and `_setvbuf`
variants as CRT identity boundaries with exact extents, hashes and provenance.
The original mask leaves these CRT bodies executable while unfinished App code
stays trapped. Their known differences from the shipped library remain recorded;
this grants no App credit or CRT body-graph equivalence. The old case-specific
mode/input/phase guards remain historical facts, not a requirement for resuming
capture. The native source still traps on
unsupported all-pinned/empty LRU selection and NULL PDB-close arguments; no
equivalence is claimed there.

The complete pre-migration tree, including ignored builds and diagnostics, plus
scoped Git diff/status, is preserved at
`/root/.ai-tmp/msvc2/vc1.0/cvpack/standardization/pre-migration-20261025T085002Z/`.
Historical acceptance/proof packets remain in Git through `faceaf25b`.
Migration command logs and authoring data are under that snapshot's parent.
Do not rerun the historical proof/negative campaigns; the local `AGENTS.md`
retains the explicit user override.
