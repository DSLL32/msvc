WLINK 10.0a has eleven completed native regression cases. The acceptance ledger
contains **956 application functions / 115,287 unique original code bytes**.
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

The NT case compares every output byte except the identified four-byte COFF
timestamp and date/time digits in the map header. Its 551 first entries match.
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

The complete audit passes for 959 source-ready identities, their actual graph,
references, ownership and extents, returning-function cleanup and the separate
main-to-exit endpoint. Shared code counts once. SetBreak, ODBISectCleanup and
EndTime and RefGroup have explicitly seeded first-entry exclusions. The latter three are
statically qualified empty shared entries excluded from the acceptance count;
no observed separate invocation is claimed. The original first-node AddAddrInfo
sentinel read is preserved on Win32; its documented native-only deterministic
zero choice is limited to the observed invalid sentinel control read.

The DLL case matches 620 first entries. It compares all defined bytes, excluding
only the known COFF timestamp, map clock digits and 25 exact two-byte imported
CodeView segment fields. The original treats live nine-byte elimination edges
as segment leaders, reading beyond their owner for the record format and segment.
Both reconstruction builds choose the observed 16-bit format and write zero for
that invalid segment result; the real CVGetSegment call remains. This is a
bounded original defect, not recovered initialized segment data. Null imported
references and all record lengths, names, offsets and directories stay strict.
The earlier raw output and failed comparisons remain untouched in scratch.

The LX case matches 522 first entries and every DLL byte, including its old
196-byte LX header, page/fixup tables, export bundles and ELF32 DWARF container.
Only the six identified map date/time digit fields are excluded. DWARF-bearing
objects take the original preexisting-debug paths; the still-unreached line
encoder and segment-info helper remain trapped. Source review also corrected
CodeView's genuine previous-offset field, with the existing DLL oracle passing
unchanged.

The NE case matches 542 first entries and every executable byte, including its
64-byte header, segment/entry/name tables and far/import relocation records.
Only the same six map clock digit fields are excluded. Its typed segment flags
retain the old 13-byte layout; communal allocation and the aggregate-argument
entry-ordinal routine retain the original register/stack ABI, including RET 12.
Undefined COMDEF leaf tags remain explicitly trapped in both builds.

The DOS case matches 583 first entries and every executable byte, including the
28-byte MZ header, real overlay-manager code, vectors, overlay table and section
relocations. Only the six identified map clock fields are excluded. Reconstruction
uses the actual 1994 overlay implementation; the later reference source's disabled
overlay entry is inapplicable. Genuine shared terminal code retains both function
identities and contributes bytes once. The case also corrects previously cold
overlay-state offsets and distinguishes the real vector-map heading from a
separate, still-trapped box-12B helper.

The NLM case matches 491 first entries. Its packed 131-byte fixed header and
124-byte extension, code/data images, imported references, exports, dependency
names, Novell debug records and metadata payloads match. The message catalog is
a valid empty catalog with the documented signature and language/count fields.
The original records its message-file offset before adding the custom-data
length; the reconstruction preserves that old ordering in both builds. The
complete NLM matched byte-for-byte on capture day. Regressions exclude only its
identified three date DWORDs and the same six map clock digit fields.

The QNX case matches 510 first entries and every executable byte. Its old packed
48-byte header, six-byte records, group protection/size descriptors, resource
chunks, verification/signature record, selector and linear relocations, EOF and
debug information remain distinct from the later released writer. A data group
provides the defined verification word; the original's uninitialized-output
path without such a group remains trapped. Only the six map clock fields are
excluded. The case also corrects cold QNX relocation flags to their real format
union field and trailing-zero trimming to the group protection flags. Earlier
failed pairs remain in scratch; their genuine originals were reused for fixes.

The Phar Lap case matches 495 first entries and every executable byte, including
the captured DOS stub, 128-byte P3 extended header, 12-byte segment-information
records, relocations, runtime-parameter block, 104-byte task state, 8-byte protected
mode descriptors and complete segment/debug images. Only the six map clock
fields are excluded. The segmented path remains distinct from the simple/REX writers, which remain
trapped, and the separately exercised extended-flat writer.

The ELF case matches 485 first entries and every executable byte, including its
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

The extended-flat Phar Lap case matches 501 first entries and every executable
byte. It resolves the real-break symbol from genuine USE16 code alongside USE32
modules, emits the runtime parameters and flat image, and retains archive/debug
and DOS-stub inputs. The real ProcUnpriv store prefix falls into the distinct
ProcPriv constant-return entry; both identities have disjoint extents and the
actual observed order matches without seeding. Only six map clock fields are
excluded. Invalid selector relocations in the first producer were corrected in
the input assembly before freezing the successful case.

The final parallel build/regression/unit/audit batch took **2.328 s**, including
necessary native and period rebuilds. All eleven cases and 959 source-ready graph/ABI
identities pass. The 22 native regression runs took **17.627–51.542 ms**. The genuine
extended-flat capture command took **2,076.208 ms**; its exact native pair took
**28.765 ms**, split **15.762 / 6.572 / 6.431 ms** for setup/execution/checks. The
map-clock-derived native comparison reused that capture and took **31.946 ms**.
The real-mode and revised main OMF objects took **381.675 / 348.441 ms** to
assemble; the successful original baseline took **585.284 ms**. Other objects,
archive and DOS stub were reused. No ELF, Phar Lap, QNX, DOS, NE, LX or DWARF
executable bytes are normalized. Raw artifacts, producer inputs, earlier failures
and diagnostics remain under `~/.ai-tmp/msvc2/watcom-shared-integration/`.
