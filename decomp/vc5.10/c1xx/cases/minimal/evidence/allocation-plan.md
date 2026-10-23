# Virtual work-buffer allocator

Candidate reserve_work_buffer owns62 hot bytes38cc9..38d07 and30 cold bytes
7208a..720a8. Its actual cdecl formals are unsigned reserve_bytes/commit_bytes;
all callers observed so far pass0x500000/0x4000. Source preserves the two genuine
VirtualAlloc calls and existing named report_compiler_error(60) failure stubs.
The sole allocator candidate is now accepted; actual bytes and ABI are audited.

The strict Rust register-import resolver verifies original IATc9204 cached inEDI,
with calls38cdf and38cf8; the rebuilt compiler emits two direct import calls.
Two finite normal-return contracts are hash-bound: actual SDK VirtualAlloc and
the inspected ordinary14-byte report_compiler_error wrapper at91a45. Static
ablation in allocation-contract-checks.json proves each contract is necessary,
without granting any callee reconstruction credit. Provider file/export identity
and SDK declaration evidence are in allocation-preservation-v1.json.

Fresh paired runtime proof verifies: actual loaded-hFile kernel32 and
kernelbase identities, resolved target IAT, all four actual API entry/return args,
EDI preservation and RET16, unchanged input LastError203 (or a demonstrated
shared-shim correction), two genuine helper returns, full initial16KiB zeros,
reserved/committed map ownership and only the known buffer/cleanup/command writes
before the next named intermediate-file helper. The uncommitted reserved remainder
is opaque. Native observations use Linux GDB/maps only; originals remain explicit
shared-prefix reference captures outside fixtures.

The natural -2/-2 pair and two explicitly separate restore auxiliaries pass.
The full original final BSS differs only at the four buffer pointers, argc/argv,
cleanup count and cleanup_main slot; both8520-byte initialized/saved raw images
remain unchanged. Every prior modeled native field and its storage map remains
checked.43 affected negative controls reject ABI/formal/provider/region/zeroing/
continuation/storage drift; earlier unchanged work controls are reused explicitly.
`allocation-trials.json` contains current validator hashes and split timing.

The natural initial -1/-1 observation lacks the final whole-BSS guard and grants
no acceptance credit. After extending the observer a proper fresh -2/-2 pair was
captured. A scratch orchestration import-path fix then validated those unchanged
raw executions; validator-only guard additions also reuse original/raw snapshots.
