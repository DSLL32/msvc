# C13232.EXE

VC1.0 frontend reconstruction, toolchain `1.0-nt`. `binary.c` is the function
registry; its RVAs use original image base `0x10000`. Reconstruction is paused.
There are 979 implemented application functions covering 228585 bytes. The
accepted code/data union is unchanged at 251323/277234 bytes (90.6537%); the
application numerator is 82.4520% of all code, not a claimed non-CRT denominator.
Eleven historical handwritten CRT bodies remain unchanged and receive no App
credit. Unknown library aliases, the `17070`/`17140` RET tails and unattributed
unlisted code remain unresolved.

Run from this directory:

```
make -j4 cases
make -j4 check
make -j4 unit
make -j4 win32 audit
```

Native app and case glue use Clang i386, PIC, `-O0 -g`, MS extensions and function
instrumentation. Runtime/shim support is uninstrumented and linked once into
`.build/libbinary.so`. Common recon-tool rules discover all numbered cases.
`win32` uses the original compiler through the shared launcher; `audit` retains
function-graph, RET, reference, ownership and extent checks, with shared runtime
identification and quiet successful output. Its explicit historical runtime
object binding permits shipped callers of the unchanged `_sopen` override.
Only grammar and fixture-source generators remain under `scripts/`.

Cases 0000–0064 preserve 65 cached public invocations. Cases 0065–0165 preserve
101 original-backed internal direct invocations; these do not establish public
source reachability. The typed shared entry hook pauses genuine main/parser/PDB
constructor contexts, preserving live owners and calling conventions. Returning
helper tests assert defined state, pending streams and owned artifacts, then
publish a harness status and terminate without flushing those paused streams.
Genuine target `exit(1)` and `exit(0x8000)` retain their full DWORD status and
normal cleanup. The narrow QuickC unit checks its established unsupported
uninitialized-default guard; the public QuickC fixture is its valid control.

Every fixture is native-only and consumes its `capture.bin`. Defined pending-stream
and PDB byte oracles are fixture-owned inputs, with typed assertions in `direct.c`. Old shared-prefix
references retain their original R:/S: paths and original bytes; migration did
not recapture or relabel them. PCH producers and consumers now run independently
against the same frozen byte contracts, rather than performing a dynamic
handoff. Only identified irdb+14 and PDB+48 clock fields are normalized; raw
within-case clock equality remains checked. Explicit seeds retain historical
first-entry projections. Legacy direct tests without an original entry-order
oracle explicitly seed all accepted App entries instead of inventing a trace.
The shared direct-original capture interface remains separate future work.

## Paused numeric work

Six helper bodies (324 bytes) were already accepted before migration. The source
also contains three **unaccepted** numeric candidates: `ten_bytes_to_extended`
(0x4c6da..0x4c6fc), `extended_to_unsigned` (0x4c77f..0x4c7c4), and
`round_extended_to_integer` (0x4c7c4..0x4c80f). Their registry flags remain zero.
`write_asm_segment_prefix` (0x4f0cd..0x4f12f) remains a trapping stub. No function
was reconstructed or promoted during this migration.

The pending six direct invocations use the genuine `residual_sink` argument
owners from the source embedded in cases 0063/0064: ten-byte expansion of the
accepted 2.75 conversion; unsigned and rounded 2.75; unsigned and rounded -2.75;
and unsigned 4294967295. Expected integer results are respectively 2, 3,
0xfffffffe (status 1, including its post-error store), 0xfffffffd and 0xffffffff.
Numeric destinations have 48-byte owned buffers and A5 guards; format/copy sizes
are 10/12/4 bytes as appropriate. They remain pending rather than falsely
passing fixtures.

The observed saved workspace differs at its control word: original `027f`, native
`037f` (`0000000020387f02` versus `0000000020387f03`). A genuine untouched C1
main entry observed CW `027f`, SW 0 and empty tags; its FP-init BSS cell was 0.
The native main counterpart and supported startup-precision option were not
adopted before pause. Existing original/native raw pairs, failed runs and this
uncommitted research snapshot are preserved outside the repository at
`~/.ai-tmp/msvc2/vc1.0/c1/standardization-migration-paused-wip/`.
Historical committed evidence remains available before this migration in Git.

Migration verification on this host: original compiler build plus audit 20.17 s;
final native case build plus incremental audit 6.57 s. All 166 cached invocations
passed in 3.79 s (setup 1.39 s, execution 1.14 s, checks 0.42 s; remaining time is
supervision/build-target overhead). The unit target checked all 166 descriptors,
including the one QuickC guard test, in 2.36 s. No original reference was captured.
