Direct image-helper finalization/checksum lifecycle (995 frontiers)
=================================================================

Two dormant nonCRT support bodies are reconstructed as readable Win32 C:
checksum_image_file4ec70..4ed57 (231bytes, cdecl int(const char*,DWORD*,DWORD*))
and make_import_section_read_only4f4f0..4f537 (71bytes, cdecl void(BoundImage*)).
Their actual caller is an external typed test driver, not a claimed public EDIT
caller. Positive ownership comes from the existing internal PE checksum graph and
the actual36-byte BoundImage/40-byte section structures produced by accepted
map_bind_image. Both bodies lack literal external callers/pointer references.
The prior read-only proposal and completed nine-member LIBCX32 provenance review
remain under cases/direct-module-diagnostics; no-match alone is not ownership proof.

The new owned input is byte-identical to the prior original-validated client.exe,
with real imports/relocations and a writable .idata section. Prior inputs remain
untouched. The existing bounded owned-stack/sentinel seam replaces main at entry,
calls actual helpers sequentially and resumes the real CRT continuation with
external test status0. It does not execute original main or claim normal LINK CLI
reach. Native __wrap_main implements the same external workflow; startup53 is
selected from process startup, not reseeded at a function or by the debugger.

Lifecycle and exact effects

1. NonNULL third output requests rb/PAGE_READONLY/FILE_MAP_READ. The helper computes
   the full physical image checksum and returns it with the previous stored value.
2. Real map_bind_image creates the live BoundImage/base/header/section owners.
   The section helper scans actual section names and changes only the first .idata
   Characteristics fromC0000040 to40000040, preserving every other bit/section.
3. Actual UnmapViewOfFile/CloseHandle persists that mutation. Recalculation uses
   this changed native-produced file, whose checksum is now2110789 versus2143557.
4. A NULL third pointer requests r+b/PAGE_READWRITE/FILE_MAP_WRITE. The existing
   checksum_mapped_image writes the stored checksum. Real unmap/fclose persists it;
   recalculation and a repeated write prove raw idempotence.
5. Ordinary missing.exe returns1, leaves both output values unchanged and yields
   actual errno/DOS/LastError2/2/2. No mapping/view provider failures are forced.

A single untouched original workflow establishes all ten phases. Seven complete
2MB stage images, final input/output inventory, status0 and empty streams match
fresh original/native and the cached baseline byte-for-byte. Independent full-file
checksum calculation proves the stored value; only the intended header DWORD and
checksum DWORD change. There is no clock input, normalization, repair or native
input recapture. The280-byte external report derives from actual baseline return
observations: phase/result/previous/computed/errno/DOS/Last per phase. Voidhelper
result0 is an explicit test protocol field, not a claim about its undefined EAX.
Neither that report nor saved stage copies are claimed original target-created
files; they are external test outputs. Native stage output feeds the next phase.

ABI, support and evidence

All actual3/1 formals, live BoundImage owner edges, full248-byte PE headers,
160-byte section array, immutable36-byte workspace, source name tails, guards,
cdecl/stdcall stack cleanup, nonvolatiles, consumed returns and errors match in
50 fresh observations. x87 CW027f/SW0/tagsFFFF and all eight empty register payloads
are strict. Raw platform save-area/context records remain preserved, without a
claim of byte equality for reserved/selectors or addresses. No state qualification
or error exclusion was required. Twenty-one focused offline controls reject wrong
formals/owners/guards/header/error/FP/return/stack/caller or missing observations.

Two reached support functions come from the shipped CRT, with zero app credit and
no new handwritten CRT C. __fileno is byte-exact FILENO.OBJ; its actual FILE* owner
and consumed descriptor+16 return3, with original returnPC4ecaa/4ed25 and native
checksum_image_file caller, are mandatory. __filelength is the112-byte FLENGTH.OBJ
variant: only reversed CMP operands before JE differ, with identical consumed ZF;
all other bytes/relocations/three_lseek calls/globals match. Its public fd3/2MB
result/errors/FP and actual4ed2e/native namedcaller are checked. No native internal
CRT-helper ABI equivalence is claimed. The old unaccepted crt_filelength stub alias
is removed; existing options and the new wrapper call ordinary _filelength.
_fileno's SDK macro is undef'd before the final recon include to retain the real
original call edge (spelling in source is _fileno).

The first native observer omitted __fileno and stopped too late; original traps
correctly exposed it. Both raw packets and the failed inventory are retained in
analysis/initial-support-observer-hold.json. Adding the actual named boundary gave
the correct subsequent frontiers; no acceptance used the failed pair. The initial
native link failure from the obsolete alias is separately retained. One baseline
and required frontier captures were used; no automatic baseline repeat/campaign.

Verification and timing

All87 cached native cases pass. The external make wall12.978s includes older host
prerequisite relinks, which were not timed separately; per-fixture receipts exclude
build. Main/newhost native+trace build5.121s and trace-cache projection1.108s are
separate. The new traced fixture totals.352209s: setup.302086s (including.192517s
trace preparation), direct ELF40.751ms, oracle9.238ms and trace comparison.127ms.
Output-only totals.154827s with direct ELF37.837ms. Those ELF timings include the
external driver's seven stage copies. No debugger/original runs inside fixtures.
The final native-only GDB refresh is a separate.372s host command, reusing unchanged
fresh original observations after adding the external scalar report.

Original VC1-j4 build20.594s,833 common CFG/runtime/ABI/reference/ownership/extent
checks,815 callback,512 contextual-data and41 ECX checks pass. Meaningful graph
negatives and21 offline state controls pass. Exact shared READYc11fa6763 C/H hashes
are unchanged across native build/observation/suite. Timings/source hashes/raw
scratch receipt paths are in timings.json, compiled-source.json and validation
records. Normal check/replay dryrun is native-only; canonical/capture is explicit.

There are798 application functions (759public/39direct), plus16 historical CRT
overrides separately. New302bytes are genuine decoded application instructions;
new128 shippedCRT extent bytes are support only. App-only union228780/281634=
81.2331%; support-inclusive259511/281634=92.1448%. Known target-body inventory
193668/194345 remains partial; the full nonCRT denominator is unavailable.
12678 unaccepted unclassified bytes remain pending, not CRT. This is direct support,
not public CLI or dynamic branch coverage.

Ordinary commands from binary root:
  make -j4 native-trace-check
  python3 scripts/check-image-checksum.py
  python3 scripts/check-image-checksum-controls.py
