# Internal switch parser

parse_internal_switch owns132 hot bytes12900..12984 and84 cold bytes304ad..30501.
Its four-entry mode jump table12988..12998 is separately identified16-byte .text
data. The four padding bytes12984..12988 remain masked and uncredited. Common
Rust metadata records exact table data and all four real targets; raw relocated
contents are checked separately. Ownership counts232 bytes, split216code/16data.

Mode0 recognizes three exact prefixes: -Bm, -ZB and -Zm. Empty suffixes retain
mode1/2/3 for the following argument; nonempty suffixes are processed immediately.
The modes store atoi's signed integer result. -Zm maps a zero result to100.
Unrecognized mode0 input returns0. The original invalid nonzero-mode loop and
NULL-input behavior are retained. Source uses explicit short-circuit character
comparisons to preserve the original inlined comparison graph, without adding
a CRT comparison vertex.

The three newly encountered initialized ints are internal_bm_flags5f2fc,
internal_zb_value5ef04 and memory_percent5f978, with canonical defaults0/0/100.
The first later consumer tests bit0x4000; the second has a divide-by8 consumer;
the third multiplies an allocation size by the percentage. Broader meanings of
the first two remain unrecovered. They join typed firstcopy/snapshot/restore,
with distinct nonzero live/saved values and exact original storage identities.
Known initialized state is64 bytes; the remaining6484 bytes stay UNMODELED.
No initialized-section blob or guessed field is introduced.

Both fresh normal and labeled restore runs independently observe all ten argv
parser entries and returns: actual pointers, complete argument bytes including
NUL, mode, return, cdecl stack cleanup and whole currently modeled state. This
case exercises ten unrecognized mode0 inputs, each returning0. Recognized-prefix
and atoi branches are reconstructed from their real instructions and strict
graph/import/RET/extent evidence, but were not executed by this case.

After those calls, actual canonical getenv("MSC_CMD_FLAGS") returnsNULL on both
sides. Its provider, exact name, return and error/x87 state are checked; the
private original environment also proves that variable is absent. The next
frontier12998 is the genuine main helper. Full original data/BSS/snapshot/plain
globals and modeled native state remain unchanged across scanning.

Fresh focused controls42/54 pass, including new defaults/aliases, typed live and
saved fields, nonzero restore, repeated-call omissions/order/formals/arguments/
returns, getenv and table target mutations. Unchanged previous binding/old-field/
allocation controls are reused with their frozen contracts and hashes; complete
current positive proofs still run. Strict audit covers57 source functions and
2877 owned original bytes, including this16-byte table. Original/native builds
3.963/1.894s, captures1.770/1.395s normal and2.071/2.312s restore, audit0.029s,
cached native lifecycle4.00ms. Focused controls40.37/68.77s are separate.
