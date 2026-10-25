CVPACK 4.25 (VC1.0 / 1.0-nt) is unfinished at **281 accepted application
functions, 78,693 original code bytes, and 1 named trapping stub**.
`binary.c` is the canonical function registry. Reconstruction has resumed after
harness migration; CRT identities grant no application reconstruction credit.
The legacy workload exposed the omitted old-directory comparator and corrected
`read_old_directory`'s previously wrong newer-callback reference. The legacy case now completes with 141 matching first entries, exit 0 and matching
streams and defined artifact bytes. The last accepted function is `convert_old_based_data32` (sequence 281,
121 original code bytes), after `convert_old_based_bprel32` (sequence 280,
121 bytes), both in `src/oldsymbols.c`. The two empty callbacks accepted at
sequences 269/270 each retain one byte of credit; their alignment remains excluded.
Encountered converter dispatch and calls identified additional App helpers;
the remaining BARRAY routine retains a named trapping stub and no accepted credit.
The reached line path also corrected `read_lines` to publish `line_directory`
(original VA `0x17f30`), rather than the old source-module directory.
The resumed workload has accepted 94 application functions.
Each has matching original/native progress and strict compiler/graph/data
checks. The previous nineteen-case/nineteen-unit aggregate passed in 0.661s.
The new callback case/check/unit, original compiler and strict 270-function
graph/ABI/data gate passed in 1.846s without another aggregate replay.
Twenty-two cases are active; twenty-one complete, with only 0013 retaining its
BARRAY frontier. The previous baseline ended at `type_server_close` (sequence 187,
commit `a00fd6a90`). There were no candidate bodies or
uncommitted source changes at migration start.


`0018-file-cache-copy` is explicitly internal file-API coverage, not ordinary
parser reach. At the genuine zero-argument `initialize_type_tables` entry after
normal CLI initialization, its typed callback releases the real input owner
through `close_file`, then opens the genuine /Zi SUPERPAD and compiler PDB.
It checks cached reuse and eviction (the actual threshold of two retains one),
creates a destination through the real file API, copies all 4,194,304 bytes,
naturally grows its writable mapping beyond the initial 2 MiB, reads every byte
back and closes the actual owners. No file-slot, page or global state is fabricated.
The initial CLI input is the independently produced /Z7 SUPERPAD; all three input
artifacts retain their existing producer hashes. The final copy matches /Zi input
SHA256 `20a3990c4e792d821e0db2a5c99e9e41ac2956293547c4cd9a9c81529f28ff05`
without comparison masks. Fresh masked original/native runs match all 22 first
entries and complete (original 1211.806 ms; native 62.183 ms, including 12.535 ms
setup, 8.361 ms execution and 41.287 ms checks). Native candidate rebuild was
0.475 s; typed original helper compilation was 6.341 s. Inputs, raw output and
command logs remain in owned `resume-20261025/0018-file-cache-copy/` scratch.

The earlier same-file setup exposed a separate unsupported sharing path:
original `close_mapped_file` leaks the mapping handle, preserving an exclusive
file share after closing the source handle. Reopening that same file falls back
to buffered I/O on Windows; the native shim currently permits mapped reopening.
The faithful application leak is unchanged, and the failed setup remains in
scratch. The distinct-file case does not qualify or fix that sharing difference.


`0019-pdb-record-api` is internal provider API coverage using the unchanged
234,606-byte compiler SUPERPAD.PDB, SHA256
`c98d80a1e225986b065e2299a55c167e38d55bddd7d6f85dd303aa43ab008463`.
After the same genuine CLI initialization hook, `type_server_open(path,"r")`
constructs its actual arenas and defines both `loaded=1` and `incremental=1`
through the original mode path. The callback queries all 6,795 genuine records
(TI 0x1000 through 0x2a8a), traverses the naturally built 3,314 occupied hash
buckets, checks every node exactly once and closes the real owner. No object,
record or global state is fabricated. It does not establish ordinary CLI `rc`
reach for these helpers. `pdb-records.bin` serializes every returned type ID,
length and record byte, plus all bucket-chain IDs in their original order.
Its exact 256,348-byte output SHA256 is
`2885c4214da8211f502714897f347f991951f8d48e9debf0c3cc7559e80c4ee1`;
all serialized records independently match the genuine input payload.
Fresh masked original/native runs match 25 first entries and complete
(original 1552.216 ms; native 94.001 ms, including 10.097 ms setup,
75.801 ms execution and 8.102 ms checks). Native candidate rebuild was
0.440 s; original typed-helper compilation was 5.941 s. Raw output,
input provenance and full command logs remain under owned
`resume-20261025/0019-pdb-record-api/` scratch.


`0020-cfig-symbol-api` is internal serializer API coverage using unchanged shipped
`toolchains/watcom10.0a/bin/CFIG386.EXE`, 80,575 bytes, SHA256
`c9adb1a1b7c9a37f8ea2f365bf4d21b1220855e01436f981b3d3430a002cfc5f`.
Its authentic NB02 payload is the historical 37,096-byte SHA256
`2e01fe1d2d150f1e20352225636081e0373cae3d79c8b601d8482a4cedc4ddf4`;
no directory, signature or record is changed. After genuine buffered CLI file
initialization, the zero-argument hook reads and closes the real input owner.
It passes all 423 actual BP-relative/data/register declarations (307/106/10) to
`convert_old_based_symbol(output,input)` with caller-owned buffers. Every input
record remains unchanged. Exact output includes module/offset, raw input bytes,
returned size and every converted byte; its 16,504-byte SHA256 is
`86c61cd8a4cd1417137e7ca21591c327f680b9958386a7f6625e6ff67efdc9b8`.
Fresh masked original/native runs match all 28 first entries and complete
(original 1279.282 ms; native 21.490 ms, including 6.881 ms setup,
12.704 ms execution and 1.906 ms checks). Native candidate rebuild was
0.477s; original typed-helper compilation was 6.665s. This does not establish
based-pointer CLI reach or wide-record coverage. The superseded mapped setup
left its small input extended because of the known leaked mapping handle;
that raw attempt remains in scratch and is not part of this buffered fixture.


`0021-noop-callbacks` is a minimal internal return test of the two genuine
zero-argument App callbacks at RVAs 0x13260 and 0x14a50. The actual main-entry
hook runs before parser work, calls both real routines and completes only after
they return. It invents no records or target globals and resumes no parser.
The sizing/conversion table call sites establish the `void __cdecl(void)` ABI;
the fresh original/native sequence is main, sizing callback, conversion callback.
Both empty C functions retain distinct emitted identities under the existing
noinline/build policy. Only each original RET byte is credited; the following
fifteen alignment bytes are excluded. The original conversion callback does not
advance `old_symbol_input`, so this test does not claim that the CLI's wide-record
dispatcher can progress. Final masked original capture took 1133.943 ms;
report-bound native replay took 8.936 ms (5.267 ms setup, 1.863 ms execution,
1.805 ms checks). Native candidate rebuild was 0.470s, original typed helper
compilation 2.409s. All receipts remain in owned `0021-noop-callbacks/` scratch.

One application stub remains: `convert_old_barray` depends on the genuinely
uninitialized stack WORD observed as both 0 and 0x7e, so the user's allowance for
a consistent observed constant does not apply. The other four former limitations
are resolved by the explicitly authorized format-authored NB08/NB02 CLI cases
0022/0023 below. Well-formed authored old-format inputs are legitimate original
oracle workloads; no historical producer provenance is claimed for them. More
inputs do not by themselves resolve the BARRAY undefined-state problem, and no
control value is guessed. Reconstruction pauses at that honest residual limit.

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
already zero, so the raw and canonical output hashes coincide.

The historical hand-authored `cases/0015-based-symbol-families` workload is
parked and removed from the active case tree at the user's direction. Its
1,320-byte input SHA256 was
`4fb473791a75b1c5aeed6ab6cd4084f8bad4f98a7deaff250c8ff9c821716e52`;
the original full oracle had 118 entries and raw 2,432-byte output SHA256
`17696a0ae7fd41cc7e5fb764b6988898d6118622b9c1678ff1157c355936d77e`.
Its last qualified partial descriptor stopped at `convert_old_based_data16`,
76 entries. That frontier is now implemented through the genuine 0020 case;
the old descriptor cannot remain a valid runnable regression. No new frontier
was invented and the old oracle was not relabelled or rerun. The entire folder,
including ignored build files, is preserved at
`~/.ai-tmp/msvc2/vc1.0/cvpack/resume-20261025/0015-based-symbol-families/parked-final-tree-20261025/`
and in Git through `d8357e775`. `PARKED.json` in that scratch parent records the
reason and confirms no unfinished source body existed only in the folder.
The historical generator remains a reproduction recipe, not the next workload.
New work uses genuine producer artifacts rather than constructed debug records.
The replacement `0016-superpad-embedded` uses the unchanged shipped MFC 2.0
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
credit. The two no-op register-relative callbacks are now accepted through the
minimal typed return test described below. The wide conversion still does not
advance its CLI dispatcher; its original behavior is preserved.

`src/` contains application reconstruction only. Shared fixture checks live in
`harness/`; this binary has no private native host or binary-specific CRT
implementation. Shared CRT compatibility remains in `lib/recon/`.

Run `make -j4 native cases`, `make -j4 check unit`, and `make -j4 win32 audit`
from this directory. Shared `recon-tool.mk` builds `.build/libbinary.so` once,
automatically discovers the twenty numbered `case.c` descriptors, and runs their
cached trace/status/stream/artifact checks in independent native workspaces.
Fixtures never invoke Wine or recapture. `win32` explicitly uses the original
compiler through the shared Wine prefix. `audit` compares full original/compiler
application CFGs, RET cleanup, extent ownership and instruction overlap;
`check-context.py` additionally checks 217 initial data objects, diagnostic
strings, typed callback identities and source/stub inventory. Target-specific
switch/SEH/compiler annotations live in `policy-data.json`.

Nineteen dispatched cases complete; 0013 retains its matching BARRAY frontier.
0015 is retired from the active tree. All active cases retain explicit unseeded
first-entry order. The PDB cases materialize owned PDB bytes under
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
are accepted empty C functions. Each has one RET byte of actual original code;
alignment is not application code credit. Other short matches are unchanged,
and the raw former rows remain in Git and the owned snapshot. The 14-row C6
symbol sizing and conversion tables establish their genuine App identity; the
typed return test grants no additional wide-record or CLI reach coverage.
`get_old_symbol` and `convert_old_based_symbol` were accepted through the
historical workload; the genuine CFIG386 API case independently exercises the
serializer dispatcher and its narrow record families. The two wide serializers now have normal-CLI coverage through the authored
NB02 storage workload (0023).
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


`0022-packed-storage` is a complete format-authored NB08 normal-CLI workload,
not relabelled NB09 or a claimed historical producer artifact. The shared
`scripts/make-old-formats.py` builds its validated 78,024-byte two-module input
(SHA256 `7386d280e034bcf060d1f7d416d07c6274a681617909b71c51ee44ea823e8387`)
and the independently valid NB02 C6 companion `0023-wide-storage`.
The packed input contains module/symbol/source subsections, four global and four
public declarations, and 3,200 distinct C7 array types. Genuine CLI processing
loads its 76,808-byte type subsection across 4 KiB buffers and rewrites the old
header-relative offsets into NB09 record-relative offsets with a 48 KiB page gap.
The reader revealed seven previously unregistered application helpers; the
canonical registry now has 282 App identities, with no CRT credit increase.
Its completed fresh masked original/native pair matches 73 first entries,
exit 0, streams and the 79,192-byte output. An initial defined-output mismatch
identified and corrected the new wrappers' reversed use of the historical
public/global arena names; the raw whole-file hashes then matched exactly.
The fixture masks only twenty known-unwritten S_DATAREF reserved WORDs and
requires native zeroes; every other byte remains checked. There is no filename
alignment tail in this input. The final paired native run took 16.456 ms
(setup 6.912, execution 4.517, checks 5.027); original progress took 1,353.324 ms.
The case/unit, original compiler and strict 279-function graph/ABI plus 217-object
context gate passed in 1.950 s. Inputs, raw original/native bytes, the offline
comparison-only derivation and full commands remain in owned
`resume-20261025/0022-packed-storage/` scratch. Original unwritten cache locals
and the writer's unbounded post-last-gap lookup remain faithfully documented;
no sentinel, allocator tuning or control-field repair was introduced.


`0023-wide-storage` completes the companion two-module NB02 normal-CLI case.
Its unchanged authored 1,219-byte input SHA256 is
`16204a790b4ebc98d00e38c27b3a0e605b20b7df9795151aff4d35f2d3c7722e`.
The builder uses the recovered target's twelve-byte directory dialect and
complete C6 records, including wide BP-relative/data declarations referenced by
valid based-pointer types, ordinary register declarations, source files and
module contributions. It excludes BARRAY and the nonadvancing no-op CLI record.
The original accepted the complete input without warnings; fresh masked
original/native runs match all 114 first entries, exit 0, streams and the complete
2,152-byte output. Original capture took 1,144.179 ms; the final report-bound
native run took 13.146 ms (setup 5.817, execution 3.328, checks 4.002).
Its raw SHA256 is `dbb4c124834106412f8aba39e829580977caf20847c51b5e695d19d9217fee13`.
Sixteen pointer attributes contain the original's proven unwritten reserved
bit 0x0800. Comparison masks the containing byte; a case-owned assertion requires
all eight native bits to be zero, independently keeping the seven defined bits
exact. Every other output byte remains hash-checked; no ordering is relaxed.
The raw oracle and comparison-only derivation remain in owned
`resume-20261025/0023-wide-storage/` scratch. The final `make -j4 check unit win32
audit` passed all 22 cached cases, all 22 unit invocations, the original compiler,
281 strict graphs/ABIs/ownership/extents and 217 contextual objects in 2.667 s.
No original is run by that regression batch. The varying BARRAY control defect,
0013 partial oracle and parked historical 0015 work remain unchanged.
