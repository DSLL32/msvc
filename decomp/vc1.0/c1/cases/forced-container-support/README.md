The public phase compiles a single program using three forced headers, nested
includes and macros, records, bitfields, callbacks, floating literals and browser
output. Its original status, both streams, browser file and all five IR files
match the native reconstruction exactly. Input timestamps are fixed explicitly;
no output fields are normalized. This phase reaches no new application bodies.

The separate direct phase stops at the real `finish_declarations` entry. It
borrows the compiler's parsed three-item `options.forced_includes` vector and,
for the type operation, the genuinely declared `container_result` function type.
It never replaces compiler globals or fabricates parser/token state. The
existing external driver supplies only an owned call stack and ordinary API
arguments. These checks establish direct support, not a public source trigger.

Nine whole application functions are accepted, totaling 646 original bytes:

| Function | Original extent | Bytes |
| --- | --- | ---: |
| clone_vector_with_extra | 32630..326ac | 124 |
| clone_vector | 326b0..326c4 | 20 |
| clone_vector_with_slot | 326d0..326e4 | 20 |
| vector_contains | 327b0..327f9 | 73 |
| first_vector_item | 32800..3280c | 12 |
| vectors_equal | 32810..32852 | 66 |
| valid_vector | 32480..324ab | 43 |
| join_truncated_names | 35910..3598c | 124 |
| make_pointer_model_type | 403e0..40484 | 164 |

Fresh masked-original/native entry observations precede reconstruction. The
clone's nested helper has its own matching entry and three actual argument
slots. Completed calls check returned ownership/content, unchanged inputs,
pending output bytes, complete owned files, stack cleanup, nonvolatile registers,
x87 control word and stack canaries. Every accepted body also has a fresh
original/native first-entry reference. Spare vector capacity is never read.
Frozen references retain actual source/type pointer relationships separately
for each process; addresses are not expected to match across processes.

`check-container.py` runs the eight direct units with the native instrumented
binary, against immutable original records. Inputs, invocation, reference files
and accepted-prefix hashes are bound explicitly. Fixtures invoke no original
program, Wine or reference refresh.

Final validation: 59 public phases and 48 direct units pass. Original/native
builds and audits took 5.826 seconds; the full native suite took 12.294 seconds
(public fixtures 1.365 seconds; these eight direct units 2.292 seconds).
Adoption checks took 9.319 seconds and graph rejection checks 3.847 seconds.
`validation.json` records timings and the stable published shared-source catalog.

Accepted extent is 248202/277234 bytes (89.52798%), including CRT/imports;
225464 bytes are application source. There are 928 accepted application source
functions and eleven unchanged historical CRT bodies. Seven named application
stubs and additional unlisted candidate bodies remain. Untested branches and
absent static callers do not establish dead code or CRT ownership.
