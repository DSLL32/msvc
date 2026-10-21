Visual C++ 1.0 for NT: LINK.EXE reconstruction

Completed case: LINK.EXE /?
Target/toolchain: ../../../toolchains/1.0-nt
Original entry: 000509d0, image base 00010000. The executable has no exports.

Build and verification, from this directory:

    make -j2 check
    make replay

Requires GNU Make 4.3+, Python 3 with capstone, and 32-bit Wine support.
The private Wine prefix is ~/.ai-tmp/vc1.0-link/wine. Original toolchain files
are never modified. The compiler and linker are the original 1.0-nt tools.

The /? case exits 3, writes 884 bytes to stdout, and writes nothing to stderr.
Both streams and exit status match the unmodified original byte for byte.
functions.json records 90 accepted trap frontiers. cases/help.json records
output hashes and completed validation. Other cases still reach named traps.

build/link.exe is the unmasked intermediate linked by the original toolchain.
build/original.exe and build/reconstructed.exe start with every code-section
byte masked by INT3. compare.py restores only accepted function ranges and
any recorded shared code/data ranges. The target's SHA-256 is checked first.

The debugger launches the masked copies concurrently and compares logical
trap sites, or compares exit status and raw output streams on completion.
Its command-line argv[0] is LINK.EXE for all three variants, because LINK
includes its invoked name in usage output. The executable is selected by
CreateProcess's separate application-name argument. Stdin is closed; stdout
and stderr are captured separately. The completed case is additionally run
against the unmodified original. make check fails if the case still traps.
make replay verifies the recorded order from both entry points masked.

The dispatcher, debug flags, file-pool initialization, switch-value parser,
help/banner, and two older CRT stream routines are in C. Eleven larger
routines are symbolic inline x86, reassembled by the original compiler.
Their complete extents and physical calling conventions are recorded in
functions.json. Naked definitions leave their original parameter access,
register preservation, return registers, and stack cleanup in the assembly.
Numeric PUSH operands explicitly specify DWORD, which this compiler requires.

Data is defined as it is encountered, with contextual names and types:

    image.c       COFF/PE headers and image-option defaults
    dos-stub.c    serialized default DOS header, message program and PE marker
    arguments.c   argument lists, response-file modes and search-path tokens
    options.c     option names, values, flags and option-list state
    repro.c       LINK_REPRO environment/file capture state
    linker.c      linker driver state and output defaults
    link.c        dispatcher state, program identity and error-reporting state
    files.c       file slots, page pools and mapping callbacks
    usage.c       help lines, version banner and command-trace state

There are no copied data-section arrays. data-symbols.json maps encountered
original data addresses to these C definitions. lift.py emits instructions and
external symbol declarations only; an unknown data reference is an error.
The generated *-asm.inc files are included by their contextual C owners.
No executable byte payloads are used for reconstructed LINK routines.

make audit checks the direct, indirect and tail-call targets of 29 accepted
source/runtime routines; the instruction semantics, operand widths, registers
and relocated addresses of all 11 assembly routines; and the initial values
and named pointer targets of 176 contextual data definitions.

The default non-wildcard CRT argument parser is linked, as in original LINK.
LIBC.LIB objects are identified using their COFF code and relocation targets.
The detector checks external REL32 targets to distinguish identical-looking
routines such as fprintf/fwprintf. Both memcpy/memmove names refer to original
address 00052b90; that one original vertex is named memmove here.

Several CRT routines differ in register allocation or equivalent comparisons:
parse_cmdline, heap_grow_emptylist, control87, write, and doexit. Their complete
bodies were compared with the original and are reused through LIBC.LIB.
The small fpmath wrapper and cfltcvt initializer were also identified manually.
setvbuf and freebuf have older stream-flag semantics and are reconstructed in
runtime.c. The _fltused reference and crt-anchors.c preserve the original FP
startup and stdio/temporary-file termination-table members. CINITTMP.OBJ owns
the rmtmp table entry; referencing rmtmp alone does not install it.

To continue with another case:

    python3 compare.py --case='arguments'
    python3 compare.py --runtime --case='arguments'

The second command advances only library matches and source routines already
marked implemented, asserting each matching frontier before unmasking it.
Unreconstructed entries stay masked. One-byte unknown ranges identify entry
sites only; establish their complete extents and ABIs when they are reached.

For symbolic assembly, record the full extent and contextual source owner,
define each encountered datum appropriately, and run make regenerate. The
accept-assembly.py helper additionally checks the current matching frontier.
Rebuild before rerunning; make audit and make replay must remain successful.

Next queued case: the shared minimal-C LINK fixture in
../fixtures/minimal-c/link, with command line -link @link.rsp.
