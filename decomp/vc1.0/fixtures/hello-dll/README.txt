Queued LINK case: optimized Win32 Hello, world! as a DLL

Original build:
    CL.EXE /O2 /G5 /LD /Fehello.dll hello-dll.c

c1/hello-dll.c derives from ../hello-win32/c1/hello.c. The window procedure,
window creation, message loop and painting code are preserved. The former
WinMain is named HelloMain, declared __declspec(dllexport) and explicitly
__stdcall, with the same four arguments. The original SDK predeclares WinMain
without dllexport, and this compiler rejects changing that prior linkage.
A minimal DllMain returns TRUE. The CRT supplies the DLL loader entry; HelloMain
is called by a host outside loader initialization.

The original DLL exports exactly _HelloMain@16, ordinal 1. dll-host.c loads
hello.dll beside itself, resolves that decorated export, calls it through the
matching stdcall function pointer, unregisters the destroyed window's class,
and unloads the DLL. The existing original-toolchain-built gui-check.c checks
HelloWindow/Hello, world!, centered painted pixels, WM_CLOSE and host exit 0.

LINK is queued after hello-win32. C1/C2 inputs are retained as independently
replayable original-only references; they are not additional reconstruction
assignments. /LD selects the multithreaded static CRT (LIBCMT.LIB).

The actual captured response file, with CRLF endings, is:
    -debug:none
    -dll
    -implib:hello.lib
    -out:hello.dll
    hello-dll.obj

Invoke original LINK.EXE -link @link.rsp in an isolated directory containing
link/hello-dll.obj and link/link.rsp. Inherit the normal environment with LIB
set to the original 1.0-nt lib directory and CL/_CL_/LINK/_LINK_/MSC_CMD_FLAGS
cleared. Dependencies are LIBCMT, KERNEL32, USER32 and GDI32. Exact stdout and
empty stderr are recorded in manifest.json. Outputs are hello.dll, hello.lib
and hello.exp, each retaining the original linker's 2 MB zero preallocation.

Export/import-library generation spawns LINK itself in -lib mode. The paired
reconstruction must follow that child and execute the corresponding masked or
reconstructed LINK image; running the untouched child would bypass frontiers.

Compare every output byte except documented timestamps: PE/COFF header dates,
export-directory dates and archive-member date fields. artifacts.py derives
these positions from the actual formats and checks all zero preallocation.
The manifest records exact positions, raw hashes and normalized hashes.
No path, padding, ordering, symbol or relocation normalization is permitted.

Verification from the repository root:
    python3 decomp/vc1.0/fixtures/hello-dll/verify.py
Select c1, c2 or link as an optional positional argument. The link check runs
twice independently, compares all three products, checks DLL/export metadata,
and exercises load, exported stdcall invocation, painting, close and unload.
validation.json records the original-only verification receipts.

Compiler invocations use no command-line options: their explicit environment
contains only MSC_CMD_FLAGS, including its final space. The manifest retains
actual captured flags and portable templates, substituting only toolchain
paths and the intermediate prefix (relative "ir"). Four raw frontend outputs
are retained under c2/. The backend object excludes only its COFF timestamp.

Capture used ../minimal-c/capture-cl.c at original CL's CreateProcessA import
thunk RVA 0x11000, copying each intermediate before its consumer started. No
diagnostic options were added. Original binaries remain unchanged; captures,
helpers, logs and the private Wine prefix are under ~/.ai-tmp/vc1.0-hello-dll/.
