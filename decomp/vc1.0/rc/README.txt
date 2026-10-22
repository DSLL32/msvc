RC.EXE reconstruction, Visual C++ 1.0 for Windows NT
===================================================

Target: toolchains/1.0-nt/bin/RC.EXE
SHA-256: eb2e4bb5994303a37326cc60107ee07be01ca267daca1f9116e3149d9ea31591
Image base: 0x01000000; entry: 0x01011950.

Initial case:
    RC.EXE /dCASE_FROM_COMMAND=7 /Iinclude /foresources.res resources.rc

The read-only case and exact original oracle are in
../fixtures/resources-rich. Expected success has empty stdout/stderr and an
exact 3,684-byte resources.res with 19 records. Original RCPP is intentionally
used as RC's child; a separate reconstruction owns that executable.

This reconstruction is INCOMPLETE. Pending functions have typed named INT3
stubs. A matching trap is a reconstruction frontier, not successful compilation.
The accepted order and original function/code-data extents are in functions.json.
validation.json records the latest durable replay milestone.

Build and differential checks
-----------------------------
    make -j2
    make -j2 check
    make replay

Builds use the original CL.EXE and LINK.EXE through Wine. wine-tool.py creates
its own prefix under ~/.ai-tmp/vc1.0-rc/wine and maps T: to the original
1.0-nt toolchain so header paths are stable. Each debugger run gets a separate
copy of the case under ~/.ai-tmp/vc1.0-rc. The local build/rcpp.exe symlink points
to original RCPP.EXE; neither original binary nor fixture is edited.

compare.py masks the entire backed code section in both executables and restores
only accepted functions plus explicitly recorded adjacent text data. It launches
both masked processes concurrently and rejects a different logical trap or an
interior trap. --accept NAME first verifies the matching current frontier,
records that whole function, and reruns. --advance-runtime advances exact CRT
and import matches one verified frontier at a time. make replay repeats every
accepted frontier from a fully masked entry through the current stopping point.
Successful final runs require exact raw streams, status and resources.res bytes.

Early PE section headers have VirtualSize=0. binary.py uses backed raw sizes
for the code extent and returns zeros for uninitialized sections; BSS
is not interpreted as executable-file bytes at raw offset zero.

Recovery and audit
------------------
The process driver is recovered C with original option mutations, INCLUDE
processing, temporary names, RCPP command construction, error paths and exception
handling. Custom CRT code retains RC's older command parser, Win32 process-heap
allocator, path helpers, locale conversion, shared file opening, 512-byte stream
buffers, formatted output and buffered write behavior. Ordinary literals remain
in their use context; no data section or machine-code implementation is copied.

RC predates the allocator in shipped LIBC.LIB and LIBCMT.LIB. Its allocator
calls GetProcessHeap/HeapAlloc/HeapFree, whereas both shipped libraries use
region/block allocation. The old routines are reconstructed explicitly rather
than substituted with a different allocator graph. identify-runtime.py searches
both libraries and checks relocation-aware code and direct call/jump targets.
Matching fragments do not authorize a changed private neighbor: differing
routines are reconstructed separately when encountered. Explicit source records
override candidate CRT aliases, preventing a small generic library body from
unmasking an unrelated function with the same candidate name.

audit-graph.py walks source-function control flow, switch-table targets and SEH
filters/handlers. It checks a bijection of direct call vertices and function
address references and compares RET stack cleanup and indirect-call presence.
It does not claim branch coverage or prove dataflow equivalence by itself;
paired execution and final byte oracles remain required.

The current resource frontier includes the top-level dispatcher, type interning,
identifiers, LANGUAGE, numeric expressions, keyword lookup, memory flags and the
initial binary payload words. Both masked programs stop at the menu parser.
Remaining resource parsers and writer functions are still named traps; no final
resource product is claimed at this milestone.

RC calls NTDLL's counted-string conversion routine directly. The shipped SDK has
no NTDLL import library. ntdll.def and import-library.py use original LIB.EXE to
generate one, retaining stdcall COFF link symbols and removing their @12 suffix
only from generated DLL lookup-name records. The script copies no target data
or code and adds no conversion-wrapper function to the original graph.

Source INT3 instructions are pending-function traps. The debugger helper is
analysis tooling, separate from the target reconstruction. Assembly listings
under ~/.ai-tmp/vc1.0-rc are analysis artifacts, not reconstruction source.
