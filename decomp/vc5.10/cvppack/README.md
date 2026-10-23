# CVPACK groundwork (project folder `cvppack`)

The target is `toolchains/5.10/bin/CVPACK.EXE`, version 5.00.7113. The requested
folder spelling is retained. Reconstruction has not started: no accepted
functions, target implementation, masked image, native runner or coverage claim.
Release awaits the shared Rust graph tool and the DLL workflow investigation.

`make inspect` reads the PE without Wine. `cases/inventory.json` records its
SHA-256, sections, imports and available original compiler inputs. This is an
i386 console EXE at image base `0x400000`, entry `0x410330`; it has no export,
TLS, relocation or debug directory. Its `.text` virtual size is 62,662 bytes,
which is a section extent, not discovered application-code coverage.

Unlike vc1.0 CVPACK, this executable imports its CRT from `MSVCRT.DLL` and eleven
PDB/type APIs from shipped `MSPDB50.DLL`, including decorated `WidenTi::fCreate`.
The latter is an additional native dependency boundary requiring actual call,
ABI, input and state evidence. `lib/recon` does not currently implement it;
its maintainer was informed, with no speculative shim work requested. Dynamic
library strings also mention USER32; that alone does not establish an executed
dependency. Mapping imports include `CreateFileMappingA` and `MapViewOfFileEx`.

`make capture-help` is an explicit, unmodified-original-only probe using the
fixed shared prefix and R:/S: drives. It refuses to overwrite a capture or create
a prefix. The recorded `/help` result is exit 1, 193 raw stdout bytes and empty
stderr; execution took 0.866 s. Native DLL overrides select shipped MSVCRT and
MSPDB50 for this capture. No fixture invokes this script. Advertised switches
are `/help`, `/minimum`, `/nologo` and `/verbose`; `/cvnew` from the old case must
not be assumed valid for this version.

`cases/plan.json` preserves the initial ordering: help, then debug-DLL compaction.
Only source/themes are seeded from vc1.0. `hello-dll.c` is a byte-identical copy
of the older GUI DLL source; `typed-debug-seed.c` is a smaller optional input for
type records. No older object, DLL, stream or trace is a 5.10 oracle. The LINK
groundwork worker owns the compiler/link debug/PDB probes to avoid duplicate
captures. Its results must establish actual formats and invocations before the
second case is frozen. Existing NB11/PDB strings are clues, not proof that any
particular debug format or command succeeds.

The original CL/C1/C2/LINK binaries, headers and CRT libraries are present.
Compilation of reconstruction source has not been attempted because no target
source exists. After release, source belongs under `src/`, native tests use
shared shims and cached 5.10-only references, and original compiler checks remain
mandatory. `all`, `native`, `check` and `audit` currently fail explicitly rather
than claiming a scaffold is a reconstruction.
