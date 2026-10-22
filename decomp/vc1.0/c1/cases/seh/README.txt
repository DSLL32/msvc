Nested structured-exception frontend case
========================================

The source nests __try/__finally inside __try/__except, includes __leave from
an if statement and a while loop, and defines a caller that checks the ordinary
result. This exercises C1 lowering into IR; the native compiler process does
not execute the source program's Windows exception handlers.

The phase inherits the captured minimal-C frontend arguments and single-entry
MSC_CMD_FLAGS environment. test.c and the C1.ERR catalog are fully hashed.
Two untouched original C1 runs reproduce the four output files exactly:
irex 1134, irsy 114, irin 6, irgl 79. Status 0 and both empty streams match. There are
no ignored fields. Original capture is explicit and outside fixture execution.

The ten accepted functions are recorded in manifest.json, with original masked
and native GDB observations at every frontier and final successful termination.
Several prior parser stub names were provisional; they are corrected to the
actual protected-body, handler and exception-filter roles established by their
original instructions and nesting counters. Original VAs establish identity
across those name changes. The full source bodies live in src/seh.c.

Each protected body owns a label-stack entry at original 0x600a4. Exception depths
are existing contiguous except/finally/protected counters. Label adjustment
preserves the original eight-bit saved depth and marks defined labels as entry
barriers. Filter mode is the high word of existing scanner_flags, not independent
storage; full-word and subword alias/default checks now cover that relationship.

Normal regression:
  make -j4 check CASE=seh

Explicit development commands, outside fixture execution:
  python3 scripts/capture-frontier.py --case cases/seh --debugger PATH --directory SCRATCH
  python3 scripts/debug-frontier.py --case cases/seh --reference CAPTURE --directory SCRATCH

The original debugger argument is required; no original helper is found or
built implicitly. The native debugger reads existing original observations and
never invokes Wine. scripts/check-native.py executes only the native ELF and
uses the immutable references already saved here. validation.json records
build/capture/debug/check timings, static audits and exact extent coverage.
