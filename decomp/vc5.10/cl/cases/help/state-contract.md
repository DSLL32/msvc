# Main-entry CRT boundary

The first support trial restores only the nine exactly verified startup/archive
and import records (440 code-extent bytes). Application main at 00401743 remains
fully masked, and native cl_main remains a named INT3 stub. No application function
is accepted by this trial.

`scripts/state.py` decodes the captured Win32 CONTEXT (204 bytes) plus 16 raw stack
words and complete argc/argv and environment vectors, retains 2,000 original
application data bytes, and decodes actual MSVCRT exported fmode/commode/multibyte values, its pgmptr string and
three 36-byte ioinfo records. The decoder checks raw stack/vector aliasing and
extracts only the architectural low 16 bits of the x87 control register; the full
raw DWORD remains evidence. ioinfo stride 36 and flag offset 4 follow the actual
shipped `_write` instructions at 78003316..7800332f. Native inputs come from GDB
reading the actual frame, process vectors, shared CRT symbols and x87 register.
Acceptance recomputes both projections using this decoder and hash-binds its raw
inputs; a separately authored fields JSON is insufficient.

Compare argc, all argv strings, the consumed environment keys, fmode, commode,
multibyte maximum, canonical program path, standard-descriptor osfile flags and
x87 control word. Original pointer addresses remain raw; vector alias checks and
string contents establish the relevant pointer relationships across hosts.
The five environment keys are all direct target getenv requests: CL and _CL_
(main calls at 0040177f and 0040178a), INCLUDE (0040230d), TMP (00402335), and PATH
(004030e0). Whole main 00401743..00401aed never reads its third envp formal; the
complete original/native environment vectors are retained, including entries
outside this application's lookup set. This does not claim byte equivalence of
unconsumed private libc buffers, locks or unrelated host environment entries.

Measured first comparison: argument vectors, those environment values, program
path, multibyte maximum 1, commode 0, and osfile C1/81/81 match. Original fmode is 0
and x87 control word is 027f, while the earlier shared native defaults were 4000
and 037f. Shared entry initialization must apply these evidenced differences;
the comparison does not filter them. Earlier raw mismatch evidence is retained
under shared scratch `vc5.10/cl/native-main-before-fp/`.
