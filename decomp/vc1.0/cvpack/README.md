CVPACK 4.25 (VC1.0 / 1.0-nt) is unfinished at **228 accepted application
functions, 70,042 original code bytes, and 40 named trapping stubs**.
`binary.c` is the canonical function registry. Reconstruction has resumed after
harness migration; CRT identities grant no application reconstruction credit.
The legacy workload exposed the omitted old-directory comparator and corrected
`read_old_directory`'s previously wrong newer-callback reference. The matching
frontier is now `convert_old_symbol_label16` (RVA `0x140f0`), on the C6 local-symbol
sizing path. The last accepted function is `size_old_block16`
(sequence 228, 28 original code bytes, `src/oldsymbols.c`). Encountered converter dispatch and its calls have identified
additional App helpers; these retain named trapping stubs and no accepted
credit. The reached line path also corrected `read_lines` to publish `line_directory`
(original VA `0x17f30`), rather than the old source-module directory.
The resumed workload has accepted 41 application functions.
Each has matching original/native progress and strict compiler/graph/data
checks. All twelve cached cases and unit executors passed the 223-function
checkpoint. The previous baseline ended at `type_server_close` (sequence 187,
commit `a00fd6a90`). There were no candidate bodies or
uncommitted source changes at migration start.

`src/` contains application reconstruction only. Shared fixture checks live in
`harness/`; this binary has no private native host or binary-specific CRT
implementation. Shared CRT compatibility remains in `lib/recon/`.

Run `make -j4 native cases`, `make -j4 check unit`, and `make -j4 win32 audit`
from this directory. Shared `recon-tool.mk` builds `.build/libbinary.so` once,
automatically discovers the twelve numbered `case.c` descriptors, and runs their
cached trace/status/stream/artifact checks in independent native workspaces.
Fixtures never invoke Wine or recapture. `win32` explicitly uses the original
compiler through the shared Wine prefix. `audit` compares full original/compiler
application CFGs, RET cleanup, extent ownership and instruction overlap;
`check-context.py` additionally checks 216 initial data objects, diagnostic
strings, typed callback identities and source/stub inventory. Target-specific
switch/SEH/compiler annotations live in `policy-data.json`.

Eleven dispatched cases complete; the derived NB02 case has an original-backed
partial frontier. All twelve retain explicit
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

`runtime.json` preserves the frozen original library/import identity index except
for the reviewed false short-signature match at RVAs `0x13260` and `0x14a50`. Their generic RET
plus fifteen alignment bytes did not prove either library alias; the genuine
application sizing table identifies zero-argument callbacks using globals.
`size_old_register_relative32` and `convert_old_symbol_register_relative32`
remain pending named App traps. Each has one RET byte of actual code; alignment
is not application code credit. Other short matches
are unchanged, and the raw former rows remain in Git and the owned snapshot.
The newly encountered 14-row C6 symbol sizing table identifies eighteen application
callbacks. Each body remains pending until its matching frontier and checks.
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

`0012-derived-nb02-old-types` is an authored input, not a historical producer
output. It preserves the authentic NB02 `cfig386.dbg` payloads (98 modules,
symbols, types and lines), widening its 212 directory entries from `<HHIH>` to
the target-supported `<HHII>` and updating the trailer container length.
Payload offsets and bytes, the count, directory offset and NB02 signature are
unchanged. Input SHA256 changes from
`2e01fe1d2d150f1e20352225636081e0373cae3d79c8b601d8482a4cedc4ddf4` to
`801316eb6012ec914c4dc8a2eb29eae2416fbdf17ddd9a8aa75e18f7ba6a5dcb`.
The untouched original accepts it and emits a packed debug container; the
full final oracle remains in the partial capture. Exact derivation and validation
are saved in `~/.ai-tmp/msvc2/vc1.0/cvpack/resume-20261025/0012-derived-nb02-old-types/input-provenance.json`.

The recovered old-module converter also left its counted filename's final byte
and alignment tail unwritten. Its reconstruction explicitly zeros that proven
uninitialized tail and documents the original defect. The current partial
frontier precedes output emission; any emitted tail must be masked when the
legacy workload reaches a completed artifact comparison.

The old basic-pointer converter left reserved attribute bit `0x0800`
uninitialized. Its reconstruction zeros this bit and documents the original
defect. It also initializes the unknown-encoding mode and procedure-convention
fallback bytes to zero where the original left them unwritten. The current
valid-input frontier passes with these corrections. No broad artifact masks or
derived type-order changes have been assumed equivalent; actual output
consequences must be diagnosed at the completed legacy path.
