Visual C++ 1.0 for NT: CL.EXE reconstruction

Completed cases: CL.EXE /?; CL.EXE test.c; CL.EXE /O2 /G5 hello.c;
all 42 cases in cases/remaining/catalog.json. Nine additional timezone cases
and three explicitly internal-state cases also pass paired verification.
test.c contains: int main () { return 0; }
hello.c includes windows.h and draws "Hello, world!" in a Win32 window.
Its pragma library directives select USER32.LIB and GDI32.LIB.
Target/toolchain: ../../../toolchains/1.0-nt
Toolchain compiler banner: 8.00; linker banner: 1.00.

Reconstructed C, headers, and native test helpers live in src/. Python tools
live in scripts/. Case inputs, metadata, and this Makefile remain here.

Build and verification (from this directory):

    make -j2
    make -j2 check
    make replay

Requires GNU Make 4.3+, Python 3, Wine with 32-bit support, Python capstone
for the call-target audit, and xvfb-run for the window case. The build creates a
private 32-bit Wine prefix at
~/.ai-tmp/vc1.0-cl/wine. No original toolchain files are changed.

The normal build produces build/original.exe and build/reconstructed.exe.
Every code-section byte starts as INT3; only the functions recorded as accepted
in functions.json, and their shared code/data ranges, are restored. The original
file's hash is checked before masking. build/cl.exe is the unmasked intermediate
linked by the original compiler/toolchain. src/stubs.c is reserved for future named stubs.

make check runs the masked original and reconstruction concurrently under the
NT debuggers in src/debug-run.c and src/paired-run.c. They compare logical trap sites,
or compare exit status and both raw output streams on completion. A completed run is also
checked against the unmodified original. Child stdin is closed and stdout and
stderr are captured separately; this preserves CL's help pagination prompts
and CRLF translation. Compile runs use separate working directories under
build/compile-{original,reconstructed,baseline}. The generated test.obj and
test.exe are compared after zeroing only their four-byte COFF timestamp fields;
all other bytes must match. Each generated executable must run without output
and return 0. build/compile-validation.json records raw and normalized hashes.
The hello case uses build/hello-{original,reconstructed,baseline} and writes
build/hello-validation.json, applying the same exact comparison rules to
hello.obj and hello.exe. src/gui-check.c launches the resulting window under Xvfb,
checks its title and painted client area, captures build/hello-window.bmp, then
posts WM_CLOSE and requires exit 0. build/hello-window.json records the result.
The compiler phases and linker invoked as child processes are the original
toolchain binaries. This reconstruction targets the CL driver.

make replay restores the accepted list one function at a time, beginning with
both entry points masked. It asserts all 233 recorded frontiers: the original
140 help, 40 compile and one hello frontiers, followed by 52 frontiers across
the remaining cases. It then checks each completed case. functions.json records
the case boundaries; every accepted frontier belongs to exactly one replay case.
Its output can be saved outside build/ if it needs to survive make clean.

The /? case prints 3,192 bytes to stdout and 131 bytes to stderr, with exit 0.
The implementation includes 110 source functions and passes 233 recorded trap
frontiers across 57 cases. All 104 native driver functions in the contiguous
[00017010,0001b081) region are reconstructed, as are six differing CRT routines.
Accepted function extents cover 37,136 of the original 40,966 .text bytes
(90.65%); these figures measure accepted extents, not executed branch coverage.
The minimal compile case produces a 286-byte object; the optimized window case produces a 2,289-byte object. Both executables
are 2,097,152 bytes. /O2 /G5 exercises detaching overridden options from the
option list. The remaining .text bytes belong to unvisited CRT paths and stay
masked. New cases may therefore still encounter CRT frontiers.

cases/remaining/catalog.json contains 42 catalog cases; internal.json adds the
OMF and IDE assertion cases, and extra.json adds timezone/DST cases. make check
runs all of them. scripts/remaining.py prepares each original/reconstruction pair in
parallel, with independent Wine prefixes exposing identical Q: working paths.
Every completed case compares exit status, raw stdout/stderr, product filenames,
and all product bytes against the unmodified original. Only parsed COFF, PE
export-directory, archive-member and map timestamp fields are excluded from
artifact comparison. validation.json under cases/remaining records the results.

/Bt uses a controlled GetLocalTime API input, stepping from 00.900s in 1.225s
increments. The CL timer, older CRT timezone parser, DST routine, and time
conversion routines execute normally. Additional dates exercise the original
April/October rules, pre-1967 behavior, and signed offsets with minutes/seconds.
Both interrupt cases call the registered stdcall handler on a new thread; they
verify child_running=0 and 1 and exit 4, without claiming OS console delivery.

/omf is still parsed as /o mf by this binary. The legacy OMF writer is verified
by setting coff_format=0 at accepted link_pass entry in both processes, retaining
the current pass record. One case prints the response; another writes it with
multiple object/library/definition inputs and substitutes a controlled linker
that returns 7. A third internal case clears the installed IDE frame pointer to
reach its assertion callback. These cases do not claim public reachability.
See cases/remaining/README.txt for the exact harness behavior and commands.
coverage.json remains the initial original-only discovery record.

src/cl.c contains the driver main and its SEH frame. src/driver.c contains reconstructed
argument parsing, option grammar expansion, option-list handling, setup, help
display, compiler/linker invocation, response-file generation, and cleanup.
src/remaining.c contains the additional switch handlers, response-file parsing,
diagnostics, IDE transport, timers, control callback and legacy OMF writer.
src/data.c contains the original option grammar and mutable compiler
pass profiles. src/cl.h records the 32-bit layouts and calling conventions.

The static CRT is linked from this toolchain's LIBC.LIB and SETARGV.OBJ.
scripts/identify-runtime.py matches COFF sections against the executable, masking
relocation operands before checking REL32 destinations against library symbols
and imported functions. This distinguishes otherwise identical wrappers such as
remove/rmdir and spawnv/spawnvp. The original's two identical memcpy/memmove
bodies are recorded as separate graph vertices. runtime.json records original
addresses and object provenance; build/runtime.json records the reconstruction's
addresses.
The strcat and strcpy entries share an implementation, which is recorded as a
separate shared range so accepting strcat does not silently accept strcpy's
entry point.

Six reached CRT functions differ from the shipped library and are implemented
in src/runtime.c: setvbuf, _freebuf, _sopen, _filbuf, __tzset, and _isindst. The stream
routines preserve CL's older buffer flags; _sopen preserves the original exact one-byte read test
before inspecting a trailing control-Z. The older timezone parser uses the TZ
string and retains the original defaults when it is absent; the older DST
routine implements its original April/October rules. scripts/identify-runtime.py also
checks relocations against source replacements when validating library callers.
scripts/audit-graph.py compares the direct callee sets and presence of indirect calls for every implemented source
function. The grammar and callback data preserve indirect dispatch.

functions.json is the durable progress record. Addresses are original virtual
addresses at image base 0x10000; the entry point is 0x1c750. All indexed source
functions have full extents and implementations. src/stubs.c is now empty. Unvisited library routines remain masked; future non-library callees
should receive named trapping stubs until reconstructed.

To continue, build and run another case with:

    python3 scripts/compare.py --case='/your arguments'

On matching traps, determine the new function's extent and ABI, replace its
stub, and add its linker name to functions.json's accepted list. Rebuild and
rerun. Keep calls to other unreconstructed functions as named traps.

    python3 scripts/compare.py --runtime

advances through exact original-library matches, asserting each matching trap
before accepting it. This defaults to /?; supply --case for another case.
Use make replay for all recorded case sequences. cases/help.json,
cases/compile.json, and cases/hello.json record output hashes and validation.

To repeat a completed additional case, use:

    python3 scripts/remaining.py nested-response
    python3 scripts/remaining.py --all
    python3 scripts/remaining.py --replay timing

Original-only discovery is still available with make probe. The historical
coverage report is not the reconstruction acceptance record; functions.json and
paired replay provide that record.
