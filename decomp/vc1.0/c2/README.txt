C23232.EXE 1.0-nt reconstruction (project vc1.0)

Status: in progress. The captured minimal-C backend case currently stops at
collect_machine_symbols, original VA 00068870. Object generation is not reconstructed
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

The older _sopen, _filbuf and _freebuf CRT variants match those already reconstructed for
CL. They are explicit source in runtime.c, with their C2 extents recorded.
Floating-point CRT startup is retained by linking __fltused. The target links
BINMODE.OBJ, giving _fmode = 0x8000; this matters because its COFF adapter opens
object files with "w". The Makefile selects the same original runtime object,
and compare.py checks this named CRT default in both images. The backend's own
extended-float routines use explicit x87/ESI private ABIs in extended.c.

Data definitions are introduced with the functions that encounter them:
options.c contains named switch destinations, typed switch/suboption tables
and option parsing; input.c contains intermediate input; memory.c owns eight
arena descriptors; symbols.c owns descriptor bookkeeping. No original data
section snapshots are linked into this reconstruction.

The graph audit checks direct calls, indirect-call presence, and tail calls
for accepted source functions. It follows reachable code, switch-table targets,
and MSVC SEH filter/handler entries, rather than decoding table data as code.
functions.json records accepted frontier order;
runtime.json records library function identities; unaccepted stub extents are
provisional and are refined when reached. Completed functions are reconstructed
in full, including branches not exercised by this case.

Queued case: ../fixtures/hello-win32/manifest.json, phases.c2, captured from
original CL /O2 /G5 hello.c. It follows the current minimal-C case. See
cases/queue.json. It has four captured frontend files and no source-file input;
the exact captured backend options include -Gs4096 -Gy -G5. The expected object
has 2289 bytes, with only its COFF timestamp excluded from comparison.

Symbol input now has contextual typed variants in symbol-input.c, scope/record
registration in records.c, decoration in names.c, logical segment definitions
in segments.c, and storage selection in segment-storage.c. The per-function
driver is in functions.c. Backend pass dispatch is in backend-flow.c;
backend-symbols.c records module requirements. output.c constructs the named
output callbacks and module directives; coff-output.c adapts those callbacks
to the typed writer configuration, and coff-writer.c owns the encountered
vectors, scopes, packed relocation/line buffers, section contributions and
symbol records. coff-segments.c maps logical segments to COFF sections;
listing.c contains module listing logic. Initial module directives now pass
through the buffered data writer. Machine-symbol collection, initializer dependency discovery, pending-symbol output,
and machine/frame reset now reach the function reader. machine-opcodes.def records
the encountered instruction operand counts with named opcodes; it contains no
original pointer table or opaque data snapshot.

The current checkpoint has 95 reconstructed source functions and 154 recorded
matching trap frontiers. All 154 frontiers replay successfully with the matching binary CRT default. Callback stubs
remain trapped until reached; their signatures and extents are refined at that
point. The Makefile follows transitive local-header dependencies and continues
to compile and link entirely with the original toolchain.

Reconstructed logic is period C. The earlier pure tail-call initialization
adapters are ordinary C wrappers. The only functional inline assembly is the x87/ESI extended-float ABI, whose
raw ST(0) input and 80-bit memory result cannot be expressed by this compiler's
8-byte long double. The two narrow blocks are documented in extended.c.
Diagnostics use ordinary fixed three-argument __fastcall calls: ECX, EDX and
one callee-cleaned stack argument, matching original RET 4. Diagnostic handling
is still a named trapping stub; the current success-path cases and graph audit
do not validate diagnostic failure behavior. Unreached
function stubs deliberately use INT3 as required by the reconstruction process.
