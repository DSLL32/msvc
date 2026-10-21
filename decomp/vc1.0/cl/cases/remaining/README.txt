Candidate cases for the remainder of CL.EXE
==========================================

All 42 catalog cases now pass paired masked-original/reconstruction verification,
as do three explicit internal-state cases and nine additional timezone cases.
The reconstruction has 110 source functions: all 104 native driver functions
and six differing CRT functions. functions.json records 233 accepted frontiers
across these and the three original cases. make replay verifies every frontier;
make check verifies every completed case. No indexed source function is stubbed.

coverage.json preserves the earlier original-only entry-probe observations,
which reached 24 of the then-known 25 remaining driver entries. Further paired
traps identified additional diagnostic, IDE and forced-input helpers and two
older CRT routines. That historical report is discovery evidence. validation.json
is the completed paired-case evidence; neither report is branch coverage.

Run from decomp/vc1.0/cl:

    make -j2 check
    make replay
    python3 remaining.py nested-response conflicting-options
    python3 remaining.py --replay timing
    python3 remaining.py --all

Original-only entry discovery remains available with make -j2 probe and
probe-cases.py. It does not accept reconstruction functions.

The original toolchain builds paired-run.c, debug-run.c and the controlled pass
helpers. Each original/reconstruction pair runs concurrently. Three private Wine
prefixes under ~/.ai-tmp/vc1.0-cl/remaining expose separate storage at the same
Q: working path; this permits exact comparison of IDE and preprocessor filenames.
The base private prefix is used only for building tools and the original cases.
No original executable on disk is modified. Runtime masks are rebuilt from the
verified original hash and accepted function ranges. Per-case streams, events,
products and validation records remain under the same scratch directory.

Case groups
-----------

Diagnostics and option relationships:
  usage, missing-input, invalid-number, missing-option-value, unknown-option,
  conflicting-options, override-side-effects, incompatible-options,
  required-option-present, required-option-missing, mixed-ordering.
  Cover usage/fatal/warning routines, override diagnostics, derived-option
  copying, prerequisite matching and the explicit C filename callback.

Response files:
  nested-response, missing-response, response-cycle.
  Exercise recursive reading, CRLF, quoted names containing spaces, open failure
  and the recursion limit. Nested-response successfully creates space output.obj.

Input types and compiler modes:
  force-c, force-cpp, force-object, undefine-predefines, preprocess-stdout,
  preprocess-file, prototype-only, syntax-only, assembly-listing.
  Cover forced language/object callbacks and the /u, frontend-only and assembly
  option handlers. force-cpp uses a small C++ class in a .src file. force-object
  reuses the independently captured minimal-C object under fixtures/minimal-c.

Output and link handling:
  link-tail, link-attached, map-and-renamed-output, dll-output, dll-exp-input,
  two-sources.
  Cover linker-tail forwarding, map filename generation, DLL mode and import
  library naming. dll-output builds a real DLL with an exported function.
  dll-exp-input uses /Bz to test suppression of a generated import-library
  argument when an .exp input is present; its empty .exp is never passed to LINK.
  two-sources compiles two translation units and links their object files.

Pass dispatch and failures:
  replace-passes, pass-loader, child-failure, child-not-found.
  /B1 and /B2 select the original compiler passes explicitly. /Bp1 selects a
  forwarding loader using spawnv with the original arguments and environment.
  A controlled pass returning 7 makes CL return 2; a nonexistent pass exercises
  the spawn failure diagnostic.

Environment and reporting:
  environment-precedence, include-search-list, include-search-expansion,
  include-unquoted-space, ide-feedback, timing.
  Cover CL/_CL_ ordering, INCLUDE list expansion, /X suppression, IDE protocol
  initialization/messages and phase timers. The original does not quote spaces
  in INCLUDE entries: include-unquoted-space intentionally expects exit 2.
  An explicit quoted /I path succeeds in the preprocessing cases.

Control handler:
  interrupt-idle, interrupt-child.
  The debugger dispatches the original registered stdcall callback on a new
  thread with CTRL_BREAK_EVENT as its argument. Idle help is held at pagination
  using an open input pipe. The child case holds a controlled replacement pass
  until the callback completes. The traces verify child_running=0 and 1,
  respectively, and both CL processes return 4. These test callback behavior;
  they do not test OS console-event delivery, which did not work in this Wine
  environment. paired-run.c supplies the same dispatch to both masked images and
  the unmodified original, asserting each child_running state.

Legacy selector investigation:
  legacy-omf-selector.
  /omf does not select OMF here: CL interprets it as /o mf and retains COFF
  output. The grammar's internal ^omf selector is gated during fixed default
  parsing; user arguments, environment arguments and response files do not
  expose it in the paths inspected. No public selector has been found for
  _write_omf_response. The catalog retains this as an investigation case. The
  writer is now reconstructed and verified by the internal-state cases below.

Compact behavioral subset
-------------------------

The following 17 catalog cases were the original compact starting set. They
remain a useful subset, while make check and make replay retain all distinct
success, failure and environment cases:

    missing-option-value conflicting-options required-option-present dll-output
    ide-feedback timing interrupt-child usage override-side-effects
    nested-response link-tail force-cpp force-object undefine-predefines
    assembly-listing map-and-renamed-output pass-loader

Paired validation
-----------------

Each completed case compares its exit status, raw stdout/stderr, the complete
set of input/output filenames and every product byte with the masked and
unmodified original. validation.json records raw and normalized product hashes,
stream contents/hashes, debugger events, and exact excluded timestamp offsets.
The expected nonzero statuses are intentional: diagnostic/compiler failures
return 2; interrupt callbacks return 4. The original INCLUDE-space failure is
preserved. The original C1/C2/LINK children are used for successful builds.

Artifact normalization excludes only parsed timestamp fields:
  - Four-byte COFF TimeDateStamp fields in objects and PE headers.
  - Four-byte export-directory TimeDateStamp fields in DLLs and .edata objects.
  - Twelve-byte archive-member decimal modification times and the preceding
    COFF/export timestamp fields in embedded object members.
  - The LINK map's single " Timestamp is ..." line, containing its hexadecimal
    timestamp and textual calendar rendering.
All remaining bytes, including linker padding, symbol records, listing text,
preprocessor output and retained linker responses, must match exactly. No path
normalization is used. The original three cases also run the generated minimal
program and validate the GUI paint/close behavior.

/Bt receives a controlled GetLocalTime API result. The debugger supplies the
same date and 00.900s initial time to each process, then advances by 1.225s per
API call. This exercises the millisecond borrow in report_timer and preserves
execution of the timer, timezone parser, DST logic and time conversions. Four
clock calls must occur per two-pass case. extra.json adds UTC, signed offsets
with minutes/seconds, April and October transition dates, a pre-1967 date, and
the older pre-1987 April rule. These are the original CRT rules, not modern OS
DST policy. /FEEDBACK uses identical visible Q: paths in independent prefixes.

Explicit internal-state cases
----------------------------

internal.json documents the exact state inputs; they are not public selector
claims. paired-run.c installs a one-shot entry breakpoint only when the relevant
function is already accepted. A masked entry remains an ordinary frontier.

internal-omf-dryrun:
  At link_pass entry, set coff_format to zero in both processes and the baseline.
  Retain the current pass record. /Bz prints the positional legacy response,
  including the supplied raw linker option, without invoking a linker.

internal-omf-file:
  Apply the same selector input, then write the response with two object, two
  library and two definition inputs. A replacement linker returns 7 without
  reading these dummy inputs, making CL return 2. /Bk:temp retains the response;
  this original syntax includes the colon in its prefix, yielding :templk.
  The retained response must match byte for byte with no normalization.

internal-ide-null-buffer:
  At ide_message entry, clear the installed frame pointer. The original and
  reconstructed registered assertion callbacks then invoke fatal(0), emit the
  same D2000 diagnostic, and return process status 2.

The two interrupt catalog cases are controlled callback tests. They verify the
registered stdcall callback, child state and status 4; OS console-event delivery
is outside their assertion. Unvisited CRT text remains masked, so future cases
may still discover additional library frontiers even though all native driver
functions are reconstructed.
