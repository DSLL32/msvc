WLINK 10.0a has eighteen completed native regression cases. The acceptance ledger
contains **1,074 application functions / 124,382 unique original code bytes**.
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

The complete audit passes for 1,078 source-ready identities, their actual graph,
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
path without such a group remains trapped. Only the six map clock fields are
excluded. The case also corrects cold QNX relocation flags to their real format
union field and trailing-zero trimming to the group protection flags. Earlier
failed pairs remain in scratch; their genuine originals were reused for fixes.

The Phar Lap case matches 496 first entries and every executable byte, including
the captured DOS stub, 128-byte P3 extended header, 12-byte segment-information
records, relocations, runtime-parameter block, 104-byte task state, 8-byte protected
mode descriptors and complete segment/debug images. Only the six map clock
fields are excluded. The segmented path remains distinct from the simple/REX writers, which remain
trapped, and the separately exercised extended-flat writer.

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
