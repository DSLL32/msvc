Candidate cases for the remainder of CL.EXE
==========================================

42 cases were run against the original 1.0-nt CL.EXE. Function-entry breakpoints
observed 24 of the 25 remaining named driver functions. All cases reached their
expected targets and exit statuses, except the explicitly marked OMF investigation
which confirmed that its intended target was not reached. coverage.json records
each run's raw stdout, stderr, entry events, exit status and target list.

This is discovery evidence, not reconstruction acceptance or branch coverage.
The reconstruction still has 76 source functions and 181 accepted frontiers
across the existing help, minimal-C and optimized Win32 cases. No new functions
were accepted by these probes. The remaining-function denominator comes from
functions.json's driver list; it does not count every unreached CRT function.

Run from decomp/vc1.0/cl:

    make -j2 probe
    python3 probe-cases.py nested-response conflicting-options
    python3 probe-cases.py --update interrupt-idle interrupt-child

make probe builds the debugger and controlled pass helpers with the original
toolchain. A positional list selects cases; --update retains unselected results
from the last scratch report. Reports, captured streams, prepared inputs and
generated artifacts live under ~/.ai-tmp/vc1.0-cl/probes. The runner uses CL's
private Wine prefix. It verifies the original executable's SHA-256, patches only
the running process, and never modifies the executable on disk. Normal make
check still checks the three completed paired reconstruction cases.

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
  environment. They will need equivalent dispatch in the paired harness.

Unreached legacy path:
  legacy-omf-selector.
  /omf does not select OMF here: CL interprets it as /o mf and retains COFF
  output. The grammar's internal ^omf selector is gated during fixed default
  parsing; user arguments, environment arguments and response files do not
  expose it in the paths inspected. No public selector has been found for
  _write_omf_response. A separate internal-state case would be needed to reach
  that legacy writer. The catalog marks this as an investigation, not coverage.

Suggested reconstruction order
------------------------------

The following 17 observed cases collectively reach all 24 newly reached function
entries. This is a greedy compact starting set, not a claim of branch coverage:

    missing-option-value conflicting-options required-option-present dll-output
    ide-feedback timing interrupt-child usage override-side-effects
    nested-response link-tail force-cpp force-object undefine-predefines
    assembly-listing map-and-renamed-output pass-loader

Start with usage and diagnostic cases, then option/response handling, then pass
dispatch and output cases. Add interrupt-idle to cover the other handler state,
and retain the other candidates for their distinct behavior and failure paths.
For each case, use the established masked-original/INT3 reconstruction workflow;
the probe itself must not add entries to the accepted list.

Future paired verification
--------------------------

The expected nonzero exits are intentional: diagnostics and compiler failures
return 2, and the controlled interrupt cases return 4. The catalog records each
expected status. Only legacy-omf-selector is allowed to miss its target.

/Bt measures wall-clock duration; strict output comparison needs a controlled
clock in both processes. IDE feedback includes the absolute Windows working
directory, so paired cases need identical visible paths with isolated storage.
Recorded timing and path output are observations, not portable golden bytes.
DLL, import-library, map and object products need their own documented artifact
oracles, accounting precisely for timestamp fields. These probes assert entry
hits and process completion; they do not yet assert artifact equivalence for
the new cases. Successful compilation uses the original C1/C2/LINK children.
