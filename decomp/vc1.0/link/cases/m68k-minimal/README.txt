The 84-byte input is generated explicitly by scripts/build-m68k-input.py --variant
minimal. COFF headers/symbols are little endian, the20-byte file header selects
machine0x268, and one .text section contains the Motorola4E75 RTS instruction.
The single external _main is a defined function. Source layouts follow shipped
NTIMAGE.H/WINNT.H and the Microsoft COFF specification; RTS follows M68000PRM
page4-169. The target's original /machine:M68K selector establishes its legacy
machine value; the modern public machine table does not list this old target.

Use exact section flags0x60000020. The rejected probes used0x60300020, borrowing
modern generic alignment bits. This linker treats overlapping0x00100000 as far
data and skips that code in its near-code layout. Both mapped and buffered
probes failed; they remain separate scratch evidence. The accepted object uses
no such alignment qualifier and succeeds under the original default mapped IO.

Two independent untouched-original runs exit0, produce exact115-byte banner and
empty stderr, and emit full2MB machine0x268 PE files with Mac resources. Only the
structurally identified COFF timestamp at0x88 differs. Do not trim allocation,
change code/resource bytes or normalize paths. No Macintosh execution is claimed.
Fixtures consume frozen references only; original capture is outside fixtures.

Reconstruction complete: 26 reached C functions add 7668 accepted extent bytes.
The fresh masked original and native GDB run both exit 0 and match this frozen
artifact and streams. All 22 native trace/output/artifact fixtures pass; canonical
original-toolchain compilation and graph/ABI/data/boundary audits pass. See
validation.json for measured times and hashes of raw scratch evidence.

The queued m68k-reloc-loader fixture adds actual relocations, multiple CODE
resources and a loader marker. Its graph is separate from this zero-relocation
case; unvisited Mac handlers remain named trapping stubs.
