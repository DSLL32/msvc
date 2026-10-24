Initial semantic-layout probe, not a validated coherent Node layout.

The generator incorrectly assumed name-list field offsets used bits, encoding
64/96 where the actual data fields are at bytes8/12. Original accepted the
record framing and emitted those offsets unchanged (bitfield64*8+1=513).
Preserve both raw original reference/baseline1 and baseline2 trees, earlier
five fresh function acceptances and source/hash references. Their source bodies
remain valid for their actual record inputs; this probe does not validate the
claimed semantic Node declaration. Corrected active input is captured separately.
