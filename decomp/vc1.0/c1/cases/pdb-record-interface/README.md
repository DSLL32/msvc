This explicit direct-support case exercises the PDB record interface using the
real frontend constructor and a frozen valid 224-byte database. It does not
claim a public source trigger or a complete frontend/backend compilation.

The natural constructor owns a 116-byte object and two 12-byte list heads. Its
real `wc` open loads eight indexed records, establishes the cumulative field
before use, and owns the descriptor and allocation chains. The seven observed
allocation sizes are 116, 12, 12, 245696, 16384, 8204 and 8204 bytes. Unwritten
constructor fields retain their actual bytes; the driver supplies no compiler
globals or token state. Type indices come from the loaded 4096..4104 range.

Nine whole application functions add 578 original bytes:

| Function | Original extent | Bytes |
| --- | --- | ---: |
| @query_program_database_record@12 | 17970..179c1 | 81 |
| @copy_program_database_record@20 | 179d0..17a70 | 160 |
| @close_program_database_handle@4 | 17b40..17b6d | 45 |
| _program_database_type_record | 17de0..17e34 | 84 |
| _copy_program_database_type_record | 17e40..17ee0 | 160 |
| _program_database_magic | 17fb0..17fb6 | 6 |
| _program_database_first_type | 17fe0..17fe9 | 9 |
| _program_database_last_type | 17ff0..17ff9 | 9 |
| _program_database_payload_bytes | 18000..18018 | 24 |

Both record views and both copying interfaces are checked. The first record
includes a two-byte stored hash and exposes a 32-byte type record. Copy capacity
36 checks the full record and untouched tail; capacity 3 checks truncation while
the length output and return still report the required 32 bytes. All 256 owned
buffer bytes are compared. Closing checks the actual descriptor transition to
-1 and unchanged live object/lists, plus errno, DOS error and LastError.
The first/last index getters return AX; undefined upper EAX bits are excluded.

Fresh original/native entry observations precede body acceptance. Each whole
body has actual argument, stack cleanup, nonvolatile register, x87 control-word,
canary, object/record, pending stream and owned-file checks, followed by a fresh
matching first-entry projection. The fixed four-cdecl-argument extension uses
the existing bounded owned launcher, not a general stepper. It receives no
application reconstruction credit. Copy operations were inline in the original;
the original compiler's memcpy intrinsic preserves that graph without adding a
call between the separate method and cdecl interface bodies.

`check-pdb-queries.py` replays nine instrumented native units against the frozen
original observations. It checks invocation, inputs, oracle file hashes,
accepted-prefix hashes and entry order. Fixtures invoke no Wine, original tool,
GDB or reference refresh. Acceptance-time GDB observations remain separate.

The new evidence contains ordinary owned files and raw logs. Generated masked
EXEs remain in scratch, with their original paths and SHA256 hashes retained in
`generated-executable-bindings.json`. `mask-recipe.json` pins the original,
accepted extent snapshot, entry plan and restoration settings; exact in-memory
regeneration of all 27 images was checked without launching a process. Existing
Git evidence is unchanged.

Retained harness failures are documented in `validation.json`: a missing native
observer include, a signed-char hex formatting index, and a stale projection
metadata binding. These were corrected in test transport; no target state or
original output was repaired. Historical initial native records remain under
scratch paths bound by that validation record.

Five named application stubs and unlisted candidates remain. The one-byte RET
ownership ambiguities at 17070/17140, record allocator, uninitialized-owner PCH
walkers and byte-copier provenance remain unresolved. No new CRT body is added;
the eleven historical source CRT bodies are unchanged. Accepted code extent is
250090/277234 bytes (90.20899%), including CRT/imports; application source owns
227352 accepted bytes and 960 functions. This is extent coverage, not branch
coverage or a complete non-CRT denominator. Final timings are in validation.json.
