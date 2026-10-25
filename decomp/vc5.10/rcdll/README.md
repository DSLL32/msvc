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
  stops at `rc_macro_identifier_follows` (RVA 5d4d). Its partial streams and actual
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

Accepted application reconstruction is **109 functions, 28,778 code bytes**
This is 38.57% of the 74,609-byte whole-text extent.
The d08815612 baseline was 95 functions / 25,732 bytes.
The provisional discovered application denominator is 299
functions / 66,295 bytes (43.41%); Ghidra discovery is not a completed ownership
classification. The entire .text virtual extent is 74,609 bytes and includes
non-code. Two historical startup CRT source bodies (555 bytes), import thunks
(12 bytes), and the provided custom-ABI __chkstk helper (47 bytes) receive no
application credit. __chkstk uses custom EAX=size / ESP=entryESP+4-size
semantics (the original caller requests 0x42c4); native execution uses its
compiler-owned frame, not a fake C helper call. Shared identify-runtime matches
the shipped MSVCRT.LIB helper. Unknown entries stay unknown.

Current accepted bodies pass both builds, the strict graph/ABI audit and
fresh matching original/native frontier evidence. The former WIP snapshot remains at
`~/.ai-tmp/msvc2/vc5.10/rcdll/migration-snapshot-20261025/`; contextual descriptor
research remains under `analysis/output-descriptor-data/rows.json` in the same
scratch namespace. Unavailable descriptors still trap before native consumption.
The latest original capture took 1.323 s; matching native replay took 182.477 ms
(setup 78.153, execution 34.622, checks 69.701 ms). Builds and strict
whole-candidate audit took 3.594 s. Argument substitution retains its WORD
record index, signed record lengths, out-parameter, and stdcall RET4; its
identifier lookahead dependency remains a named no-argument trap. The invalid
index unit checks that only the input cursor advances and the out-parameter
remains untouched. The preceding collector's initially incorrect local mode
was corrected to the captured zero before acceptance. Native numeric/frame
guards permit the reached rc_number_buffer_active=2, rc_expanding_macro=2
state with existing spelling/workspace checks and verified parent restoration;
other unproved modes remain trapped. No numeric body credit was added.
Earlier failures and the unchanged original-reuse evidence remain in
`~/.ai-tmp/msvc2/vc5.10/rcdll/resume-rich-0110/`.
Two custom argument helpers have named ABI-correct traps and no body credit.
Packed arguments currently require even alignment natively; the original
odd-address padding branch is retained in Win32 source, but is unclaimed.
A focused native unit checks the copy helper's terminator return, WCHAR16 content, empty
input and untouched tail; it took 147.958 ms (3.183 ms execution).
The earlier external loader
INT3 failure and its shared-tool diagnostic remain in scratch; no program
behavior was changed to bypass that stop.

Layout-only validation: original/native/case build 10.468 s; both native IO
checks and four unit groups together 0.244 s; 97-function graph and SEH-role
audit 0.501 s. Help IO took 8.048 ms (setup 4.396, execution 2.459, checks 1.193);
rich partial IO took 174.830 ms (76.011 / 30.249 / 68.571). Application bodies,
registry identities, case captures and policy are unchanged by this reorganization.

Comment preservation uses the distinct `-C` option, not the preprocessor-active
flag. A zero-credit correction binds both original/rebuilt operands and an
opposite-state native unit (C=0, active=1). Earlier partial matches did not
establish that storage identity. The corrected native replay against the
unchanged original took 192.661 ms; the branch unit took 151.867 ms. No new
original capture or additional code credit was needed.
