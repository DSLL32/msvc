RCPP.EXE, MSVC 1.0 NT / vc1.0
============================

The resources-rich starting case succeeds with exact output. Masked original,
masked reconstruction and untouched original all exit zero, write no stdout or
stderr, and produce the same 336,686-byte RD000360:

    a4600f6681973b55ad820fb1cc624b04d8ea4490cdaae3613812767dcb765762

This completes that case, not the whole binary. functions.json records 156
accepted frontiers: 138 non-import functions and 18 Win32 import interfaces.
Every accepted function's complete call-target set matches the original.
Diagnostic, alternate-option, floating/character-expression and pragma paths
still contain named trapping stubs. No claim of complete branch coverage is
made. Unvisited complete helpers remain masked until a case reaches them.

    make -j2 check
    make -j2 replay
    make -j2 classifiers

Builds use original CL.EXE and LINK.EXE from toolchains/1.0-nt. GNU Make supports
parallel builds. Scratch, a private Wine prefix, case directories, debugger logs
and generated analysis listings live under ~/.ai-tmp/vc1.0-rcpp. T: maps to the
original toolchain directory. The fixture is never modified.

Case and masking
----------------

../fixtures/resources-rich supplies the captured RC-prepared RC000360 input,
expected RD000360, complete source/assets and capture provenance. RCPP receives
real argv and inherited environment; MSC_CMD_FLAGS is removed. The captured
basenames, working directory layout and T: paths preserve #line output exactly.
No bytes, fields, timestamps or paths are ignored or rewritten.

The untouched target's SHA256 is
    a66382a8fa84432a1fc380cea85fcdb3305603e5656638f1d959cdb5d25ca6dd
Its image base is 01000000 and entry RVA is 8d80. The original is never patched
on disk. compare.py fills every code section in build copies with INT3 and
restores only accepted full function extents and their recorded .text data.
Both copies run concurrently under the debugger. Each new trap must match its
logical function before acceptance. A successful run also checks the untouched
original and immutable fixture. validation.json records the final artifacts;
progress.json records the replay evidence and current limitations.

These early PE headers have VirtualSize=0. binary.py uses backed raw size where
appropriate and rejects file access to uninitialized/raw-offset-zero sections.
The Windows loader supplies BSS zeros; the DOS header is never read as BSS.

Recovered native code
---------------------

Readable C covers option dispatch, input contexts and encoding detection,
line output, include-name/header search, complete directive dispatch, the main
lexer, numeric/string tokens and type/constant nodes, comments and splicing,
conditional expression evaluation, inactive directives, and macro definitions,
actual arguments, nested expansion, stringizing and token pasting.

Native structures describe macro symbols, packed argument records, scanner
contexts, tokens and types. The macro stack's typed shared storage retains the
first active frame's saved cursor used by include buffering, and the original
last-frame/workspace overlap. Ordinary literals stay at their use sites;
shared token spelling/category records and semantic lexer tables have explicit
contexts. No target data section was bulk copied. The only inline assembly in
reconstruction sources is INT3 in named frontier stubs.

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
needed in this case: accepted native and CRT call-target sets all match.
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
initializer, wrappers and _initterm; and seven complete stdio objects' code and
relocations. library-proof.json records the results. Runtime data is supplied
by the original library, not copied from the target.

Some small recovered variants retain concrete differences. File opening rejects
RCPP's original invalid disposition combinations. _freebuf clears _IOMYBUF
without the later library's additional 0x400 clearing. _getbuf/_filbuf retain
512-byte buffers and their state handling. The latter choice pragmatically
aligns strict paired flush checkpoints; flush timing alone is not evidence of
native dependence on that size or of observable incompatibility in LIBC.
CLOSE and handle release retain library implementations after checking their
complete equivalent bounds tests, errors, calls and shared handle contracts.

Additional verification
-----------------------

audit-graph.py follows complete CFGs and switch tables, not linear decoding of
jump-table bytes. Known tail calls count as callee edges. Startup's detached
SEH blocks are inspected separately. Original ranges are checked for overlap;
ftell ends at 0100f8b4, before separate native directive lookup 0100f8d0.

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
