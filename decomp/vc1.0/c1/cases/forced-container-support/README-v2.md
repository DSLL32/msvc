The second version extends the same parsed context with nine application bodies
(436 original bytes). The original v1 inputs, invocation and eight frozen
oracles remain unchanged. All new support remains explicitly direct; no public
source producer is claimed.

| Function | Original extent | Bytes |
| --- | --- | ---: |
| store_vector_iterator | 32860..32877 | 23 |
| valid_vector_iterator | 32880..328ab | 43 |
| clone_bitset | 32410..32442 | 50 |
| subtract_bitset | 32450..32479 | 41 |
| free_bitset | 32400..32410 | 16 |
| merge_selected_flags | 3f950..3fa34 | 228 |
| new_empty_vector | 324b0..324bf | 15 |
| hash_pointer_low12 | 328b0..328ba | 10 |
| browser_macro_end | 27620..2762a | 10 |

Only fixed accepted constructor stages were added to the existing external
driver. Actual `allocate_permanent(8)` storage holds an ordinary iterator over
the real parsed vector; its store replaces the second item with the real third
item. Actual `new_bitset(31)` allocations receive defined bit values, including
two independent owners for subtraction. The original constructor returns have
fresh RET4, nonvolatile/x87 and canary observations. The flag word is allocated
by `allocate_permanent(4)` and initialized from the actual parsed function's
attributes. No parser globals, input streams or token state are invented.

Bitset comparisons include the constructor's sentinel; subtraction retains it.
Free checks never read the released allocation. Empty-vector checks read only
count, capacity and arena, leaving its unwritten items field alone. Pointer
hashing is checked against each process's own pointer. The browser helper uses
the naturally open browser output; its complete pending bytes match. Both cdecl
and stdcall stack cleanup are checked, and void return registers are ignored.
There is no instruction stepping or continuation through the stopped parent.

The initial iterator observer trial was retained. Its partial DWORD read was
replaced with separate typed pointer/short reads, and affected original entry
observations were recaptured before acceptance. No target state was repaired.

All 59 public phases and 57 direct units pass. Original/native builds and audits
took 5.714 seconds, the native suite 13.372 seconds, adoption checks 8.859 seconds
and graph rejection checks 3.768 seconds. The 17 container/type units took
4.310 seconds. `validation-v2.json` binds the stable shared-source catalog and
current reconstructed source hashes.

There are 937 accepted application source functions and eleven unchanged
historical CRT bodies. Total accepted code extent is 248638/277234 bytes
(89.68525%): application 225900, CRT 22552, imports 186. Five named application
stubs and additional unlisted bodies remain; neither alignment candidates nor
unknown code are counted as CRT.
