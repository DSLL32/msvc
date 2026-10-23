The 368-byte input is generated explicitly by scripts/build-m68k-input.py
--variant reloc-loader. It has three CODE sections, initialized data, five
external symbols including ___Init32BitLoadSeg, and four native M68K relocations:
CTOT16(type8), CTOABSC32(type11), CTOABSD32(type10), DTOABSD32(type4).
Startup calls helper through an A5-relative jump-table slot. Non-startup code
calls a leaf and reads data through absolute relocated fields; data also holds
an absolute data pointer. The defined loader marker selects linker-generated
40-byte non-startup prologues. The target is linked, never executed by fixtures.

The earlier absolute-startup variant was rejected by the untouched original with
exit4/LNK1101: run-time fixups in startup segment. Its bytes/capture remain in
separate scratch. The accepted version keeps runtime fixups outside startup;
its section bits also follow the validated legacy Mac interpretation described
in ../m68k-minimal/README.txt.

Two independent untouched-original runs exit0 with exact115-byte banner/empty
stderr. Full2MB outputs agree beyond structural PE COFF timestamp0x88. Captured
section bytes show patched operands, A5 jump-table slots, code/data fixup streams,
loader prologues and resource map. The manifest retains all bytes and section
inspection. No padding/path/output-order normalization or Mac runtime claim.
Fixture execution never invokes Wine, original LINK or a PE helper/child.

Reconstruction complete: thirteen further C functions close the four-relocation
case, including the full m68k backend and its signed-value classifier. Fresh
masked-original/native-GDB runs agree through exit 0 and the full frozen artifact.
All 23 native trace/output/artifact cases pass with 566 application first entries.
Canonical original-toolchain compilation and graph/ABI/data/boundary audits pass.
See validation.json for times and raw evidence hashes, body-inventory.json for
the complete Mac static CFG accounting, and components.json for partial target,
CRT, import, inline-data and alignment accounting. This is not branch coverage.

Both selected disassembly stages and both Mac inputs are complete. remaining.json
contains unexecuted next-case suggestions; no additional scenario is assigned.
