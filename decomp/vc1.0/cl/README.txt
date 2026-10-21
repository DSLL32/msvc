Visual C++ 1.0 for NT: CL.EXE reconstruction

Completed case: CL.EXE /?
Target/toolchain: ../../../toolchains/1.0-nt
Original compiler version: 8.00; linker version: 2.23.

Build and verification (from this directory):

    make -j2
    make -j2 check
    make replay

Requires GNU Make 4.3+, Python 3, Wine with 32-bit support, and Python capstone
for the call-target audit. The build creates a private 32-bit Wine prefix at
~/.ai-tmp/vc1.0-cl/wine. No original toolchain files are changed.

The normal build produces build/original.exe and build/reconstructed.exe.
Every code-section byte starts as INT3; only the functions recorded as accepted
in functions.json, and their shared code/data ranges, are restored. The original
file's hash is checked before masking. build/cl.exe is the unmasked intermediate
linked by the original compiler/toolchain. Source stubs are in stubs.c.

make check runs the masked original and reconstruction concurrently under the
small NT debugger in debug-run.c. It compares logical trap sites, or compares
exit status and both raw output streams on completion. A completed run is also
checked against the unmodified original. Child stdin is closed and stdout and
stderr are captured separately; this preserves CL's help pagination prompts
and CRLF translation. All run products are under build/.

make replay restores the accepted list one function at a time, beginning with
both entry points masked. It asserts each recorded frontier, then leaves the
final executables restored and checks the completed case. Its output can be
saved outside build/ if it needs to survive make clean.

The /? case prints 3,192 bytes to stdout and 131 bytes to stderr, with exit 0.
The implementation includes 55 source functions and passes 140 recorded trap
frontiers. Other paths remain incomplete and will encounter named INT3 stubs.
This is a reconstruction of this case, not yet a working C/C++ compilation path.

cl.c contains the driver main and its SEH frame. driver.c contains reconstructed
argument parsing, option grammar expansion, option-list handling, setup, and
help display. data.c contains the original option grammar and mutable compiler
pass profiles. cl.h records the 32-bit layouts and calling conventions.

The static CRT is linked from this toolchain's LIBC.LIB and SETARGV.OBJ.
identify-runtime.py matches COFF sections against the executable, disregarding
only relocation operands. Small jump wrappers are additionally checked against
their actual target. runtime.json records the original addresses and object
provenance; build/runtime.json records the current reconstruction's addresses.
The strcat and strcpy entries share an implementation, which is recorded as a
separate shared range so accepting strcat does not silently accept strcpy's
entry point.

Four CRT functions differ from the shipped library and are implemented in
runtime.c: setvbuf, _freebuf, _sopen, and _filbuf. The stream routines preserve
CL's older buffer flags; _sopen preserves the original exact one-byte read test
before inspecting a trailing control-Z. audit-graph.py compares the direct
callee sets and presence of indirect calls for every implemented source
function. The grammar and callback data preserve indirect dispatch.

functions.json is the durable progress record. Addresses are original virtual
addresses at image base 0x10000; the entry point is 0x1c750. Unknown callees have
named stubs. One-byte ranges on some unreconstructed callbacks identify entry
sites only; their complete extents must be established when reached.

To continue, build and run another case with:

    python3 compare.py --case='/your arguments'

On matching traps, determine the new function's extent and ABI, replace its
stub, and add its linker name to functions.json's accepted list. Rebuild and
rerun. Keep calls to other unreconstructed functions as named traps.

    python3 compare.py --runtime

advances through exact original-library matches, asserting each matching trap
before accepting it. This defaults to /?; supply --case for another case.
Use make replay for the recorded /? order. cases/help.json records final
output hashes and validation results.
