# Empty hook, real CRT namespace and GetSystemInfo

The candidate is exactly RET1B38df0..38df1. Source hook is empty Win32 C. Its
original caller38c3d and native actual CALL/raw no-argument entry are verified.
The accepted scanner source is renamed scan_internal_switches, with explicit
Win32 driver.symbol and native target-TU macro alias preserving the historical
logical identity. No wrapper/function or historical evidence is relabeled.

After the hook, the accepted main's one actual GetSystemInfo call is observed at
38c42/38c48, IATc91f4, SDK destinationcd7b8. Actual loaded kernel32 hFile/export,
all36 input/output bytes, stdcall4/nonvolatile/error203/CW027f and exact original
neighbors are verified. Native uses the reviewed shared104-byte RSI1 profile,
bound to this original receipt/complete trial/provider/inputs. GDB sees capability
bound2, calls0->1, memory_calls0; no MEMORYSTATUS value/capability is fabricated.

A host gap found during this work is corrected explicitly. Earlier native hosts
seeded a few Linux variables; their real NULL MSC_CMD_FLAGS result did not prove
a full target-bound CRT namespace. The original host now captures actual canonical
MSVCRT __argc/__argv/_environ immediately before Invoke. Native binds those CRT
inputs with the existing shared API, while Invoke retains its own26-entry vector.
Actual host CRT argv has3entries, environment85 or86. Ordered bytes remain in
private immutable scratch; repo metadata records hashes/counts only. GDB observes
actual bound global vectors and the genuine getter/caller/return. Vector/string
storage is disjoint on each side and mapped by owned ordered identity, not by
physical pointer equality. __argc is compared by value; no unseen address consumer
or successful getenv pointer arm is certified. Every host control uses explicit
recon_host_getenv after binding. This does not claim a Win32 OS-environment model.

All3 fresh pairs match the next void environment-initialization frontier38df1.
Both nonzero restore modes prove complete36-byte output overwrite and untouched
modeled/raw neighbors.23 affected negative controls pass; unchanged earlier
switch controls are reused explicitly. A native-only host-variable collision,
recorded by hash alone, still yields target NULL and the same frontier using the
cached original2: no extra Wine. Failed original1(exit253, debugger stack overflow)
is retained without credit; a heap-only observation-buffer correction produced
proper fresh original2. Auditor version drift required only static re-audit.

system-info-trials.json freezes source/controller/input/audit identities and split
costs. Original observations8.43–8.64s; native GDB4.50–4.65s, collision4.53s. These
are capture costs. Completed lifecycle fixture remains native-only, with its
build/setup/execution/checks reported separately. Hook promotion is held for the
orchestrator's requested namespace/profile proof review.
