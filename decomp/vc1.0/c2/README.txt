C23232.EXE 1.0-nt reconstruction (project vc1.0)

Status: in progress. The captured minimal-C backend case currently stops at
initialize_codegen, original VA 00057be0. Object generation is not reconstructed
yet. No completed C2 output-equivalence claim is made.

Target: toolchains/1.0-nt/bin/C23232.EXE
SHA256: 0a64eab35cfb4bfb8af452653d21707947ada33952a82a5e712c275ab7712d26
Original image base: 00010000; entry point: 00073470.

The starting case is ../fixtures/minimal-c/manifest.json, phases.c2. Each run
gets an isolated working directory containing the four captured frontend files
irex, irsy, irin and irgl. No test.c file is needed. The debug launcher passes
exactly one environment variable, MSC_CMD_FLAGS, with the captured options and
the portable relative intermediate prefix "ir". The child has no extra argv
arguments. The original target hash and fixture input hashes are checked.

    make -j2 all       build with the original toolchain using Wine
    make frontier     assert the current logical trap and graph audit
    make replay       rerun every recorded accepted trap frontier
    make check        require complete exit 0 and exact object output

make check intentionally fails until the case reaches successful completion.
For the final output comparison, only the four-byte COFF TimeDateStamp field
at object offset 4 is ignored. Every other byte must match the captured
286-byte test.obj. Both output streams must be empty and exit status must be 0.

wine-tool.py owns ~/.ai-tmp/vc1.0-c2/wine. Toolchains and fixtures are read-only.
The Makefile supports parallel builds. Generated files are under build/.

Both images have all code-section bytes replaced with INT3. compare.py restores
only accepted function ranges, including function-owned .text tables. CRT code
is identified from original LIBC.LIB using relocation-masked byte matches plus
relative-call target checks. An individual-function fallback handles the older
ieee87 object; private function symbols bound its extents. Short candidates
with unresolved call relocations are rejected. The original linked CRT group
begins at freopen (00071aa0); generic RET/JMP shapes in backend code do not
establish CRT identity. Distinct compiler functions have named trapping stubs.

The older _sopen and _filbuf CRT variants match those already reconstructed for
CL. They are explicit source in runtime.c, with their C2 extents recorded.
Floating-point CRT startup is retained by linking __fltused. The backend's own
extended-float helper routines are separate graph vertices and remain trapped.

Data definitions are introduced with the functions that encounter them:
options.c contains named switch destinations, typed switch/suboption tables
and option parsing; input.c contains intermediate input; memory.c owns eight
arena descriptors; symbols.c owns descriptor bookkeeping. No original data
section snapshots are linked into this reconstruction.

The graph audit checks direct calls, indirect-call presence, and tail calls
for accepted source functions. functions.json records accepted frontier order;
runtime.json records library function identities; unaccepted stub extents are
provisional and are refined when reached. Completed functions are reconstructed
in full, including branches not exercised by this case.
