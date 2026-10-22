C++ frontend starting case, MSVC 1.0 NT / vc1.0

The source c1/cpp-basic.cpp exercises a class with a private data member,
an out-of-line constructor and member function, implicit this, prefix increment,
a stack object, calls and a conditional return. The resulting program prints
nothing and returns zero.

The requested driver invocation was CL.EXE /Od /c cpp-basic.cpp. The unmodified
CL process was observed at its CreateProcessA import thunk (VA 0x21000), using
the existing ../minimal-c/capture-cl.c debugger. The application and command
line are recorded in manifest.json. No conventional frontend command-line
options were supplied: the command is only the executable path. CL passes an
explicit environment block containing only MSC_CMD_FLAGS, including the final
space in its value, and a null child working-directory parameter. The child
inherits CL's working directory and closed standard input.

The portable environment templates replace only the intermediate basename
with relative "ir" and toolchain roots with {bin}/{include}. Independent replay
confirmed that the four C1XX outputs match the actual captured outputs exactly:
irex 1136 bytes, irsy 123 bytes, irin 9 bytes, irgl 137 bytes. These are phase
inputs/outputs produced by the compiler, not copied executable data sections.
Hashes and exact environment values are recorded in manifest.json.

A second original-driver invocation, CL.EXE /Od cpp-basic.cpp, captured the
actual LINK response and object before launching LINK. Its frontend outputs
were verified byte-identical to the /c invocation. Downstream replay needs
only the four intermediate files for C2, or the captured object and response
for LINK. Original C2 produces a 479-byte COFF object; original LINK produces
the executable under expected/. The independent runs exclude precisely four
bytes: object COFF TimeDateStamp at offset 4, and executable PE COFF
TimeDateStamp at offset 0x88. All other bytes and all phase output streams
must match. The verifier also executes the linked program and checks that
it returns zero with empty stdout and stderr.

Run all independent original phases with:

    python3 decomp/vc1.0/fixtures/cpp-basic/capture-original.py

The verifier builds run-phase.c with the original toolchain,
reproducing the explicit one-variable environment for compiler phases and
inherited environment for LINK. It uses private scratch and Wine prefix
under ~/.ai-tmp/vc1.0-c1xx/replay. A c1, c2 or link argument selects one phase;
--prefix PATH selects an already existing private prefix.

This fixture is an exact frontend oracle. It does not claim that the C1XX
reconstruction has yet reproduced these outputs; see ../../c1xx/README.txt
for the currently accepted reconstruction frontier.

command_line_template preserves the actual executable spelling in argv[0]
while permitting the toolchain root to move. The launcher passes that command
line independently of lpApplicationName. The paired reconstruction uses the
same original argv[0] for both images, preventing executable filenames from
changing the CRT argument-buffer allocations between the pair.
