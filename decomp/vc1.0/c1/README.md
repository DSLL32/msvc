# C13232.EXE

VC1.0 frontend reconstruction, toolchain `1.0-nt`. `binary.c` is the function
registry; its RVAs use original image base `0x10000`.
There are 987 implemented application functions covering 229236 bytes. The
accepted code/data union is 252119/277234 bytes (90.9409%); the
application numerator is 82.6868% of all code, not a claimed non-CRT denominator.
Eleven historical handwritten CRT bodies remain unchanged and receive no App
credit. Unknown library aliases and unattributed unlisted code remain unresolved.
The PDB interfaces now own their previously ambiguous `17070`/`17140` return
bytes through genuine whole-function entries; the old one-byte library aliases
never established independent CRT ownership.

Run from this directory:

```
make -j4 cases
make -j4 check
make -j4 unit
make -j4 win32 audit
```

Native app and case glue use Clang i386, PIC, `-O0 -g`, MS extensions and function
instrumentation. Runtime/shim support is uninstrumented and linked once into
`.build/libbinary.so`. Common recon-tool rules discover all numbered cases.
`win32` uses the original compiler through the shared launcher; `audit` retains
function-graph, RET, reference, ownership and extent checks, with shared runtime
identification and quiet successful output. Its explicit historical runtime
object binding permits shipped callers of the unchanged `_sopen` override.
Only grammar and fixture-source generators remain under `scripts/`.

Application reconstruction is under `src/`; existing CRT exceptions are under
`crt/`. Case-specific adapters/tests stay beside their numbered descriptors;
root `entry.c` is the canonical native executable host, while there is no shared
case C helper requiring `harness/`. Shared compatibility stays in `lib/recon/`.

`crt/runtime.c` retains ten historical variants: `setvbuf`, `_freebuf`, `_filbuf`,
`ftell`, `fseek` and `fclose` preserve this executable's older stream flags and
buffer/seek/close behavior; `_sopen` preserves its descriptor flags and Win32
open/share call graph; `_unlink` retains its separate forwarding entry;
`__tzset` and `_isindst` retain its timezone defaults and historical DST rules.
`crt/decimal.c` retains the existing `__strgtold12` scanner, its twelve-byte
result/end-pointer/status contract, 25-digit workspace and conversion-helper
graph. Its private representation is in `crt/decimal.h`. These are documented
compatibility exceptions already present before this layout change, not new
CRT reconstruction or application credit.

Cases 0000–0064 preserve 65 cached public invocations. Cases 0065–0173 preserve
109 original-backed internal direct invocations; these do not establish public
source reachability. The typed shared entry hook pauses genuine main/parser/PDB
constructor contexts, preserving live owners and calling conventions. Returning
helper tests assert defined state, pending streams and owned artifacts, then
publish a harness status and terminate without flushing those paused streams.
Genuine target `exit(1)` and `exit(0x8000)` retain their full DWORD status and
normal cleanup. The narrow QuickC unit checks its established unsupported
uninitialized-default guard; the public QuickC fixture is its valid control.

Every fixture is native-only and consumes its `capture.bin`. Defined pending-stream
and PDB byte oracles are fixture-owned inputs, with typed assertions in `direct.c`. Old shared-prefix
references retain their original R:/S: paths and original bytes; migration did
not recapture or relabel them. PCH producers and consumers now run independently
against the same frozen byte contracts, rather than performing a dynamic
handoff. Only identified irdb+14 and PDB+48 clock fields are normalized; raw
within-case clock equality remains checked. Explicit seeds retain historical
first-entry projections. Legacy direct tests without an original entry-order
oracle explicitly seed all accepted App entries instead of inventing a trace.
Case 0172 uses the shared typed original-DIRECT capture interface; cached replay
remains entirely native.

## Residual interface case

The six interfaces (324 bytes) accepted before migration are now joined by
`ten_bytes_to_extended` (0x4c6da..0x4c6fc), `extended_to_unsigned`
(0x4c77f..0x4c7c4), and `round_extended_to_integer` (0x4c7c4..0x4c80f):
three functions, 178 bytes. Cases 0166–0171 finish the six original-backed
numeric invocations from the same genuine parsed `residual_sink` arguments.
They check ten-byte expansion of 2.75; unsigned and rounded 2.75; unsigned and
rounded -2.75; and unsigned 4294967295. Results are 2, 3, 0xfffffffe (status 1,
including its post-error store), 0xfffffffd and 0xffffffff respectively.
The exact borrowed literals, complete eight-byte numeric workspace, unchanged
argument nodes, 48-byte owned destinations/guards, pending streams and files
are checked. These direct interfaces do not establish public-source reachability.

The native build selects the existing `RECON_NATIVE_STARTUP_CONTROLFP_53`
host boundary option. Genuine untouched C1 main entry has CW 027f, SW 0 and
empty tags. Native entry was 037f; the precision-only option produces 027f,
retaining SW 0 and empty tags. No saved control-word field is normalized and
no application source was changed to repair it. Fresh native entry/return checks
retain cdecl stack cleanup, nonvolatile registers and control word. The six
saved original invocations and original masked frontiers remain unchanged;
no Wine recapture was required. Raw originals, the earlier 037f mismatch and
new debugging logs are in `~/.ai-tmp/msvc2/vc1.0/c1/` scratch space.

`write_asm_segment_prefix` (0x4f0cd..0x4f12f, 98 bytes) is accepted through
case 0172. Genuine main parses the existing `mov eax, fs:[0]` source and stops
at `normalize_asm_addresses` (RVA 3eb41), before parser segment ID 29 becomes
encoded value 12. The typed cdecl call uses those live operands/addresses and
stream owners. It writes `05 af 00 01 0c 00` and appends the expected eight-byte
IR record. The test checks all 1688 bytes of the before/after pending-stream
snapshot, defined workspace prefix, within-side untouched tails/owners,
stack cleanup, control word and nonvolatile registers; it never resumes parsing.
The final fixture binds all on-disk outputs, separate stdout/stderr and status.

The common original harness adds its instrumentation DLL import to a masked
scratch EXE; the normal loader and local entry hook supply the live context.
The pristine executable is unchanged. Original and native first trap at 3f0cd,
then both complete after that whole body is reconstructed. All other accepted
App entries are explicitly seeded; the new original order is `[3f0cd]`.
This proves a direct support interface, not a public-source caller.

Initial original setup exposed `_strncpy` at RVA 46bd0..46bf5. All 37 bytes
exactly match shipped `LIBC.LIB`'s `string\obj\i386\STRNCPY.OBJ` `.text`,
with no relocations; existing original cdecl callers corroborate it. The
registry now restores this CRT boundary without reconstructing CRT or adding
App credit. An inherited native-only stderr merge caused a first partial
mismatch; this new phase now preserves separate handles on both sides.
Unknown unlisted code and ambiguous RET tails remain separate from CRT; the
completed residual case is not a claim that the binary is complete.

Migration verification on this host: original compiler build plus audit 20.17 s;
final native case build plus incremental audit 6.57 s. All 166 cached invocations
passed in 3.79 s (setup 1.39 s, execution 1.14 s, checks 0.42 s; remaining time is
supervision/build-target overhead). The unit target checked all 166 descriptors,
including the one QuickC guard test, in 2.36 s. No original reference was captured.

Numeric checkpoint verification: all 172 native invocations pass with `make -j4
check` in 1.18 s; summed setup/execution/checks are 1.32/1.15/0.40 s across
concurrent cases. The unit target passes in 0.80 s. The final case rebuild took
9.75 s; combined original-toolchain/native build and graph/ABI audit took
22.39 s. The original map is now a declared grouped link output, so a missing
map is regenerated before runtime identification. No new original capture was
run; the six saved numeric body captures originally took 12.46 s including setup.
The ASM prefix was finished afterward through case 0172 as described above.

Four existing ASM parser error calls now forward the original context (408/409)
and token-name/context pair (426). Original C1 call bytes and rebuilt caller
cleanup confirm the correction; it adds no functions, credit or error-path
reachability claim. Existing native cases and graph/ABI checks remain required.

ASM completion verification: original/helper/native build took 21.12 s; the
final bundled incremental build, 173 case-library rebuilds and strict audit took
11.33 s. The audit checked 994 source functions (983 App plus 11 historical CRT).
All 173 cached invocations passed in 1.48 s; summed setup/execution/checks were
1.84/1.12/0.68 s across parallel cases. All unit executors passed in 1.16 s.
The successful new original frontier and completed typed invocation took
1.64 s and 1.40 s respectively; final native comparison took 20.23 ms.
Earlier failed/mismatched captures and the exact CRT qualification remain in
scratch; there was no additional original replay for final bookkeeping.

## PDB record and arena support

Case 0173 runs genuine `/Zi` input to `create_program_database`, retaining the
actual 116-byte owner and two initialized twelve-byte list heads. Accepted
`open_program_database_object` loads the captured valid eight-record database in
`wc` mode, defining cumulative/index/record state through its normal interface.
No token, compiler flag or owner field is seeded. The full resolved database
path is an assertion on the actual producer, not a harness-supplied owner.

The unlisted `codeview_leaf_offset` (17040..17071, 49B, cdecl one pointer)
reads real leaf-code addresses and retains its encountered CodeView catalog
scan before returning unsigned leaf-minus-two. Its table's names are ordinary
literals in their catalog entries, including the original duplicate8000 code.
`reserve_program_database_record` (17080..17141, 193B, cdecl three arguments)
reserves prefix-plus-record bytes from the owner list. The case tests growth
for a 34-byte stored record followed by a ten-byte reservation that fits. It
checks list topology, capacity/cursors/counts, source records, the initialized
first reservation, stack cleanup, nonvolatiles and control word. Unwritten new
capacity is not read or compared. The actual deleting close frees the owner;
parsing never resumes and the writer is not called on these unindexed records.

Both original/native frontiers agree, then final order is `[7040,7080]` with
all983 preexisting App entries explicitly seeded. The independent 56-byte
scalar result is captured as fixture input; complete status, separate streams
and on-disk outputs also match. This is direct support, not public reach.
Both complete extents include their genuine final RETs; no alias or tail was
credited twice. The original success path took1.411s and native20.236ms;
bundled original/native build and strict audit took5.390s (996 source functions).

The bounded survey left a36B stdcall byte copier42bf0..42c14 and73B decimal
wrapper58660..586a9 without sufficient origin/identity. Lack of callers is not
proof of CRT or dead code. Alignment/data patterns are also not new functions;
no binary completion/non-CRT denominator claim follows from this case.

The final174-case gate passed: bundled incremental build/case rebuild/audit8.03s,
all cached native invocations1.48s, and unit executors1.14s. The original
frontier captures took1.995s (7040) and1.394s (7080); no final bookkeeping
original replay was performed. The source/ABI/trace assertions and compact
raw results remain in shared fixtures and scratch, without a private runner.

### Permanent release and calendar extension

The same case now includes `release_permanent` (44bf0..44bf3, 3B,
void stdcall2). Its provenance is the exact corresponding C1XX permanent-arena
family and accepted release caller, not a generic RET8 byte match. Actual
`allocate_permanent(32)` supplies the initialized owner. The call leaves its
32 bytes, current page entry, next cursor, remaining capacity and page count
unchanged, with correct two-slot cleanup, nonvolatiles and control word.

`dos_date_weekday` (4be50..4bed2, 130B, cdecl1/RET0) is attributed by inference
to shared frontend DOS-portability support: both C1 and C1XX retain the same
bespoke timestamp formatter/time/fstat/weekday/packed-option family, with this
whole130B body identical at C1XX74600. No original source name or CLI caller
is claimed. The six ordinary valid DOS timestamps exercise January, February,
March, leap/nonleap years and2100. Results are2,5,6,0,3,2; the last preserves
the original four-year calendar rather than applying a Gregorian century fix.
The initial provisional stdcall declaration was corrected from the actualRET0
before any body-return acceptance; fresh corrected-ABI original/native proof
is retained. No parser state or globals are synthesized for these scalar inputs.

The case now covers four interfaces/375B, with fresh order
`[7040,7080,34bf0,3be50]` after its explicit983-entry historical seed. Its
scalar fixture grows from56 to80 bytes; old case/oracle remains in Git history.
The final shared original run passed in1.835s, fresh native comparison21.048ms.
The canonical original/native build and strict998-function audit took20.905s.

Two positively identified CRT boundaries were added without source or App
credit: `_ctrlevent_capture@4` (53010..53062,82B) matches the shipped WINSIG.OBJ
role, actual `_signal` registration and SIGINT/SIGBREAK slots/handler semantics;
`__clearfp` (54670..5468a,26B) matches shipped ieee87.obj, with two original
FWAIT prefixes and the same proven `__abstract_sw` callee. The73B decimal
wrapper's four-argument/status contract differs from shipped `__atodbl`, so
that name and identity are not assigned.

The finite origin survey found no COFF symbols, debug directory or exports in
C1 and no decoded call/jump into any byte of the remaining36B copier.
Whole-byte scans of shipped1.0-nt providers do not attribute the copier. This
negative evidence is retained as a limitation, not CRT/dead-code proof. No
additional exhaustive search or speculative CRT reconstruction is implied.

The final extension gate passes all174 cached native invocations in1.47s and
all unit executors in1.14s. The bundled incremental build/case rebuild/audit
is8.20s. These are native regressions; no original is rerun for bookkeeping.

## Reserved assembler identifiers

Case 0174 compiles valid declarations and inline assembly using register labels
and mnemonic-named variables. It reaches both warning-405 sites in
`classify_asm_identifier`. The original calls at4d84e..4d860 and4d96c..4d97e
push the actual interned name before405 and1, with twelve-byte cdecl cleanup.
Both reconstructed calls now forward that missing variadic argument. This
corrects diagnostics without adding functions or reconstruction credit.

The fresh original succeeds with four reserved-name warnings and two unused-label
warnings. Native replay matches its complete340-entry unseeded application order,
separate streams, five IR files and browser output. Original capture took1.973s;
native setup/execution/checks took13.081/3.733/4.410ms (21.224ms total).
The original compiler build and graph/ABI checks pass. The final incremental
build/audit took1.525s; all175 cached cases pass in1.493s and unit executors
in1.170s, using the shared fresh-before-load errno correction e0116ffcb.
No existing oracle was recaptured for that runner update.

## Canonical native executable

`make -j4 native-bin` links `build/c1` using the existing application, historical
CRT and shared shim objects. Root `entry.c` is native host glue only: the shared
partial link privately renames App `main`, leaving fixture objects and the Win32
build unchanged. Descriptor, entry-hook and fixture-provider objects are excluded.
The VC1.0 coordinator installs this ELF under its ordinary `C13232.EXE` sibling
name for CL's real child invocation.

Shared startup owns mutable actual arguments/environment and the Windows command
line, module path, cwd and CRT environment. It works with only the child-supplied
`MSC_CMD_FLAGS`, without `PATH` or `RECON_*` settings. Existing support retains
verified 53-bit FP precision and binary default file mode. The application owns
its normal initialization and exit cleanup; no guessed CRT startup is invoked.

The shipped 57919-byte `C1.ERR` is embedded in read-only sections, with SHA256
`8f9f3304a1a893e588df75d1259e305d3bb3d15a4a4cc5b457265ad774a03775`.
Startup registers only default relative `c1.err` and the actual executable's
installed `C1.ERR` path, deduplicating them when cwd is the install directory.
An alternate `-ef` path uses the ordinary filesystem. No catalogs are extracted;
no captured dependency response or original executable is needed for startup.

Canonical verification: minimal-environment compilation succeeds with all five
IR files; default embedded and real external `-ef` diagnostics both match the
selected catalog (3.0–3.9ms per invocation). The installed sibling also passes
real CL→C1→C2 compilation with an empty top-level environment, producing a286B
COFF object in8.159ms. Initial host-module registration used a fixed name that
rejected local `build/c1`; it now derives the actual executable basename, so
both local and installed identities agree with their paths.

Native build took5.711s, then the metadata-only host relink0.584s. The175-case
cached gate passes in11.489s including case-library rebuilds; parallel summed
setup/execution/checks are1.900/1.132/0.700s. All175 unit executors pass in1.180s.
The linked catalog bytes match the source exactly, and fixture descriptors/hooks
are absent from the canonical executable. No application/CRT bodies, cached
references, Win32 inputs or reconstruction credit changed; no Wine ran.
