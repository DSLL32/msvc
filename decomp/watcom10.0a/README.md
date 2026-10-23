# Watcom 10.0a

Reconstruct `binb/WCC386.EXE`, `binb/WPP386.EXE`, `binb/WASM.EXE`,
`binnt/WLIB.EXE`, and `binnt/WLINK.EXE` in the corresponding binary folders.
`toolchain-import.json` records the copied originals and selected hashes.

Application reconstruction is paused for the user's plumbing review and explicit
case assignments. Do not select cases, reconstruct further application functions,
or accept pending candidates autonomously. Only the already agreed stopped-loader
CF capture/gate and minimal normal-setjmp compatibility work may finish before
that review. Existing accepted functions and pending source remain distinct.

The initial Open Watcom source release is available read-only at
`/root/.ai-tmp/watcom10/open-watcom-initial-6b9cb443`, commit
`6b9cb4438959d845a2b4dd4bf19f7908ecd608e9` (2002-05-15). It may supply source,
names, and design context; the 1994 binaries remain authoritative for behavior,
function boundaries, and ABIs.
Retain applicable copyright/license notices when reusing released source and
record its exact path, commit, and binary-specific adaptations.

WLIB and WLINK are PE32 programs. Their Wine captures use separate
`.patched/WLIB.EXE` and `.patched/WLINK.EXE` copies for the proved resource-reader
GS compatibility substitutions. WASM uses the unchanged `.patched/binnt/WASM.EXE`
loader with `.patched/binb/WASM.EXE`. Patch hashes, exact sites, and selector and
buffer-ownership limits are recorded in the corresponding case metadata. Keep
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

Before accepting functions, verify original/native compiler probes, CF code
ownership and mask handling, and graph-audit input support. A blocked build or
unrun case does not count as a successful reconstruction.

`tooling/graph-audit/` supports explicit CF image-offset inputs and actual Watcom
PE MAP files (`--map-format watcom10`). Declare proved code, shared data, and SEH
roots explicitly. CF import and callback-table checks remain unsupported; RET
cleanup comparison does not replace the target's Watcom ABI checks. The separate
`tooling/watcom-capture/` controller must verify observed allocation and relocated
bytes and stop unknown transfers before execution; its byte verifier is not a
code classifier or proof that a process was observed.

Use the shared repository Wine prefix for explicit original builds and captures.
Cached fixtures and debugging run natively on i386 Linux, without Wine. Shared
compatibility changes belong to `/root/recon_lib_maintenance`.
