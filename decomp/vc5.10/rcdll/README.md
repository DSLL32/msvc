# RCDLL (Visual C++ 5.10)

All six current cases complete using the native i386 shared library. Run `make -j4
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
- `0002-mac-resources`: `/r /m /fomac-resources.rsc mac-resources.rc`,
  combining two raster fonts, RIFF/ACON animations, monochrome icons/cursor,
  bitmap, menu/dialog, strings, accelerators and raw resources. The complete
  Macintosh resource fork has SHA-256
  `f0a4323f141ec15431e746e12bfa75777f8185108a63347eaf7937aa899238d2`.
  It emits the genuine RC4509 warning that types 10 and 240 both map to `HEXA`,
  with empty stderr and exit zero. All 174 original first-entry encounters match.

- `0003-editor-resources`: an editor UI combining nested MENUEX menus, toolbar,
  DIALOGEX class/help/creation data, stringification of `1.25`, character and
  integer-kind conditionals, and `#pragma code_page(1252)`. It produces the exact
  RES SHA-256 `78c839c66d51f02cf24789894a69ea787b6a0af945fbe8d4950419a4fab3de72`,
  empty streams and exit zero, with all 200 original first-entry encounters.

- `0004-apstudio-resources`: `/r /s`, combining editor symbol/source recording,
  classic menus, toolbar, DIALOGEX, strings, bitmap, TEXTINCLUDE, DLGINCLUDE,
  escaped constants and raw data. It binds the genuine target input `TMP=.` and
  produces RES SHA-256
  `a220acae2562c6a505a4fd567bf0ceac2423f56d47b35c7f0313e738c15a5af8`,
  empty streams and exit zero. All 190 original first-entry encounters match;
  the final strict file inventory confirms temporary-file cleanup.

- `0005-mac-cursor-compat`: Macintosh 4-bit color-icon synthesis and a
  monochrome 32-pixel cursor reduced to 16 pixels in both dimensions. It emits
  the exact RSC SHA-256
  `e8892c7e6d16ec5502b7bfe557c85217160977a94e388d7796a290626d7ce462`,
  the genuine RC4506 scaling warning and RC4509 type warning, empty stderr and
  exit zero. All 133 first-entry encounters and strict cleanup match.

The rich invocation uses stable `S:` workload paths. Only INCLUDE and absent
RCCODEPAGE are explicit Windows target environment inputs; native adapter
controls are set separately. Its fresh final masked-original capture uses the
unchanged shipped RC.EXE host and the identified shipped MSVCRT. The native
comparison explicitly selects that original receipt. It matches all 217 actual
first-entry encounters, final status, streams, artifacts and file inventory.
Original pointed-memory and per-call CRT error/FP state were not universally
captured; no whole-memory equivalence is claimed.

Application reconstruction now comprises **276 functions and 63,980 unique
original code bytes**, **85.75% of the 74,609-byte whole-text extent**. Summed
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

The Mac case adds 21 application bodies and 6,508 code bytes to the completed
rich-case baseline of 220 / 51,779. The editor case adds 20 application bodies and 3,473 code bytes, including eight
newly identified MENUEX/toolbar helpers. The current registry has 34 pending App
records with 4,085 known code bytes, plus four unknown records spanning 497 bytes
whose classification is unresolved. Remaining areas include APSTUDIO recording,
codepage changes, escape/error handling and unvisited Macintosh resizing and
byte mapping. These records are a classified subset, not a complete non-CRT
denominator.

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

The preceding rich-case final original capture took 839.197 ms. Native adoption took 203.741 ms
(setup 76.448, execution 57.043, checks 70.249 ms). Case-end parallel builds,
both cached case checks and 92 unit groups passed in 0.553 s; strict graph/ABI
and SEH-role audit passed in 0.838 s. The first final audit exposed an undeclared
compiler-emitted VERSIONINFO switch table; exact COFF labels, five pointer
relocations and its 102-byte remap now describe only that data. No target source
or capture changed for this metadata correction.

The Mac font inputs retain valid packed FNTv3 fields and even bitmap row width.
The target reads 148 bytes directly and interprets file offsets 136/140 as name
pointers; those are reserved packed-header bytes and remain zero. Its resulting
empty FONTDIR names match the original. A named-device branch and an OS font
renderer were not tested. Macintosh image paths retain the actual bounded
palette tables; unavailable eight-bit lookup rows trap in both builds. The
original white-first cursor inversion begins outside its initialized bitmap;
that branch also traps before the invalid read. Current assets use black-first
monochrome palettes. These limits do not rely on allocator slack.

Shared support for this case is limited to the reached ASCII MacRoman conversion,
its count-zero invalid-parameter result, exact numeric `wsprintfA("%d")`, and the
registered five-insert RC4509 message. Other formatter grammars and codepage
conversions remain unsupported. Original per-call API bytes/error/FP state were
not directly measured; ordinary static ABI facts and final actual output remain
distinct from such measurements.

The Mac final masked-original capture took 1,364.983 ms. After the compiler
intrinsic correction, native comparison against that unchanged original took
12.758 ms (setup 5.646, execution 5.610, checks 1.502 ms). The completed-case parallel
batch passed all three cached cases, 99 unit groups and both builds in 0.418 s;
strict graph/ABI plus SEH-role checks passed in 0.912 s. The first final audit
exposed an imported `memcpy` where the original used compiler intrinsics. The
VC5 intrinsic annotation preserves the algorithm and original function graph;
its fresh native refresh reused the unchanged original. No new original was
captured solely for that correction. The untouched corrected-font final oracle
independently agrees on inputs, status, streams and complete artifact hashes.

Mac source snapshots, matching raw/reference, formatter denials/adoptions,
accounting and final gates are retained under
`~/.ai-tmp/msvc2/vc5.10/rcdll/mac-resources/0025-notice-complete/`, with earlier
steps alongside it. Rich-case evidence remains under
`~/.ai-tmp/msvc2/vc5.10/rcdll/resume-rich-0222/`. The initial ordinary 0023 native
report was inadvertently overwritten during adoption; its separate genuine GDB
denial report/log and failed ELF remain preserved, and the derivation records
that limitation. Earlier WIP and failure evidence retains its original scope.
Further substantial cases will target the remaining application owners.


The editor case retains valid RC input and initialized asset extents. Its final
masked capture is byte-for-byte identical to the untouched valid-input capture.
Earlier invalid source drafts, native service denials and the packed-argument
guard failure remain separate scratch evidence. Character class 6 constants and
MENUEX caption concatenation beyond the caller's 257-WCHAR object trap in both
builds. The three contextual MENUEX grammars include only their finite field and
end-token sets; the header's original zero end-count WORD has no surrounding
storage or mutable-alias claim. Typed 532-byte menu items preserve the actual
fields, caption offset 18 and low-WORD output offsets. Toolbar dimensions are
WORD outputs and its count deliberately shares the existing dialog count owner.

The numeric guard now admits the current frame's selected packed argument only
after checking its record chain, initialized payload/NUL/A trailer, first-character
alias, alignment and upper-workspace capacity. It does not admit arbitrary lower
workspace addresses or replace the existing live-owner premise. The actual
fraction spelling is `1.25`; exponent/suffix checks are native units, not extra
original API observations. Shared additions are limited to explicit VC5.10 validity
of codepage 1252 and the exact default-C pragma `swprintf` format. Original per-call
API state remains unmeasured.

The editor final original took 1,365.445 ms; final native comparison took 14.823 ms
(setup 7.179, execution 6.115, checks 1.530 ms). All four cached cases, 106 unit groups
and both builds passed in 0.437 s; strict graph/ABI and SEH-role checks passed in
0.945 s. A final descriptor compile error was corrected before the passing batch.
The scratch updater had also left earlier WIP cached descriptors with empty
orders and their initial DLL path; every fresh pair explicitly selected the
actual original report and compared its full trace/frontier. Final cached data
now includes all 200 encounters and actual DLL identity. Historical WIP reports
are unchanged and are not presented as cached-descriptor validation.

Editor evidence, exact compiled sources, failed drafts, matching captures,
accounting and end gates are retained under
`~/.ai-tmp/msvc2/vc5.10/rcdll/editor-resources/0025-control-patch/`, with earlier
frontiers alongside it. Further coherent workloads will target the remaining
application routines without changing these accepted limitations.

The APSTUDIO case adds 11 application bodies and 1,882 code bytes, including two
newly identified character/whitespace helpers. The contextual token fields retain
their exact existing storage and widths. Directive and marker readers hold
oversized names, filenames and decimal strings before an unsupported write in
both builds. The macro identifier consumes the actual low WORD of its physical
four-byte argument; the captured upper half is not normalized or claimed equal.
The header writer retains its fixed version 2, shared language WORD and distinct
characteristics DWORD. Unknown token roles and untouched tails remain unclaimed.

Temporary names are internal open/remove identities. The native shim uses the
bound target CRT `TMP=.` namespace and generates independent absent names without
reserving files or seeding original counters. Old no-TMP inputs and captures stay
unchanged. Partial progress compares the declared RES prefix and streams, without
claiming equality of internal temporary filenames. Final normal-exit comparison
uses the complete strict output inventory. The untouched new-input oracle and
final masked capture agree on invocation, inputs, status, streams and artifacts.
Original per-call API state and pointed bytes were not universally measured.

The final APSTUDIO original took 695.604 ms. After correcting two unvisited error
edges to the actual resource-error routine, native adoption against that unchanged
original took 14.766 ms (setup 5.697, execution 7.606, checks 1.462 ms). All five
cached cases, 109 unit groups and both builds passed in 0.428 s; strict graph/ABI
and SEH-role checks passed in 0.978 s. The audit exposed those wrong error edges;
failed artifacts remain preserved. Historical APSTUDIO entry-hold assertions were
retired when the genuine path completed; their false-branch state checks remain.
One obsolete hold group was removed, and four current-case groups were added.

APSTUDIO source snapshots, old input variants, guard failures, formatter denials,
matching references/adoptions and final gates remain under
`~/.ai-tmp/msvc2/vc5.10/rcdll/apstudio-resources/`, with closeout in
`0017-complete/`. There are 25 known pending application routines (2,252 bytes)
and two unclassified routines (448 bytes); this is not a complete non-CRT
coverage denominator. Further coherent workloads target those remaining owners.

The cursor compatibility case adds four application bodies and 338 code bytes:
weighted palette brightness, the file-warning wrapper and both cursor reducers.
The original reducer operands establish source first and destination second;
the old unaccepted prototype labels were reversed. Fresh actual calls alias
both pointers, while focused native tests also use distinct buffers and verify
untouched tails. The mono loader reads exactly 256 initialized plane bytes;
width reduction writes 128 bytes and height reduction then reads those 128 bytes
and writes 64. No allocator slack is consumed. Invalid/unproved dimensions stay
held in both builds. The color path uses the previously bounded 16-row table;
the unavailable eight-bit lookup and white-first inversion defect remain held.
Adjacent source strings complete without reaching the separate concatenator.

Shared formatting support is limited to the authenticated RC4506 template and
its actual ANSI filename, signed line number and ASCII WCHAR filename inserts.
The line comes from the resource token DWORD at 25064, distinct from reader
line 27f7c. The exact resource template is contextual source data; no surrounding
resource image is copied. Original per-call API state remains unmeasured. The
untouched and final masked captures agree on complete inputs and outputs.

The cursor final original took 1,237.936 ms; native matching took 10.414 ms
(setup 5.356, execution 3.692, checks 1.365 ms). All six cached cases, 111 unit
groups and both builds passed in 0.448 s. Strict graph/ABI and SEH checks passed
in 0.972 s. An initial adapter suffix mismatch and scratch output-inventory
selection errors were corrected against the same originals; their failures are
not acceptance evidence. Raw source, assets, formatter denial/adoption, corrected
ABI roles, bounds and final receipts remain under
`~/.ai-tmp/msvc2/vc5.10/rcdll/mac-cursor-compat/`, with closeout in `0007-complete/`.
There are now 21 known pending application routines (1,914 bytes), plus the same
two unclassified routines (448 bytes); the whole-text percentage remains the
only qualified global coverage figure.
