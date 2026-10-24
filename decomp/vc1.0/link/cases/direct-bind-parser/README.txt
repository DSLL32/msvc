Direct embedded BIND parser and mapped-input lifecycle (997 frontiers)
====================================================================

parse_bind_arguments4faf0..4fbd6 is readable cdecl int(int,char**) Win32 C.
Its230-byte extent contains224 CFG instruction bytes and6 alignment bytes.
This is standalone direct support: no static caller/pointer reference was found,
and the accepted public bind_main has a separate inline parser. No original
public caller, private mode or internal global initialization is invented.
Positive ownership is the exact BIND diagnostic and shared editor/BIND globals,
not merely a failed CRT library match. Prior complete library review is referenced
in the approved proposal; unknown resource/far-string code remains unclassified.

The single coherent baseline starts at genuine loader-zero globals and never
resets them. Four distinct caller-owned argv vectors and string blocks remain
live through all calls and normal CRT exit. Empty argc1 returns0. The valid
-u -p . client.exe parse returns1 and supplies its actual image pointer/update
mode to accepted map_bind_image, then real UnmapViewOfFile/CloseHandle. Only
after map/close does grouped -Uv enable verbosity. Finally -?Z sets the usage
flag, emits the exact invalid-Z diagnostic and returns0, retaining earlier fields
where no write occurs. The initial proposal's verbose-before-map order would
print an uncontrolled mapping address; the revised order was approved before the
first original run. It preserves grouped/casefold/P/U/V/?/unknown semantics with
no global reset, output normalization or address-bearing stdout.

The actual36-byte caller-owned BoundImage is produced by the real mapping API.
Only initialized fields12..32 (file/base/header/current/count/sections), actual
owned pointer edges and live248-byte headers/160-byte sections are compared and
consumed. The first three words0,4,8 are neither initialized nor read by this
path; their raw bytes are retained without an output-equality claim. Exterior
workspace guards are strict. The caller's original VirtualAlloc/native BSS zero
storage is actual test input, not an invented target write. After unmap, no freed
view is dereferenced: only stored identities and initialized section_count remain
in the live workspace. bind_imports is not called; no private cache-list setup is
fabricated to make it work. Full2MB client.exe/inventory remains unchanged.

The existing sequential cdecl owned-stack/sentinel seam supplies all real formals
and resumes the saved CRT continuation with explicit external test status0.
Support-call return values are recorded, not accidentally used as process status.
Native __wrap_main supplies the same direct workflow; startup53 is selected from
process startup. Fresh masked first entry and14 entry/return observations match:
actual2/4/1 formals, fullowned argc/argv/string tails/guards, defined global stores
and persistence, initialized mapping owners/headers, consumed EAX/ESP/nonvolatiles,
errno/DOS/Last0, CW027f/SW0/tagsFFFF/all eight empty payloads. Post-call by-value
parameter slots remain raw representations, not invented immutable ABI outputs.
No error or FP qualification is used. Unexpected callees stay trapped.

One untouched original baseline succeeds; no repeat was needed. Its empty stdout,
27-byte CRLF stderr,1344-byte external report, unchanged2MB image and exit0 match
fresh original/native and final native-only GDB reobservation. The external report
contains actual defined scalar/error/name-offset observations plus immutable
1024-byte caller payload. It is a test-driver artifact, not target-created output.
Hyphen options are exercised; the slash-prefix true edge, nonASCII/missing-P inputs,
provider failures and other dormant callers are not claimed dynamically covered.

All89 cached native cases pass in9.878s external make wall after a separate13.345s
native/trace/all-host rebuild and1.201s trace-cache projection. The new traced case
is.247138s: setup.233212s includes.204940s trace preparation, direct ELF5.062ms,
oracle8.721ms and trace.139ms. Output-only is.035959s with ELF2.626ms. Fixture runs
never invoke Wine, a debugger, PE helpers or original capture. Original controller,
masked observations, native GDB and intermediate builds are separately accounted.
The initial native host link omitted the shared CPPFLAGS/wrappers and failed before
execution; correcting that Makefile command did not alter target behavior.

Original VC1-j4 build20.820s and835 common CFG/runtime/ABI/reference/ownership/extent,
817 callback,514 contextual-data and41 ECX checks pass. Twenty-nine focused offline
packet denials and meaningful graph mutations pass. Exact published READY4021ad738
C/H hashes remain unchanged through the final build/suite. No new CRT C, support
acceptance, shim or broad historical replay was required. Timings, compiled-source,
raw scratch paths/hashes and validation/audit records preserve the evidence.

There are997 accepted frontiers and800 application functions (759 public/41 direct),
with16 historical CRT overrides separate. App-only229242/281634=81.3971%, support-
inclusive259973/281634=92.3088%. Partial known target instruction inventory is
194120/194797; it is not a complete nonCRT denominator.12216 unaccepted unclassified
bytes remain pending, not CRT. Detailed accounting is in coverage/components/CFG.
