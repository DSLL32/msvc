# Pending virtual work-buffer allocator

Candidate reserve_work_buffer owns62 hot bytes38cc9..38d07 and30 cold bytes
7208a..720a8. Its actual cdecl formals are unsigned reserve_bytes/commit_bytes;
all callers observed so far pass0x500000/0x4000. Source preserves the two genuine
VirtualAlloc calls and existing named report_compiler_error(60) failure stubs.
No allocator bytes are durably accepted yet.

The strict Rust register-import resolver verifies original IATc9204 cached inEDI,
with calls38cdf and38cf8; the rebuilt compiler emits two direct import calls.
Two finite normal-return contracts are hash-bound: actual SDK VirtualAlloc and
the inspected ordinary14-byte report_compiler_error wrapper at91a45. Static
ablation in allocation-contract-checks.json proves each contract is necessary,
without granting any callee reconstruction credit. Provider file/export identity
and SDK declaration evidence are in allocation-preservation-v1.json.

Required fresh runtime proof before promotion: actual loaded-hFile kernel32 and
kernelbase identities, resolved target IAT, all four actual API entry/return args,
EDI preservation and RET16, unchanged input LastError203 (or a demonstrated
shared-shim correction), two genuine helper returns, full initial16KiB zeros,
reserved/committed map ownership and only the known buffer/cleanup/command writes
before the next named intermediate-file helper. The uncommitted reserved remainder
is opaque. Native observations use Linux GDB/maps only; originals remain explicit
shared-prefix reference captures outside fixtures.
