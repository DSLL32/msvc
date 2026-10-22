Direct function/ABI cases: two stateless, apparently unreferenced native helpers
=============================================================================

These are direct tests, not RC command-line cases. Historical provenance of
these two utilities is uncertain. No production caller has been invented.

  whitespace: original 0x0100f3d0..0x0100f3fb, 43 bytes, stdcall(char), RET 4.
    One loop supplies all256 low-byte values with nonzero high argument bytes.
    Native -1 means byte ff, tab, LF, CR or space; all other bytes return0,
    including VT and FF. The signed-char/ff behavior is preserved.

  basename: original 0x0100f530..0x0100f58d, 93 bytes, stdcall(path,dest), RET 8.
    Six examples cover empty/plain paths, mixed slash/backslash, drive colon,
    drive/directory and trailing separator. The returned count includes NUL.
    Ninety-six destination bytes include leading/trailing canaries and unused
    destination space; every byte is checked after each call.

For each helper the first return also checks callee stack cleanup and preserved
EBX, ESI, EDI and EBP. The original was captured and independently repeated
before implementation. results.bin retains actual returned values and, for
basename, the actual guarded buffer; paired execution requires byte identity.
The expected original streams are empty.

The tiny external direct-run.exe debugger is built from src/direct-run.c and is
excluded from RC's production link. It stops both images at their PE entry,
keeps that INT3 as a return barrier, supplies a labelled direct stdcall frame
on existing target stack memory and starts the selected helper. It injects no
implementation code. At an unaccepted helper the existing mask traps normally;
that logical frontier is compared before acceptance. Accepted helper bytes and
full extents use the normal mask/replay records. RC startup and CLI are not run.
Successful test completion terminates the target with0 under controller control;
that status means direct-test success, not a natural RC process exit.

    make -j2 build/direct-run.exe
    python3 scripts/capture-direct.py

The capture verifier uses this worker's existing private Wine prefix. --record
is allowed only before manifest.json exists; the frozen original oracle cannot
be overwritten in that mode. The argument words whitespace/basename select the
external test mode; they are never presented as resource compiler CLI options.
