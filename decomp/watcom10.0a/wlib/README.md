# Watcom 10.0a WLIB

Read-only scaffold for `toolchains/watcom10.0a/binnt/WLIB.EXE`; no original
execution, accepted functions or native fixtures yet. `make check` verifies the
original hash and PE metadata in `binary.json`. Native and original build targets
deliberately fail until the shared compiler/ABI plan is reviewed.

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
preservation, stack cleanup and return forms need per-signature proof. A common
solution is being coordinated before native application bodies are introduced.

Source reference: scratch checkout
`/root/.ai-tmp/watcom10/open-watcom-initial-6b9cb443`, exact initial commit
`6b9cb4438959d845a2b4dd4bf19f7908ecd608e9` (2002-05-15). Relevant files are
`bld/nwlib/c/cmdline.c`, `bld/nwlib/c/error.c`, `bld/nwlib/c/wlib.c` and
`bld/nwlib/wlib.msg`. The newer tree has `nwlib`, not the old `wlib` directory,
and includes IDE/DLL, ELF/COFF and ORL architecture absent or unproved in this
1994 target. `WlibMainLine` documents a July 1999 reset-flow change. Reuse source
only where the original instructions establish matching logic and function
edges; preserve its applicable license notices when copying code.

Initial case is a candidate `WLIB -?` in an empty directory, documented under
`cases/help/plan.json`. The historical switch, stream bytes and status are not
assumed accepted from newer source. After architecture review, explicit reference
capture will establish them and the application/CRT boundary. Fixtures will
compare only the native ELF against that cache, without Wine or recapture.
