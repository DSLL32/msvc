# RCDLL (Visual C++ 5.10)

Both current cases complete using the native i386 shared library. Run `make -j4
native cases check unit original` and `make -j4 audit` from this folder.
Fixtures never launch Wine. `binary.c` is the sole function registry;
`policy-data.json` contains finite graph/ABI contracts. Application source is in
`src/`, two historical startup bodies with per-file rationale are in `crt/`,
and native adapters and test glue are in `harness/`.

The native adapter registers the actual DLL identity, binds captured arguments
and environment, attaches the CRT, calls
`RC(0,0,diagnostic,NULL,argc,argv)`, and detaches. Only a NULL resource callback
is supported. Encountered message resources are generated from `src/help.mc`.

- `0000-help`: `/help`, 589 stdout bytes, empty stderr, exit zero.
- `0001-resources-rich`: includes, command/source macros, conditionals and the
  resource corpus, using one quoted `/dCASE_FROM_COMMAND=7 ` argument with a
  trailing space. It produces exactly 3,672 RES bytes, empty stdout/stderr and
  exit zero. The RES SHA-256 is
  `ad4feb23c9a981122246969f9bdaf8bf9a4978cd722f50a67b1e8884835b9318`.
  The corpus covers menus, dialogs, string tables, accelerators, icon/cursor
  groups, bitmap conversion, raw data, DLGINCLUDE and VERSIONINFO. Final cleanup
  closes streams, removes temporary files and destroys its tracked private heap.

The rich invocation uses stable `S:` workload paths. Only INCLUDE and absent
RCCODEPAGE are explicit Windows target environment inputs; native adapter
controls are set separately. Its fresh final masked-original capture uses the
unchanged shipped RC.EXE host and the identified shipped MSVCRT. The native
comparison explicitly selects that original receipt. It matches all 217 actual
first-entry encounters, final status, streams, artifacts and file inventory.
Original pointed-memory and per-call CRT error/FP state were not universally
captured; no whole-memory equivalence is claimed.

Application reconstruction now comprises **220 functions and 51,779 unique
original code bytes**, **69.40% of the 74,609-byte whole-text extent**. Summed
accepted code extents equal their owned-byte union. This denominator includes
non-code and CRT; complete non-CRT coverage has not been established. The
migration baseline d08815612 was 95 functions / 25,732 bytes. A bounded accounting
read found that the registry at 2fffb5e7d contained 37,282 bytes, whereas its
running report said 37,222; the 60-byte understatement was reporting only.
Earlier running receipts remain unchanged. Final accounting uses the registry
and independently agrees with shared registry-export, rather than incremental
status totals. A superseded final scratch parser crossed NULL-symbol record
boundaries and reported 51,823 bytes; the corrected record-bounded count is
51,779, with no code or credit added by that accounting correction.

The existing registry has 52 pending App records with 9,861 bytes of known code
extents, plus 27 unknown records spanning 4,702 bytes whose classification is
unresolved. Substantial remaining areas include font/RIFF resources, Macintosh
output, APSTUDIO recording, pragma/codepage handling and diagnostics. An extended
resource corpus and a preprocessor/diagnostic workload are possible future
cases; neither has been started.

The two historical startup CRT bodies (555 bytes), import thunks (12 bytes)
and provided 47-byte __chkstk helper receive no application credit. __chkstk
uses custom EAX/ESP semantics; native callers allocate their own real frames.
Shared identify-runtime binds the shipped MSVCRT.LIB helper. Compiler-emitted
switch tables have exact COFF-backed text-data bounds and targets. The strict
whole graph, RET, reference, ownership and extent checks remain enabled. A tiny
target-specific SEH check separately proves parent RET8 and local-filter RET0,
EAX1 and qualified handler identity; native SEH recovery remains unclaimed.

Supported native storage is bounded to encountered initialized objects and
payloads. Unknown descriptors, class-index256's mutable union alias, unsupported
numeric/storage modes, longer unproved buffers and APSTUDIO branches retain
production traps. The old command without the trailing space causes an original
read beyond its requested allocation. Its historical raw evidence is preserved;
a narrow host entry-hook unit stops before that copy. This is an unsupported
invocation checkpoint, not proof of a production copy guard or SEH recovery.

Zero-credit storage corrections preserve distinct command result1853c and
resource status25d3c, expression lookahead149dc and lexer token144f4, macro depth
and source depth, and the real shared APSTUDIO/heap owners. Opposite-state tests
cover affected identities. Earlier equal-zero/value-only evidence and mistaken
correction history retain their limitations. Resource count and image IDs,
string-list head and last-resource pointer also remain separate owners. Packed
resource layouts retain original allocation requests and consumed offsets;
unknown fields and untouched tails are not treated as initialized values.

The final original capture took 839.197 ms. Native adoption took 203.741 ms
(setup 76.448, execution 57.043, checks 70.249 ms). Case-end parallel builds,
both cached case checks and 92 unit groups passed in 0.553 s; strict graph/ABI
and SEH-role audit passed in 0.838 s. The first final audit exposed an undeclared
compiler-emitted VERSIONINFO switch table; exact COFF labels, five pointer
relocations and its 102-byte remap now describe only that data. No target source
or capture changed for this metadata correction.

Current final raw/reference, failed native HeapDestroy denial, native-only
adoption, exact source snapshots, accounting and gate logs are retained in
`~/.ai-tmp/msvc2/vc5.10/rcdll/resume-rich-0222/`. Earlier WIP and failure evidence
remain in their scratch namespaces and Git history. Reconstruction is paused
after this completed case; no next case has been started.
