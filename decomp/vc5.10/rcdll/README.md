# RCDLL (Visual C++ 5.10)

The native i386 library uses the shared reconstruction harness. Run `make -j4
native cases check unit`, `make -j4 original`, and `make -j4 audit` from this
folder. Fixtures never launch Wine. `binary.c` is the function registry; policy
contains only live graph/ABI contracts. The native adapter registers the DLL,
binds the captured argument/environment vectors, performs CRT attach, invokes
`RC(0,0,diagnostic,NULL,argc,argv)`, and detaches. The resource callback is opaque
and only NULL is supported. The two encountered message resources are generated
from `src/help.mc` for both builds.

Cases retain the original invocations and outputs:

- `0000-help`: complete `/help`, 589 stdout bytes, empty stderr, status zero.
  Historical trace labels `rcdll_main`/`rc_main` map to DllMain/main.
- `0001-resources-rich`: include/macro/conditional/resource corpus with the
  single quoted `/dCASE_FROM_COMMAND=7 ` argument. The current reconstruction
  stops at `rc_run_preprocessor` (RVA be41). Its partial streams and actual
  temporary/output files are checked separately from the final 3,672-byte RES
  oracle. The partial invocation preserves parameter-parser-original-02's
  S: paths and environment; the final RES oracle is the independently captured
  rich-trailing-space-baseline-01 output. Both already existed before migration.

The old command without the trailing space provokes an original read beyond its
requested allocation. Its former debugger guard is retained as a narrow shared entry-hook unit:
the actual public invocation stops at rc_define_macro before the unsupported
copy. This is a host checkpoint, not a production copy-body guard or a
fabricated pending-function frontier. Native execution uses proved readable normal-path copies; native SEH recovery
is unclaimed.
Other unavailable native data/modes retain production traps.

Accepted application reconstruction is unchanged from d08815612: **95 functions,
25,732 code bytes**. The provisional discovered application denominator is 299
functions / 66,295 bytes (38.81%); Ghidra discovery is not a completed ownership
classification. The entire .text virtual extent is 74,609 bytes and includes
non-code. Two historical startup CRT source bodies (555 bytes), import thunks
(12 bytes), and the provided custom-ABI __chkstk helper (47 bytes) receive no
application credit. __chkstk uses custom EAX=size / ESP=entryESP+4-size
semantics (the original caller requests 0x42c4); native execution uses its
compiler-owned frame, not a fake C helper call. Shared identify-runtime matches
the shipped MSVCRT.LIB helper. Unknown entries stay unknown.

Unverified work is excluded from this build: the 124-byte rc_run_preprocessor
body (RVA be41..bebd) and additional token descriptors. Its exact source/config
snapshot and patch remain at
`~/.ai-tmp/msvc2/vc5.10/rcdll/migration-snapshot-20261025/`, with descriptor research
under `analysis/output-descriptor-data/rows.json` in the same scratch namespace.
No functions were accepted or original references captured during migration.
Historical proof packages remain available in Git history.

Migration checks: original/native/case build 9.360 s; both native IO checks and
four unit groups together 0.213 s. Help IO took 5.246 ms (setup 3.213, execution
1.341, checks 0.691); rich partial IO took 171.750 ms (72.120 / 98.777 / 0.853).
The 97-function graph audit and SEH-role residual passed. Its 10.169 s command
included a Makefile-triggered original/native rebuild, leaving the tested native
ELF byte-identical. The final policy-only validation took 19.697 s including a
shared-tool rebuild; neither figure is an audit-only measurement.
