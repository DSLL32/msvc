# Watcom 10.0a

Reconstruct `binb/WCC386.EXE`, `binb/WPP386.EXE`, `binb/WASM.EXE`,
`binnt/WLIB.EXE`, and `binnt/WLINK.EXE` in the corresponding binary folders.
`toolchain-import.json` records the copied originals and selected hashes.

All five binaries use the standardized shared harness and have completed their
initial help cases. See [MIGRATION-REPORT.md](../../docs/reports/MIGRATION-REPORT.md) and the
binary READMEs for committed coverage and measured checks. Further cases follow
the user's current authorization; without autonomous authorization, finish the
case, run its final checks, commit and pause for review. Keep verified functions
separate from pending source.

The initial Open Watcom source release is available read-only at
`/root/.ai-tmp/watcom10/open-watcom-initial-6b9cb443`, commit
`6b9cb4438959d845a2b4dd4bf19f7908ecd608e9` (2002-05-15). It may supply source,
names, and design context; the 1994 binaries remain authoritative for behavior,
function boundaries, and ABIs.
Retain applicable copyright/license notices when reusing released source and
record its exact path, commit, and binary-specific adaptations.

[COPYING](COPYING) contains the complete Sybase Open Watcom Public License
version 1.0, copied byte for byte from the official upstream repository's
[license.txt at commit 1b9e0ce0d53e289b194120e3e1cba76c6cab867e](https://github.com/open-watcom/open-watcom-v2/blob/1b9e0ce0d53e289b194120e3e1cba76c6cab867e/license.txt).
Its SHA-256 is `b5e6676779126951c3bf84547b577ec7370b073791ca6072aa5568c62a2d7aef`.
This is separate license-text provenance: the initial source commit above
contains the in-source notices, but no standalone license file. Reused source
retains those notices and records its adaptations; neither builds nor audits
require access to the scratch reference checkout.

WLIB and WLINK are PE32 programs. Their Wine captures use separate
`.patched/WLIB.EXE` and `.patched/WLINK.EXE` copies for the proved resource-reader
GS compatibility substitutions. WASM uses the unchanged `.patched/binnt/WASM.EXE`
loader with `.patched/binb/WASM.EXE`. Patch hashes, exact sites, and selector and
buffer-ownership limits are recorded in canonical binary metadata and capture
bindings. Keep
originals intact and distinguish patched capture provenance; the patches grant
no application reconstruction credit.

The other tools contain CF payloads in their `binb` containers. Their `binnt`
stubs allocate and relocate the payload, then enter its native i386 code and
provide a DOS register-service far-call gateway. Address identities must be
payload-relative and bound to the original container, relocation table, and
observed load base. Neither the loader's PE RVA nor the container's LX entry
identifies an application function.

The gateway is an explicit compatibility boundary: reconstructed application
code may call the required Win32 or CRT API normally through `lib/recon`.
Compare the actual service inputs, results, and effects; exclude only identified
loader, gateway, CRT, and shim internals. Application functions and their ABIs
remain subject to the reconstruction requirements.

The user explicitly permits GCC's calling convention in native Linux tests.
Original Watcom-built Win32 reconstructions must preserve and audit the actual
Watcom register convention, signatures, exports, and stack cleanup. Native
observers decode GCC formals and compare semantic values and ownership with the
original; they do not compare unlike register or stack encodings. Application
function identities and logical call graphs remain the same in both builds.
No per-function ABI veneers or object-format translation are required.

Use the already qualified shared build and capture support. During a case,
run the current-case pair, necessary builds and narrowly targeted checks; run
the complete regression suite and full audit when the case finishes. A blocked
build, an unrun case or merely matching a candidate's entry trap does not prove
execution of its implementation. Update a binary's README and commit when the
case is complete. Keep diagnostics in scratch and only the scoped case source
and `capture.bin` under cases, as described in
[STANDARDIZATION.md](../../docs/STANDARDIZATION.md).

`tooling/graph-audit/` supports explicit CF image-offset inputs and actual Watcom
PE MAP files (`--map-format watcom10`). Declare proved code, shared data, and SEH
roots explicitly. CF import and callback-table checks remain unsupported; RET
cleanup comparison does not replace the target's Watcom ABI checks. Use
`recon-tool original capture` for both PE and CF captures. The ordinary CF
debugger verifies the actual loaded allocation and relocated image, preserves
declared data, masks code, restores classified CRT and implemented App extents,
and observes real entry breakpoints or exit. The original instructions and
gateway execute normally. The separate Watcom command and instruction-admission
controller are retired; canonical profiles and registry metadata describe the
actual format, code/data layout and ownership.

Use the shared repository Wine prefix for explicit original builds and captures.
Cached fixtures and debugging run natively on i386 Linux, without Wine. Shared
compatibility changes belong to `/root/recon_lib_maintenance`.
