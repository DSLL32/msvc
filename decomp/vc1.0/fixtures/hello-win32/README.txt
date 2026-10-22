Queued C1/C2/LINK cases: optimized Win32 Hello, world!

Captured from unmodified original CL.EXE /O2 /G5 hello.c using toolchain 1.0-nt.
The source includes windows.h, supplies WinMain and a window procedure, and uses
DrawText to paint "Hello, world!" in the window's client area. Pragma directives
select USER32.LIB and GDI32.LIB. It is the same source as CL's committed case.

Queue order for each worker:
 - C1: minimal-c, then this C1 case.
 - C2: minimal-c, then this C2 case.
 - LINK: initial /?, minimal-c, then this LINK case.

manifest.json records the actual application names, command lines, environment
values, input/output hashes, original dependency hashes, and predecessor case.
Only toolchain paths and the shared temporary prefix are substituted in the
portable templates. Preserve the final space in MSC_CMD_FLAGS. Relative prefix
"ir" reproduces the captured frontend outputs byte for byte.

C1:
 - Copy c1/hello.c to an isolated working directory.
 - Launch C13232.EXE with no command-line arguments and an explicit environment
   block containing only the phase's MSC_CMD_FLAGS value.
 - Substitute {bin} and {include} with original toolchain Windows paths.
 - The /O2 expansion and /G5 behavior are captured exactly. In particular,
   original CL passes frontend -G4 together with -D_M_IX86=500.
 - The original SDK include directory is required. The 36 headers observed
   during a separate file-access trace are named and hashed in the manifest.
 - Expected outputs: irex (2066 bytes), irsy (422), irin (46), irgl (629).
   The reference files are stored under c2/; compare every byte.

C2:
 - Copy the four files under c2/ into an isolated working directory.
 - Launch C23232.EXE with no command-line arguments and an explicit environment
   block containing only its MSC_CMD_FLAGS value. Use relative prefix "ir".
 - Captured backend options include -Gs4096 -Gy -G5.
 - No hello.c or frontend process is required.
 - Compare hello.obj with link/hello.obj (2289 bytes), ignoring only bytes 4..7
   of the COFF header (TimeDateStamp).

LINK:
 - Copy link/hello.obj and link/link.rsp to an isolated working directory.
 - Invoke LINK.EXE -link @link.rsp, inheriting the environment with LIB set to
   the original toolchain's lib directory. Clear LINK, _LINK_, and MSC_CMD_FLAGS.
 - The 40-byte response file has CRLF endings and contains:
       -out:hello.exe
       -debug:none
       hello.obj
 - Dependencies include the original LIBC, KERNEL32, USER32, and GDI32 libraries.
 - Compare hello.exe to expected/hello.exe (2097152 bytes), ignoring only the
   four-byte PE COFF TimeDateStamp at offset 0x88.
 - The result uses the Windows GUI subsystem. The window must paint the text,
   then exit 0 when sent WM_CLOSE.

All compiler phases return 0 without stdout or stderr. LINK returns 0, writes
158 bytes to stdout, and writes nothing to stderr; the exact text is recorded
in the manifest. The generated window was exercised under Xvfb with the
original-toolchain-built checker in decomp/vc1.0/cl/src/gui-check.c.

Verification from the repository root:

    python3 decomp/vc1.0/fixtures/minimal-c/verify.py --fixture decomp/vc1.0/fixtures/hello-win32

Append c1, c2, or link to select one phase. The shared verifier builds its
launchers using the original toolchain, uses its own Wine prefix, and creates
isolated working directories under ~/.ai-tmp/vc1.0-capture/hello-win32/.
The LINK check captures gui-window.bmp and gui-window.json there, verifies the
visible window's class/title, centered painted content, and clean close.

Capture provenance:

The debugger in ../minimal-c/capture-cl.c stopped original CL at the
CreateProcessA import thunk (VA0x21000/RVA0x11000). It recorded real Win32 launch
arguments and copied each intermediate input before its consumer was started.
No /Bd or /Bk options were added. The original CL finished compiling/linking
normally. The snapshots are generated compiler outputs and test inputs; they
are not copied executable data sections.

For this case the capture helper's final two optional arguments are the complete
CL argument string "/O2 /G5 hello.c" and the object filename "hello.obj". The
first four arguments remain original CL path, thunk RVA, capture directory, and
working directory. Scratch files and private Wine prefixes stay in ~/.ai-tmp.
