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

The reconstruction covers 469 application functions and 101,605 unique original
code bytes. Of these, 398 have original/native execution-pair evidence, 70 have
source/disassembly, period-compiler, graph/ABI and authored native-unit
qualification, and one is statically empty. Units carry no original-runtime
claim. The output/group-index units check exact ASCII/high-byte/read-ahead
output and real segment lookup without changing borrowed owners. The resource
units cover the older v2 WRes and Win16 MRes directory readers: counted names,
case-insensitive lookup, language links, duplicate suppression, exact payload
positions/content, EOF/partial records and list teardown. Their inputs are
source-authored typed resource records, not copied executable data. The older
MRes header failure path retains its original partial-name allocation leak;
allocation failures and arbitrary malformed-resource domains are not qualified.
The WOMP units use real object allocation, attachment and output APIs to check
physical references,16/32-bit miscellaneous records, nested iterated data,
COMDAT explicit bases, line-symbol widths and relocation deltas. Exact OMF
headers, payloads and checksums match their source-authored expectations;
borrowed data and record cursors are preserved. The PharLap path widens each
nested16-bit repetition count to32 bits through the real recursive helper.
These units do not claim assembler CLI reach for WOMP-only record variants.
The object-buffer units add real record API roundtrips for16/32-bit fields,
flag-selected widths and one/two-byte OMF indices through0x7fff; they assert
exact endian bytes, cursor movement, flag stability and borrowed-owner bounds.
Resource-writing units add valid v2/Win16 serialization and real-reader
roundtrips with exact scalar/header/name bytes and cursor/payload checks. The
actual default byte-to-wide callback uses its shared512-byte buffer or an owned
long-string allocation. The32-bit-name reader projects each UTF16 code unit's
low byte; this is not general Unicode support. Caller records and conversion
inputs remain unchanged; short counted IDs and long terminated strings retain
their distinct limits.
Collection units cover three64-element array blocks, borrowed defaults,
callback traversal/early stop, queue splicing, carved fixup shallow copies and
successful live-owned24→64→8 reallocations. They preserve links and defined
payload prefixes, freeing only the final allocation owner. The original
`AsmRealloc` incorrect NULL-input check remains; allocation failure, zero-size,
NULL-input and private-heap behavior are not qualified. Queue tests exclude
the original both-empty splice defect. The CLI dispatcher still rejects
`.startup`/`.exit` despite retaining an unused helper body; it is not enabled.
The separate `Startup` unit invokes that retained helper with typed module and
token owners, checking exact DOS near/far and DOS/OS2 exit source templates
through real queue allocation, consumption and teardown. It preserves the
shared startup-label/flag state and omits later-source fields absent in1994.
This is explicit helper qualification, not CLI availability.
Two terminal units initialize the real hash-bound message resources, then check
`InternalError("contract.c",37)` and `DiskError` for exact diagnostics, exit255
and an empty file inventory. Suppressing the banner is an explicit unit
precondition, not an observed CLI state. The selected internal-error format
fits its original80-byte buffer. The final rebuilt InternalError E8 is checked
as a process-exit call at its real owner end; no return is invented.
The retained six-byte `BeQuiet` helper is separately checked for return1 and
unchanged options, module state and line number; it adds no CLI reach claim.
The empty `PObjUnRegList` is statically qualified at the RET shared with
`PObjRegList`: zero additional bytes and explicitly excluded from the assembly
cases' entry orders, without a runtime observation claim.
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
All 469 graph/reference/ownership/extent checks pass, with 364 ordinary RET
contracts and 105 finite cold-error, tail-wrapper or exit checks. Reports and
raw artifacts remain in scratch.

The ten cached CLI fixtures and ten authored native unit cases pass. The
quiet-helper unit takes10.90ms (setup6.81ms, execution2.09ms, checks1.99ms).
`make -j4 check unit` took691.52ms, including fixture relinks; the full
graph/finite audit took960.90ms. Native and period builds passed; separate
build time was not measured. Unit cases perform no original capture and retain
the ten unchanged CLI references.

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
