PDB-bearing PCH library binding
===============================

One coherent producer/consumer workflow uses public /Zi /Od /G5 /W3,
/Ycbinding.h /Fpbinding.pch /YlBindingArchive, followed by /Yubinding.h.
The header combines signed/wide enums, nested records, bitfields, packed
layout, function pointers and supported named-base record links. Untouched
original CL accepts both phases, links their objects, and the checksum
program silently exits 0. Final adjusted draft3 retains all these features.
The first exploratory runtime expected an incorrect
checksum 83; retained draft1 exits 2. Corrected draft2 checks 70 and passes.

Two full original CL transport captures preserve complete pre/post file sets.
The 39884-byte PCH and four primary IR streams are identical. Only the proven
PDB creation signature DWORD differs: PDB+48 and companion irdb+14 agree raw.
See evidence/clock-provenance.json; paths and PCH bytes are never normalized.
The consumer starts from the actual producer PCH/PDB. Its producer.obj is a
frozen untouched backend dependency, not a native C2 output. Native replay
must first validate the producer frontend input contract for that object.

Original child argv is executable-only; MSC_CMD_FLAGS is its sole environment
entry. Relative ir replaces captured temporary prefix for standalone replay.
Canonical S:\vc1.0\c1\pdb-pch-binding and exact input mtimes remain explicit.
Native fixtures never invoke Wine. /Yd is not used.

The initial named pack(push/pop) header exposed an original serialization
limitation: its retired packing free-list pointers are not relocated. That
variant is a failed preflight, preserved under evidence/probes/packing-freelist.
The current header uses pack(2)/pack(); both full original workflows were
recaptured explicitly. No pointer bytes were rewritten, normalized or zeroed.

All ten newly reached whole functions are accepted in original/native entry
order (921 through 931). Both phases complete; the consumer reads actual
native producer PCH/PDB bytes after validating the complete original backend
input contract. It uses the frozen original producer.obj with captured mtime;
no native C2 is invoked. Raw database signature must propagate unchanged into
the consumer PDB/reference, beyond their mutual cross-output equality.
The PDB enum-sort callback matches every original comparator operand pair.
