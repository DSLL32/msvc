Analysis evidence, never reconstruction implementation

native-body-inventory.tsv is a Ghidra 11.4.2 function inventory from the original
LINK.EXE (SHA256 3a53166175a2e98179d4da41bb0f3bfafec0ecf78884441736f138c8ab4d3718).
Its four tab-separated columns are entry address, analysis label, comma-separated
inclusive body ranges, and comma-separated direct-call targets. It contains
addresses/labels only, no copied instructions or executable payload. It is not
used by the build, function-acceptance decisions, masks or replay controller.

This older database contains entry-only seeds and misses some complete bodies,
including substantial accepted functions. scripts/measure-components.py excludes all
one-byte entries, verifies every retained range decodes completely from the
original executable, unions ranges and intersects the current accepted extent
union. Current recovered source names replace stale analysis labels in reports.
Inter-function/block gaps are omitted. Native-versus-CRT is a layout partition
at the first relocation-verified CRT function, mainCRTStartup at 000509d0;
import thunks are independently matched to the original IAT. This is a partial
inventory, not a complete compiler/linker provenance proof or a dynamic coverage
measurement. Unclassified bytes must not be called padding.

cases/components-after-omf-undecorate.json contains the deterministic current
accounting, excluded entry list, inventory hash and remaining closure body ranges
with original byte hashes and direct callers. All ranges in that report are
half-open. The separate scripts/measure-coverage.py remains the authoritative accepted
.text extent-union metric. Neither analysis tool changes acceptance.

Remaining-code recommendations in cases/remaining-after-omf-undecorate.json
are analysis only. No further public-input scenarios were executed or queued.
The OMF debug conversion closure includes internal -g behavior that the public
LINK wrapper does not select; it cannot be advertised as ordinary input gains.

Current augmented checkpoint is cases/m68k-reloc-loader/components.json.
Reproduce it with scripts/measure-disassembler.py --family macintosh --output
cases/m68k-reloc-loader/body-inventory.json, then scripts/measure-components.py
--cfg-body-report cases/disassembler-x86-rich/body-inventory.json
--cfg-body-report cases/m68k-reloc-loader/body-inventory.json --output
cases/m68k-reloc-loader/components.json. These read-only tools execute no target.
The two CFG reports add 5734 genuine instruction bytes absent from the old body
inventory and separately establish 420 inline-data/353 alignment bytes. Their
partial component totals still do not supply a complete target-body denominator.
