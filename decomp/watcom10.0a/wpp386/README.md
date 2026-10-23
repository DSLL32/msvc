# WPP386 10.0a

Groundwork only; no original execution or accepted functions yet. `make inspect`
validates the copied payload/stub and reports CF header/relocation facts without
executing either. `make native`, `make original`, and `make fixtures` deliberately
fail until their prerequisites are implemented. The `src/main.c` INT3 scaffold
is not an accepted reconstruction or an operational test harness.

The program is `binb/WPP386.EXE`. Its `binnt/WPP386.EXE` PE stub loads the CF
payload directly and supplies a DOS-register-to-Win32 service gateway. Original
logical addresses must be CF payload offsets, independent of the allocated base;
the PE stub address is not the application's identity.

Reference source is read-only scratch checkout
`/root/.ai-tmp/watcom10/open-watcom-initial-6b9cb443`, exact commit
`6b9cb4438959d845a2b4dd4bf19f7908ecd608e9` (2002-05-15). Relevant files:

- `bld/w32loadr/loader.c`, `loader.h`, `int21.asm`, `int21nt.c`: later loader and gateway.
- `bld/plusplus/c/plusplus.c`: `parseCmdLine`, `front_end`, later `WppCompile`.
- `bld/plusplus/c/cusage.c`: `CCusage`, paging and usage text.
- `bld/plusplus/c/watcom.c`: `CBanner`.

These are analysis aids, not proof that the 1994 program has identical functions,
globals, interfaces or output. Preserve applicable original notices when adapting
source. Do not transplant the 2002 DLL driver into the older executable.

Static discovery identifies CF startup at `0x664f4`, gateway wrapper at `0x665c7`,
and a `__CMain` candidate at `0x703f8` calling the application `main` candidate at
`0x43fef`. These names and ranges remain provisional until observed and audited.
Startup clears `0xbc438..0xbf6a0`; this overlaps the consumed relocation stream.
Masking must therefore preserve typed data and loader relocation inputs rather
than blindly replacing the whole payload with traps. Payload offset zero is
data, not the entry point. No executable extent map has been accepted.
The CF relocation stream has 15,085 writes in twelve groups. The original file
also has 24 bytes after the loaded payload, containing its patch-level marker;
these bytes are not loaded and are not counted as reconstructed data.

The help proposal uses bare `?` with redirected handles, as supported by the later
source; the actual 1994 behavior, paging, status and streams must be captured.
`cases/help/case.json` records the proposal without fabricated master outputs.

The user selected ordinary GCC i386 ABIs for native tests, and exact original
Watcom ABIs for the original-toolchain build and its audit. Native observers must
decode the actual GCC formals and compare semantic values, pointer ownership,
aliases and effects with independently decoded original register/stack formals.
Register locations need not match across builds. There are no ABI veneers,
OMF-to-ELF conversions or disguised extra application vertices. In particular,
GCC `regparm(3)` is not a substitute for Watcom's EAX/EDX/EBX register convention.

Remaining gates are CF layout/discovery/masking/observer integration with the
PE-oriented graph auditor, positive library/startup provenance, and the common
original-compiler probe. Do not silently filter application vertices. The shared
compatibility maintainer owns native host support; application code may call
ordinary Win32/CRT APIs across the explicitly reviewed service boundary.
The concrete capture/build proposal is in `cases/help/capture-plan.md`; it must
be reviewed before reference execution or application-function acceptance.

Builds will retain original Watcom compiler checks through shared Wine prefix
`/root/.ai-tmp/vc1.0-capture/wine`, with native i386 fixtures and `make -j4`.
No private prefix, drive mapping, target-local shim library or CRT reconstruction
is introduced here.
