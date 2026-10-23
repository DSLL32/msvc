WLINK 10.0a is a native Win32 PE executable. It does not use the binnt compiler
stub or a CF payload. The explicitly authorized, unmasked patched `WLINK ?`
baseline exits zero with 8,475 stdout bytes, empty stderr and no case-directory
artifacts. One repeat with the same actual cwd, host environment and known input
identities produced identical raw output. Execution took 0.365 and 0.616 seconds.
References are under `cases/help/reference/`; no application function or masked
replay has been accepted. Actual external file reads remain unobserved.
The help contains one pagination prompt even with DEVNULL stdin; its actual
console/read behavior remains an observer obligation despite successful exit.

`make inspect` verifies the pristine binary and the separately supplied Wine GS
patch, then reports PE metadata without running either file. `make -j4 native`
builds the real i386 INT3 main scaffold and directly compiles the shared C at final
link. GDB confirms SIGTRAP in main; this is not an accepted implementation.
`original` uses the Watcom compiler and patched linker. Its INT3 scaffold compiles
with zero warnings/errors and links successfully; the resulting PE has not been
executed. Watcom's `#pragma aux reconstruction_trap = 0xcc;` emits the trap inline
without a helper function. Both build checks are recorded in `cases/help/scaffold-build.json`.
`fixtures` remains blocked until an actual reconstruction and trace exist.

Source reference: `/root/.ai-tmp/watcom10/open-watcom-initial-6b9cb443`, commit
`6b9cb4438959d845a2b4dd4bf19f7908ecd608e9` (2002). Likely reference units are
`bld/wl/c/wlink.c`, `cmdline.c`, `wlnkmsg.c` and WRes. They guide identification;
the target's function extents, callers, argument registers and state still need
independent proof. Preserve the upstream license on any reused source.

The sole initial case is `WLINK ?`; its capture plan is in `cases/help/plan.json`.
`scripts/capture-baseline.py` is an explicit reference action, never a fixture
dependency. The exact host environment is retained privately in scratch with a
hash in the receipt; the target's Windows environment has not yet been observed.
Use only the shared Wine prefix for explicit reference captures/builds. Fixtures
must run a native i386 reconstruction against cached references without Wine.

The original image remains authoritative for addresses and unmodified bytes.
Reference execution will use `.patched/WLINK.EXE`, whose four documented sites
preserve size and addresses but change GS-dependent resource accesses. Keep both
hashes and the explicit substitution map in masking/audit evidence. This patch
does not establish equivalence for arbitrary far pointers. App code, internal
resource helpers, CRT and imports require explicit classifications; no entire
unidentified range is excluded as CRT.

The code section is `BEGTEXT`, not `.text`. Its entry jump is followed by inline
CRT identification data. The appended resource overlay lies outside all mapped
PE sections; preserve its identity when preparing masked reference copies.

The common WCC386 probe passed with `-bt=nt -3r -od -d2`; its receipt is under
`~/.ai-tmp/msvc2/watcom10.0a/wcc386/abi-probe/`. Target headers, library selection,
link and startup still need proof. WLINK itself is the patched Win32 linker for
explicit original builds. By explicit user decision, native i386 C uses GCC's
ABI; the original Win32 build must preserve and audit the actual Watcom ABI.
Compare native semantic arguments and ownership rather than requiring the same
raw argument registers. Do not add per-function assembly veneers or OMF transport.
Shared host support belongs to the recon library maintainer, not this binary.

The shipped `h/STDIO.H` declares a packed, 26-byte Watcom `FILE` on i386. Existing
MSCRT `ShimFILE` is not layout-equivalent. Preserve original field accesses and
prove reached buffer/position/error semantics before requesting a shared Watcom
CRT profile; the original build's register-call ABI is a separate proof obligation.

`cases/help/architecture.json` records the proposed masking/startup procedure;
`observer-plan.json` proposes a bounded common-debugger observation of main and
actual console/input APIs. Static analysis identifies main at VA `0x4242e0`
called with EAX=argc and EDX=argv by CRT code at `0x438199`. Its Spawn callback
and cleanup tail differ from the later source; names/extents and any shared
entry tails must be proved against this binary before acceptance.
