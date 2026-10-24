The third version adds fourteen application functions (874 original bytes) to
this existing direct-support case. The public source and all v1/v2 references
are unchanged. These are explicit support calls, without a public-producer claim.

| Function | Original extent | Bytes |
| --- | --- | ---: |
| make_unsigned_integer_constant | 27910..27926 | 22 |
| make_right_typed_expression | 3f340..3f35b | 27 |
| last_type_shape | 40cb0..40cc8 | 24 |
| identity_type | 40020..40027 | 7 |
| identity_type_shape | 40030..40037 | 7 |
| default_expression_conversion | 418d0..418e6 | 22 |
| can_convert_expression | 418f0..4190f | 31 |
| print_function_prototype | 422b0..422d6 | 38 |
| report_three_type_conflict | 3c290..3c375 | 229 |
| take_unnamed_entity_index | 35890..358a0 | 16 |
| current_aggregate_tag | 2a230..2a247 | 23 |
| current_aggregate_name | 2a250..2a26f | 31 |
| layout_type_member | 29220..29333 | 275 |
| validate_symbol_storage | 2d1d0..2d24a | 122 |

Integer and binary IR values come from accepted constructors. Parsed callback
shapes, function and volatile-object types provide the other arguments. The
prototype uses the naturally open global IR stream. The seven-argument formatter
uses a genuine four-string C2248 message and actual parsed names/types; its own
error/output oracle is intentional and does not assert C++ source reachability.
The image-initialized unnamed index is observed, never seeded.

Aggregate units stop at the first genuine `append_aggregate_member` entry for
ContainerItem.key. The actual unsigned-short type and initialized aggregate
context are retained. The layout helper changes size 2 to 4 and returns offset 2;
its oracle describes that direct mutation, not normal public continuation.
Only constructor-defined IR fields and initialized aggregate fields are read.
The existing driver has a bounded seven-slot/48-byte frame, 16-byte canary and
fixed constructor returns; no instruction stepper or parser-state seeding.
Fresh entry, whole-body and accepted first-entry pairs check cleanup,
nonvolatile/x87 state, streams, all pending bytes and complete owned files.

All 59 public phases and 71 direct units pass. Original/native builds and audits
took 9.022 s, the complete native suite 18.748 s, adoption 9.523 s and graph rejection
checks 3.917 s. The 31 container units took 8.975 s. `validation-v3.json` binds the
stable published 61155226d shared sources and reconstructed C/H catalog.

Generated masked EXEs are retained in scratch with path/hash bindings, rather
than committed repeatedly. `mask-recipe-v3.json` preserves the accepted extents,
order and per-capture mask recipe; in-memory regeneration matched all 42 hashes.
Cached native fixtures need no scratch-only executable. Raw observations and
owned input/output files remain ordinary committed files.

The parent-reported interrupted turn and exact pre-resume WIP/checkpoint were
preserved before continuing the same approved work. Initial helper transport
build failures remain in the validation evidence. No repeated interruption
occurred, and no completed original capture was repeated for that continuation.

There are 951 accepted application source functions and eleven unchanged
historical CRT bodies. Accepted extent is 249512/277234 bytes (90.00050%):
application 226774, CRT 22552, imports 186; public 245156, direct 4356. Five named
application stubs and unlisted bodies remain. Unknown bytes, ambiguous empty
CRT-name matches and alignment candidates are not evidence of CRT-only completion.
