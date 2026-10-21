Visual C++ 1.0 for NT: CL.EXE reconstruction

Completed cases: CL.EXE /?; CL.EXE test.c
test.c contains: int main () { return 0; }
Target/toolchain: ../../../toolchains/1.0-nt
Toolchain compiler banner: 8.00; linker banner: 1.00.

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
and CRLF translation. Compile runs use separate working directories under
build/compile-{original,reconstructed,baseline}. The generated test.obj and
test.exe are compared after zeroing only their four-byte COFF timestamp fields;
all other bytes must match. Each generated executable must run without output
and return 0. build/compile-validation.json records raw and normalized hashes.
The compiler phases and linker invoked as child processes are the original
toolchain binaries. This reconstruction targets the CL driver.

make replay restores the accepted list one function at a time, beginning with
both entry points masked. It asserts the 140 help frontiers, then the 40 new
compile frontiers, and checks each completed case. functions.json records the
case boundaries. Its output can be saved outside build/ if it needs to survive
make clean.

The /? case prints 3,192 bytes to stdout and 131 bytes to stderr, with exit 0.
The implementation includes 75 source functions and passes 180 recorded trap
frontiers across both cases. The compile case produces a 286-byte object and
a 2,097,152-byte executable. Other paths remain incomplete and will encounter
named INT3 stubs.

cl.c contains the driver main and its SEH frame. driver.c contains reconstructed
argument parsing, option grammar expansion, option-list handling, setup, help
display, compiler/linker invocation, response-file generation, and cleanup.
data.c contains the original option grammar and mutable compiler
pass profiles. cl.h records the 32-bit layouts and calling conventions.

The static CRT is linked from this toolchain's LIBC.LIB and SETARGV.OBJ.
identify-runtime.py matches COFF sections against the executable, masking
relocation operands before checking REL32 destinations against library symbols
and imported functions. This distinguishes otherwise identical wrappers such as
remove/rmdir and spawnv/spawnvp. The original's two identical memcpy/memmove
bodies are recorded as separate graph vertices. runtime.json records original
addresses and object provenance; build/runtime.json records the reconstruction's
addresses.
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
Use make replay for both recorded case sequences. cases/help.json and
cases/compile.json record output hashes and validation results.
