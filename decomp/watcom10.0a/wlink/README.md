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

The compiler/build probe must establish the Watcom binnt WCC386 stub's embedded
CF invocation, `-bt=nt` headers/library selection, and register-call convention
before building target C. WLINK itself is the patched Win32 linker for explicit
original builds. Native GCC/Clang must preserve the independently established
application ABI through reviewed support; normal calls at a precisely declared
Win32/CRT boundary may differ internally. Shared support belongs to the recon
library maintainer, not this binary.

The shipped `h/STDIO.H` declares a packed, 26-byte Watcom `FILE` on i386. Existing
MSCRT `ShimFILE` is not layout-equivalent. Preserve original field accesses and
prove reached buffer/position/error semantics before requesting a shared Watcom
CRT profile; register-call transport is a separate proof obligation.
