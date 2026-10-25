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
  stops at `rc_collect_resource_word` (RVA 9d3f). Its partial streams and actual
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

Accepted application reconstruction is **133 functions, 35,071 code bytes**
This is 47.01% of the 74,609-byte whole-text extent.
The d08815612 baseline was 95 functions / 25,732 bytes.
The provisional discovered application denominator is 299
functions / 66,295 bytes (52.90%); Ghidra discovery is not a completed ownership
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
The resource dispatcher now has its complete readable source and ABI-correct
named traps for its unreconstructed dependencies. Its compiler-generated pointer
and byte-remap tables use exact COFF-proven text-data intervals. Its built-in initializer now registers the 22 encountered literal name/ID pairs.
The fresh original/native pair reaches the first CURSOR lookup with both actual
formals matched: 1.447 s original and 181.037 ms native (setup 75.287, execution
35.817, checks 69.933 ms). Both builds and strict audit took 2.823 s. Further list
mutations and dispatcher object/callback/conversion paths remain unvisited.
The type lookup now performs its linked-list search and empty-list insertion.
The fresh pair reaches the name-copy helper: 1.387 s original and 181.035 ms
native (36.462 ms execution). Both builds/full audit took 3.826 s. A focused
lookup unit checks name priority, DWORD/WORD ID behavior and returned identity.
Original pointed node bytes were not directly observed; native ownership and
zeroing use the independently established allocator contract.
The dispatcher reuses existing command/result/language globals and token fields.

The raw UTF16 reader preserves its initialized index 4097, 8,194-byte buffer and
non-advancing EOF. Its byte wrapper uses the real input FILE, fread and ferror.
Shared explicit-510 ASCII/default-C WCHAR16 towlower support was adopted against
an unchanged original; original per-call API state was not measured. Later
recording and APSTUDIO paths remain unsupported.
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

The resource status at RVA 25d3c is distinct from the command result at
1853c. A mistaken source-name-based correction in 4f7a7b5e merged them; that
history and its equal-zero evidence remain preserved with their limitation.
The corrected mapping binds actual main/initializer/resource-parser operands,
restores distinct storage, and replaces old reset placeholders with the genuine
font count, type-list head and HWB offset. Seventeen bounded fields have explicit
operand/MAP identities. Noncoincident command/resource reset units and a fresh
native pair against the unchanged original pass. This adds no code credit.

The unvisited callback context at RVA 25f54 is the existing rc_heap HANDLE,
also used by main for HeapDestroy. Its duplicate placeholder was removed after
binding both actual consumers. The callback remains NULL-only natively; this
storage correction supplies no callback behavior or reconstruction credit.

The custom resource-name allocator/copy now executes for all 22 built-ins.
A fresh original/native pair reaches resource-buffer initialization; full build
and graph/ABI preflight passed. The name helper retains its NULL branch and
uses the existing application allocator and shared WCHAR16 CRT operations.

The 128,000-byte resource-buffer initializer is accepted with its two actual
storage operands bound. The fresh pair reaches the pending name reader in
1.614 s original / 183.577 ms native (36.191 ms execution); both builds and strict
audit took 3.948 s. The shared allocator's existing ownership contract applies;
no new original pointed-buffer snapshot is claimed.

The complete resource-name classifier is accepted through its first collector
call. Its later uppercase and decimal/four-digit-hexadecimal branches have strict
static graph/ABI proof but are not yet executed. At that milestone shared _wcsupr had only a typed
fail-closed surface; its subsequent ASCII adoption is described below.
iswctype mask 128 remains unsupported. The fresh original
capture took 1.442 s; native timing is recorded in resume-rich-0128. Header fanout
made both builds and audit take 10.508 s.

The resource-name collector now preserves marker/whitespace/EOF handling and its
actual no-argument ABI. The contextual 257-WCHAR workspace has a native bound
guard reserving its terminator; the original complete capacity and longer-input
behavior remain unclaimed. A 256-character case passes, and GDB confirms the
production guard rejects 257 before overwriting the terminator. Original/native
advance to the language reader in 878.779 / 179.832 ms. Shared explicit-510
default-C ASCII _wcsupr was adopted by native-only retry against that unchanged
original; original per-call pointed memory, errors and FP were not measured.
The typed-denial failure and exact source/artifact identities remain in
`~/.ai-tmp/msvc2/vc5.10/rcdll/resume-rich-0129/`.

The language reader preserves the complete validation/error flow and actual
WORD-returning ABI. The packed DWORD token load shares storage with its WORD
value and BYTE kind; the caller consumes only AX. Both builds/full strict audit
took 4.181 s. The fresh original/native pair reaches the token scanner with
actual flags=0 in 1.385 s / 179.115 ms; later language-result behavior remains
unvisited until that scanner advances. The separate GDB inspection was terminated
after observing the real flags/caller and is retained as partial debug evidence.

The resource-token scanner preserves marker/whitespace, string/symbol/numeric
dispatch, source-position fields, flags and BYTE token-kind return promotion.
Its genuine unread dependencies have named ABI-correct traps. The fresh pair
reaches the no-argument resource-number reader in 835.084 / 180.662 ms; both
builds and strict graph/ABI preflight took 4.251 s. No dependency body or later
scanner dispatch path receives credit from this advance.

The numeric-expression wrapper explicitly resets its two actual DWORD owners
at 27f88/27f90 and preserves numeric value/long-kind/error flow. A noncoincident
entry-hook unit confirms both resets while distinct preprocessor/token state
stays untouched. The original/rebuilt operand identities and corrected typed
unit callback are retained in resume-rich-0132, including the initial failed
unit adapter. Fresh original/native advance to the expression helper in
1.297 s / 180.438 ms. Both builds/full audit took 3.170 s; the focused reset
unit took 147.349 ms. Later expression results are not yet executed.

The resource expression helper now retains the actual flat arithmetic/bitwise
operator loop, parenthesis handling and token side effects. Its first operand
reader remains a genuine named trap. Fresh original/native reach that no-argument
term reader in 1.233 s / 183.163 ms; both builds/full audit took 2.708 s.
Unreturned term/operator effects are not assumed by the current acceptance.

The resource-term reader preserves parenthesized recursion, signed/complement
spelling dispatch, numeric token result and genuine error-name dependency. The
new adjacent helper exposed a compiler-generated switch table on the preceding
expression owner. Exact COFF relocation/87-byte remap proof now bounds that
107-byte text-data interval; strict audit passes without a discovery waiver.
The failed preflight is preserved, and no original was captured before its
correction. Fresh original/native reach the integer parser in 1.415 s /
181.601 ms. No integer-parser body is credited yet.

The full integer parser preserves sign/complement/radix/suffix flow and token
outputs. Its numeric character/cursor owners are the actual WORD 27f74 and
pointer 27f80; no prior defaults are claimed. Fresh original/native reach the
word collector with its actual mutable resource-name pointer in 1.021 s /
180.134 ms, with both builds/full audit taking 4.110 s. The native pointer alias
is directly verified; original pointed bytes were not captured and are not
claimed equal. Radix readers and the collector remain pending.
