WLINK main is accepted: **1 application function / 232 bytes**, zero CRT
functions. The help case remains in progress at `Spawn` (VA416d3c), before its
body. Paired evidence matches argc/argv, callback identity, the stored argv alias
and initial application state. The original's entire 169,984-byte executable
section was masked at main; only main's 232 bytes were restored. Source and exact
proofs are in `cases/help/main-prefix-01/` and `main-source-04/`.

Use `make -j4 native` for the instrumented i386 GCC build and GDB debugging.
`make -j4 prefix-fixtures` checks the accepted native entry trace, expected INT3,
streams and artifacts against cached original evidence, with measured timings.
It never launches Wine. Full `fixtures` remain blocked until help completes.
`make -j4 original` explicitly compiles/links with Watcom10 under the shared Wine
prefix; the rebuilt PE is not executed. `-s` avoids an extra compiler stack-check
call absent from the target. Native GCC calling conventions are permitted by the
user; the original Win32 build retains Watcom's genuine register ABI.

Main's original and rebuilt bodies have no RET and end with their authentic
exit tail. A real Watcom `#pragma aux exit aborts;` probe establishes the source
annotation. Root approved this main-only positive endpoint proof, supplementing
canonical CFG/call/reference/ownership/extent audits. Strict RET checks remain
required for returning application functions; no CRT body is accepted by it.

First masked capture took 0.716s, fresh native GDB 0.125s, and direct native entry
tracing 0.0037s. Earlier separate unmasked help captures returned exit0 and 8,475
stdout bytes. The later import observer confirms resource reads from the tool's
own executable, five failed wlink.hlp searches and pagination EOF from NUL.
Those observations do not relabel the earlier subprocess transport's unknown
inputs. Raw Windows environments/logs remain private0600; public compressed
logs are explicitly derived redacted views, with raw identities retained.

Pristine `binnt/WLINK.EXE` remains unchanged. Explicit reference captures/builds
use the independently verified `.patched/WLINK.EXE`; its four GS substitutions
change nine bytes without shifting addresses. Metadata/patch identity checks
are in `make inspect`. No arbitrary far-pointer equivalence is claimed. The
BEGTEXT section mixes code and data; appended WRes resources are outside mapped
sections. Future restores require actual code/text-data provenance.

Later source reference: `/root/.ai-tmp/watcom10/open-watcom-initial-6b9cb443`,
commit6b9cb4438959d845a2b4dd4bf19f7908ecd608e9 (2002). Names/source guide
identification; original1994 function extents, graph, ABI and data remain
authoritative. Shipped Watcom FILE is packed26B, not shared MSCRT32. The next
Spawn function uses a52B jmp_buf; normal-only shared caller-frame setjmp has now been validated in an isolated
pending-source build. The52B owner stays untouched; real return0, its aliases
and native FP state are preserved (startupCW127f). Bare/indirect saves and all
longjmp/SEH unwind remain unsupported125. The production main/stub build was
not changed or relabeled. See `cases/help/normal-plumbing-01/summary.json`.
Application reconstruction is paused for user review; Spawn is not accepted.
