WLINK 10.0a has forty-four completed paired CLI cases and eight source-backed
native unit cases. The acceptance ledger contains **1,258 application functions /
137,644 unique original code bytes**. Paired CLI evidence covers **1,182 /
131,924 bytes**; **76** further functions have source/static/native-unit
qualification only, owning **5,734 bytes**. Those add **5,720** unique bytes;
14 bytes are genuinely shared with already qualified CLI bodies and count once.
Unit-only evidence is not claimed as observed original CLI execution.
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
Eight further cases cover the terminal format-help selectors, one actual
invocation per format. Two OS/2 policy cases add a fullscreen NE executable
with manual data/explicit stack and a non-DLL LX virtual-device image.

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
| `0031-help-dos` | Exit 0; exact 2,620-byte stdout, empty stderr, no artifacts |
| `0032-help-os2` | Exit 0; exact 3,384-byte stdout, empty stderr, no artifacts |
| `0033-help-windows` | Exit 0; exact 3,091-byte stdout, empty stderr, no artifacts |
| `0034-help-nt` | Exit 0; exact 3,082-byte stdout, empty stderr, no artifacts |
| `0035-help-pharlap` | Exit 0; exact 2,393-byte stdout, empty stderr, no artifacts |
| `0036-help-novell` | Exit 0; exact 2,783-byte stdout, empty stderr, no artifacts |
| `0037-help-qnx` | Exit 0; exact 2,502-byte stdout, empty stderr, no artifacts |
| `0038-help-elf` | Exit 0; exact 2,270-byte stdout, empty stderr, no artifacts |
| `0039-os2-fullscreen` | Exit 0; exact 246-byte stdout, empty stderr, 615-byte image and 2,983-byte map |
| `0040-os2-virtual-device` | Exit 0; exact 262-byte stdout, empty stderr, 1,644-byte image and 3,325-byte map |
| `0041-resource-units` | Native-only typed resource assertions; no original execution/oracle claim |
| `0042-cache-spill-units` | Native-only cache/dictionary/spill ownership assertions; no original execution/oracle claim |
| `0043-stdin-overlay-trace` | Exit 0; exact 294-byte stdout, empty stderr, 10,395-byte MZ, 6,516-byte map and 48/32-byte overlays |
| `0044-policy-units` | Native-only language/relocation-policy/disabled-ORL assertions; no original execution claim |
| `0045-debug-name-units` | Native-only descriptor/debug-buffer/empty-callback assertions; no original execution claim |

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

The complete audit passes for 1,258 source-ready identities, their actual graph,
references, ownership and extents, returning-function cleanup and the separate
main-to-exit endpoint. Shared code counts once. SetBreak, ODBISectCleanup,
EndTime and RefGroup have explicitly seeded first-entry exclusions in the CLI
references. The latter three now have direct native-unit qualification only;
no observed separate original invocation is claimed. The base_32_num sibling's
primary body also has native-unit qualification, while its terminal suffix is
genuinely shared with base_10_num. TempFileOpen now has native-unit qualification
for real existing-file opens; the spill first-open environment/collision branch
is still unqualified.
The original first-node AddAddrInfo
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


The format-help family qualifies all eight actual wrappers. Each terminates
the process after its selector, so DOS, OS/2, Windows, NT, Phar Lap, NetWare,
QNX and ELF remain distinct cached CLI cases rather than a fabricated combined
trace. Each matches 115 first entries, exact status/streams and empty artifact
inventory. The source preserves each real resource-ID range, WriteGenHelp and
WriteHelp edges, and console/pagination flag. No output normalization or new
resource-reader owner was needed.

One family build took **1.276 s**, its scoped graph/ABI check **352.676 ms**.
The final full build/regression/unit/audit took **1.960 s**; all thirty-eight
cases and 1,177 source-ready graph/ABI identities pass. The 76 native runs took
**17.765–58.340 ms**. Final selector captures took **763.386–3,362.126 ms** per
command (**754.405–1,753.715 ms** owned execution); native comparisons took
**24.010–28.839 ms**. Individual setup/execution/check splits, raw streams,
artifacts and commands remain in scratch wlink-format-help. Only the actual
selector invoked to completion gains ledger credit; the seven other prepared
wrappers remained explicitly unaccepted until their own comparisons passed.
All eight case folders contain only source and capture data.


The paired OS/2 policy batch qualifies fullscreen, NOAUTODATA and virtual-device
handlers through separate genuine invocations. The NE executable supplies its own
DGROUP setup, explicit 4,096-byte stack, a real archive helper and named far
DOSCALLS import. The emitted NE header has no automatic data segment. The LX
non-DLL virtual-device variant preserves the existing real object/archive and
relocation workload. These are linker-output tests; neither emitted OS/2 image
is executed. They match 526 and 547 first entries respectively, complete executable
bytes, streams and status. Only the six established map-clock digit fields are
excluded; no new resource-reader owner or trace exception is introduced.

The necessary final family build took **1.506 s**, its scoped graph/ABI check
**352.670 ms**. The complete forty-case regression/unit/audit pass took
**1.826 s**, with all 1,180 source-ready graph/ABI identities passing; its 80
native runs took **17.799–56.236 ms**. Final original captures took
**1,914.190 / 2,084.880 ms** (owned execution **1,899.847 / 2,069.726 ms**).
Clock-adjusted native replays reused those originals and took **28.723 / 29.675 ms**,
split **17.360 / 4.824 / 6.537 ms** and **17.774 / 4.772 / 7.128 ms** for
setup/execution/checks. The fullscreen assembly took **662.561 ms**; clean original
baselines took **622.109 / 483.564 ms**. Earlier exact-clock regression failures
and untouched originals remain in scratch wlink-os2-fullscreen, alongside the
explicit offline comparison derivations. The intermediate family ledger was
corrected to exclude the prepared virtual-device body until its separate actual
execution passed; no original observation was relabeled. Both new case folders
contain only source and capture data.


The combined resource-version unit case qualifies valid named WRes v2 and Win16
Microsoft resource files that the linker's immutable self-resource input cannot
select through ordinary CLI arguments. Typed tests use real open/read/seek/close
and the genuine WResRtns allocation/free owners. They verify fixed record bytes,
counted-name continuation and case folding, exact file positions, directory/type/
resource/language counts and links, language lookup, duplicate-resource handling,
payload bytes and complete directory cleanup. The error hook is the actual typed
one-byte RET and acquires no fabricated error side effects. There is no original
DIRECT controller, saved original trace, provider response replay or CRT credit.
Allocation failure, malformed records and other unknown error domains remain
unqualified. Existing valid authored resource bytes and source algorithms from
WASM were reused only after checking WLINK's own packed layouts, body boundaries,
control edges and original ABI; WResAddResource retains its real RET16 cleanup.

The 23 unit-only identities are WresRecordError, WResReadFixedResRecord,
WResReadExtraWResID, WResIDNameCmp, readLangInfoList, readMResDir,
MResReadResourceHeader, MResFreeResourceHeader, WResIDFromNameOrOrd,
WResIDFromNum, WResIDFromStr, newTypeNode, newResNode, newLangNode,
WResAddResource, ResReadUint8, ResReadUint16, ResReadNameOrOrdinal,
VarStringStart, VarStringAddChar, ComputeVarStringLen, VarStringEnd and
ResReadString. Their ordinary CLI reach is not established by these native tests.
The shared resource declarations now live in src/resource.h with exact original
packed owners, rather than duplicating private layouts in test glue. The actual
inline strlen instruction sequence required the period compiler's strlen
intrinsic setting; the initial extra-call audit finding is recorded in scratch.

Final full build/regression/unit/audit took **1.878 s**; all forty CLI cases and the
resource DIRECT case pass, including four focused valid-file/state tests and
1,203 source-ready graph/ABI identities. The 82 native invocations took
**14.535–58.819 ms**. The initial finite build took **1.384 s**; the MRes closure build
**1.347 s**, and the required strlen graph correction build **1.463 s**. Its scoped
23-function graph/ABI check took **387.707 ms**. The final standalone resource DIRECT
run took **17.639 ms** (setup/execution/checks **11.281 / 2.259 / 4.099 ms**), and
its four-test run **19.567 ms** (**8.819 / 6.605 / 4.143 ms**). No new original
application invocation or Wine fixture was used. Commands, static analysis and
explicit split acceptance ledger remain in scratch wlink-resource-units; the
case contains only case.c, direct.c and capture.bin.


The cache/spill unit family adds fifteen unit-only App identities: CacheRelease,
FreeUpMemory, SwapOutVirt, SwapOutRelocs, SpillAlloc, SpillWrite, SpillRead,
QDelete, DumpObjCache, DiscardDicts, PermShrink, SpillRelocList, SpillSections,
SpillAreas and MakeTempName. The actual non-paging SwapOutVirt is only a three-
byte false return, not the later released virtual pager. The original call graph,
TmpFSize sector alignment, real file cursor effects and relocation ownership
transitions are preserved. PermShrink uses the existing exact shared-epilogue
rule for ShrinkBlock's C3; that physical byte contributes no second coverage byte.
The five tests use genuine PermAlloc-owned empty or exactly-full 16,384-byte
arenas with no resize, AllocFileEntry/CacheOpen/CacheRead-created object cache,
a typed malloc-owned OMF dictionary vector/chunk, a pre-opened owned spill file,
and a typed nested overlay section/relocation tree. Assertions check zero reclaim
where appropriate, preserved arena bytes, cache/dictionary pointer clearing,
512-byte allocation token alignment, complete roundtrip bytes and canaries,
relocation spill flags/token/size, repeated exhaustion, and real close/deletion.
No allocator/fd limit or allocation failure is injected, and no stale pointer is
used across a possibly moving resize.

SpillAlloc's actual first-open logic is inlined, unlike the newer OpenTempFile
helper, so no synthetic helper edge is introduced. Its real 13-byte TempFileOpen
wrapper remains source-ready/unaccepted; env/collision/first-open/error behavior,
successful arena shrinking and unknown allocation/resize outcomes are outside
this unit's qualification. Only the finite public remove_ trampoline identity is
added for static graph naming, correlated with the shipped remove member and
initial remove.c; no CRT instructions are restored or counted. Current tests use
the already supported real native successful deletion of an owned relative file.
The first relocation unit failed because a new source mask misread the actual
16-bit LinkState location; correction to its genuine 0x40 format-decided flag
then passed native-only tests. Failed results and GDB diagnostics remain in scratch.

The initial necessary native/period build took **3.397 s**; the state-width
correction build **1.141 s** and scoped sixteen-function graph/ABI check
**386.440 ms**. Five corrected native tests took **19.702 ms**, split
**7.341 / 8.420 / 3.941 ms** for setup/execution/checks. The final full
build/regression/unit/audit took **2.025 s**, with forty CLI cases, both unit
cases and all 1,219 source-ready graph/ABI identities passing. The 84 native
invocations took **16.486–56.239 ms**. No original application run or Wine
fixture was needed. Authoring/analysis/diagnostics and the exact union ledger
remain in scratch wlink-cache-spill-units; the case contains only case.c,
direct.c and capture.bin.


The stdin-overlay case supplies complete real directive lines through a regular
stdin file with an empty command tail, reusing the genuine assembler/archive/
WOVL-manager inputs. A trace of ovla.obj exercises the actual overlay-section
CheckFileTrace callback; the original still intentionally excludes Root from
that walk. OutPutPrompt and QReadStr preserve the device test, real reads,
CR suppression, LF/NUL termination and EOF behavior. The valid redirected-file
case does not claim console-device prompting. CheckFileTrace retains its actual
16-bit 0x0400 module-trace flag and the older missing name-free edge, rather than
importing the later source's graph. Three new CLI-qualified App bodies complete
with 569 matching first entries and exact executable/overlay bytes, streams and
status. Only the six established map-clock digit fields are excluded.

The original rejects VERBOSE together with module tracing; the first failed
input/output remains separately preserved. The valid baseline explicitly uses
regular-file stdin and took **525.515 ms**. A full comparison then detected a
wrong new field mask before acceptance: 4 affected the low byte, whereas the
actual OR instruction updates the high byte of the status word. Correcting to
0x0400 and reusing the retained original restored the genuine TraceSymList
entry and complete output match. No synthetic entry, trace seeding or diagnostic
original replay was used.

The stdin-reader build/scoped check took **1.819 s / 389.590 ms**, and trace
callback build/check **1.080 s / 380.922 ms**. The necessary field-width correction
build took **1.554 s**. Final original capture took **1,140.877 ms** (owned
execution **1,129.913 ms**); corrected native comparison took **33.751 ms**,
split **19.130 / 6.612 / 8.009 ms** for setup/execution/checks. Final full
build/regression/unit/audit took **1.920 s**, passing forty-one CLI cases, both
unit cases and all 1,222 source-ready graph/ABI identities; its 86 native
invocations took **16.409–56.571 ms**. Authoring, exact stdin inputs, failed
comparisons and raw artifact files remain in scratch wlink-stdin-overlays.
The new case contains only case.c and capture.bin.


Two further source/static/native-unit cases qualify fourteen helpers without
claiming new CLI reach. The policy case checks ProcPENoRelocs's actual bit clear,
which the old ordinary NORELOCS dispatcher shadows. Real NewCommandSource-owned
ASCII buffers select Japanese, Chinese and Korean, preserving unrelated flags
and releasing parser owners immediately afterward. This qualifies ProcLanguage
and its three real table/flag handlers; it stops before any subsequent multibyte
tokenization, which remains unsupported. The original disabled IsORL, ORLPass1
and ORLPass2 entries are false/zero/no-op glue, not COFF reader functionality.
Their genuine shared final C3 retains both graph identities with one-byte union
ownership; no synthetic original entry is supplied.

The debug/name case checks forceSuppression and its existing inverse against
whole typed descriptor snapshots, and directly executes base_32_num on small
valid numbers with exact decoded values and input cursor bounds. ZeroAddrInfo
uses real DBIAlloc/AllocStg ownership for its14-byte empty address header/entry;
all bytes, descriptor fields, cursor bounds and adjacent canaries are asserted,
then FreeVirtMem releases the allocation. Actual calls to the already reconstructed
empty EndTime, RefGroup and ODBISectCleanup preserve typed owners/control state
and upgrade only their native-unit evidence. Their original CLI first-entry
exclusions and absence of separate observed-original invocation claims remain.

The family native/period build took **1.777 s**, and its scoped fourteen-function
graph/ABI check **391.527 ms**. Policy and debug/name DIRECT runs took
**19.504 / 17.382 ms**; their three-test runs took **14.229 / 19.904 ms**,
with setup/execution/check splits retained in scratch wlink-helper-units.
The full build/regression/unit/audit took **1.960 s**, passing forty-one CLI
cases, four unit cases and all 1,232 source-ready graph/ABI identities. Its 90
native invocations took **14.333–58.680 ms**. No original application or provider
run was used. Each new case contains only case.c, direct.c and capture.bin.


The file-reopen native unit qualifies TempFileOpen, the genuine 66-byte ExeOpen
wrapper, and OpenOvlFile's normal owned-file transitions. Existing binary data
includes NUL, CRLF, control-Z and high bytes; independent descriptors preserve
separate cursors. A real outfilelist root, closed node, open overlay and target
prove that OpenOvlFile closes and marks the old handle -1, reopens an existing
target at its nonzero saved cursor, and preserves prefix/suffix bytes around a
continued binary write. The separate zero-position branch genuinely truncates
through ExeCreate. Source-state GDB assertions verify OpenFiles is 1 before and
after each transition, then 0 after cleanup. No exhaustion or error is injected.
PrintIOError remains masked/unimplemented; its complete original 34-byte body
supplies only static ABI facts for the cold graph edge, with no execution credit.

The first build caught a case-side QPos prototype error; the corrected necessary
native/period build took **4.601 s**, with a **394.764 ms** scoped graph/ABI check.
The DIRECT run took **19.541 ms** (setup/execution/checks **13.786 / 1.650 /
4.105 ms**); its three tests took **19.877 ms** (**8.558 / 7.342 / 3.977 ms**).
Final full build/regression/unit/audit took **1.985 s**, passing forty-one CLI
and five native-unit cases and all 1,234 source-ready graph/ABI identities.
Its 92 native invocations took **14.262–56.274 ms**. No original application
run was used. Exact adoption bindings, diagnostics and the generated split
acceptance ledger remain in scratch wlink-reopen-units; the case contains only
case.c, direct.c and capture.bin.

Three completed diagnostic invocations exercise real missing FORMAT operands,
a well-framed SEGDEF32 with an invalid big attribute, and a missing readonly
object. They qualify Syntax, BadObject and PrintIOError through genuine original
exit 1 and exact entry/status/stream/artifact matches, without normalization.
The missing-file diagnostic uses the target errno 1 string through the explicit
io.c-only RECON_WATCOM_STRERROR_ENOENT shim opt-in; other translation units keep
their former host surface. Only the positively identified 23-byte CRT strerror
provider and reached literal are additionally restored, with no App credit.

The original short-record/truncated and absolute-alignment malformed shapes
reached BadObject, but subsequent original runs hit the broker's 30-second
operation-7 deadline. Native-only GDB identified a repeated record: attribute
0x02 selects absolute alignment and advances the cursor past the short record,
then the old outer loader repeats the failed location. Those failed inputs,
raw artifacts, timeout and explicitly revised native diagnostic remain in
scratch; none is a completed oracle. The final diagnostic instead uses 0x22
(alignment 1 plus the invalid big bit) and a real MODEND. It does not claim the
failed shapes or EarlyEOF's NULL-read branch were observed successfully.

A final source/static/native-unit case qualifies ExecSystem, EarlyEOF and
TrapBreak. ExecSystem runs a real ProcSystem-created SYSTEM directive list
(OPTION QUIET and OSNAME), then genuinely frees its SYSTEM/command-source owners;
it is not host process spawning. The exact older eight-row directive table and
RestoreCmdLine error edge are preserved. EarlyEOF receives valid module/file
owners with message 29 disabled through actual GetToken/AddDisable policy;
whole owner snapshots and GDB prove its IOERR bit and Locator filename/NULL
memory/record-0 effects. The similarly message-disabled TrapBreak callback and
CheckBreak prove CaughtBreak 0→1→0. These units claim no diagnostic text, short
read, console delivery or original CLI observation. Initial unit setup omitted
GetToken before AddDisable; that failed native attempt is retained, and the
public parser sequence was corrected before qualification.

Final diagnostic original/native times were **1,595.840 / 24.111 ms** (126
entries), **1,593.206 / 22.093 ms** (195), and **1,177.075 / 25.454 ms** (161).
The initial necessary diagnostic build took **4.598 s**. Final-unit builds took
**2.337 s** and **1.707 s**, with a **394.042 ms** scoped graph/ABI check; the
first check correctly rejected an incorrectly identified cold call before its
RestoreCmdLine correction. Native unit DIRECT/tests took **20.363 / 14.418 ms**;
setup/execution/check splits and actual GDB state assertions remain in scratch.
The final full build/regression/unit/audit took **5.385 s**, passing forty-four
CLI and six unit cases and all 1,240 source-ready graph/ABI identities. Its 100
native invocations took **14.258–58.524 ms**. Cases contain only their standard
source and capture.bin; diagnostics and generated ledgers remain in scratch
wlink-diagnostics and wlink-final-units.

All currently identified App identities are now qualified in their recorded
CLI or native-unit domains. This is not a whole-code only-CRT claim: the bounded
existing-registry assessment leaves **21,183 bytes** in the code-mask envelope
unattributed, including padding/data, unknown CRT and potentially further App.
The existing full audit resolves every named graph edge to a registered identity;
its remaining generic indirect marker does not identify a new helper or prove
all possible callback destinations. No positively identified App callee is
being discarded as CRT by elimination. Explicit guarded domains remain:
multibyte lexing, wildcard recovery with an undefined result, invalid communal
number tags, and PE layouts missing required code/import or nonempty group
owners. These residuals and failed malformed-input behavior remain unqualified.


Source/reference-led gap review found additional SDK application writers outside
the earlier reachable graph. The resource writer unit qualifies fifteen genuine
entries from DefaultConversion through MResWriteResourceHeader, preserving exact
1994 extents and excluding intervening padding. It checks scalar/identifier
records, both named and numeric directory records, language records, header
seek/overwrite and untouched file tails, string/name/ordinal forms and the old
Win16 MRes header using real WResRtns callbacks and files. The expected bytes are
format-derived, with exact cursors, input/canary preservation and cleanup.

DefaultConversion is the actual byte-to-two-byte zero-extension loop, including
high bytes and no-output sizing. Both static-buffer strings and a real allocated
602-byte conversion are covered; no host encoding/general Unicode conversion
is claimed. The old signed-int work count/low-word file length and the ignored
second ordinal-writer error result are preserved. ResWriteString owns its real
conversion/write/free graph, without the later ResWriteStringLen helper or MRes
Win32-header branch. Only normal valid owned inputs and successful real file
operations have unit qualification; failure/malformed domains remain separate.

The original/native build took **4.347 s**, scoped fifteen-function graph/ABI check
**379.116 ms**, and DIRECT/four-test runs **18.634 / 19.579 ms**.
Final full build/regression/unit/audit, including the explicit low-word pointer
typing cleanup, took **2.861 s**, passing forty-four CLI and seven unit cases
and all 1,255 source-ready identities. Its 102 native invocations took
**14.130–59.141 ms**. Source/body/callback correlations, timing and the
split ledger remain in scratch wlink-gap-source. No original was launched.
The earlier no-known-pending conclusion was bounded to that registry: this
source-led review continues, including positively identified demangler and
module-reset routines. The remaining gaps are not classified as CRT by absence.


The next source-led unit qualifies the actual __demangled_basename,
__parse_mangled_name and __mangle_operator public helpers. Detailed original
argument/flag flow identifies these entries; the similarly named newer
__demangle_r and __demangle_t APIs are not substituted. Basename tests cover
ordinary/global names, constructors/destructors, explicit input length and a
bounded output buffer. The old NULL-output mode deliberately retains its
inconsistent count: foo sizes as four because zapSpace cannot inspect/remove
its trailing blank, although the real buffer result is foo/three. The tests
also preserve declared-capacity canaries without imposing an unsupported promise
that all intermediate working bytes after the final NUL stay unchanged.

The callback parser checks the exact sixteen-event stream for a simple mangled
variable, identifier pointer/value and character-encoding values, plus its
single plain-copy event. Actual callback updates of the live cookie pointer
must reach subsequent calls through the same slot. Operator tests cover all
three existing tables, explicit-length exact matches, failed lookup and exact
three-byte nonterminated output/canaries. Its only new runtime edge is the
positively identified 37-byte strncmp CRT variant, distinct from the shipped
41-byte member. The application consumes equality only, covered by native
strncmp; no CRT body is reconstructed and no original is launched.

The necessary native/period build took **1.241 s**, scoped graph/ABI check
**380.519 ms**, and DIRECT/final three-test runs **20.046 / 17.690 ms**.
Earlier rejected test assumptions and native GDB findings remain in scratch
wlink-demangler-units. Final full build/regression/unit/audit took
**2.140 s**, passing forty-four CLI and eight unit cases, with
1,258 source-ready graph/ABI identities. Its 104 native invocations took
**14.157–58.664 ms**. The resource writer's stale peer-binary provenance
comment is corrected to actual WLINK addresses. Source-led gap assessment
continues; no conclusion that the remaining bytes are all CRT is drawn.


The module-boundary source review next identifies nine standalone old subsystem
reset entries. These are source/static/native-unit qualifications, with no new
startup calls or original-execution claim. ResetComdef restores the full 19-byte
name, four leader pointers, saved-segment head and counter, but clears only the
first 20-byte saved node; its second node and alternating index stay unchanged.
ResetCmdAll includes the old map/name/system-list fields, ResetObjIO combines
cached lists with OMF cursors, and ResetObjPass1 also clears the saved-public and
segment-mask fields. ResetDistrib, ResetMsg, ResetObjStrip and ResetSymTrace
preserve the exact old field sets. ResetMsg clears the banner/index but leaves
argument payloads and the enable bitmap unchanged. ResetODBI is a descriptive
name for the separate old debug-header initialization entry: it resets the
six leading header bytes/language state while preserving eight output bytes.
The later ODBIInit source correlates this role, without supplying the old name.

The unit retains independent aliases to borrowed stack and real heap owners,
checks their bytes/canaries, and cleans them up explicitly after reset forgets
the pointers; no free/shutdown semantics are inferred. COMDAT names restart
without corrupting earlier allocated names, and repeated empty resets are
idempotent. Seventeen native GDB assertions verify private fields and untouched
neighbors in addition to the two ordinary unit tests. Original spans exclude
surrounding module padding. The necessary native/Watcom build took **7.002 s**,
selected graph/ABI/ownership checks **387.523 ms**,
and DIRECT/tests **16.606 / 12.174 ms**; setup/execution/check splits are retained
in scratch wlink-reset-units. Final full build/regression/unit/audit took
**2.076 s**, passing forty-four CLI and nine unit cases and all **1,267**
source-ready graph/ABI identities. Its 106 native invocations took
**12.241–56.671 ms**. The generated ledger records
**138,023** qualified unique App bytes, with CLI/unit provenance kept separate.

Fresh registry subtraction leaves **19,280** mask-envelope bytes
unattributed. This is a derived remainder, not a classification as CRT: padding,
data and further App code remain possible. The small DOS-adjacent entries and
unused SDK read family remain under bounded source/reference investigation.


The resource wide-reader unit qualifies the actual ResRead32NameOrOrdinal and
ResRead32String helpers with real WResRtns file/read/seek/allocation/free
operations. Numeric, empty and named forms have exact cursor and untouched-tail
checks. The old algorithm reads 16-bit units but appends their low byte; a full
zero unit terminates, while a nonzero unit with low byte zero appends an embedded
zero. This projection is tested explicitly without a host/general Unicode
conversion claim. A 300-unit input crosses several genuine VarString chunks;
its returned name/string, exact length, terminator and independent cleanup are
checked, preserving input bytes and length-owner canaries. Short-read/error and
allocation-failure domains remain unqualified. No original was executed.

The necessary corrected native/Watcom build took **1.208 s** after a retained
compile failure from a reused private error-constant name; the selected
strict graph/ABI check took **395.149 ms**. The initial DIRECT trace expectation
omitted existing setup/allocation/position/cleanup calls; source call order
confirmed them and the native-unit expectation was corrected, without a claim
of observed original entries. DIRECT/three-test runs took **18.769 / 16.936 ms**.
Final full build/regression/unit/audit took **2.125 s**, passing forty-four CLI
and ten native-unit cases and all **1,269** source-ready identities. Its
108 native invocations took **14.380–59.039 ms**. Scratch
wlink-wide-read-units retains source/body bindings, failed attempts, timing
splits and the generated ledger (**138,351** qualified unique App bytes).
The fresh **18,952**-byte registry remainder remains unattributed;
nearby FILE-like code is not restored or classified as CRT by resemblance.


The closing source-backed unit batch qualifies eight further old App entries.
ResetOvlSupp writes only AreaSize=ffff; SetDosFmt writes only Extension=4.
ResetObjOMF clears CurrNode while preserving OMF cursors, unlike the newer
source reset. The LastSym-only ResetSym prefix retains its actual fallthrough
into the separate empty InitSymStorage identity; no overlap, synthetic entry
or new startup call is introduced. Borrowed symbol/segment owners and unrelated
fields remain unchanged. SpillNull uses a genuine pre-opened spill descriptor
and token, seeks and zeros exactly its requested subrange through the real
WriteNulls graph, preserving prefix/suffix, file size/accounting and zero-size
seek behavior. First-open, pressure and error domains remain separate.

The public __demangle_r and __scope_name signatures preserve their genuine
five-argument/RET4 original ABIs. A bounded fixed output and real live-owner
reallocator callback cover returned length/content, pointer updates and cleanup;
no allocation address or moving/nonmoving outcome is prescribed. Scope results
are exact borrowed slices into unchanged live input, with normal no-scope
outputs and pointer/length canaries. DW_ULEB128 has exact independently encoded
boundary tests through unsigned32 maximum, return-pointer and buffer canaries.
Its true six-byte terminal block is shared with DW_LEB128 under the existing
strict epilogue metadata; both identities/extents and unique-byte accounting
remain intact. These are native-unit qualifications, not original CLI/runtime
observations; no original was launched.

After correcting missing case-only declarations, the necessary native/Watcom
build took **4.418 s**; selected strict graph/ABI/ownership checks took
**391.524 ms**. The two DIRECT runs took **16.784 / 19.343 ms**, and their
five tests **14.572 / 14.924 ms**, with splits retained in scratch
wlink-closing-units. One grouped final full build/regression/unit/audit took
**2.135 s**, passing forty-four CLI and twelve native-unit cases and all
**1,277** source-ready identities. Its 112 native invocations took
**12.338–59.068 ms**. The generated qualified-code union is
**138,716 bytes**; CLI and unit-only provenance remains separate.

The fresh registry remainder is **18,587 bytes**, including zero-fill,
alignment, runtime/data-like areas and possibly code without an established App
reference. Source review has recovered all positively correlated routines in
the finite App-adjacent gap set. That is a bounded attribution result, not proof
that every remaining byte is CRT. No unknown span is restored by resemblance
or elimination, and existing semantic guards/unsupported domains stay explicit.
