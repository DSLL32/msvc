vc1.0 RC.EXE reconstruction
==========================

Target: toolchains/1.0-nt/bin/RC.EXE (resource compiler 3.10).
SHA256: eb2e4bb5994303a37326cc60107ee07be01ca267daca1f9116e3149d9ea31591.
Reconstructed C and headers live in src/; utilities live in scripts/.
Run make and scripts from this binary folder. Original builds/captures use the
root-managed ~/.ai-tmp/vc1.0-capture/wine prefix with fixed R: (repository) and
S: (~/.ai-tmp/msvc2) drives. Captures hold per-case locks and use
S:\vc1.0\rc\<case> directories; no helper creates, remaps or stops a prefix.
Native runs mirror the same S: case names in isolated native namespaces.

Native reconstruction tests
---------------------------
    make -j4 native
    make -j4 native-trace
    make -j4 check
    make -j4 replay
    python3 scripts/check-native-rejections.py

check and replay both run the native instrumented i386 executable against the
cached original application first-entry segments and existing case oracles.
Neither target has a Wine or original-toolchain build prerequisite. native-check
runs the separate output-only ELF build. Native objects use -O0 -g; GDB can use
source names and preserved stdcall/fastcall declarations. The shared compatibility
sources under ../../../lib/recon compile directly at final link, without an
archive or application instrumentation. Include <recon.h> last in reconstructed
C/header include lists; it is empty for the original Win32 compiler.

The suite has 16 CLI cases and two explicitly labelled direct helper cases:
resources-rich, help, invalid-switch, missing-input, alternate, lexical-error,
parser-error, resource-error, kanji, wide-long, preprocessor-error, rcinclude,
scanf-pushback, console-interrupt, verbose-names, path-fallback, and the whitespace
and basename helpers. The rich case covers menus, dialogs/controls, multilingual
strings, accelerators, icon/cursor groups, bitmap/raw/custom resources, FONT,
DLGINCLUDE, message tables and VERSIONINFO. Other cases cover older image formats,
CP932 KANJI ranges, Unicode escapes, native RCINCLUDE rewriting and diagnostics.

scripts/native-traces.py projects the frozen original acceptance history onto
137 source-backed application definitions. Both sides exclude CRT, imports and
shims. Prefix functions are seeded as already seen; repeated calls are not a
new entry. Native object definitions must match this classification, including
the three explicitly retained mixed-file CRT routines. The native entry adapter
and shared shims are not reconstructed application functions. Each case has a
fresh trace/state file; the shared state descriptor supports synchronous child
history, although RC currently uses captured external RCPP responses. This is
first-encounter regression evidence, not branch coverage or complete dataflow.

Captured RCPP boundary
----------------------
Thirteen cases require RCPP. They consume independently verified original-only
captures under cases/native-boundaries; no fixture launches RCPP.EXE or Wine.
Each capture records exact invocation, relevant environment, full prepared input,
every attempted input read including transitive includes and missing candidates,
raw child streams, status and emitted bytes. Every response was independently
reproduced with untouched original RCPP before adoption. Capture elapsed times
are separate from native fixture setup/execution/check times.

The broker checks executable spelling and hash, mode, argv, cwd, environment,
complete prepared input and the full recorded read set before replying. Unknown
invocations or changed bytes fail. Only recorded temporary filename tokens in
RCPP #line directives are bound to RC's actual native PID; every other response
byte is unchanged. PATHfallback requests a natural ENOENT for absent sibling,
current-directory and first PATH candidates, then replays the verified original
RCPP response at the declared existing target. No PE execution is permitted.

Every final case checks status, streams, output bytes and the complete remaining
file set, including unchanged source files and cleanup. verbose-names compares
all 349 stdout bytes, binding only six-digit fields at offsets 161 and 173 to the
actual process PID and native RC/RD names. No PID override or broad path/stream
normalization is used. All resource products are byte-exact.

Specialized cases
------------------
console-interrupt runs the real native SIGINT shim at a GDB pre-initialize_lexer
boundary. It snapshots exact source/temporary/empty-output files before signal,
observes callback argument zero, and requires status 1, empty streams and exact
cleanup. Windows console membership, Windows signal-created callback threads
and Windows SEH delivery remain original-only evidence; native Linux signals do
not claim to reproduce those OS mechanisms.

The two direct cases run the existing stateless helpers without inventing CLI
callers: one loop checks all 256 low-byte whitespace inputs; six basename cases
check NUL-inclusive counts and every output canary. Returned data matches frozen
original results. Canonical Win32 stdcall cleanup/nonvolatile ABI evidence and
current original-toolchain function-graph/RET audits are retained separately.

Original compiler and reference analysis
----------------------------------------
    make -j4 reference-build
    make -j4 reference-audit
    make -j4 graph-audit

reference-build compiles Win32 C with original CL/LINK under Wine; reference-audit
refreshes that build and its indexes before invoking the host-only graph-audit.
The latter uses existing PE/MAP/index artifacts without Wine or recapture. Shared
tooling/graph-audit is built quietly once through graph-audit.mk, then invoked
directly. Its full JSON stdout is saved in build/graph-audit.json; actual error
stderr remains visible. No cargo run or legacy Python graph walker is used.

Common CFG checks preserve graph aliases/indirect topology, inherited RET cleanup,
function references, ordered accepted initializer/termination callbacks and all
six floating-conversion slots. The explicit flags are --check-ret,
--check-references, --check-callbacks, --check-ownership, --check-extents and
--check-accepted-extents. The last guard checks every accepted indexed extent on
both sides, including CRT/imports outside the 193-function graph comparison.
Ownership requires disjoint original file-backed driver ranges; whole-extent
checks supplement reachable-instruction endpoint guards. Overlapping alternative
instruction streams are not prohibited by an unsupported new assumption.

scripts/audit-data.py retains only RC's non-graph data assertions: twenty 32-byte
FILE objects and last-stream displacement, flags/descriptors, 257 C-locale class
entries including EOF, multibyte/decimal storage, and zero-filled signed Kanji
range words plus their exact MOVSX access offsets/stride. Its report is
build/data-layout.json. Callback identities belong to the common Rust metadata.
scripts/reference_indexes.py contains pure PE/map indexing, with no paired runner
or execution fallback. The gap inventory consumes the fresh shared report via
--graph-report and retains all14 original uncovered incoming-reference targets;
it does not rediscover graphs in Python. None of these audits run regression
fixtures. Integration checks and timings are in cases/rust-graph-validation.json.

Explicit original-only capture is also separate. capture-native-boundaries-reference
builds the small original debugger and calls scripts/capture-native-boundaries.py;
it refuses to overwrite a frozen case. CAPTURE_OPTIONS must explicitly select a
new output directory with --directory; --case selects individual cases. Each
original RC run records fresh one-shot application entry observations, raw
streams and the complete final file set. Each required original RCPP response
is independently reproduced at the same canonical cwd before registration. capture-direct-reference independently verifies
the two original helper oracles. These operations are never fixture dependencies.

RC predates the allocator in shipped LIBC/LIBCMT; heap-crt.c preserves its
GetProcessHeap/HeapAlloc/HeapFree graph. heap.c contains the separate checked
application wrappers. Native builds substitute the shared CRT boundary while
canonical builds retain all recovered original CRT implementations. Early PE
VirtualSize=0 uses backed raw extents, and uninitialized sections are zero-fill.

Original accepted coverage remains 46,249 / 53,248 code-extent bytes (86.86%),
including accepted CRT, shared text data and import thunks. It is not branch
coverage. All 254 accepted frontiers and the 193 source-function graph/ABI audits
remain recorded in validation.json/validation-frontiers.txt as historical
original-backed evidence; native-adoption receipts are under cases/. Remaining
unaccepted bytes are 3,377 CRT/common-runtime bytes and 3,622 recognized padding;
no additional reconstruction was performed during native workflow adoption.

Shared-prefix maintenance
--------------------------
The 16 CLI references were explicitly recaptured under fixed R:/S:, including
13 independent original RCPP reproductions. Native fixtures validate the exact
canonical cwd against its isolated S: mirror and retain the host cwd check in
the spawn protocol. PATH fallback mirrors only its declared fixed S: launch
and child directories. No private T:/Z: mapping, PE execution, alternate
transport bundle or Wine fallback remains in fixture code. Earlier first-entry
acceptance history is retained as semantic reconstruction evidence. Current
case proofs, split timings and prefix-retirement records are indexed by
cases/native-all-validation.json. Resource/source bytes and application code
are unchanged by this maintenance.
