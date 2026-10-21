RCPP.EXE, MSVC 1.0 NT / vc1.0
============================

Status: partial reconstruction. The resources-rich case currently reaches a
matching trap at rc_initialize_preprocessor (original 01009d90). Startup,
argument transport, top-level option matching and option actions are recovered;
no successful preprocessing output is claimed yet. The oracle is 336,686 exact
bytes, exit zero and empty stdout/stderr. No fields or paths are normalized.

    make -j2 check
    make -j2 replay
    python3 prove-library.py
    python3 audit-graph.py

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
sets. Future compatible CRT substitutions may use it, and must be recorded in
crt-boundaries.json; that file initially records no substitutions.

IOINIT and STDENVP are linked from the shipped LIBC.LIB. prove-library.py checks
all code bytes outside relocation fields, every REL32 call destination, every
DIR32 data reference, and all 392 initial IOINIT data bytes against the target.
STRNCPY is an exact 37-byte object match. library-proof.json records these
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
coverage or final artifact equivalence. Further focused/error cases follow the
broad starting case. No preprocessing output has passed reconstruction yet.
