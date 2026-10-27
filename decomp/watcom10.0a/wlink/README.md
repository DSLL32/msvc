WLINK 10.0a has thirty completed native regression cases. The acceptance ledger
contains **1,165 application functions / 131,124 unique original code bytes**.
The initial help case is followed by a substantial NT console link with two
OMF objects, a static archive, imported functions, an exported symbol, cross-module
data and relocations, embedded Watcom debug information, and a verbose map.
The next case links a C++ DLL with templates, COMDATs, dead-code elimination,
an archive, an import alias, ordered exports, description/version metadata,
and CodeView debug information. A fourth workload creates an OS/2 LX DLL
with structured data, an archive, ordinal imports, resident/nonresident exports
and an appended ELF32 DWARF image. The fifth links a 16-bit Windows NE DLL
from USE16 assembly, communal storage, a static archive, far calls, ordinal
imports/exports, movable/discardable code and Watcom line information.
The sixth produces a DOS MZ executable with two parallel overlays, cross-section
far calls, distributed archive code, communal storage, the shipped WOVL manager,
paragraph/sector padding, relocations and Watcom debug information.
The seventh produces a NetWare NLM from USE32 modules and an archive, with code/data
imports, exports, start/exit/check routines, BSS, Novell debug records, module flags,
screen/thread/version/copyright settings and custom/message/help/RPC payloads.
The eighth creates a QNX386 flat executable with code/data/selector relocations,
an archive, protected segment classes, privilege/heap/stack options, an embedded
resource file, linear fixups and Watcom debug information.
The ninth links a Phar Lap segmented executable with a real USE32 stack, an
archive, selector/offset fixups, the shipped DOS stub and Watcom debug data.
The tenth links ELF386 load groups with an archive, imported/exported symbols,
a dependency module, aligned dynamic tables and exercised DWARF generation.
The eleventh exercises extended-flat Phar Lap image output, runtime memory/buffer/
interrupt-stack/privilege settings, an explicit base and a USE16 real-break symbol.
The twelfth links an OS/2 NE DLL with global initialization, PM compatibility,
protected/multiple-data/new-files policies and type-wide and named-segment
load, access, sharing, conformance and IOPL flags.
The thirteenth links a DOS COM program through a reusable SYSTEM block, nested
response files, environment-expanded object and library paths, explicit LIBFILE
inclusion, case-sensitive symbols, module/symbol tracing and separate debug data.
The fourteenth upgrades an OS/2 LX DLL, preserving old export ordinals while
adding a new export and module metadata, with code/data/BSS page policies.
The fifteenth performs a self-contained PE DLL ordinal upgrade from genuine
USE32 OMF code/data, using old PE header mode and the native subsystem.
The sixteenth combines C++ virtual dispatch, templates, array types, COMDAT
dead-code removal, selected Watcom debug records and an alphabetically sorted map.
The seventeenth links dynamic DOS overlays with external section files, automatic
sections, explicit area size, indirect-call policy and forced vectors.
The eighteenth complements it with standard short overlay vectors, fixed library
placement and an explicit NOVECTOR symbol.
The nineteenth emits Phar Lap REX with a DOS stub, flat offset relocations,
data/BSS, stack, min/max memory settings and Watcom debug information.
The twentieth emits a non-DLL OS/2 LX physical-device image with explicit
internal/self-relative relocation policy, toggled flags and single autodata.
The twenty-first combines TNT DLL thread metadata with genuine OMF aliases, long
public names, duplicate/undefined-symbol policies and their complete diagnostics.
The twenty-second combines NetWare LAN shared-image rebasing, referenced/export
debug filtering, numeric string escapes and a caret response-source handoff.
The twenty-third reads WLIB export-command text, including quoted/renamed and
anonymous exports, with NE segment boundaries, alignment and relocation checks.
The twenty-fourth combines compiler-generated OMF export definitions and iterated
data, a WLIB import-definition member, DWARF and POSIX PE subsystem metadata.
The twenty-fifth combines explicitly format-authored virtual-table metadata, nested
iterated COMDATs, additive backpatches and legacy debug names in a NetWare NAM.
The twenty-sixth resolves conditional weak/pure virtual references through a real
vtable archive member, alongside compiler exports and an import-definition
member, in a NetWare DSK image.
The twenty-seventh combines QNX16 floating-emulation loader relocations, an
absolute hardware segment, explicit archive-member loading and quiet/segment
policies.
The twenty-eighth combines Windows NE implicit-stack selection, reserved BSS
boundaries, brace lists and real unordered16/32-bit line metadata.
The twenty-ninth combines old ELF DLL policy, dead-code selection and DWARF
line accounting with real imports/exports and a dependency module.
The thirtieth uses seventeen genuine OMF files to exercise ordinary cache-handle
cleanup in a PE OS/2-character-subsystem link.

| Case | Result |
| --- | --- |
| `0000-help` | Exit 0; exact 8,475-byte stdout, empty stderr, no artifacts |
| `0001-nt-link` | Exit 0; exact 243-byte stdout, empty stderr, 7,588-byte PE and 11,855-byte map |
| `0002-nt-dll` | Exit 0; exact 253-byte stdout, empty stderr, 34,475-byte DLL and 44,357-byte map |
| `0003-os2-lx` | Exit 0; exact 272-byte stdout, empty stderr, 5,132-byte LX DLL and 4,274-byte map |
| `0004-win16-ne` | Exit 0; exact 248-byte stdout, empty stderr, 905-byte NE DLL and 3,508-byte map |
| `0005-dos-overlay` | Exit 0; exact 254-byte stdout, empty stderr, 8,223-byte MZ overlay executable and 9,395-byte map |
| `0006-netware-nlm` | Exit 0; exact 255-byte stdout, empty stderr, 1,701-byte NLM and 3,064-byte map |
| `0007-qnx-flat` | Exit 0; exact 248-byte stdout, empty stderr, 820-byte QNX executable and 2,990-byte map |
| `0008-pharlap-segmented` | Exit 0; exact 259-byte stdout, empty stderr, 28,536-byte executable and 3,166-byte map |
| `0009-elf386` | Exit 0; exact 245-byte stdout, empty stderr, 32,848-byte executable and 3,147-byte map |
| `0010-pharlap-flat` | Exit 0; exact 258-byte stdout, empty stderr, 28,347-byte executable and 3,373-byte map |
| `0011-os2-ne` | Exit 0; exact 256-byte stdout, empty stderr, 943-byte NE DLL and 3,507-byte map |
| `0012-dos-com` | Exit 0; exact 249-byte stdout, empty stderr, 47-byte COM, 316-byte debug file and 1,958-byte map |
| `0013-os2-upgrade` | Exit 0; exact 267-byte stdout, empty stderr, 5,113-byte LX DLL and 4,268-byte map |
| `0015-pe-ordinal-upgrade` | Exit 0; exact 232-byte stdout, empty stderr, 11,776-byte PE DLL and 3,038-byte map |
| `0016-cpp-vf` | Exit 0; exact 253-byte stdout, empty stderr, 46,257-byte PE DLL and 60,582-byte map |
| `0017-dynamic-overlay` | Exit 0; exact 254-byte stdout, empty stderr, 10,395-byte MZ, 48/32-byte overlays and 9,126-byte map |
| `0018-short-overlay` | Exit 0; exact 254-byte stdout, empty stderr, 8,223-byte MZ and 9,063-byte map |
| `0019-pharlap-rex` | Exit 0; exact 261-byte stdout, empty stderr, 27,811-byte REX image and 3,070-byte map |
| `0020-os2-driver` | Exit 0; exact 262-byte stdout, empty stderr, 1,645-byte LX image and 3,324-byte map |
| `0021-tnt-symbols` | Exit 0; exact 422-byte stdout, empty stderr, 9,119-byte TNT DLL and 3,235-byte map |
| `0022-novell-shared` | Exit 0; exact 255-byte stdout, empty stderr, 3,276-byte LAN image and 3,066-byte map |
| `0023-wlib-exports` | Exit 0; exact 483-byte stdout, empty stderr, 935-byte NE DLL and 3,730-byte map |
| `0024-omf-directives` | Exit 0; exact 253-byte stdout, empty stderr, 10,369-byte PE DLL and 3,276-byte map |
| `0025-omf-records` | Exit 0; exact 255-byte stdout, empty stderr, 2,232-byte NAM and 3,860-byte map |
| `0026-omf-conditional` | Exit 0; exact 255-byte stdout, empty stderr, 2,505-byte DSK and 4,713-byte map |
| `0027-qnx-float` | Exit 0; empty stdout/stderr, 670-byte QNX16 image and 3,658-byte map |
| `0028-ne-lines` | Exit 0; exact 248-byte stdout, empty stderr, 771-byte NE executable and 3,212-byte map |
| `0029-elf-dll` | Exit 0; exact 255-byte stdout, empty stderr, 32,848-byte ELF image and 3,126-byte map |
| `0030-pe-modules` | Exit 0; exact 222-byte stdout, empty stderr, 4,576-byte PE and 4,290-byte map |

The NT case compares every output byte except the identified four-byte COFF
timestamp and date/time digits in the map header. Its 552 first entries match.
The help reference was explicitly refreshed after correcting the genuine
BurnSystemList/PruneSystemList/CleanSystemList function boundaries; the earlier
raw capture is retained in scratch, not relabeled.

Build and replay from this directory:

```
make -j4 native cases build/WLINK.EXE
make -j4 check unit audit
```

Fixtures use the shared native i386 DSO runner and cached `capture.bin`, without
Wine. Original builds use the shared launcher/prefix, Watcom register ABI
(`-3r`) and disabled added stack checks (`-s`). Native GCC ABI is permitted at
the compatibility boundary. Actual argv, raw command line and the exact masked
self-image/resource overlay are captured separately.

Only the hash-pinned GS-compatible `.patched/WLINK.EXE` is captured; the pristine
binary remains unchanged. The four-site, nine-byte patch is limited to the
proved DS-backed 128-byte resource-reader owners reached by these cases.
Unknown callers remain guarded. Finite CRT restoration retains explicit older
library variants. Native nonlocal jump support is limited to value 1 in a live
Spawn context, without SEH unwinding or expired contexts.

The complete audit passes for 1,114 source-ready identities, their actual graph,
references, ownership and extents, returning-function cleanup and the separate
main-to-exit endpoint. Shared code counts once. SetBreak, ODBISectCleanup,
EndTime and RefGroup have explicitly seeded first-entry exclusions. The latter three are
statically qualified empty shared entries excluded from the acceptance count;
no observed separate invocation is claimed. The source-ready base_32_num sibling
shares a genuine terminal suffix with base_10_num, but its own primary body has
not executed and is also excluded from acceptance. The original first-node AddAddrInfo
sentinel read is preserved on Win32; its documented native-only deterministic
zero choice is limited to the observed invalid sentinel control read.

The DLL case matches 621 first entries. It compares all defined bytes, excluding
only the known COFF timestamp, map clock digits and 25 exact two-byte imported
CodeView segment fields. The original treats live nine-byte elimination edges
as segment leaders, reading beyond their owner for the record format and segment.
Both reconstruction builds choose the observed 16-bit format and write zero for
that invalid segment result; the real CVGetSegment call remains. This is a
bounded original defect, not recovered initialized segment data. Null imported
references and all record lengths, names, offsets and directories stay strict.
The earlier raw output and failed comparisons remain untouched in scratch.

The LX case matches 523 first entries and every DLL byte, including its old
196-byte LX header, page/fixup tables, export bundles and ELF32 DWARF container.
Only the six identified map date/time digit fields are excluded. DWARF-bearing
objects take the original preexisting-debug paths; the still-unreached line
encoder and segment-info helper remain trapped. Source review also corrected
CodeView's genuine previous-offset field, with the existing DLL oracle passing
unchanged.

The NE case matches 543 first entries and every executable byte, including its
64-byte header, segment/entry/name tables and far/import relocation records.
Only the same six map clock digit fields are excluded. Its typed segment flags
retain the old 13-byte layout; communal allocation and the aggregate-argument
entry-ordinal routine retain the original register/stack ABI, including RET 12.
Undefined COMDEF leaf tags remain explicitly trapped in both builds.

The DOS case matches 584 first entries and every executable byte, including the
28-byte MZ header, real overlay-manager code, vectors, overlay table and section
relocations. Only the six identified map clock fields are excluded. Reconstruction
uses the actual 1994 overlay implementation; the later reference source's disabled
overlay entry is inapplicable. Genuine shared terminal code retains both function
identities and contributes bytes once. The case also corrects previously cold
overlay-state offsets and distinguishes the real vector-map heading from a
separate, still-trapped box-12B helper.

The NLM case matches 492 first entries. Its packed 131-byte fixed header and
124-byte extension, code/data images, imported references, exports, dependency
names, Novell debug records and metadata payloads match. The message catalog is
a valid empty catalog with the documented signature and language/count fields.
The original records its message-file offset before adding the custom-data
length; the reconstruction preserves that old ordering in both builds. The
complete NLM matched byte-for-byte on capture day. Regressions exclude only its
identified three date DWORDs and the same six map clock digit fields.

The QNX case matches 511 first entries and every executable byte. Its old packed
48-byte header, six-byte records, group protection/size descriptors, resource
chunks, verification/signature record, selector and linear relocations, EOF and
debug information remain distinct from the later released writer. A data group
provides the defined verification word; the original's uninitialized-output
path without such a group is covered separately by the later QNX16 case. Only the six map clock fields are
excluded. The case also corrects cold QNX relocation flags to their real format
union field and trailing-zero trimming to the group protection flags. Earlier
failed pairs remain in scratch; their genuine originals were reused for fixes.

The Phar Lap case matches 496 first entries and every executable byte, including
the captured DOS stub, 128-byte P3 extended header, 12-byte segment-information
records, relocations, runtime-parameter block, 104-byte task state, 8-byte protected
mode descriptors and complete segment/debug images. Only the six map clock
fields are excluded. The segmented path remains distinct from the separately
qualified MQ/REX and extended-flat writers; the MP simple branch is source-ready
but has not executed in a completed case.

The ELF case matches 486 first entries and every executable byte, including its
52-byte ELF32 header, 32-byte program headers, groups, relocations, imports/exports,
hash chains, symbols, strings and dynamic entries. The old writer advertises
program headers at byte 52 while placing them at the selected alignment; that
defined layout bug is preserved. DWARF line/range generation runs, but this old
ELF writer does not emit the collected debug image. Later OBJAlign, EXPORTAll
and EXTRASections options are unsupported and absent from the case. Only six
map clock fields are excluded. The semantic prime constant has a real data
public so the preceding function's MAP extent ends at its actual boundary.
The import-index helper has a distinct descriptive name to avoid colliding with
the source-known GetELFImport parser under the old linker's case folding.

The extended-flat Phar Lap case matches 502 first entries and every executable
byte. It resolves the real-break symbol from genuine USE16 code alongside USE32
modules, emits the runtime parameters and flat image, and retains archive/debug
and DOS-stub inputs. The real ProcUnpriv store prefix falls into the distinct
ProcPriv constant-return entry; both identities have disjoint extents and the
actual observed order matches without seeding. Only six map clock fields are
excluded. Invalid selector relocations in the first producer were corrected in
the input assembly before freezing the successful case.

The OS/2 NE case matches 559 first entries and every DLL byte, including the
OS/2-specific header flags, segment access policies, communal allocation,
far/import relocations, export ordinal gaps and Watcom debug data. Its type-wide
and named-segment passes use the actual old linear class/segment lists. Only six
map clock digit fields are excluded. Packing options disambiguate the old NE
format; unsupported OSName/version spellings were removed before freezing.
A separate exploratory Microsoft COFF workload was rejected: this original's
IsORL always returns false. That failed baseline remains in scratch and is not
a reconstruction case or a claim of working COFF support.

The DOS COM case matches 489 first entries and every COM/debug byte. The old
writer cuts the actual group buffer and adjusts section file positions directly;
it has no later WriteCOMGroup helper. The parser owns the actual old inline
SYSTEM command block and linear path lists, with five-byte packed path headers.
Only six map clock digit fields are excluded. Trace and verbose modes are
mutually exclusive in the original, so this workload selects real module/symbol
tracing rather than verbose output.

The SYSTEM end callback exposes a distinct three-byte true-return entry at
412164, reached by fallthrough from the successful ProcVerbose prefix. The source
and registry now preserve both identities with disjoint extents and a genuine
edge; no trace seeding suppresses it. Exactly the eleven previously completed
verbose cases were explicitly recaptured, adding that actual entry to their
traces. Every resulting status, stream and artifact was independently checked
against the old oracle with its unchanged exclusions before adoption. Their old
source/capture files and existing raw histories remain in scratch. Help was
unaffected. The eleven necessary correction captures totaled **21.206 s**.

The LX upgrade case matches 552 first entries and every DLL byte. It reads the
actual old 248-byte union header and resident/nonresident name tables, matches
exports through the selected comparator and reinserts them by ordinal. Existing
API_ENTRY/API_ALIAS/API_WAIT ordinals 3/7/11 remain; new API_NEW receives ordinal 1.
The old input DLL is a captured ordinary input. The original keeps the new
unassigned exports' resident flags when assigning old ordinals, so previously
nonresident names become resident here; that defined behavior is preserved.
That LX input does not exercise the separate PE export-table reader; the PE
ordinal-upgrade case below now qualifies it. Only six map clock fields are
excluded in the LX case.

ProcModName's successful prefix falls into the already known ProcLE true entry;
its failed parse branches to the same final C3. The disjoint prefix, displaced
one-byte shared terminal chunk, actual source edge and both identities are
retained. That instruction contributes unique bytes once. No trace seeding or
historical recapture is needed for this newly reached caller.

The final parallel build/regression/unit/audit batch took **1.238 s**. All fourteen
cases and 1,020 source-ready graph/ABI identities pass. The 28 native regression
runs took **17.850–50.969 ms**. The genuine upgrade capture command took
**1,115.731 ms** (owned target **1,101.048 ms**); its exact native pair took
**27.641 ms**, split **16.288 / 4.311 / 7.042 ms** for setup/execution/checks.
The map-clock-derived comparison reused that capture and took **29.778 ms**.
The successful baseline took **615.574 ms**, reusing genuine OS/2 OMF objects,
archive and the prior DLL. Every upgrade DLL byte stays strict. Raw artifacts,
producer inputs, earlier failures and diagnostics remain under
`~/.ai-tmp/msvc2/watcom-shared-integration/`.


The self-contained PE ordinal upgrade matches 488 first entries and preserves
alpha/zebra/omega ordinals 2/7/11, assigning the new API ordinal 1. It reads the
prior ordinary PE DLL's export/name/ordinal arrays through genuine ReadExports
(RET 12), then writes the actual old 192-byte PE header with subsystem 1. Only
the four-byte COFF timestamp and six map clock digit fields are excluded;
every other DLL/map byte, status and stream remains strict. ProcOldPE's actual
eight-byte prefix falls into the separate ProcPE entry, with no overlap or seed.

An earlier large C++ PE-upgrade invocation remains unsupported, not a completed
fixture. Its matched prefixes qualified lazy-symbol resolution, symbol welding
and destructor/operator demangling, but complete replay exposed an allocator
boundary difference: the original avoids LnkReAlloc while native's honest
capacity-shortfall _expand failure takes that fallback. Complete DLL/map bytes
matched after the same clock exclusions. The host backend has no supported
nonmoving growth for that block, so no allocator imitation or trace weakening
was introduced. This input, failed comparisons, raw outputs and frozen DSO/source
remain in scratch under wlink-pe-upgrade. Its uncommitted case working state was
retired there; the distinct successful workload is numbered 0015. The tried
NOCACHE spelling is merely a true-return no-op in this 1994 binary, unlike the
later source implementation; no streaming-policy claim is made.

The final parallel build/regression/unit/audit batch took **1.287 s**. All fifteen
cases and 1,036 source-ready graph/ABI identities pass. The 30 native regression
runs took **17.639–48.075 ms**. The genuine ordinal-upgrade capture command took
**1,876.341 ms** (owned target **1,859.121 ms**); the clock-derived native pair
reused that capture and took **30.295 ms**, split **16.806 / 6.004 / 7.486 ms**
for setup/execution/checks. The clean baseline took **707.998 ms**, and genuine
main/worker assembly took **706.224 / 485.530 ms**. Authoring files, raw artifacts
and diagnostics remain in scratch; case folders contain only source/capture data.


The C++ virtual-dispatch workload matches 656 first entries and complete DLL/map
outputs, excluding only the established four-byte COFF timestamp and six map
clock digit fields. Genuine period C++ objects and an archive exercise restored
COMDATs, selected lines/types/locals/static debug information, global alphabetical
sorting and demangling of internal names, nested templates, arrays and based
encodings. The 1994 anonymous-enum decoder requires its suffix immediately,
unlike the later released source loop. RestoreComdat now checks the actual
LinkState second-pass bit; checking command flags instead overwrote six resolved
COMDAT addresses with relative offsets. The retained original capture exposed
this error, and the corrected native replay matches without extra exclusions.

Although the producer used its real -zv option, these objects contain no P/V/R
virtual-function extension records. ProcVFRemoval is qualified as an option
handler, but VFTable/Reference processing is not. Copy constructors and the pure
Node::value are removed; unused virtual overrides remain. No virtual-override
removal or synthetic-record coverage is claimed.

The final parallel build/regression/unit/audit batch took **1.351 s**. All sixteen
cases and 1,063 source-ready graph/ABI identities pass. The 32 native regression
runs took **18.103–56.627 ms**. The final genuine C++ capture command took
**2,057.686 ms** (owned target **2,017.035 ms**); corrected native replay against
that retained oracle took **60.181 ms**, split **18.355 / 32.908 / 8.917 ms** for
setup/execution/checks. The successful baseline took **519.543 ms**; main/worker/
helper compilation and archive production took **497.620 / 537.919 / 656.351 /
542.296 ms**. All raw outputs, producer inputs and failed comparisons remain in
scratch under wlink-cpp-vf; the case contains only source and capture data.


The dynamic DOS overlay case matches 573 first entries, every MZ/external-overlay
byte and all map bytes except the six established clock digit fields. Genuine
USE16 modules, an archive and the shipped overlay manager exercise dynamic area
allocation, explicit 64 KiB area size, NOINDIRECT, VECTOR/FORCEVECTOR, automatic
section parsing and separate INTO files. The global map uses the authentic
unsigned segment/offset comparator. This complements the prior standard-overlay
workload; it does not qualify the separate short-vector emitter or unexecuted
external-file reopen helper.

The final parallel build/regression/unit/audit batch took **1.361 s**. All seventeen
cases and 1,073 source-ready graph/ABI identities pass. The 34 native regression
runs took **17.798–56.888 ms**. The genuine final capture command took
**1,994.917 ms** (owned target **1,978.188 ms**); its native pair took **29.469 ms**.
The clock-derived replay reused that capture and took **32.157 ms**, split
**17.900 / 6.879 / 7.379 ms** for setup/execution/checks. The clean original baseline
took **418.469 ms**, reusing previously captured genuine objects/archive/manager.
Producer inputs and complete raw artifacts remain in scratch under
wlink-dynamic-overlay; the numbered case remains source plus capture only.


The standard short-overlay case matches 595 first entries, every executable byte
and all map bytes except six established clock digit fields. Its genuine USE16
modules/archive and shipped manager exercise SMALL vectors, fixed library
placement and explicit NOVECTOR/VECTOR/FORCEVECTOR policies. The actual packed
eight-byte record contains near CALL/JMP opcodes, two signed displacement fields
and the section number; both displacement ranges retain their real diagnostics.
This is distinct from the previously qualified long and dynamic vector formats.

The final parallel build/regression/unit/audit batch took **1.406 s**. All eighteen
cases and 1,078 source-ready graph/ABI identities pass. The 36 native regression
runs took **17.740–55.883 ms**. The genuine final capture command took
**1,426.042 ms** (owned target **1,409.869 ms**); its native pair took **27.559 ms**.
The clock-derived replay reused that capture and took **30.001 ms**, split
**16.888 / 5.800 / 7.312 ms** for setup/execution/checks. The clean baseline took
**647.404 ms**, reusing genuine prior objects/archive/manager. Producer inputs,
raw outputs and diagnostics remain under scratch wlink-short-overlay; the case
contains only source and capture data.


The Phar Lap REX case matches 498 first entries and every image byte, including
the real 30-byte MQ header, relocation list, captured DOS stub and debug records.
Only six map clock digit fields are excluded. The packed header keeps the old
page-count/remainder fields, memory limits, entry/stack offsets and relocation
position. Its actual start/header-size arithmetic is preserved. REX remains
distinct from the earlier extended and segmented formats; the writer's MP simple
branch is source-ready but this invocation selects MQ. The rejected first
baseline reused selector relocations from a segmented object; the successful
input instead uses the existing genuine flat-model object, and the failed
input/output remains separately retained.

The final parallel build/regression/unit/audit batch took **4.070 s**. All nineteen
cases and 1,080 source-ready graph/ABI identities pass. The 38 native regression
runs took **17.706–58.463 ms**. The genuine final capture command took
**1,257.190 ms** (owned target **1,245.823 ms**); its native pair took **29.311 ms**.
The clock-derived replay reused that capture and took **34.469 ms**, split
**16.152 / 6.200 / 12.118 ms** for setup/execution/checks. The clean baseline took
**223.787 ms**, reusing genuine objects/archive/stub. Authoring files, failed
baseline and raw artifacts remain in scratch wlink-pharlap-rex; case data is flat.


The OS/2 device-policy case matches 547 first entries and every LX byte. Its real
MODEND assembly entry, code/data/BSS relocations, archive and exports exercise
PHYSDEVICE, single autodata, internal/self-relative relocation generation and
the toggle-relocations flag. The actual old FindOS2ExportSym walks the export list
and allocates the five-byte ordinal import descriptor, unlike the later source's
symbol-owned export shortcut. The resulting non-DLL header has flags 0x20200,
including the physical-device bit. The initial DLL+PHYSDEVICE baseline instead
followed DLL precedence; that earlier input/capture remains separately retained
and is not relabeled as the device result. No device loading/execution is claimed.
Only the six established map clock digit fields are excluded.

The final parallel build/regression/unit/audit batch took **1.444 s**. All twenty
cases and 1,086 source-ready graph/ABI identities pass. The 40 native regression
runs took **17.903–58.458 ms**. The genuine final capture command took
**1,884.849 ms** (owned target **1,870.726 ms**); native replay took **26.566 ms**.
The clock-derived replay reused that capture and took **26.815 ms**, split
**15.555 / 4.790 / 6.469 ms** for setup/execution/checks. The clean non-DLL baseline
took **242.869 ms**, reusing genuine USE32 objects/archive. Raw artifacts, earlier
DLL-shape captures and authoring inputs remain in scratch wlink-os2-driver;
case folders retain only source/capture data.


The TNT symbol-policy case matches 527 first entries and complete diagnostics,
DLL and map bytes, excluding only the known four-byte timestamp and six map clock
fields. Genuine period assembly objects contain an OMF C6 alias, long public
names, repeated initialized data and a duplicate definition; an intentionally
unresolved hook exercises the explicitly enabled permissive policy. Nineteen
application functions qualify, including alias decoding, duplicate/undefined
handling, diagnostic location and argument ordering, name-length/case policy,
TNT/DOSstyle selection and thread settings. The output has the real PL signature
and DOSstyle subsystem 0x42. It deliberately permits an unresolved reference;
no successful loading or execution of that DLL is claimed. The producer emitted
ordinary LEDATA for its repeated data, not synthetic LIDATA/backpatch records.

ProcNoCaseExact's real setter/call prefix falls into the distinct three-byte
ProcNoCache true-return entry. Both identities and their genuine edge are now
preserved without seeding; the retained original already recorded both entries,
so only native correction was needed. NOCACHE still selects no streaming policy.
OMFProcAlias and ProcPubdef share an exact ten-byte terminal epilogue, counted
once while retaining both source bodies and identities.

The old NORELOCS dispatch first tests a mask that includes NT, so this case
actually invokes the QNX setter and retains a 24-byte PE relocation directory.
That defect is preserved; the separate PE setter remains masked/unimplemented
with ABI-only static extent qualification and no execution credit. DOSstyle has
no later version-suffix parser. The memory/font setters are the actual aliases
used by INITTHREAD/TERMTHREAD, with no extra invented wrapper identities.

The final parallel build/regression/unit/audit batch took **1.457 s**. All twenty-one
cases and 1,105 source-ready graph/ABI identities pass. The 42 native regression
runs took **17.996–56.295 ms**. The final genuine capture command took
**2,079.985 ms** (owned target **2,064.250 ms**); clock-derived native replay reused
it and took **28.777 ms**, split **17.345 / 4.782 / 6.649 ms** for setup/execution/
checks. The final supported baseline took **672.128 ms**; main/worker assembly
needed **534.294 / 591.620 ms**. Authoring errors, unsupported runtime-version/
resident spellings, all raw captures and diagnostics remain in scratch
wlink-tnt-symbols; the case retains only source/capture data.


The shared NetWare LAN case matches 501 first entries and complete output. A
captured genuine NLM supplies the shared code/data/fixup/public/debug image;
its exact embedded bytes remain strict while the outer headers rebase the real
source offsets. Referenced/export-only Novell debug filtering and the LAN module
kind are exercised. The response file's caret returns to the parent command line
to read main.obj, then restores its own parser state. Hex/octal escapes and TAB/LF
metadata exercise real token-buffer mutation; the original octal 8/9 admission
bug is preserved. Integer ctype booleans avoid truncating the native library's
nonzero predicate flags; the final retained original caught and verified that
source correction. DISABLE consumes the ordinary diagnostic number and updates
the existing bitset.

Only six map clock digit fields and the outer VeRsIoN# year/month/day DWORDs
206..218 are excluded. The prior embedded NLM's date is captured input and is not
masked. The final parallel build/regression/unit/audit batch took **1.511 s**.
All twenty-two cases and 1,114 source-ready graph/ABI identities pass; the 44
native regression runs took **18.487–58.602 ms**. The final genuine capture
command took **1,064.790 ms** (owned target **1,050.044 ms**); corrected clock-derived
native replay reused that capture and took **31.394 ms**, split **19.153 / 4.855 /
7.385 ms** for setup/execution/checks. The clean baseline took **632.405 ms**, reusing
genuine objects/archive/resources and the fixed shared NLM. Authoring inputs,
raw artifacts and the failed ctype comparison remain in scratch wlink-novell-shared;
the numbered case contains only source and capture data.


The WLIB export-file case matches 550 first entries and complete output. Two
ordinary `.lbc` files provide quoted symbol names, explicit ordinals, an external
name mapped to a different internal symbol, and anonymous exports. The original
locked child-command parser returns to the parent response file. Its 1994 export
allocator owns the copied name, unlike the later released-source interface.
NEWSEGMENT marks the real preceding object; fixed CODE, readonly DATA, alignment
rounding and RWRELOCCHECK exercise the actual NE policies and preserve both
relocation warnings. Eight additional application functions are qualified; no
shared shim or CRT implementation was added.

Only the established six map-header clock digit fields are excluded. Every NE
DLL byte, remaining map byte, diagnostic, stream and exit status is strict.
The final parallel build/regression/unit/audit batch took **1.498 s**. All
twenty-three cases and 1,122 source-ready graph/ABI identities pass; the 46 native
regression runs took **17.973–55.966 ms**. The final genuine capture command
took **2,153.624 ms** (owned target **2,138.034 ms**); clock-derived native replay
reused it and took **26.882 ms**, split **15.464 / 4.597 / 6.820 ms** for setup/
execution/checks. The supported original baseline took **362.512 ms**, reusing
genuine USE16 objects and archive. The initial full check exposed the missing
map-clock comparison rule; offline derivation corrected only that established
rule, without another original run. Raw captures, producer inputs and all
failed setup/check diagnostics remain in scratch wlink-wlib-exports; the case
contains only source and capture data.


The compiler-directive case matches 537 first entries and complete output.
Period WCC386 emits two function EXPDEF comments and one LIDATA record from the
real initialized array. A genuine WLIB member supplies the GetTickCount IMPDEF;
its name is bound explicitly in the producer source, and the small authored
startup resolves the compiler's DLL-entry reference. POSIX selects the actual
PE subsystem value 7. MSExportKeyword retains the old direct ObjBuff interface,
copy/name ownership, IOPL/resident/ordinal fields and real symbol/export edges.
The already reconstructed import and iterated-data readers need no new bodies;
this case does not claim the remaining iterated-COMDAT or alternate debug readers.

The complete-output comparison exposed a prior DWARF tuple-field error:
DwarfGenSegInfo must read the actual leader+0x34 prev_offset, not saved_size at
+0x30. The original assembly establishes that layout. Correcting both tuple
start and size makes the original eight-byte startup range match; no debug-byte
normalization was added and the retained original was reused for native-only
verification. Only the COFF timestamp DWORD at e_lfanew+8=136..140 and six known
map-clock digit fields are excluded.

The final parallel build/regression/unit/audit batch took **1.520 s**. All
twenty-four cases and 1,124 source-ready graph/ABI identities pass; 48 native
regression runs took **17.709–56.132 ms**. The final genuine capture command took
**1,172.380 ms** (owned target **1,161.560 ms**); corrected native replay took
**28.909 ms**, split **16.479 / 5.796 / 6.634 ms** for setup/execution/checks.
The clean original baseline took **643.436 ms**; revised C/assembly production
needed **587.281 / 677.304 ms**, and WLIB response-file production **483.803 ms**.
An earlier direct import-production invocation was stopped after 44.706 s;
its actual failure and the earlier unresolved-symbol baseline remain retained
in scratch wlink-omf-directives with all authoring inputs/raw artifacts. No
original was repeated to diagnose or fix the DWARF mismatch. The numbered case
contains only source and capture data.


The OMF-record case matches 567 first entries and complete output. Its supplement
is explicitly format-authored, not represented as compiler output: 22 checksummed
records describe real code/data publics, table/method relocations, conditional
virtual-table metadata, nested iterated implicit COMDAT data and segment/named
additive backpatches. Genuine USE32 startup objects and an archive complete the
NAM link. The original output contains the independently calculated patched
DWORD 0x12132526 at file offset 482 and the exact 20-byte nested expansion with
its named DWORD addition at offset 522. Nineteen additional application bodies
are qualified, including recursive length counting, conditional graph edges,
implicit COMDAT naming/grouping/restoration and backpatch storage. The existing
qualified base36 conversion supplies COMDAT_SEG names; no shared edit was needed.
The table/reference helpers retain their real shared terminal C3 and distinct
identities, counting that instruction only once. That case does not exercise virtual-list generation or weak definitions; the
following conditional-symbol case qualifies those paths.

Complete-output comparison exposed an original debug-output defect. Its module
region reserves 131 bytes, but four structurally complete name records consume
126; the remaining five malloc-backed bytes are emitted unwritten. Both builds
now zero only the bounded unused module tail before emission. Comparison excludes
only the actual original tail1792..1797, in addition to the six established map
clock fields and outer VeRsIoN# year/month/day DWORDs224..236. Every complete name,
record, index, header, executable/relocation byte and patched data byte is strict.
No allocator tuning or diagnostic original was used; retained original/raw data
verified the native correction.

The final parallel build/regression/unit/audit batch took **1.586 s**. All
twenty-five cases and 1,143 source-ready graph/ABI identities pass; 50 native
regression runs took **18.022–58.513 ms**. The final genuine capture command took
**3,041.557 ms** (owned target **1,612.559 ms**); corrected native replay reused it
and took **29.610 ms**, split **18.980 / 3.648 / 6.981 ms** for setup/execution/checks.
The clean supported original baseline took **591.340 ms**. The explicit generator,
field manifest, semantic patch assertions, source review, failed comparison and
raw artifacts remain in scratch wlink-omf-records. The numbered case contains only
source and capture data.


The conditional-OMF case matches 581 first entries and complete output. Its
explicitly format-authored supplement provides real default/method relocations,
consistent repeated V records, a pure P record and an R reference. A later
module defines the conditional class token; the non-pure reference becomes a
regular library search and extracts an actual vtable member, while the pure
reference becomes lazy. Nine new bodies include list rewind, weak creation,
conditional conversion, DSK selection and Novell export/import-comment readers.
The export records are genuine WCC386 output and the import member is produced
by WLIB. This does not claim compiler-produced V/P/R records.

Only the six established map-clock fields and the outer VeRsIoN# year/month/day
DWORDs 227..239 are excluded; the actual signature is at 207. All other image,
debug, relocation, symbol and map bytes remain strict. The existing bounded
module-tail zero correction needs no additional comparison exclusion here.
The final parallel build/regression/unit/audit batch took **1.588 s**; all
twenty-six cases and 1,152 source-ready graph/ABI identities pass. The 52 native
regression runs took **17.825–58.409 ms**. Final original capture took **1,428.501 ms**
(owned target **1,413.299 ms**); replay with the final clock policy took
**29.819 ms**, split **17.687 / 5.046 / 7.087 ms** for setup/execution/checks.

The successful original baseline took **506.467 ms**. C compilation/import
production took **603.171 / 599.941 ms**; the real vtable archive took
**299.910 ms**. Earlier missing-real-vtable baselines remain failed in scratch.
A long module name also exposed the original fixed 13-byte NLM-name storage; the
valid workload uses cond.dsk. The hyphenated WLIB member invocation stalled and was stopped after 48.824 s;
the distinct realvt.obj producer
then succeeded. All authored source, record manifests, commands, raw artifacts
and failed setup results remain in scratch wlink-omf-conditional. The numbered
case contains only source and capture data.


The QNX16 case matches 507 first entries and complete output. Genuine WASM
-fpi output supplies the FIWRQQ/FIDRQQ x87 fixups; the shipped NOEMU87 archive
provides their actual absolute marker definitions, consistent with the released
emustub/initemuq source. The resulting image has four real six-byte floating
loader relocations. A valid explicitly format-authored OMF SEGDEF describes the
absolute hardware segment; WASM10 rejects SEGMENT AT, so no assembler origin is
claimed for that one module. The verbose map lists the absolute segment and
public. Explicit helpers.lib(helper) input exercises alignment and actual member
header/name matching, without substituting an on-demand library search.
Eight new application bodies also cover execute-only CODE, NOREDEFSOK and quiet.
The aliased constant-zero ORL member/provider branch remains trapped/unqualified.

The retained original exposed a previously guarded output defect: with only
eight initialized data bytes, no eligible data group initializes rw_end.verify.
The original nevertheless emits that stack word in its type 7 six-byte payload.
Both reconstruction builds now emit zero for only that undefined field; normal
initialized verification still uses the original XORffff. Comparison excludes
only the structurally validated original bytes 88..90, plus the six established
map-clock fields. The type 7 header, following signature DWORD, all floating
relocation records and every other executable/debug/map byte remain strict.
No extra original capture or input padding was used to correct the mismatch.

The final parallel build/regression/unit/audit batch took **2.843 s**; all
twenty-seven cases and 1,160 source-ready graph/ABI identities pass. The 54 native
regression runs took **17.647–57.193 ms**. Final original capture took
**1,968.960 ms** (owned target **1,952.724 ms**); corrected native replay reused it
and took **26.566 ms**, split **16.684 / 3.541 / 6.342 ms** for setup/execution/
checks. Successful ordinary baseline took **558.398 ms**; assembler production
needed **382.457 / 486.403 / 456.603 ms**, archive production **578.646 ms**.
Earlier automatic-format/absent-marker attempts remain failed: the supported
workload uses an explicit local QNX initialization file and the real marker
library, and does not claim ExecSystem execution. Authoring, diagnostics,
input provenance and raw artifacts remain in scratch wlink-qnx-float; the
numbered case contains only source and capture data.


The NE-line case matches 514 first entries and every executable/debug byte.
Genuine WASM startup/archive inputs define the reserved BSS bounds in the entry
module. Explicitly authored OMF LINNUM16/32 records name real MOV instruction
offsets 0/4/8 in valid USE16 segments, deliberately arrive out of order, and
produce the exact sorted line tables. The header uses autodata/SS 2 with an
explicit 4096-byte stack and no physical STACK input segment. Five new bodies
cover brace fences, startup-module BSS ownership, implicit-stack policy and
both line comparators. CheckFence/EatWhite share only their actual POP EDX/RET;
CheckBSSInStart/DefABSSSym share only POP ECX/POP EBX/RET. Both identities/edges
remain intact and shared bytes count once; no trace seeding was added.
Only the six known map-clock fields are excluded.

Full parallel build/regression/unit/audit took **2.841 s**: all twenty-eight
cases and 1,165 source-ready graph/ABI identities pass. The 56 native runs took
**17.621–56.774 ms**. Final original capture took **2,028.731 ms** (owned target
**2,015.660 ms**); final clock-adjusted native replay took **27.959 ms**, split
**17.308 / 3.596 / 7.055 ms** for setup/execution/checks. The clean original
baseline took **666.892 ms**; assembly/archive production took **445.609 /
472.416 / 580.266 ms**. All authoring, manifests, diagnostics and raw artifacts
remain in scratch wlink-ne-lines; only source/capture data are in the case.


The ELF-DLL case matches 507 first entries and complete output, qualifying
ProcELFDLL and the real saved-line-size addition in DwarfAddModLines under
dead-code selection. Inputs reuse genuine USE32 assembly objects and an archive
with line records, imports/exports and a dependency module. The old writer
announces a DLL but still emits ET_EXEC 2; that actual 1994 behavior remains strict,
without a later shared-object ABI claim. Only six known map-clock fields are
excluded; every ELF/code/relocation/debug byte is checked.

Full parallel build/regression/unit/audit took **2.670 s**; all twenty-nine cases
and 1,167 source-ready graph/ABI identities pass. The 58 native regression runs
took **18.471–56.220 ms**. Final original capture took **1,173.228 ms** (owned
target **1,159.992 ms**); clock-adjusted replay took **34.228 ms**, split
**18.032 / 8.877 / 7.319 ms** for setup/execution/checks. The clean original
baseline took **580.618 ms**, with existing producer bytes reused rather than
recompiled. Authoring/raw diagnostics remain in scratch wlink-elf-dll; the case
contains only source and capture data.


The many-module PE case matches 501 first entries and complete output. Sixteen
genuine assembler functions are called by startup and referenced by a real
data pointer table. Opening seventeen input objects crosses the normal twelve-
handle threshold: CleanCachedHandles closes an inactive cached handle, clears
its saved position and leaves it ready for real reopening. No fd/malloc limits,
private allocator state or captured API responses force that outcome. The
subsystem is the actual OS/2 character value 5; the 40-byte relocation directory
remains present. Two new application bodies are qualified.

The old runtime handler accepts `runtime os2`, without the later version suffix.
There is no English entry in its language table, and the inherited NORELOCS
option does not reach the PE-only helper; neither unsupported feature is claimed.
Only the actual four-byte COFF timestamp at e_lfanew 112+8=120..124 and six map-
clock fields are excluded. All other image/debug/relocation bytes are strict.

Full parallel build/regression/unit/audit took **2.535 s**; all thirty cases and
1,169 source-ready graph/ABI identities pass. The 60 native runs took
**17.695–58.539 ms**. Final original capture took **1,818.211 ms** (owned target
**1,499.248 ms**); final clock-adjusted replay reused it and took **26.952 ms**,
split **15.406 / 5.048 / 6.497 ms** for setup/execution/checks. The clean baseline
took **576.446 ms**. Sixteen helper assemblies took **170.146–382.753 ms** each
with four producer jobs; main took **655.324 ms** after splitting declarations
and pointer entries into ordinary short lines. The earlier long-line assembler
invocation timed out after 30 s and its owned process was killed/waited; the
failed runtime-version baseline and first raw timestamp mismatch are preserved.
Authoring, commands and raw artifacts remain in scratch wlink-pe-modules; the
case contains only source and capture data.
