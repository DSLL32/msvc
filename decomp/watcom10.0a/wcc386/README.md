# WCC386, Watcom 10.0a

Two cases are complete: basic `?` help and an optimized C compilation workload.
The latter preprocesses a header, parses packed records, bitfields, enums and
unions, and compiles function pointers, aggregate arguments/results, varargs,
K&R parameters, integer loops/switches and software floating arithmetic into an
OMF object. Native execution matches the original's complete streams, exit 0,
1,948 observed application first entries and exact 1,710-byte object. No object
bytes are masked or normalized. Other application paths retain named INT3 stubs
or explicit guards for unqualified semantic domains.

The target is `toolchains/watcom10.0a/binb/WCC386.EXE`, whose CF payload runs
through the corresponding `binnt` loader during original captures. Original
addresses in `binary.c` are CF image offsets. The shared debugger masks the
conservative CODE envelope and restores declared functions and encountered
text data; fixtures use only the native i386 DSO and cached captures.

Qualified coverage is **1,961 logical application functions and 254,301 unique
original code[] bytes**. This is a union across the two cases, not the records
case's observed entry count. Its explicitly seeded static-empty hooks are
`CPragmaInit`, `CSkip`, `ChkPragmas`, `MemCoalesce`, `EmitAbort`, `ScoreSegments`,
`FiniZeroPage`, `WVFiniDbgInfo` and `TypesPurge`. Their empty source/ABI/shared-RET
proofs supply static qualification, zero additional unique bytes, and no claim
of a separately observed hook frontier or entry order. Help additionally observes
`CCusage`, `ConsMsg`, `Wait_for_return` and `MyExit`, which the compilation case
does not. No source-available body remains credited merely by association with
another body. A total application-code denominator has not been established.

Run these commands from this folder:

```sh
make -j4 check       # both cached native regressions; no Wine
make -j4 original    # original Watcom compiler/linker build check
make -j4 audit       # full shared graph, ownership, extent and ABI checks
make -j4 native-bin  # canonical native CLI: build/wcc386
```

The full graph audit selects 1,964 owners: the qualified application functions
and three justified paired CRT adapters (`malloc_segment_wrapper`, `calloc`,
`free_segment_wrapper`). It checks 1,958 explicit return-cleanup contracts:
1,955 application owners and those three adapters. Six application owners have
separately checked finite nonlocal-exit residuals: `NewSym`, `NewLeaf`, `NewQuad`,
`CCusage`, `MyExit` and `FEMessage`. Their full raw graphs remain checked;
recursive cleanup stays unknown rather than treating a nonreturning path as
RET0. `scripts/check-nonlocal-tails.py` verifies the actual normal returns and
exit/error endpoint roles. Pending error bodies receive no execution credit.

`scripts/audit-policy.py` binds real object contributions for `Wait_for_return`,
`Length`, `TransferMemoryFlags` and `Zoiks`, excluding only proven zero alignment
from their current rebuilt extents. The separate external CRT body view avoids
decoding selected overlapping support tails twice while retaining canonical
CRT extents and endpoint identities. Pending `WantZoiks2`/`DGBlip` and
`FreeTrySymBackInfo`/`FreeTryTableBackHandles` sharing obligations remain explicit:
identity-only pending endpoints are not a proof of joint body ownership.

The fixture adapter in `harness/startup.c` and reusable
`harness/startup-runtime.c` preserve genuine FILE-list allocation, stream-owner
reuse, reversed CRT environment pointers and initializer/finalizer order.
Callbacks use the reconstructed private heap before `main` and on shutdown.
A normal `main` return forwards its real status through the existing exit
lifecycle. Root `entry.c` selects this runtime without case/provider metadata,
binds arguments once and uses the actual command line. Messages are compiled in;
no original executable/resource asset is required. Justified glue in `crt/`
preserves the older CRT's real application allocator callbacks without
interposing on host or shim allocation.

Qualification remains bounded. Native `SafeRecurse` uses the source-backed
normal callback path; original low-stack relocation is unqualified. The
`PARMS_BY_ADDRESS` fallback remains guarded. `Low64Reg` admits only the separately
proved ST0/ST1 exceptions to the original unbounded scan. `CGetMsg` currently
admits the encountered code-size message, not an invented complete message
table. Native private stream ownership covers the encountered static roles,
not a FILE26 overlay or arbitrary stream growth. Interactive stdin/getch and
console-event delivery are unqualified. The empty native argv vector does not
claim the original unconsumed NULL argv identity.

Source was adapted where useful from Open Watcom's initial commit
`6b9cb4438959d845a2b4dd4bf19f7908ecd608e9`, checked against the 1994 image.
Applicable source notices and `COPYING` are retained. Older structure layouts,
semantic tables and control flow are recovered for this binary.

Final validation: parallel period/native build 5.484 s; full audit 7.993 s.
Cached help took 27.783 ms (19.041 setup, 2.480 execution, 6.262 checks); cached
compilation took 51.967 ms (21.999 setup, 23.178 execution, 6.789 checks).
The retained successful final original took 5,895.479 ms; its matching native
catch-up took 49.368 ms (20.271 setup, 22.398 execution, 6.699 checks).
A canonical CLI smoke using the same source and pinned mtimes took 16.348 ms,
exit 0, identical streams and valid OMF framing/checksums. Its actual scratch
working directory is a different invocation input, so its recorded source path
and object hash differ; it is not relabeled as the cached T: reference.
Reports and raw artifacts remain in scratch. Cases contain only their source
descriptors and uncompressed `capture.bin` files.
