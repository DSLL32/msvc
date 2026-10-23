# Watcom 10.0a WLIB

Entry scaffold for `toolchains/watcom10.0a/binnt/WLIB.EXE`; zero accepted
application functions and no successful native fixture yet. `make check`
verifies the original hash and PE metadata in `binary.json`. `make -j4 native`
builds the native i386 named `main(argc, argv)` INT3 stub with shared recon C
compiled at final link. `make -j4 reference-build` builds its Win32 PE with
original Watcom 10.0a and the separately verified patched shipped WLINK.
Native tests use GCC i386 conventions and semantic arguments/ownership;
the Win32 build preserves and audits exact Watcom conventions.

Both builds passed. Watcom compilation took 0.532 seconds with zero warnings or
errors; linking took 0.624 seconds. A measured fresh native build took 1.217
seconds and GDB verification 0.165 seconds: real SIGTRAP at the named `main`,
actual argc 2, argv option `-?`, and byte CC immediately before the stopped PC.
The pragma intrinsic emits INT3 inline and adds no function/helper vertex.
Source/object/ELF/MAP/PE and build-log hashes are recorded under
`cases/help/scaffold/`. The rebuilt Win32 PE was not executed. `make fixtures`
still fails explicitly on the held original help/GS reference.

The original is a native i386 PE32 console program, SHA256
`f20631839a9c1c38969f0c5930e26010361c6175790992181455b5c0ffb1cbb3`.
Image base is `0x400000`; entry RVA `0x18690` jumps to `0x41992b`, where startup
calls `0x418e8b` then `0x41afaa`. These are observed addresses, not accepted
application identities. There are 25 KERNEL32 imports, no exports, no debug
directory, and no COFF symbols. Executable `BEGTEXT` includes code and literal
data: immediately after the five-byte entry jump is a CRT copyright banner.
Never treat linear disassembly of that section as a function inventory.

The five section records and appended 3,858-byte overlay are recorded in
`binary.json`. Help/error strings occur in the overlay; their resource encoding
and lookup boundary remain to be established. No message table is copied into
reconstructed source. The binary identifies itself as WATCOM Library Manager
Version 3.0, copyright 1988, 1994, with patch level `.a`.

Internal code uses Watcom register arguments, including EAX, EDX, EBX and ECX.
The path-splitting helper at `0x418733` also consumes two stack arguments with
`ret 8`. GCC `regparm(3)` does not implement this ABI. Exact register assignment,
preservation, stack cleanup and return forms need per-signature proof in the
original-toolchain build. GCC native tests intentionally use GCC's own ABI,
without per-function veneers or raw-register identity claims.

Source reference: scratch checkout
`/root/.ai-tmp/watcom10/open-watcom-initial-6b9cb443`, exact initial commit
`6b9cb4438959d845a2b4dd4bf19f7908ecd608e9` (2002-05-15). Relevant files are
`bld/nwlib/c/cmdline.c`, `bld/nwlib/c/error.c`, `bld/nwlib/c/wlib.c` and
`bld/nwlib/wlib.msg`. The newer tree has `nwlib`, not the old `wlib` directory,
and includes IDE/DLL, ELF/COFF and ORL architecture absent or unproved in this
1994 target. `WlibMainLine` documents a July 1999 reset-flow change. Reuse source
only where the original instructions establish matching logic and function
edges; preserve its applicable license notices when copying code.

Original startup loads `argc` from `0x430464` into EAX and `argv` from
`0x430460` into EDX, then calls `0x412a67` at `0x41afea`. This establishes
the candidate application `main` entry and its first two original formals;
native entry observation will read the corresponding GCC stack formals.
This does not classify the entire startup or all subsequent calls as CRT.

Initial case is a candidate `WLIB -?` in an empty directory, documented under
`cases/help/plan.json`. The historical switch, stream bytes and status are not
assumed accepted from newer source. After architecture review, explicit reference
capture will establish them and the application/CRT boundary. Fixtures will
compare only the native ELF against that cache, without Wine or recapture.
`cases/help/capture-plan.md` provides the concrete execution and boundary plan
submitted for review. Its baseline execution and diagnostic results follow.

The subsequently approved unmasked baseline timed out with empty streams after
30.019 seconds. Two explicit diagnostics reached the resource reader, observed
GS change from Wine's `0x63` to flat DS `0x2b`, and reached `ExitProcess(1)` while
GS remained `0x2b`; process exit stalled. Each diagnostic terminated its owned
child with status 126 and observed the real exit debug event. These are failed
captures, not successful help or reconstruction acceptance. Raw event logs and
hash-bound receipts are in `cases/help/diagnostic/`.

The corrected diagnostic proved a caller-owned 128-byte stack buffer, actual
far selector equal to DS, completed `option\0` writes and unchanged remaining
121 bytes. Its after-character breakpoint is a shared loop join reached before
the first actual character store; it is explicitly not an adjacent post-store
observation. The proposed four-site Wine-only patch is documented in
`cases/help/diagnostic/gs-patch-proposal.json`, including exact bytes, candidate
hash, relocation nonoverlap and future-selector restrictions. No patched WLIB
file has been created or executed. The attempted `-?` currently takes a
diagnostic path; historical successful help syntax remains to be established.

The reference resource reader in
`bld/sdk/rc/wres/c/loadstr.c:GetString` closely matches the original 128-byte
chunking, string selection, bounded byte copies and final NUL. This supports
source correlation, not wholesale acceptance. WRes support logic is not assumed
to be CRT merely because it is in a library.
