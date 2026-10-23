WLINK 10.0a is a native Win32 PE executable. It does not use the binnt compiler
stub or a CF payload. No original execution or application acceptance has taken
place in this scaffold.

`make inspect` verifies the pristine binary and the separately supplied Wine GS
patch, then reports PE metadata without running either file. `native`, `original`
and `fixtures` deliberately stop until architecture review; use `make -j4` once
their actual compilation rules are established.

Source reference: `/root/.ai-tmp/watcom10/open-watcom-initial-6b9cb443`, commit
`6b9cb4438959d845a2b4dd4bf19f7908ecd608e9` (2002). Likely reference units are
`bld/wl/c/wlink.c`, `cmdline.c`, `wlnkmsg.c` and WRes. They guide identification;
the target's function extents, callers, argument registers and state still need
independent proof. Preserve the upstream license on any reused source.

The sole initial case is `WLINK ?`; its capture plan is in `cases/help/plan.json`.
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
CRT profile; register-call transport is a separate proof obligation.

`cases/help/architecture.json` records the proposed capture, masking, startup and
build procedure. Static analysis identifies a main candidate at VA `0x4242e0`
called with EAX=argc and EDX=argv by CRT code at `0x438199`. Its Spawn callback
and cleanup tail differ from the later source; names/extents and any shared
entry tails must be proved against this binary before acceptance.
