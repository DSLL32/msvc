Ordinary OMF LIDATA16/32 and executable-string records

The284-byte generated OMF input follows TIS OMF1.1 record/checksum formats.
Nested LIDATA16 and LIDATA32 blocks repeat leaves and nested groups, with a
section-offset gap; their expected expansion is64bytes. Three COMENT classA4
EXESTR strings follow. Source/record layout is fixture input, never generated
reconstruction implementation. The public -lib wrapper uses LINK32_DB=27, the
original's supported buffered backend; no internal -g/debug mode is requested.

Two untouched originals succeed, preserving full716-byte lidata.lib plus exact
streams/status. Only archive member dates and converted COFF TimeDateStamp are
normalized. The converted .data bytes exactly equal the documented recursive
expansion. Native EXESTR serialization is unusual: although .comment is40bytes,
its writer treats comment count3 as a byte length and copies only 'com' from the
first pointer; the remaining37bytes are zero. Full original bytes establish
this behavior, which is reproduced without corrections or exclusions.

Three whole functions are readable C: LIDATA handler45e00..45e60, recursive block
expander46df0..46f20 and comment writer48ba0..48c60. The recursive path records
ftell, restores remaining-byte state and seeks back for each non-final repeat.
Leaf/block counts,16/32-bit widths and signed repeat test preserve the native
ABI/control flow. The existing reconstructed CRT ftell/fseek vertices remain
in the graph; they are not new application gains. memcpy stays an original-
compiler intrinsic where the original used inline copies.

Fresh original/native GDB first entries and physical partial streams match for
all3functions; final masked original exits0 with no more traps. Fresh original
entry capture records all3names. Native archive/status/streams and entries match.
All39native cases pass625application entries in2.479s external, one sample.
Final canonical/native builds and audits take6.979s unsplit. A previous23.630s
attempt was rejected by graph audit because a newly inserted pragma cancelled
memcpy intrinsic scope in an older function; that source scope was corrected,
and its failed timing/log retained.652graphs,807original/804rebuilt extents,
642callback sets,454data objects and41ECX cleanups pass. Four graph/extent
mutations, six reference/namespace and two CVPACK boundary negatives also pass.

New CFG bodies total569instruction and23alignment bytes, no inline data. The
prior475-byte suggestion omitted94known bytes in the record handler (present in
the old inventory under a FUN label); no extra uninventoried gain is claimed.
Accepted union229573/281634=81.5147%,801frontiers is not branch coverage.
Both selected case families are complete. Root approved the next single combined
ROM/local-import/duplicate-warning/MAP/LINK_REPRO case for original validation
after this checkpoint;1072known target bytes are opportunities, not promised gain.
