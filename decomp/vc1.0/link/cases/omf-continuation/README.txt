Ordinary OMF continuation/displacement/LINSYM checkpoint952

Two302-byte valid generated OMF inputs use TIS OMF1.1 record formats. They contain
three32-bit segment-target fixups (target+4 twice and target+8), a COMDAT split
across two8-byte data records, and C4/C5 LINSYM records. The companion changes
only a data-segment USE16 Pbit; wide99 record fields remain legal and distinct
from the segment's default operand/address size. It is inspect-only data, not
an attempted16-bit program execution. Primary format URL/hash/printed pages,
record generator/lengths/checksums and complete inputs are frozen here.

Public -lib /out:continued.lib continued.obj and the equivalent use16 phase
succeed twice on the untouched original. USE16 prints the actual stdout warning
"cvtomf() : warning : 16-bit segment". The converted COFF member has16joined
COMDAT bytes and3real DIR32 relocations to two synthesized $$R0001/2 symbols at
section2 offsets4/8. The original section auxiliary retains length8 despite the
actual16-byte raw span; this quirk is preserved. LINSYM is consumed without
emitting COFFline records. Actual native-created archive bytes feed a separate
public symbol/relocation dump; no frozen input replacement hides output defects.

Five whole readable application C bodies, in actual reached order:
  omf_insert_relocation_symbol472d0..47341:113instruction bytes
  omf_extend_section48aa0..48ad1:49
  omf_record_symbol_lines45f20..45f67:71
  omf_lookup_relocation_symbol47350..47386:52+2internal alignment
  omf_warning48150..481a6:86
All actual cdecl arguments/callers and consumed input data/state are observed.
Insertion leaves unresolved node+8 undefined; lookup observes it after the actual
finish_symbols writer. The line handler observes exact buffered record bytes,
width and remaining count. Warning's plain format consumes no variadic words.
No dormant argument is read or initialized to make a comparison pass.

Warning reaches shipped _vfprintf535f0..53630,55CRT instruction bytes+9alignment.
This is support only, with zero app credit and no new target-local CRT source.
Exact shipped library identity/callees/extent remain in the canonical graph.
Native public correspondence is qualified by actual omf_warning caller and
original48177call/4817creturn. Prior recon_printf/recon_fprintf shim-internal
aliases are explicitly logged and correspond to already accepted public CRT
boundaries; any other caller is rejected. Three actual arguments prove stdout,
plain format and va_list ownership without reading nonexistent variadic data.
Fresh complete packets additionally compare FILE21pending input bytes/count2027,
flags10/bufsiz2048 and full errors. Result14 yields35pending bytes/count2013,
errno2/DOS2/Last0 and exact complete streams/archive. Source/ELF bindings are in
warning-result/binding.json and native-complete-stream/receipt.json. No private
native CRT graph/ABI equivalence is claimed.

The first uncontrolled native poststate had errno0 versus original2; raw failure
is retained. Root approved separately named controlledTMP variants. Original
actual caller20b68/20b6d proves directory\\/prefix tmp, existing empty canonical
TMP, counter1->2, owned tmp2 result after actual missing-candidate access and
errors2/2/2. Native enables the existing strict libc100tempnam profile only from
hashed case-specific proof. There is no candidate seed, errno reseed, reference
overwrite, broad error exclusion or default shim change. Earlier application
acceptance provenance stays tagged to the original uncontrolled cases; fresh
controlled entry histories independently prove the projected4/1/0app segments.

Only three10-digit archive dates and one4-byte COFFclock normalize. The Date
initializer's fixed suffix "0 " remains exact, as do paths/order/padding/auxiliary
bytes. Both controlled and uncontrolled original repeats are retained. The
missing-context-label, early expected-caller/total-length corrections and cache
bootstrap failures were analysis failures, not target rejections or promoted
observations. See failed-attempts.json; final cache validates all fresh references.

All74 cached native cases/758appentries pass. Incremental -j4 native build1.841s;
external suite6.037s, summed setup2.103/ELF.723/oracle2.871/tracechecks.008s.
New phases take.0133/.0124/.0066s including setup/checks; direct ELF is6/5/3ms.
Original entry histories3.390s, canonical -j4 compile/audits7.331s, graphnegative
18.061s are separate.791CFG graphs,775callback sets,490data definitions and41ECX
routines pass, plus36host-only ABI/FILE/result/date controls and prior native,
resource and cross-import controls. Actual shared C/H snapshot at held6b6e20b4
is independently hashed before/after; no source mutation occurred during freeze.

Extent248147/281634=88.1097%,952frontiers. Partial body accounting separately has
183041/190690known target instruction bytes,28167/37088CRT,258/348imports.
Verified contextual data1936 and alignment3065 remain separate.6996known OMF
bytes still include mode2 internal-g paths absent from the public adapter; do
not claim them reachable.16827remaining unclassified bytes are not CRT or proven
padding. target-local-crt-inventory.json records16historical runtime.c overrides
read-only; this family added none. next-proposal.json awaits root review.
