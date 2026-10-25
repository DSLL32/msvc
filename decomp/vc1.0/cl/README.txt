Visual C++ 1.0 for NT: CL.EXE reconstruction

The completed driver implements all 104 application functions in the original
[00017010,0001b081) region. Six historical CRT source overrides are retained
without application reconstruction credit; further CRT work is out of scope.
This harness migration changes no accepted function, extent, ABI or invocation.

    make -j4 native cases
    make -j4 check
    make -j4 unit
    make -j4 recon-original audit

binary.c is the canonical function registry. Native source is built once as
.build/libbinary.so with i386 -O0 -g, and shared automatic rules link the 57
numbered case libraries against it. Application entries alone are instrumented;
CRT overrides and compatibility/tracing support are uninstrumented. Each case
has its original first-entry segment and explicit preceding seen-prefix.

Every capture.bin contains the cached original invocation's actual inputs,
status, raw streams and final artifact masters. Reference packing performs no
Wine execution or recapture. Compiler/linker PE children are captured synchronous
responses with exact command, environment, cwd and complete workspace input/output
effects. Their specific executable images and CL.ERR/CL32.MSG parent resources
are hash-bound inputs; this does not claim a captured transitive SDK readset.
Unknown requests fail. Harness-only images, streams and file payloads remain in
read-only capture assets outside the application workspace. Final comparisons
retain only the previously identified COFF/PE/export/archive/map clock-field
exclusions; raw artifact masters remain available unchanged in the assets.

The original target cwd remains S:\vc1.0\cl\<case>, mapped to an isolated native
workspace. It is distinct from the exact host cwd used by process transport.
TMP/TEMP remain relative ".". R: retains the repository spelling. Fixtures never
remap a Wine drive, execute an original child, or regenerate references.

/Bt invocations preserve the captured local calendar and legacy timezone/DST
calculations. The two interrupt cases exercise the registered handler in idle
and child-running states through native signaling; they make no claim about
Windows console-event delivery. Three internal-state invocations preserve the
existing one-shot OMF/IDE entry mutations through the shared native entry
observer, then resume the real function and caller. They do not establish public
option reachability. Original SEH exception delivery remains outside the native
scope.
Generated products are compared as bytes and are not launched.

Use the shared recon-tool run/test commands for individual cases and GDB:

    ../../../tooling/recon-tool/target/i686-unknown-linux-gnu/release/recon-tool run cases/0001-compile --gdb

Shared execution reports split setup, execution and checks. Keep reports and
diagnostic logs under ~/.ai-tmp/msvc2/vc1.0/cl, outside cases/. Original compilation
uses the shared fixed-drive Wine launcher only through recon-tool original tool.
The shared Rust graph audit retains call/tail, RET-cleanup, ownership and extent
checks for all 110 source functions. Shared identify-runtime recognizes the shipped
LIBC/SETARGV code and the explicitly qualified six source overrides. Their
runtime implementation flags remain zero.

src/cl.c contains main; driver.c handles options and invocation; remaining.c
contains diagnostics, response files, IDE callbacks, timers and the OMF writer;
data.c holds encountered grammar/profiles. crt/runtime.c retains the historical
setvbuf, _freebuf, _sopen, _filbuf, __tzset and _isindst overrides. These shipped
EXE variants differ from LIBC.LIB in stream flags, text-file handling and legacy
timezone/DST behavior; retaining them preserves the already qualified graph and
observed driver behavior without adding CRT reconstruction credit.

Application source lives only in src/. The six justified runtime overrides live
in crt/; native fixture hosts and helper glue live in harness/. Shared
compatibility stays in lib/recon/. The metadata registry is native-only.

Migration validation retained all 57 invocation traces, statuses, streams and
artifacts. Six scratch-only rejection controls checked wrong first-entry order,
changed initial input, changed input at the child boundary, wrong child cwd,
corrupted final executable bytes and wrong expected status. The parallel native
batch took 0.631 s wall; summed per-case setup/execution/check times were
0.667/0.434/0.511 s, separate from that concurrent wall time. The clean native
build took 2.370 s; the final case-only rebuild was not timed. The original
toolchain rebuild took 3.162 s and the 110-function graph/ABI audit 0.291 s.
No original reference was recaptured.
