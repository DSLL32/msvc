# C13232.EXE

VC1.0 frontend reconstruction, toolchain `1.0-nt`. `binary.c` is the function
registry; its RVAs use original image base `0x10000`.
There are 982 implemented application functions covering 228763 bytes. The
accepted code/data union is unchanged at 251501/277234 bytes (90.7180%); the
application numerator is 82.5162% of all code, not a claimed non-CRT denominator.
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

Cases 0000–0064 preserve 65 cached public invocations. Cases 0065–0171 preserve
107 original-backed internal direct invocations; these do not establish public
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

## Residual interface case

The six interfaces (324 bytes) accepted before migration are now joined by
`ten_bytes_to_extended` (0x4c6da..0x4c6fc), `extended_to_unsigned`
(0x4c77f..0x4c7c4), and `round_extended_to_integer` (0x4c7c4..0x4c80f):
three functions, 178 bytes. Cases 0166–0171 finish the six original-backed
numeric invocations from the same genuine parsed `residual_sink` arguments.
They check ten-byte expansion of 2.75; unsigned and rounded 2.75; unsigned and
rounded -2.75; and unsigned 4294967295. Results are 2, 3, 0xfffffffe (status 1,
including its post-error store), 0xfffffffd and 0xffffffff respectively.
The exact borrowed literals, complete eight-byte numeric workspace, unchanged
argument nodes, 48-byte owned destinations/guards, pending streams and files
are checked. These direct interfaces do not establish public-source reachability.

The native build selects the existing `RECON_NATIVE_STARTUP_CONTROLFP_53`
host boundary option. Genuine untouched C1 main entry has CW 027f, SW 0 and
empty tags. Native entry was 037f; the precision-only option produces 027f,
retaining SW 0 and empty tags. No saved control-word field is normalized and
no application source was changed to repair it. Fresh native entry/return checks
retain cdecl stack cleanup, nonvolatile registers and control word. The six
saved original invocations and original masked frontiers remain unchanged;
no Wine recapture was required. Raw originals, the earlier 037f mismatch and
new debugging logs are in `~/.ai-tmp/msvc2/vc1.0/c1/` scratch space.

`write_asm_segment_prefix` (0x4f0cd..0x4f12f, 98 bytes) remains a trapping stub.
The approved source has `mov eax, fs:[0]`; a direct test needs the genuine
`serialize_asm_instruction` entry's initialized operands/addresses, instruction
workspace and stream owners. A new original-backed direct capture is needed;
the standardized typed original transport is a shared-tooling dependency.
It remains unaccepted. Unknown unlisted code and ambiguous RET tails remain
separate from CRT; this is not a completion claim.

Migration verification on this host: original compiler build plus audit 20.17 s;
final native case build plus incremental audit 6.57 s. All 166 cached invocations
passed in 3.79 s (setup 1.39 s, execution 1.14 s, checks 0.42 s; remaining time is
supervision/build-target overhead). The unit target checked all 166 descriptors,
including the one QuickC guard test, in 2.36 s. No original reference was captured.

Numeric checkpoint verification: all 172 native invocations pass with `make -j4
check` in 1.18 s; summed setup/execution/checks are 1.32/1.15/0.40 s across
concurrent cases. The unit target passes in 0.80 s. The final case rebuild took
9.75 s; combined original-toolchain/native build and graph/ABI audit took
22.39 s. The original map is now a declared grouped link output, so a missing
map is regenerated before runtime identification. No new original capture was
run; the six saved numeric body captures originally took 12.46 s including setup.
The residual case remains open for the assembly-prefix interface.

Four existing ASM parser error calls now forward the original context (408/409)
and token-name/context pair (426). Original C1 call bytes and rebuilt caller
cleanup confirm the correction; it adds no functions, credit or error-path
reachability claim. Existing native cases and graph/ABI checks remain required.
