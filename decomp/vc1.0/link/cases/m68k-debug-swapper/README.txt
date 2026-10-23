Macintosh full COFF debug and swapper resources

The401-byte explicit test input derives from the frozen368-byte Mac relocation/
loader object. Its only added symbol is public ___InitSwapper, section1 offset6,
an existing RTS. The three CODE sections, initialized data and four relocations
are unchanged. scripts/build-mac-debug-input.py reproduces it from the pinned
base object; no modern section alignment bits are introduced.

The untouched original succeeds with /machine:M68K /debug:full /debugtype:coff,
/nodefaultlib /entry:main /fixed. The output has a COFF debug directory and symbol
table, five12-byte Mac CODE/DATA debug records, the .mscv resource and a12-byte
swapper payload in .swap. The swapper version WORD is native-order; subsequent
count/size fields are big-endian, as emitted by the original. No Macintosh
runtime execution, CodeView packing or external-process emulation is claimed.

The .mscv resource stores the absolute output filename. Active references were
freshly captured twice at S:\vc1.0\link\m68k-debug-swapper and preserve that
spelling byte-for-byte. cases/shared-prefix/m68k-debug-swapper is the current
manifest/oracle; earlier case validation/state files are historical evidence.
Only three structurally identified clock fields differ between repeated outputs.
No path, padding, ordering or size normalization is used.

Four readable C functions close the case in src/mac-debug.c: allocate the record
array, collect section records, write Mac debug data and write swapper data.
Fresh masked-original/native-GDB stops match each logical entry, then both sides
exit0 with exact115-byte stdout, empty stderr and full artifact agreement against
the frozen reference. Native fixtures launch only ELF and consume cached inputs/
outputs; never Wine, original executables or PE helpers/children.

An extra broad state sampler exposed an old uninitialized Mac relocation cursor
that is copied into Section.relocation_offset. Native code starts this cursor0.
The13 defined section scalars and3 debug counters agree. The original serializer
never reads the scratch field and explicitly writes relocation pointer/count0;
the debug record writer also never reads it. The initial rejected state sample,
precise producer/read/serialization evidence and hashes remain in
transient-state-analysis.json. No output bytes are excluded for this discrepancy.

All25 native trace/status/stream/artifact cases pass with574 application first
entries. The single measured suite is1.801s externally:1.610s case loop and0.104s
trace preparation; summed setup0.351s,execution0.197s,oracles1.047s,trace checks
0.003s. Combined canonical original-compiler/audit/native-trace build14.768s.
Audits pass600 call graphs,591 callback sets,453 contextual data definitions,
754 accepted instruction boundaries and38 ECX cleanup checks.

Coverage is219208/281634=77.8344%,748 frontiers. The43 accepted Mac bodies contain
13993 genuine CFG instruction bytes,268 inline data and140 alignment. These are
static instruction counts, not branch coverage. components.json retains honest
partial target/CRT/import/data/padding accounting; remaining.json contains only
unexecuted recommendations. Both selected families are complete; work is paused.
