# Watcom 10.0a

Reconstruct `binb/WCC386.EXE`, `binb/WPP386.EXE`, `binb/WASM.EXE`,
`binnt/WLIB.EXE`, and `binnt/WLINK.EXE` in the corresponding binary folders.
`toolchain-import.json` records the copied originals and selected hashes.

The initial Open Watcom source release is available read-only at
`/root/.ai-tmp/watcom10/open-watcom-initial-6b9cb443`, commit
`6b9cb4438959d845a2b4dd4bf19f7908ecd608e9` (2002-05-15). It may supply source,
names, and design context; the 1994 binaries remain authoritative for behavior,
function boundaries, and ABIs.

WLIB and WLINK are PE32 programs. WLINK reference execution uses only the
separate `.patched/WLINK.EXE`; its GS compatibility substitutions, original and
patched hashes, and validation provenance are recorded beside it. Keep the
unmodified executable intact and distinguish patched capture provenance.

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

Initial workers are inspecting and scaffolding. Before accepting functions,
resolve exact Watcom register conventions, native/original compiler probes,
CF code ownership and mask handling, and graph-audit input support. GCC
`regparm(3)` does not implement Watcom's EAX/EDX/EBX/ECX convention. No blocked
build or unrun case counts as a successful reconstruction.

Use the shared repository Wine prefix for explicit original builds and captures.
Cached fixtures and debugging run natively on i386 Linux, without Wine. Shared
compatibility changes belong to `/root/recon_lib_maintenance`.
