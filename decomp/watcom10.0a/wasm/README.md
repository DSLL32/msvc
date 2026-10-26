# Watcom 10.0a WASM

Two cases are complete: `/?` (exit 1) and `/q /fo=transform.obj transform.asm`
(exit 0). The assembly workload combines includes, macros, conditionals, typed
records, integer/x87 instructions, branches and external relocations; its
485-byte OMF object matches exactly. Inputs and references are frozen in the
numbered case captures, with no Wine during fixtures.

The reconstruction covers 329 application functions and 80,523 unique original
code bytes. One empty function, `PObjUnRegList`, is statically qualified at the
RET shared with `PObjRegList`: zero additional bytes and explicitly excluded
from the assembly case's entry order, without a runtime observation claim.
Original CRT bodies execute unchanged and receive no application credit.
Unsupported instruction fields and pending functions still trap. The original
empty-line read-before-buffer defect remains in the period build; the native
branch uses the documented, twice-observed no-join outcome.

Use `make -j4 native cases win32`, `make -j4 check`, and `make -j4 audit`.
All 329 graph/reference/ownership/extent checks pass, with 258 ordinary RET
contracts and 71 finite cold-error, tail-wrapper or exit checks. Pending error
handlers stay masked. Reports and raw artifacts remain in scratch.

Final native fixtures: help 15.02 ms and assembly 16.24 ms (assembly setup
8.54 ms, execution 4.29 ms, checks 3.41 ms). Parallel Make check took 70.82 ms;
incremental native/case/period build setup took 136.84 ms; the full audit took
675.70 ms. The final explicit original capture took 1.59 s. These are measured
closeout runs, not clean-build timings.

The captured original is `.patched/binb/WASM.EXE` through the byte-identical
`.patched/binnt/WASM.EXE` loader, hash-bound by `harness/cf-profile.json`. Its
four fixed-size GS-preservation substitutions are at CF offsets `1e611`
(`8e6d18`→`909090`), `1e634` (`65`→`3e`), `1e653`
(`0fb55514`→`8b551490`), and `1e65b` (`65`→`3e`). Resource bytes and relocation
sites are unchanged. This help path supplies the caller-owned 128-byte buffer,
capacity 128 and DS far selector; no general segmented-pointer support is claimed.
Historical patch creation/qualification remains in Git commits `8b898631` and
`25bcbf9e`; historical captures are not the current oracle.

Released-source adaptations retain copyright/license headers and their source
paths from Open Watcom commit `6b9cb4438959d845a2b4dd4bf19f7908ecd608e9`
(read-only reference: `~/.ai-tmp/watcom10/open-watcom-initial-6b9cb443`). Shipped
1994 bytes govern differences from that source, including scanner behavior,
token IDs, resource layouts and the two distinct private hash bodies. Watcom
builds retain the original register/far-pointer ABIs; native builds use the
explicitly authorized GCC ABI and shared `lib/recon` support.
