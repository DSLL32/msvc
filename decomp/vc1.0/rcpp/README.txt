RCPP.EXE, MSVC 1.0 NT / vc1.0
============================

Status: partial reconstruction. The resources-rich case currently reaches a
matching trap at rc_expand_macro (original 0100ad30) while reading headers.
Startup, native directive dispatch, the full main lexer, numeric/string tokens,
include handling, header search, #if expression evaluation, inactive branches,
macro definition encoding and the main output loop are recovered;
no successful preprocessing output is claimed yet. The oracle is 336,686 exact
bytes, exit zero and empty stdout/stderr. No fields or paths are normalized.

    make -j2 check
    make -j2 replay
    python3 prove-library.py
    python3 audit-graph.py
    python3 audit-data.py
    python3 identify-runtime.py
    make -j2 classifiers

Builds use the original CL.EXE and LINK.EXE from toolchains/1.0-nt. Scratch,
Wine prefix, paired case directories and debugger logs are under
~/.ai-tmp/vc1.0-rcpp. T: maps to that toolchain in this private prefix.
The original executable is never changed on disk. Masked copies are generated
in build/. All code sections are first filled with INT3, then only accepted
function extents are restored. Each accepted frontier can be replayed in order.
Unknown native callees remain named trapping stubs. The comparison launches
both copies concurrently and maps addresses to logical functions. A completed
case additionally checks the untouched original and the immutable fixture.

The target SHA256 is a66382a8fa84432a1fc380cea85fcdb3305603e5656638f1d959cdb5d25ca6dd,
image base 01000000, entry RVA 8d80. Its early PE headers have VirtualSize=0.
binary.py uses backed raw size for these sections, and refuses file access to
uninitialized sections (raw offset zero). BSS contents are zero-filled by the
Windows loader; they must never be read from the executable's DOS header.

The fixture ../fixtures/resources-rich contains the captured RC-prepared
RC000360 input, expected RD000360 output, complete source tree and provenance.
RCPP receives real argv with inherited environment; MSC_CMD_FLAGS is removed.
Its inherited cwd and T: header spelling preserve emitted #line paths exactly.

Runtime classification and explicit exception
---------------------------------------------

The user explicitly authorized relaxing preservation of the original function
graph inside identified CRT routines for RCPP only. Native preprocessing/driver
functions and their ABIs still require the original graph. CRT calling
conventions, observable behavior and shared data contracts remain requirements.
The exact originating runtime library is unknown; identification is not based
solely on failed byte signatures.

A strong identification anchor is STDENVP.OBJ: its entire nonrelocation code and
all eight relocation destinations match the original, and its _malloc calls
resolve to 01008960. This proves that address is CRT malloc. RCPP's allocator
calls HeapAlloc/HeapFree on GetProcessHeap with the CRT new-handler retry hook.
Shipped LIBC instead has a region/block allocator and VirtualAlloc helpers;
LIBCMT and MSVCRT10 do not provide this same direct-heap graph either. Parent
inspection established those alternatives. The small recovered heap startup
and malloc implementations remain because they are straightforward and work.

The current milestone has not needed to exercise the relaxed graph exception:
every accepted non-import function still has equal direct/indirect call-target
sets. A measured flush-frontier mismatch exposed the original
512-byte stdio buffers versus the larger shipped LIBC buffers. Original _getbuf
and _filbuf are now reconstructed in readable C to retain their size and state
behavior. This is a pragmatic choice for strict paired checkpoints, not proof
of native dependence on the size or of observable incompatibility in LIBC.
crt-boundaries.json records this evidence and currently records no
graph substitutions; future compatible substitutions must be recorded there.

IOINIT and STDENVP are linked from the shipped LIBC.LIB. prove-library.py checks
all code bytes outside relocation fields, every REL32 call destination, every
DIR32 data reference, and all 392 initial IOINIT data bytes against the target.
STRNCPY is an exact 37-byte object match. Additional FOPEN, STREAM, _OPEN,
_FLSBUF, LSEEK and FGETC objects have all code and relocation targets checked.
library-proof.json records these
checks. Shared runtime data comes from the original library, not copied target
data. Any new routine must be individually classified and its layout/semantics
assessed before taking advantage of the CRT exception.

The small source runtime portions are ordinary C. The only inline assembly in
reconstruction sources is INT3 in named frontier stubs. Disassembly is used for
analysis and extent/call-graph verification, never as implementation source.

Coverage limits
---------------

functions.json records accepted order and discovered function extents, including
unaccepted callees for all recovered branches. Native control-handler,
diagnostic and alternate option paths remain trapping stubs where not reached.
A matching trap is a reconstruction checkpoint, not evidence of complete branch
coverage or final artifact equivalence. The recovered expression evaluator
preserves eager operand evaluation and its single equality/relational/shift
operator limits. Further focused/error cases follow the
broad starting case. No preprocessing output has passed reconstruction yet.

Current input milestone
-----------------------

114 recorded logical frontiers replay to the native macro-expansion entry.
The 99 accepted non-import functions have equal native/CRT call-target sets.
No full preprocessed output has been accepted. Macro entries are typed linked nodes;
input files use typed 40-byte saved contexts and shared segmented byte/word
buffers. The lexer tables contain encountered semantic categories and radix/
identifier flags, verified independently against original initial values.
Original .text switch data is recorded separately with each accepted function.
The main lexer spans [0100f970,010106dc), with 818 reachable instructions, 44
dispatch entries and 23 callees; its C reconstruction retains this single graph
vertex. Native token descriptions are shared typed spelling/category records
and are checked against all 132 valid target entries. No preceding padding or
invalid descriptor bytes were copied.

identify-runtime.py inventories LIBC and LIBCMT byte candidates in scratch.
Its output is explicitly identification evidence, never automatic acceptance
or a claim of behavioral equivalence. The original file-open disposition
checks differ from CL/later LIBC and are implemented as readable C with the
RCPP-specific invalid combinations retained.

Graph and ABI evidence
----------------------

Call audits follow conditional branches and indexed switch tables rather than
linearly interpreting table bytes as instructions. Tail jumps to known function
entries count as callee edges, so source-level tail calls retain their graph.
Detached startup SEH blocks are included separately. Original function ranges
are checked for overlap as new entry points are identified. In particular,
ftell ends at 0100f8b4; the native directive lookup at 0100f8d0 is a separate
function and remains subject to native reconstruction/masking requirements.

The original integer-kind classifier tests have unreachable promotion arms on
this 32-bit target. Their four source functions therefore return the observed
constant kind while retaining four individual stdcall graph vertices.
make classifiers loads both full binaries without executing their startup,
calls their leaf entries with the actual ABI, and verifies 40,048 calls per
binary (boundary values plus a deterministic sample). classifier-validation.json
records this independent probe; it is not whole-program completion evidence.
