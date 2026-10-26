vc1.0 RC.EXE reconstruction
==========================

Target: toolchains/1.0-nt/bin/RC.EXE (resource compiler 3.10).
SHA256: eb2e4bb5994303a37326cc60107ee07be01ca267daca1f9116e3149d9ea31591.
Application Win32 C and headers live in src/; historical CRT bodies live in crt/.
Shared native test glue lives in harness/. binary.c is the function registry;
policy-data.json describes the ordered CRT callback tables. Only application
records receive reconstruction credit. Historical reconstructed CRT bodies stay
in the original-toolchain source build without becoming application targets.
The policy retains the exact 61 previously accepted support-boundary names so
all historical callback-table gates stay active; they add no graph bodies.

From this folder:
    make -j4 cases           build shared native library and numbered cases
    make -j4 check           replay saved original outputs and entry traces
    make -j4 unit            run applicable narrow case tests
    make -j4 win32           compile/link with the original VC1.0 toolchain
    make -j4 audit           check canonical function graph, ABIs and extents

Shared tooling/recon-tool owns builds, native process isolation, fixture unpacking,
tracing, output comparison, debugging and original-tool invocation. Native cases
never run Wine or refresh original references. Each numbered folder contains its
case descriptor, a packed cached capture, and optional direct/test glue. Diagnostic
reports and authoring files belong in scratch space. Native code is i386 -O0 -g;
case glue links to one .build/libbinary.so. The same reconstructed production
sources remain compatible with original CL/LINK. The small scripts/import-library.py
fixes the original librarian's decorated NTDLL import strings without changing
object sizes, offsets or hints; it is not an execution/capture helper.

Canonical native CLI
--------------------
    make -C .. -j4 native-bin-rc native-bin-rcpp

The coordinator installs native ELF programs as ../build/native/bin/RC.EXE and
RCPP.EXE. Run RC.EXE from the resource source directory, supplying INCLUDE for
the SDK headers as with the original compiler. The binary-local native-bin target
also remains available; it needs a reconstructed RCPP.EXE beside it or on the
Windows-style PATH. SDK headers and user resource inputs remain ordinary files.

Root entry.c is native host code and is excluded from fixtures and Win32 builds.
It reuses the approved resource_main bridge with shared CRT support, the retained
locale converters and wide-copy implementation. Shared bootstrap owns mutable
arguments/environment, the Windows command line and actual executable/cwd Z:
identities, explicitly updating constructor-owned _pgmptr. Existing normal exit
and resource cleanup remain active. No Wine mapping or fixture environment is
required. VC1.0 RC needs no external diagnostic catalog or RCDLL.
The shared native bootstrap currently accepts ASCII arguments and executable/cwd
paths; resource contents retain the tested resource encodings.

Ordinary compilation launches the actual reconstructed RCPP child, trying the
executable's sibling first and PATH after ENOENT. The shared spawn/bootstrap
boundary preserves the old Windows serialization and child CRT tokenization of
RC's two-element argv with its packed option string. This CLI uses neither the
fixture process broker nor captured preprocessor responses.
The real rich-resource CLI smoke produced the cached 3684-byte resource exactly,
with empty streams, unchanged inputs and no temporary files left behind. All
eighteen cached cases and both narrow dependency rejection tests also pass.

Cases
-----
The suite contains sixteen CLI invocations and two labelled direct helper cases.
The rich resource case covers includes, macros, conditional preprocessing, menus,
dialogs/controls, multilingual strings, accelerators, icons/cursors, bitmaps,
raw/custom data, FONT, DLGINCLUDE, message tables and VERSIONINFO. Other CLI cases
cover diagnostics, older image formats, CP932 Kanji ranges, Unicode escapes,
native RCINCLUDE rewriting, scanf pushback, console cleanup, verbose resource names
and natural PATH fallback to RCPP.
Two narrow unit tests reject altered prepared input and an unknown child
executable. The shared harness owns trace-mismatch rejection.

First-entry expectations project the original accepted histories onto 137
application definitions. CRT, imports, shims and case glue are excluded on both
sides. Every seeded case declares its own prior-seen set; it does not depend on
other cases running first. These checks measure first encounters, not branches
or complete dataflow.

Thirteen CLI cases use independently reproduced, frozen original RCPP responses.
The captured boundary must validate invocation, environment, prepared input and
all recorded include reads before returning the recorded status, streams and
preprocessed bytes. It never executes the original child. Temporary RC/RD names
bind to the actual native PID; only explicitly recorded filename fields may vary.
The verbose case checks all 349 stdout bytes, with six-digit PID fields only at
offsets 161 and 173. Resource bytes, statuses, other streams and complete final
file sets remain exact, including unchanged source files and cleanup.

The console case runs real input to the initialize_lexer entry using the common
typed entry hook, asserts the exact source/prepared/preprocessed/empty-output file
state, then delivers genuine native SIGINT. A second exact-ABI hook asserts
CTRL_C_EVENT (zero) and invokes the actual cleanup callback. The expected result
is exit 1, empty streams and only the unchanged source file remaining. Windows
console membership, signal-created callback threads and SEH delivery remain
historical original-only evidence; Linux delivery makes no claim about those
Windows mechanisms.

The whitespace direct case checks all 256 low-byte inputs, including signed 0xff.
The basename direct case checks six empty/plain/separator/drive/trailing-path
inputs, NUL-inclusive return counts and complete output canaries. Their recorded
results are frozen original direct-function oracles; these cases do not establish
public CLI reachability. Current original-toolchain graph/RET checks retain the
individual Win32 ABIs. KanjiRange size/field-offset assertions remain in source,
and the CP932 resource case checks actual behavior.

Canonical analysis
------------------
The shared graph checker compares the original and rebuilt source graphs, RET
cleanup, direct references, ordered initializer/termination tables and six
floating-conversion callback slots. Ownership and extent guards cover original
file-backed ranges and every accepted function extent, including CRT/import
boundaries outside the source graph. Early PE VirtualSize=0 uses backed raw
sizes; uninitialized sections are zero-fill. Runtime identification is shared
read-only COFF/library analysis, separate from case execution. runtime.json is the
unchanged cached original COFF identity catalog; it includes identity aliases
beyond the authored function ledger and adds no reconstruction credit.
The canonical comparison covers 193 source graphs. Runtime-data checks preserve
the twenty FILE objects, C locale/decimal storage, six floating callback slots
and signed Kanji range layout/accesses. The six floating slots use the existing
hash-qualified LIBC CRT0FP.OBJ __fptrap identity, with zero reconstruction credit.
Support-extent checks retain original bounds for classified CRT/import records
and proven rebuilt bounds, including the three historical routines whose
callees use compiled CRT overrides. Five old unaccepted rebuilt endpoints have
no proven bounds: __exit, __cexit, __c_exit, _get_short_arg and __amsg_exit. The
shared report lists them explicitly; no next-MAP-public extent is inferred.

Historical accepted coverage is 46,249 / 53,248 code-extent bytes (86.86%), including
CRT, shared text data and import thunks. It is not branch coverage. Remaining
unaccepted bytes consist of 3,377 CRT/common-runtime bytes and 3,622 recognized
padding bytes; no native RC-specific reconstruction candidates remain. Migration
does not accept new functions or alter recovered production implementations.
