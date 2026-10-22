Original-only 32-bit OMF conversion fixture, target VC1.0 NT LINK.EXE.

Inputs are generated test data; none is reconstruction implementation.
sources/make-omf.py creates the frozen264-byte omf-probe.obj. It uses checked
record checksums, two USE32 public/dword-aligned segments, FLAT group, public
and external names, explicit-allocation pick-any COMDAT32 and three FIXUPP32
relocations: external REL32 call, segment DIR32 and COMDAT external DIR32.
The ordinary data is7, COMDAT data15, external helper20. Original CL's runner
returns0 iff the converted function evaluates42. Archive and direct-link paths
both succeed on untouched LINK; direct-link runner executes successfully.

Record evidence: TIS OMF1.1, record80/96/99/9a/91/8c/c3/88/a1/9d/8b definitions:
https://openwatcom.org/ftp/devel/docs/omf.pdf
https://sigops.acm.illinois.edu/old/rsrc/pfmt11.pdf
NASM OMF documentation and independent base.asm output confirm USE32 and the
three fixup encodings: https://www.nasm.us/doc/nasm09.html#section-9.4
Microsoft's LNAMES-index COMDAT name differs from IBM's length-prefixed variant.
Native466a0 reads type/group/segment/name indices. Explicit allocation is valid
and works; an initial implicit DATA32 probe reached native strncpy(NULL), so it
was discarded before freezing. This fixture makes no implicit-allocation claim.

Two independent original runs have exact streams/status and matching full output
bytes after only structural archive decimal dates/generated member COFF timestamp
and PE COFF timestamp. Paths, padding and order are exact. No outputs are trimmed.
The buffered backend LINK32_DB=27 is the same original-validated library workaround
as link-library-tools. Inputs/expected/manifest become immutable after capture.

From repository root:
 python3 decomp/vc1.0/fixtures/link-omf/verify.py
Scratch receipt: ~/.ai-tmp/vc1.0-link/omf-undecorate-capture.
