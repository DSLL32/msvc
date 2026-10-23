# Authored Win32 channel proof

This preserves the explicitly reviewed capture-tool experiment, not a fixture
or reconstruction. The source and runner are immutable copies of the successful
scratch attempt; their absolute scratch paths remain deliberate provenance.
Do not run this historical runner as a regression or silently recapture it.

The original Watcom compiler and patched linker built the authored helper.
Python mmap and its real Win32 MapViewOfFile/InterlockedExchange peer passed six
probes in 3.078s, including 0.320s compilation and 0.219s linking. The normal run
exchanged 64 mixed-size payloads and EXIT. Other runs checked unpublished partial
payloads, a rejected wrong sequence, invalid initial lease, actual Python owner
exit after a proven round trip, and the independent global deadline with a live
heartbeat. Every helper's natural exit status was collected from its owned
process. No original application or debug child ran.

`receipt.json` binds commands, source/runner/link/IPC snapshots, PE/MAP/OBJ hashes,
statuses, streams and timings. Compiler/link streams are raw bytes; helper
status streams were captured with `subprocess` text mode, including its newline
normalization. They are retained as recorded, not labeled as raw helper bytes.
`independent-review.json` records the parent's
cached-evidence review. Full environment contents remain only in the referenced
0600 scratch file; the public receipt contains its digest and path.

`attempt1/` retains the failed first compile: exit15, 0.352s, missing general C
headers when only h/NT was supplied. Attempt2 added the explicit h include,
changed scratch paths and retained the same C source. No link/helper ran during
the failed attempt, and its evidence was not relabeled as a success.

This proves the channel profile only. Debug events, TF ownership, CF runtime
identity, callback closure and application equivalence require separate tests.
