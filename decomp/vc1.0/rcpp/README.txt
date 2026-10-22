RCPP.EXE, MSVC 1.0 NT / vc1.0
============================

The resources-rich starting case succeeds with exact output. Masked original,
masked reconstruction and untouched original all exit zero, write no stdout or
stderr, and produce the same 336,686-byte RD000360:

    a4600f6681973b55ad820fb1cc624b04d8ea4490cdaae3613812767dcb765762

The assigned 13-case follow-up suite also passes with exact stdout, stderr,
exit statuses and complete or partial output files. Cases cover pragmas,
character and floating tokens, comments, -P/-C/-J, packing options, CP932,
UTF16LE, macro-arity warnings and preprocessing errors. The invalid-option
case checks that no output file is created. The target's CP932 trail-backslash
failure is retained, including RC4413 and RC2001 diagnostics.

The two targeted follow-ups also pass: trailing tokens after #endif retain
RC4067 with status zero and a 70-byte output; a 1,200-byte macro argument
retains fatal RC10056's empty message, status one and 32-byte partial output.
The original 16 cases assert the entire generated file set as well as exact
bytes.

The console-interrupt case adds a real Ctrl+C while the primary thread is
blocked in an empty named-pipe ReadFile. The original registered callback
receives argument zero on a distinct OS-created thread, closes user streams
and exits one. Exact empty streams, pipe disconnect, exclusive output reopening
and the retained 46-byte flushed prefix all match. The output is zero bytes at
the blocked-read boundary. The immutable fixture preserves two original raw
captures; independent replay and repeated paired runs check the same relations.

functions.json records 176 accepted frontiers: 158 non-import functions and
18 Win32 import interfaces. Every accepted function's complete call-target set
matches the original. acceptance_cases preserves all 156 initial frontiers
and records the case used for each subsequent acceptance. All 173 previous
frontiers are retained. Two named trapping stubs remain: exception filter
and CRT fatal startup abort. Unvisited complete helpers also remain masked
until a case reaches them. This completes the assigned cases, not the whole
binary.

coverage.json measures the clipped union of accepted full code extents,
including shared .text data, identified CRT and import thunks: 35,515 of
40,448 original code bytes (87.80%). This is neither branch coverage nor
executed-instruction coverage. Gaps include alignment padding as well as
unaccepted functions.

    make -j2 check-suite
    make -j2 replay-suite
    make -j2 classifiers messages coverage
    make -j2 interrupt-check

Builds use original CL.EXE and LINK.EXE from toolchains/1.0-nt. GNU Make supports
parallel builds. Scratch, a private Wine prefix, case directories, debugger logs
and generated analysis listings live under ~/.ai-tmp/vc1.0-rcpp. T: maps to the
original toolchain directory. The fixtures are never modified.

Case and masking
----------------

../fixtures/resources-rich supplies the captured RC-prepared RC000360 input,
expected RD000360, complete source/assets and capture provenance. RCPP receives
real argv and inherited environment; MSC_CMD_FLAGS is removed. The captured
basenames, working directory layout and T: paths preserve #line output exactly.
../fixtures/rcpp-followups supplies 13 immutable source/argument/output captures.
../fixtures/rcpp-targeted adds the two diagnostic/macro-limit captures.
../fixtures/rcpp-interrupt adds the original-backed console/pipe case.
Each manifest source, target, catalog and output hash is verified. The complete
source tree is copied into a private work directory and checked unchanged after
the run. Captured relative paths and source encodings are retained. No bytes,
fields, timestamps or paths are ignored or rewritten.

The untouched target's SHA256 is
    a66382a8fa84432a1fc380cea85fcdb3305603e5656638f1d959cdb5d25ca6dd
Its image base is 01000000 and entry RVA is 8d80. The original is never patched
on disk. compare.py fills every code section in build copies with INT3 and
restores only accepted full function extents and their recorded .text data.
Both copies run concurrently under the debugger. Each new trap must match its
logical function before acceptance. A successful run also checks the untouched
original and immutable fixture. validation.json records the final artifacts;
followup-validation.json records the additional exact artifacts and streams,
and original-followup-replay.json independently checks all untouched original
cases. progress.json records the replay evidence and current limitations.

These early PE headers have VirtualSize=0. binary.py uses backed raw size where
appropriate and rejects file access to uninitialized/raw-offset-zero sections.
The Windows loader supplies BSS zeros; the DOS header is never read as BSS.

Recovered native code
---------------------

Readable C covers option dispatch, input contexts and encoding detection,
line output, include-name/header search, complete directive dispatch, the main
lexer, numeric/string tokens and type/constant nodes, comments and splicing,
conditional expression evaluation, inactive directives, and macro definitions,
actual arguments, nested expansion, stringizing and token pasting. Follow-up
cases add pragma output, complete character/escape and floating-spelling
scanners, option-number/suffix parsing, directive-tail skipping and diagnostic
reporting. The console callback retains its original unconditional stream-close
and exit behavior. The macro-fatal helper restores the opening expansion line
before lookup, formatting and fatal reporting. Escaped characters retain signed-byte
behavior unless -J is active; octal and hex escapes retain the target's
three-digit limits.

Native structures describe macro symbols, packed argument records, scanner
contexts, tokens and types. The macro stack's typed shared storage retains the
first active frame's saved cursor used by include buffering, and the original
last-frame/workspace overlap. Original-toolchain compile-time size/offset
assertions verify those aliases, FILE and input context layouts. Ordinary
literals stay at their use sites; shared token spelling/category records and
semantic lexer tables have explicit contexts. No target data section was bulk
copied. The only inline assembly in reconstruction sources is INT3 in named
frontier stubs.

Original .text switch tables are recorded with their owning functions. The
main lexer is one original graph vertex, [0100f970,010106dc), with 818 reachable
instructions, 44 dispatch entries and 23 callees. The expression evaluator
retains eager operand evaluation and the target's single-operator limits at
equality, relational and shift precedence levels.

CRT classification and reuse
----------------------------

The user explicitly permits different internal graphs inside individually
identified embedded CRT routines for RCPP only. Native graph/ABIs, CRT external
ABIs, observable behavior and shared data contracts remain requirements. The
originating runtime variant is still unidentified. No graph exception has been
needed: accepted native and CRT call-target sets all match.
crt-boundaries.json records the classification evidence and compatibility
choices, rather than treating failed byte signatures as incompatibility proof.

STDENVP.OBJ's exact code and all eight relocation destinations identify target
01008960 as CRT malloc. That allocator uses GetProcessHeap/HeapAlloc/HeapFree
and a new-handler retry hook. LIBC's region/block allocator differs; parent
inspection also checked LIBCMT and MSVCRT10. The small readable heap routines
remain because they preserve the identified allocation/free contract simply.

Compatible CRT functions use shipped LIBC.LIB. C initialization, exit wrappers
and their private _initterm now reuse CRT0DAT.OBJ, with explicit reconstruction
symbol names in functions.json. The verifier maps those symbols to their
original logical identities, including private symbols from the link map.
Common exit retains callbacks, stdio cleanup and ExitProcess; its private extra
termination-done flag does not replace any native state contract.

prove-library.py independently checks complete IOINIT code, all relocations
and its 392 initial data bytes; STDENVP; STRNCPY/STRNCMP leaves; CRT0DAT's C
initializer, wrappers and _initterm; and eight complete stdio objects' code and
relocations. library-proof.json records the results. Runtime data is supplied
by the original library, not copied from the target. The pipe case reaches
FGETC.OBJ's already-proven fgetc and CLOSEALL.OBJ's exact fcloseall. The fgetc
extent ends at its actual return (0100e8e1), excluding padding and the separate
getc wrapper; fcloseall retains slots three through _lastiob and its count.

Some small recovered variants retain concrete differences. File opening rejects
RCPP's original invalid disposition combinations. _freebuf clears _IOMYBUF
without the later library's additional 0x400 clearing. _getbuf/_filbuf retain
512-byte buffers and their state handling. The latter choice pragmatically
aligns strict paired flush checkpoints; flush timing alone is not evidence of
native dependence on that size or of observable incompatibility in LIBC.
CLOSE and handle release retain library implementations after checking their
complete equivalent bounds tests, errors, calls and shared handle contracts.
The option parser uses shipped ATOX.OBJ's atol after whole-body comparison;
its ASCII whitespace/sign/digit arithmetic and wrapping result match. Its
locale-size field is wider, with the same supported locale-length values.

Native byte classification needs an explicit data compatibility adjustment.
RCPP passes signed bytes to PRINT/SPACE/DIGIT ctype macros. On negative indices,
its original embedded runtime prefix includes unused FILE slots and locale
pointers, rather than LIBC's different layout. This caused a measured missing
RC4413 warning in the trail-backslash case. legacy-ctype.c defines only the
semantic flags those native macros test, retaining compatible LIBC data for
ordinary indices and the original _isctype call edge. No pointer words or raw
prefix data were copied. audit-data.py checks all -128..255 native indices.
The unused FILE slots remain zero within the original ten-input-file bound.

Additional verification
-----------------------

audit-graph.py follows complete CFGs and switch tables, not linear decoding of
jump-table bytes. Known tail calls count as callee edges. Startup's detached
SEH blocks are inspected separately. Original ranges are checked for overlap;
ftell ends at 0100f8b4, before separate native directive lookup 0100f8d0.
All 352 accepted original/reconstruction ranges (functions and import thunks)
also receive a 15-byte lookahead check for split instructions, including RET
immediates. Reachable CFG instructions and the final detached/alignment segment
are checked; switch data is treated as data. boundary-validation.json records
this check. No accepted range required repair for a split instruction.

audit-data.py checks both 256-entry semantic lexer tables and all 132 valid
token descriptions/categories against the target. Invalid preceding padding
was not copied. identify-runtime.py inventories LIBC/LIBCMT byte candidates in
scratch for identification only; it never accepts or unmasks functions.

The integer-kind classifier promotion arms are unreachable on this 32-bit
binary. Their four separate stdcall C functions retain the observed constant
results. make classifiers independently calls original and reconstruction
leaf entries without startup: 40,048 calls per binary, including boundary
values and a deterministic sample. classifier-validation.json records this
ABI probe; it is separate from whole-program artifact validation.

The diagnostic lookup has a contextual typed catalog of 372 encountered
code/text records, its sentinel, and its original lookup/fallback behavior.
make messages probes all 65,536 16-bit codes plus two out-of-range arguments:
131,080 calls per binary check exact text, stable repeated pointers, distinct
pointers for duplicate-spelling records and the shared unknown-code fallback.
message-validation.json records the matching probe artifact. The analysis-only
probe maps both EXEs without startup; PE base relocations are applied in memory
when a mapped EXE is not at its preferred address.

The real-console runner is analysis tooling excluded from reconstructed RCPP.
It follows RC's corrected entry-barrier lesson: observe native registration,
allow an actual empty pipe read into the OS, suspend and verify its live call
frame, queue real console-local Ctrl+C, briefly resume for Wine dispatch, then
suspend at OS callback-thread creation or a retained read-return breakpoint.
DBG_CONTROL_C is passed not handled. Separate private prefixes isolate the
identically named pipes during paired execution. No native callback invocation,
thread injection, PID rewriting or artifact normalization is used.

interrupt-validation.json records ten repeated paired/baseline checks with
actual callback IDs and raw proof hashes. Final raw proof, streams, events and
before/after files live under evidence/console-interrupt. Intermediate stress
proof lives in ~/.ai-tmp/vc1.0-rcpp/interrupt/stress. The fixture controller and
capture script are pinned by its manifest; original independent recapture uses
the same controller without masking or reconstruction.

A useful next proposal is an arithmetic-fault conditional, such as #if 1/0,
targeting the still-trapping exception filter. Its first-chance exception
disposition and exact original outcome would need capture before dispatch.
This proposal has not been run; no further cases were started.
