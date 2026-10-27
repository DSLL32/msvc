C1XX.DLL (Visual C++ 5.10)
=========================

The registry in binary.c contains 681 implemented application functions and
213,645 code bytes (28.44% of the original 751,216-byte .text extent). This
whole-section denominator includes runtime code, data and alignment; it is not
a non-CRT denominator. Startup support and import endpoints have separate
classification and receive no application credit.

`make -j4 check unit` runs both completed native cases and the focused tests.
`make -j4 win32 audit` builds with the original 5.10 compiler and checks the
full function graph, imports, references, ownership, extents and ABIs. Fixtures
use native i386 Linux and never invoke Wine. Application source is in src/,
retained startup support and its rationale in crt/, and hosts/checks in harness/.

The two completed cases are:

- 0000-dll-lifecycle: DLL attach, export presence and detach.
- 0001-minimal-cpp: actual CL-derived InvokeCompilerPass(argc 26, argv, mode 1)
  through compilation, IL output, registered cleanup and DLL teardown. The fresh
  original and native both exit 0, match all 681 first entries, and match complete
  streams and output artifacts. The original took 1,467.278 ms; native took
  27.075 ms (setup 15.441, execution 6.248, checks 5.386). These are invocation
  measurements; the original capture command also performs staging.

The small Win32 host in harness/original-host.c uses real LoadLibrary, the typed
stdcall export and FreeLibrary. Build it with `make -j4 original-host`.
Explicit references use shared original capture with
`--host build/host/c1xx-host.exe --progress --native-msvcrt --artifacts-dir ...`.
The minimal case's stable cwd is T:\vc5.10\c1xx\0001-minimal-cpp. Only consumed
MSC_CMD_FLAGS absence is declared. Native setup binds the forwarded arguments
and this finite environment scope, without historical private HOST vectors or
error-state seeding. System-info profile bytes are a harness asset.

The completed invocation exercises lexer/parser setup, builtin declarations,
the minimal translation unit, output records, symbol naming and normal cleanup.
Readable whole owners include the 21,826-byte LR Run function and the global
record writer; their unvisited semantic branches are not additional runtime
coverage. Logical grammar generators remain binary-specific analysis/build
inputs, rather than copied compressed tables. Unknown data domains trap before
consumption. Native SEH support is limited to the exercised normal paths.

The original 40-byte semantic-value representation and native member-pointer
layouts retain explicit boundaries. Action 102 and unresolved declaration
by-value/raw-transfer domains remain guarded. The compatible literal arm retains
its full opaque storage without interpreting or comparing unknown bytes.
Typed views model only established fields and allocations; they confer no
whole-object, padding or unvisited-pointee equality. Focused tests cover the
modeled reset/restore, alias/vtable identities, callback layouts, literal and
semantic-slot copies, output growth and record encodings.

symbol-aliases.json maps real shared physical bodies through the shared code-free
COFF/native alias helper. Aliases add no application bodies or code credit.
The output destructor likewise has one physical body and eight genuine
receiver-loading entries. Its +0x124..12b gap stays opaque and its +0x12c owned
pointer keeps the actual offset, without inventing a total object size.

The full audit retains all 683 selected source owners. Shared graph-audit checks
682 explicit whole-CFG RET cleanup sets. The sole split CoUninitialize tail uses
harness/check-com-teardown.py: exact image-bound guard/clear/local RET/import
instructions and the SDK void WINAPI(void) contract establish stdcall with no stack arguments cleanup.
It is a finite static ABI residual, not an audit-selection waiver or a COM
success claim; this case's zero COM flag skips the API. The six-byte import
endpoint is excluded from application extents and credit.

Raw references, artifact copies, immutable build/source snapshots, timings and
failed attempts remain under
~/.ai-tmp/msvc2/vc5.10/c1xx/minimal-resume-20261025/.
The final corrected-mask receipts are minimal-cpp-final-{original,native}-report.json.
Earlier successful receipts retain their old mask identity. Full historical
proof revalidation depends on external scratch; case capture.bin contains actual
fixture inputs/oracles, not observer reports. Legacy capture/replay frameworks
remain retired in favor of the shared tools.
