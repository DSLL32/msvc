# Watcom 10.0a WASM

Ten cases are complete: `/?` (exit 1), five successful assembly workloads,
configuration/error-limit diagnostics (exit 7 and exit 1), and a missing-source
fatal error and invalid-option diagnostic (both exit 1).
The buffer transform combines includes, macros, conditionals, typed records,
integer/x87 instructions, branches and external relocations (485-byte OMF).
The DOS record formatter adds small-model segments, IRP, macro-local labels,
stack arguments/locals, COMMON and debug lines (561-byte OMF, 54 lines without
diagnostics). The flat32 vector library adds CPU/model and name options,
include-path lookup, aliases, string conditionals, register arguments, quoted
identifiers, TBYTE data and embedded-code data records (362-byte OMF). Each
object matches exactly. The segmented286 x87 library adds emulation patch
fixups, explicit segment frames and C naming (359-byte OMF), also matching
exactly. The diagnostic workload exercises included macro error directives,
string/value/defined-symbol conditions, macro-call notes and alignment warnings.
Its seven errors and one warning match exactly; redirected stdout suppresses
`/fe` error-file creation, and the erroneous object is deleted.
The DOS screen library adds long286 conditional-branch expansion, group
relocations and unrolled segment-override stores (445-byte OMF); both
jump-extension warnings and the object match.
The record-contract case adds nested procedures, paired16-bit address registers
and the error-limit exit. Its exact diagnostics include the original embedded
NUL in the final prefix; immediate termination leaves the captured zero-byte
object rather than performing normal deletion.
The missing-source case matches its fatal message and empty file inventory.
The fatal callback records are checked against the original CF fixups and
rebuilt symbols; the selected NULL-action record makes no shutdown-call claim.
The option-contract case expands nested environment option variables and rejects
the later-only `/fr` spelling with the exact original message. The1994 `@`
syntax names an environment variable, not a response file.
Inputs and references are frozen in numbered captures;
fixtures never invoke Wine.

The reconstruction covers 401 application functions and 94,646 unique original
code bytes. Of these, 398 have original/native execution-pair evidence. The
separate native unit case qualifies `output` and `GetGrpIdx` through released
source/1994 disassembly, period compilation, graph/ABI checks and typed assertions;
it carries no original-runtime claim. Output checks include ASCII, high bytes
and an owned two-zero-byte empty buffer matching the original read-ahead.
Group-index checks use the real segment lookup and preserve borrowed owners. One empty function, `PObjUnRegList`, is statically qualified at the
RET shared with `PObjRegList`: zero additional bytes and explicitly excluded
from the assembly cases' entry orders, without a runtime observation claim.
Original CRT bodies execute unchanged and receive no application credit.
Unsupported instruction fields and pending functions still trap. The original
empty-line read-before-buffer defect remains in the period build; the native
branch uses the documented, twice-observed no-join outcome. Native `GetAssume`
projects the original selector 8 read onto its actual `ModuleInfo.flat_idx`
owner rather than depending on host global placement; other unknown selectors
remain unsupported. The original WOMP warning callers have an incompatible
`PrtMsg` declaration: period builds preserve their actual calls, while native
builds explicitly trap those unreached malformed-call paths. The proper
assembler diagnostic interface retains its two independent `va_list` arguments.

Use `make -j4 native cases win32`, `make -j4 check`, and `make -j4 audit`.
All 401 graph/reference/ownership/extent checks pass, with 317 ordinary RET
contracts and 84 finite cold-error, tail-wrapper or exit checks. Pending error
handlers stay masked. Reports and raw artifacts remain in scratch.

The ten cached CLI fixtures and the authored native unit case pass. The unit
case takes 8.38 ms (setup 5.63 ms, execution 1.12 ms, checks 1.63 ms), and its
two forked assertion groups also pass. Parallel Make check took 114.35 ms;
the full graph and finite RET audit took 835.06 ms. Native and period builds
passed; incremental build time was not measured. This unit case performs no
original capture and retains the ten unchanged CLI references.

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
