# RCDLL (Visual C++ 5.10)

The native i386 library uses the shared reconstruction harness. Run `make -j4
native cases check unit`, `make -j4 original`, and `make -j4 audit` from this
folder. Fixtures never launch Wine. `binary.c` is the function registry; policy
contains only live graph/ABI contracts. Application source is in `src/`, the two historical startup bodies in `crt/`
(with per-file rationale), and native glue in `harness/`. The native adapter
registers the DLL,
binds the captured argument/environment vectors, performs CRT attach, invokes
`RC(0,0,diagnostic,NULL,argc,argv)`, and detaches. The resource callback is opaque
and only NULL is supported. The two encountered message resources are generated
from `src/help.mc` for both builds.

Cases retain the original invocations and outputs:

- `0000-help`: complete `/help`, 589 stdout bytes, empty stderr, status zero.
  Historical trace labels `rcdll_main`/`rc_main` map to DllMain/main.
- `0001-resources-rich`: include/macro/conditional/resource corpus with the
  single quoted `/dCASE_FROM_COMMAND=7 ` argument. The current reconstruction
  stops at `rc_skip_inactive_quoted` (RVA 6cbc). Its partial streams and actual
  temporary/output files are checked separately from the final 3,672-byte RES
  oracle. The fresh partial invocation uses the stable rich-case-0001 S: workload
  paths and only INCLUDE/absent RCCODEPAGE as explicit target environment inputs.
  Native host controls are established by case setup. The final RES oracle
  remains the independently captured rich-trailing-space-baseline-01 output.

The old command without the trailing space provokes an original read beyond its
requested allocation. Its former debugger guard is retained as a narrow shared entry-hook unit:
the actual public invocation stops at rc_define_macro before the unsupported
copy. This is a host checkpoint, not a production copy-body guard or a
fabricated pending-function frontier. Native execution uses proved readable normal-path copies; native SEH recovery
is unclaimed.
Other unavailable native data/modes retain production traps.

Accepted application reconstruction is **99 functions, 26,309 code bytes**
(the d08815612 baseline plus the preprocessor output loop, block-comment
preservation, source-frame closure and its custom copy-end helper). The provisional discovered application denominator is 299
functions / 66,295 bytes (39.68%); Ghidra discovery is not a completed ownership
classification. The entire .text virtual extent is 74,609 bytes and includes
non-code. Two historical startup CRT source bodies (555 bytes), import thunks
(12 bytes), and the provided custom-ABI __chkstk helper (47 bytes) receive no
application credit. __chkstk uses custom EAX=size / ESP=entryESP+4-size
semantics (the original caller requests 0x42c4); native execution uses its
compiler-owned frame, not a fake C helper call. Shared identify-runtime matches
the shipped MSVCRT.LIB helper. Unknown entries stay unknown.

The preprocessor output loop, block-comment preservation, source-frame closure, the custom copy-end helper and their encountered
literals/descriptors now pass the
original/compiler and native builds, the strict graph/ABI audit, and a fresh
matching original/native frontier pair. The former WIP snapshot remains at
`~/.ai-tmp/msvc2/vc5.10/rcdll/migration-snapshot-20261025/`; contextual descriptor
research remains under `analysis/output-descriptor-data/rows.json` in the same
scratch namespace. Unavailable descriptors still trap before native consumption.
The latest original capture took 1.527 s; native replay took 176.039 ms
(setup 75.758, execution 30.986, checks 69.293 ms). The copy-end
original/native builds and whole-candidate graph preflight took 3.688 s.
A focused native unit checks its terminator return, WCHAR16 content, empty
input and untouched tail; it took 147.958 ms (3.183 ms execution).
The earlier external loader
INT3 failure and its shared-tool diagnostic remain in scratch; no program
behavior was changed to bypass that stop.

Layout-only validation: original/native/case build 10.468 s; both native IO
checks and four unit groups together 0.244 s; 97-function graph and SEH-role
audit 0.501 s. Help IO took 8.048 ms (setup 4.396, execution 2.459, checks 1.193);
rich partial IO took 174.830 ms (76.011 / 30.249 / 68.571). Application bodies,
registry identities, case captures and policy are unchanged by this reorganization.
