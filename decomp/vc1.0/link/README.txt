VC1.0 LINK.EXE reconstruction

Build the native i386 shared library and run the cached cases from this folder:

    make -j4 native
    make -j4 check unit

The shared recon-tool harness discovers the 91 numbered case directories. Native
fixtures use only captured original inputs/results. They never run Wine or
refresh references. The original compiler and graph/ABI audit are separate:

    make -j4 win32 audit

src/ contains application reconstruction only. harness/ contains native setup,
artifact/process policies and other host glue. crt/runtime.c retains the 16
historical binary-specific overrides: LINK's older stdio buffering/positioning,
low-level open/seek, locale case conversion, timezone/DST and process argument
handling differ from the shipped LIBC objects. Their existing body/ABI proofs
and canonical graph checks justify retaining them; migration adds no CRT code.
crt/anchors.c selects the original static CRT termination-table objects and is
link scaffolding, not an application function or new CRT implementation.

binary.c is the canonical original-RVA function registry. Its implemented App
set is unchanged from be93d9074: 803 application functions (759 public-reached,
44 direct-support), with accepted App extent union 229595/281634 (81.5224724%).
Support-inclusive
historical accepted extent is 260534/281634 (92.5080068%). The 1002 historical
frontier count includes shipped CRT/import support; 16 old handwritten crt/runtime.c
overrides are preserved separately, without new application credit. A complete
non-CRT denominator is unavailable; 11655 remaining bytes are still unclassified.
These extent totals are not a count of executed instructions. Unknown provenance
is not treated as CRT. Direct callbacks remain explicitly
external test inputs and do not establish public command-line reachability.

Each capture preserves its original application first-entry suffix and explicit
previously-seen set, full status/streams, inputs and output inventories. Ordered
historical workflows now use independently frozen prior-stage inputs: each
producer checks its complete output and each consumer receives the same saved
contract. The migration no longer claims a live file handoff between invocations.

harness/fixture-artifacts.c is native test support. It validates structural clock
field inventories and raw PE checksums before the harness compares all remaining
bytes. The compact release variant retains the original physical 2MB/logical 3072
checksum defect. Host-only guards retain the unsupported buffered function-export
heap-pointer serialization and C6 LINNUM stale-use routes; Win32 source behavior
is preserved. No allocator repair or pointer/padding normalization is applied.

Reconstruction is paused. The prepared demangle_name_plus_reference body remains
unaccepted in binary.c and is a named trapping stub in active source;
append/assignment/error-merger work is not promoted.
The held reference-composition experiment is deliberately not a 92nd case. Its
exact prepared pre-migration source, external driver, raw inputs/results, failed pairs and
patch are preserved at:

    /root/.ai-tmp/msvc2/vc1.0/link/standardization-pre-20261025/
    git-owned.diff SHA256 ccc2226bd8d338a57714839d6ab6881c3a7e4a5026391857cc80c47eae196869

Resume from the version 2 valid-source workflow. The approved but untested
qualification applies only to 403f0's real 8-byte stack temporary at entryESP-8:
exact head and low 6 status bits, with full raw flags retained. Caller-owned names
and hidden results remain full 32-bit strict. Complete the current callee-closure
proof before applying it or accepting append. Restore only the held plus body from
the snapshot when reconstruction is explicitly resumed; the current source keeps
the same signature with an INT3 scaffold. Earlier failed 26 observer join and
version 1 setter-quirk observations remain distinct. No new original run occurred
as part of migration.

Current shared validation checks 840 graphs for calls, RET cleanup, live function
references, indirect topology, ownership and instruction boundaries. The 41 ECX
member declarations and existing layout assertions are retained. Historical
validation of 514 contextual initializers is preserved in Git; it is not a
recurring shared parity check. data-symbols.json retains useful contextual
ownership metadata. The superseded linear relocation scan is retired.

Migration validation passes all 91 cases and 7 narrow tests (91 unit-target
invocations, including empty test sets). The final parallel -j4 check+unit run
took 1.697s external wall time. Summed case times were setup 1.131s, execution
0.540s and checks 1.715s; summed unit-invocation times were 0.768/0.177/0.451s.
These component sums overlap across four workers and are not wall time. The
incremental native/case build took 3.656s. The layout-final native/original
build+audit+case-library retry passed in 8.976s combined (incremental, unsplit);
the final host-only 840-function audit took 1.464s. No original reference was
captured or refreshed during migration.

Both registered LINK selfchildren execute native code in distinct fresh
executors and share the application first-entry bitmap. The debug DLL case
checks the complete prepack input before replaying the captured CVPACK response;
the ROM case permits only its two captured copy commands and exact file effects.
The four policy-denial tests reject unknown invocations and changed input bytes.

Migration corrections restored the original unset-then-set LINK32_DB=27 input
for 15 buffered phases and the existing clock comparison ranges in three fixed
clock phases; no new normalization was introduced. The common executor's reviewed
errno boundary fix preserves fresh CRT/constructor/setup/return state across
host bookkeeping. It closes the four initial errno-only report differences
without a case seed, changed expected value or output mask.

Final runner: recon-tool e0116ffcb, SHA256
bd66eb34a22d7cfc42c071bed478292c842ee0e56791df1ddf5d61517bf397c4.
Full commands, initial failures, GDB writer proof, per-case reports and source
hashes remain in /root/.ai-tmp/msvc2/vc1.0/link/standardization/; see
final-summary.json, migration-accounting.json and final-reports-v3/.
Existing evidence removed from the active layout remains available through Git
history. Current cases contain only descriptors, typed drivers, narrow tests
and capture.bin. Nine retained project-specific input generators take explicit
input/output paths; regression tests consume captures without regenerating them.
The three obsolete shared LINK capture scripts were retired with this migration.
