CVPACK 4.25 (VC1.0 / 1.0-nt) is unfinished at **262 accepted application
functions, 74,815 original code bytes, and 13 named trapping stubs**.
`binary.c` is the canonical function registry. Reconstruction has resumed after
harness migration; CRT identities grant no application reconstruction credit.
The legacy workload exposed the omitted old-directory comparator and corrected
`read_old_directory`'s previously wrong newer-callback reference. The legacy case now completes with 141 matching first entries, exit 0 and matching
streams and defined artifact bytes. The last accepted function is
`queue_type_match` (sequence 262, 69 original code bytes, `src/types.c`).
Encountered converter dispatch and calls identified additional App helpers;
remaining helpers retain named trapping stubs and no accepted credit.
The reached line path also corrected `read_lines` to publish `line_directory`
(original VA `0x17f30`), rather than the old source-module directory.
The resumed workload has accepted 75 application functions.
Each has matching original/native progress and strict compiler/graph/data
checks. All seventeen cached IO cases and seventeen unit executors, the original
compiler build and strict graph/ABI/data audits passed the262-function gate
in3.578s. Fifteen cases complete;0013 and parked0015 retain explicit frontiers. The previous baseline ended at `type_server_close` (sequence 187,
commit `a00fd6a90`). There were no candidate bodies or
uncommitted source changes at migration start.

There is no candidate body at this checkpoint. `0013-mixed-c6-wide` is a
format-authored NB02 workload with two modules, twelve C6 types per module,
valid 16/32-bit local symbols, multiple files and contributions. Its exact
1,127-byte input SHA256 is
`1477fef370c99e7e5b51291f4aa90f99f5c3b1afb9394cbc9f4d5733cc4cb137`;
`scripts/make-mixed-c6-input.py OUTPUT_DIRECTORY` reproduces and validates it.
This is explicitly authored data, not historical producer output. The original
accepted it without warnings (exit 0, 142 first entries); its raw 1,796-byte
output SHA256 is `c7ae04c29ec0b09bc33c246d62edbf3cca5b0c98977d58e559698bddec36fb1e`.
That case remains partial at `convert_old_barray` (`0x122e0`): the original
reads an uninitialized stack word to choose storage. This is an unresolved
application control defect, not output padding. A candidate preserving the
Win32 read and explicitly trapping native execution is saved in owned scratch;
no body is accepted and no control variable is normalized. One bounded natural
original observation measured `0x0000` in module1 (resize) and `0x007e` in
module2 (reuse), unchanged between entry and the actual test. The original
then exited0. The value is not consistent, so no hardcoded substitute is
justified; original raw captures and the two-visit diagnostic remain in scratch. The mistakenly
zeroed candidate was withdrawn in `37f62be32`; its raw history remains intact.
`0014-mixed-c6-no-barray` is a separate authored variant excluding that type,
with eleven types per module and adjusted references. Reproduce it with
`scripts/make-mixed-c6-input.py OUTPUT_DIRECTORY --without-barray`.
Its 1,079-byte input SHA256 is
`337f36d97009e2cf90d12bb0a1bdb3685f3ed646944d78cdd9c9bfc98c84d237`;
the original accepted it without warnings (exit 0, 141 first entries), producing
1,744 bytes with raw SHA256
`4693013813fc9f19c1bb7fd8f07eb3672a01aeccc59d17827f0d1746837fde57`.
The versioned workload now completes with all 141 unseeded first entries,
exit 0, matching streams and exact defined output bytes. Its two unwritten
module-filename padding bytes (`0x33` and `0x5f`) are excluded from the comparison
hash, and its callback requires actual native zeroes. Both original bytes were
already zero, so the raw and canonical output hashes coincide. `0015-based-symbol-families` independently exercises C6 based pointers whose
embedded symbol offsets identify actual bprel/data/register records in the same
module's loaded local stream. Its two modules cover narrow and wide families,
with self/type/segment forms and base kinds93/94/97/98; BARRAY is excluded.
`scripts/make-based-symbol-input.py OUTPUT_DIRECTORY` reproduces the authored
1,320-byte input SHA256 `4fb473791a75b1c5aeed6ab6cd4084f8bad4f98a7deaff250c8ff9c821716e52`.
The original accepts it without warnings (exit0,118 entries), raw2,432-byte
output SHA256 `17696a0ae7fd41cc7e5fb764b6988898d6118622b9c1678ff1157c355936d77e`.
This hand-authored workload is parked unfinished at
`convert_old_based_data16` (`0x13920`), with 76 matching unseeded first entries.
At the user's direction, new work uses ordinary compiler/linker-produced
artifacts rather than additional constructed debug records. The replacement `0016-superpad-embedded` uses the unchanged shipped MFC 2.0
SUPERPAD text-editor sample from `toolchains/1.0-nt/mfc/SAMPLES/SUPERPAD`.
Original NMAKE/CL/RC/LINK build its supplied project with normal debug/PCH
options, `/Z7` embedded types (also the option in supplied `SAMPLE_.MAK`),
explicit MFC include/library paths and LINK `/nopack` to separate the CVPACK
stage. No source, debug-record or binary-format edits are made. The genuine
4,194,304-byte input SHA256 is
`6c5e2e9f12cd30c408a729a1166e027585dd4df3be7005ad66acfe1954a3fc09`.
Normal invocation is `CVPACK.EXE /nologo /cvold /verbose SUPERPAD.EXE`.
The full original exits0 with127 first entries and raw2,558,036-byte output
SHA256 `47105870ef1ae343bacd5465b9bb72822cd7815d276eddd8dfc161d2f5feffd8`.
The masked original and native now both complete with127 unseeded entries,
exit0 and matching streams/artifacts. `queue_type_match` is accepted. The
existing recursive comparator now preserves the original deterministic defect:
a nonempty1..3-byte interval rejects because zero-count REPE CMPSD retains
the pointer-addition flags. This fixes the defined type/symbol mismatch; no
type order or defined-content comparison is relaxed. Six genuine deferred
queue nodes retain their next pointers across the original free-then-read
ordering in this native run. The original leaves S_PROCREF/S_DATAREF output
alignment WORDs unwritten; reconstructed records now explicitly write zero.
This case masks exactly3,205 such structural tails (6,410 bytes) and requires
native zeroes. All other bytes match canonical full-file SHA256
`ce16118bbb1831d75257ccdc96a334ddf851efdc53e8b613dca0d086cf35cd3f`;
the raw original and its hash above remain preserved in scratch. Comparison
metadata adds35,257 serialized bytes without a duplicate output asset. Producer
setup/build took18.316s; final masked-original capture1052.664ms and explicitly
report-bound native replay107.177ms (setup8.677/execution68.555/checks29.945). Actual expanded recursion-list allocations
are now reached; the existing original storage/growth/free logic is unchanged,
with historical native coverage-only rejections lifted and bounds retained.
`0017-superpad-pdb` uses the supplied `/Zi` debug configuration, also built
unchanged. Its ordinary LINK automatically invokes CVPACK (producing NB09);
a normal `/nopack` relink separates the CVPACK invocation and preserves the
real 234,606-byte compiler PDB, SHA256
`c98d80a1e225986b065e2299a55c167e38d55bddd7d6f85dd303aa43ab008463`.
The unpacked executable SHA256 is
`20a3990c4e792d821e0db2a5c99e9e41ac2956293547c4cd9a9c81529f28ff05`.
The original TypeServer constructor leaves its `incremental` DWORD unwritten
in exact `"rc"` mode. Two independent natural original invocations each had
one TypeServer lifetime: its value was zero at open entry, record-read entry
and the actual control test, and both processes exited0. This is evidence for
those two runs, not a universal allocator property. Under the user's explicit
allowance, `type_server_open_file` now initializes that exact mode to the
observed zero, with a source defect comment in both builds. The varying BARRAY
word remains pending; it was never constant. The genuine PDB case now completes
with138 matching entries, exit0, exact streams and the 2,506,972-byte output
SHA256 `6ef7547e30dd814adac91915596c76e95584f9928d0659271564580f444853b9`.
Its3,180 proven reference padding words are masked and required to be zero;
the raw original already had zeroes there, so its canonical hash is unchanged.
Final original capture took895.397ms; report-bound native replay108.706ms
(setup9.791/execution65.949/checks32.967), separate from prior producer work
(15.863s compile/setup,3.169s corrected ordinary link,4.133s `/nopack` relink).
The original PDB, executable, output and earlier unqualified history remain in
owned scratch; no reconstruction credit is added for this case. The superseded malformed constant
encoding and its CK4003 warnings remain in owned scratch, without coverage
credit. The two no-op register-relative callbacks remain unaccepted; the wide
conversion no-op does not advance the CLI dispatcher, so it needs a bounded
callback probe. Remaining mapping/TypeServer paths may also require typed probes.

`src/` contains application reconstruction only. Shared fixture checks live in
`harness/`; this binary has no private native host or binary-specific CRT
implementation. Shared CRT compatibility remains in `lib/recon/`.

Run `make -j4 native cases`, `make -j4 check unit`, and `make -j4 win32 audit`
from this directory. Shared `recon-tool.mk` builds `.build/libbinary.so` once,
automatically discovers the seventeen numbered `case.c` descriptors, and runs their
cached trace/status/stream/artifact checks in independent native workspaces.
Fixtures never invoke Wine or recapture. `win32` explicitly uses the original
compiler through the shared Wine prefix. `audit` compares full original/compiler
application CFGs, RET cleanup, extent ownership and instruction overlap;
`check-context.py` additionally checks 217 initial data objects, diagnostic
strings, typed callback identities and source/stub inventory. Target-specific
switch/SEH/compiler annotations live in `policy-data.json`.

Fifteen dispatched cases complete;0013 retains its matching BARRAY frontier
and0015 its parked based-symbol frontier;0016 and0017 SUPERPAD both complete. All retain explicit unseeded first-entry order. The PDB cases materialize owned PDB bytes under
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
The 14-row C6 symbol sizing and conversion tables have exercised their valid
16/32-bit families. Their two nonadvancing/no-op callbacks remain pending.
The based-pointer converter also identifies pending `get_old_symbol` and
`convert_old_based_symbol` helpers through genuine optional calls; these grant
no body credit.
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
final capture retains its complete oracle. Exact derivation and validation
are saved in `~/.ai-tmp/msvc2/vc1.0/cvpack/resume-20261025/0012-derived-nb02-old-types/input-provenance.json`.

The old-module converter left one byte after every counted filename and its
unused allocation alignment tail unwritten. Its reconstruction zeros this tail
with an original-defect comment. The emitted legacy artifact contains exactly
98 of these post-filename bytes. `capture.bin` masks those fixed one-byte fields;
the case callback requires actual zeroes. One raw original byte was `09` at file
`0xfd`; the other 97 were already zero. All other bytes, section offsets and type
ordering match exactly. The 63,960-byte raw original has SHA256
`0901776a8202d75eee36d42df0980f493ad2dc3801b433bd621f7726a24d9082`;
zeroing only those 98 fields gives the native/comparison SHA256
`46a29369441111914418faabd7b028f17e4789c7ee7c71cf7fa648218840ea2c`.
The raw original, native file, structural field inventory and command logs remain
under `resume-20261025/0012-derived-nb02-old-types/` in owned scratch; the original
is `original231-export/files/legacy-wide.dbg`. No raw artifact is hidden in the
capture or duplicated as an asset.

The old basic-pointer converter left reserved attribute bit `0x0800`
uninitialized. Its reconstruction zeros this bit and documents the original
defect. It also initializes the unknown-encoding mode and procedure-convention
fallback bytes to zero where the original left them unwritten. The completed
legacy workload matches all defined type bytes and ordering with these
corrections. No type fields or derived ordering differences are masked.
