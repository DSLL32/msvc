WLINK 10.0a has four completed native regression cases. The acceptance ledger
contains **720 application functions / 89,805 unique original code bytes**.
The initial help case is followed by a substantial NT console link with two
OMF objects, a static archive, imported functions, an exported symbol, cross-module
data and relocations, embedded Watcom debug information, and a verbose map.
The next case links a C++ DLL with templates, COMDATs, dead-code elimination,
an archive, an import alias, ordered exports, description/version metadata,
and CodeView debug information. A fourth workload creates an OS/2 LX DLL
with structured data, an archive, ordinal imports, resident/nonresident exports
and an appended ELF32 DWARF image.

| Case | Result |
| --- | --- |
| `0000-help` | Exit 0; exact 8,475-byte stdout, empty stderr, no artifacts |
| `0001-nt-link` | Exit 0; exact 243-byte stdout, empty stderr, 7,588-byte PE and 11,855-byte map |
| `0002-nt-dll` | Exit 0; exact 253-byte stdout, empty stderr, 34,475-byte DLL and 44,357-byte map |
| `0003-os2-lx` | Exit 0; exact 272-byte stdout, empty stderr, 5,132-byte LX DLL and 4,274-byte map |

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

The complete audit passes for 723 source-ready identities, their actual graph,
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

Final full audit/regression/unit batch: **0.808 s**. All four cases and 723
source-ready graph/ABI identities pass. Native regression runs took
**17.582–48.053 ms**. The genuine LX original took **1,813.948 ms**; the corrected
native comparison reused that capture and took **26.085 ms**, split
**14.986 / 5.104 / 5.994 ms** for setup/execution/checks. The final selected
check took **0.159 s**. No LX or DWARF bytes are normalized.
Raw artifacts, producer inputs, intermediate failures and diagnostics remain
under `~/.ai-tmp/msvc2/watcom-shared-integration/`.
