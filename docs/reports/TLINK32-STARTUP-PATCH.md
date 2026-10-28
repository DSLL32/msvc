# BC4.0 TLINK32 startup compatibility copy

The pristine executable remains `toolchains/bc4.0/BC4/BIN/TLINK32.EXE`.
The shared original-tool launcher selects `toolchains/bc4.0/.patched/TLINK32.EXE`
for explicit `original tool bc4.0 TLINK32` invocations and verifies its SHA-256.
The adjacent `TLINK32-startup-patch.json` records both hashes and a reproducible
seven-byte recipe. This is a runtime compatibility change, with no application
reconstruction credit.

## Observed failure

The startup routine at VA `0x4394b` passes `EBP-0x3c` to `GetStartupInfoA`.
The first 56 bytes, through `lpReserved2`, fit before the separate `gotinfo`
local at `EBP-4`. Wine writes the full 68-byte structure: `hStdInput` overwrites
that flag, `hStdOutput` overwrites saved EBP, and `hStdError` overwrites the
return address. Debugger inspection across the call at `0x43981` confirmed that
the saved frame and return changed to `0xffffffff`, causing the startup crash.

The shipped public headers and runtime source describe the full 68-byte layout.
An older private truncated definition is a plausible explanation for the binary,
but neither its original build inputs nor period Windows behavior were proven.
This patch is a local workaround, not a conclusion about a Wine API defect.

## Patch

The local frame grows from `0x4c` to `0x50` bytes. The structure moves to
`EBP-0x44`, with its two used field accesses shifted accordingly. The independent
`gotinfo` local moves to `EBP-0x50`. Other locals reuse structure storage only
after the required fields have been loaded. Instruction lengths and application
addresses are unchanged.

| File offset | Before | After |
|---|---|---|
| `0x2474c` | `4c` | `50` |
| `0x24778` | `fc` | `b0` |
| `0x2477f` | `c4` | `bc` |
| `0x24789` | `f6` | `ee` |
| `0x2478c` | `f8` | `f0` |
| `0x247bf` | `fc` | `b0` |
| `0x24856` | `fc` | `b0` |

Original SHA-256:
`476fdc04c03e9610763f6d06a8bd59904ead98fe7018b3a84d6aa6e066bfe522`

Patched SHA-256:
`5d9f6feb63f2a588936cd46cd616ec223cddd62dc7505880c611af1fca488ed2`

## Validation and scope

The scratch candidate printed 924 usage bytes and exited 1, linked a
Borland-compiled hello object into a working PE executable, and generated a
31,665-byte detailed MAP. Absent, valid 19-byte, and size-mismatched inherited
handle information were exercised. A breakpoint confirmed the valid-information
branch; debugger inspection confirmed the patched call preserves EBP and return.
After installation, the shared launcher passed the same no-argument and
hello/MAP link checks; all three focused toolchain-selection tests passed.
Raw diagnostic evidence remains under
`~/.ai-tmp/msvc2/bc4-tlink-investigation-20261027/`.
Broader linker workloads and oversized inherited-handle counts are unqualified.

The launcher does not redirect a TLINK32 child spawned internally by BCC32.
Use compile-only BCC32 invocations and a separate explicit linker command.
The BCC reconstruction starts with its ordinary no-argument invocation; the
compatibility copy does not change BCC32's reference identity.
