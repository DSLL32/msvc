Public EDIT image mutation workflow

Original target SHA3a53166175a2e98179d4da41bb0f3bfafec0ecf78884441736f138c8ab4d3718.
Sources provider.c/client.c were compiled by original1.0 CL /O2 /G5, linked without
CRT with actual exported imported_value and three client HIGHLOW relocations.
Original default-mapped setup gives2MB files; no trimming/patching was used.
All tools/captures ran explicitly outside fixtures, fixed shared R:/S: paths.

Active native cases are editor-rebase, editor-bind, editor-release and
editor-release-compact, registered under cases/shared-prefix. The first three are
an ordered workflow: bind consumes the actual validated native rebase output,
and release consumes the actual native bound image. Missing/changed/different
input or a registry cycle rejects before the next executable launches. Provider
is pinned byte-for-byte. Running only editor-release schedules its prerequisites.

    make -j4 native native-trace
    python3 scripts/check-native.py --trace --case editor-release
    python3 scripts/check-native.py --trace --case editor-release-compact
    python3 scripts/check-editor-flow.py
    python3 scripts/check-editor-checksum.py
    python3 scripts/check-module-memory.py
    python3 scripts/check-call-edge.py
    python3 scripts/check-crt-observations.py

Normal fixtures launch only native ELF. capture-case.py/capture-shared.py and
capture-helper are explicit original-only diagnostics, never fixture dependencies.
spec-rebase/bind/release/compact-release describe original inputs; spec.json is
historical combined-mode probe only. Old compact-probe spec paths are preserved
raw provenance, not active manifests. Do not silently refresh any frozen reference.

Rebase3words/base400000→500000; bind3046→601000/provider timestamp/ffffffff chain;
release stack200000/3000. Each positive raw file has a correct independent whole-
file checksum. Canonicalize only the COFF clock at136..139, then recompute derived
checksum; never ignore checksum. The separate valid compact-input original defect
grows3072→2MB while retaining checksum over3072. That case compares all physical
bytes with zero ignored fields, verifies the3072 checksum and requires a mismatch
against the full2MB checksum. It does not join the positive-checksum claim.

capture/original-validation.json freezes two original successes of each positive
stage; capture/compact-input-probe retains the distinct valid original defect.
Fresh entry captures8/6/0/0 have identical full artifacts/streams to those frozen
runs (capture/entry-capture-artifact-comparison.json). capture/frontiers preserves
compact fresh matching original/native GDB proofs in actual acceptance order from
functions.json/frontier_cases; the CRT helper uses the separately reviewed public
strtoul inventory instead of pretending native internal-helper ABI equivalence.

capture/ntdll-boundary proves the actual resolved OS relocation primitive, not
LINK's internal fallback. capture/bind-apis records actual module-global flush
success, time conversion and last-write update. Native-only host registration in
scripts/native-module.c binds exact SHA, PE ImageBase10000, .bssVA6d470/RVA5d470,
36-byte global ownership and native address. It is outside reconstruction graph,
uninstrumented and absent from Win32 build. Native GDB capture validates its actual
call/result/errors, UTC/FILETIME relationship and resulting filemtime. Its initial
observer macro error and other rejected preparatory attempts are documented.

validation.json is the compact closure record. timing.json splits all measured
build/capture/GDB/setup/ELF/trace/oracle costs and labels unmeasured work. The
scalar checksum oracle remains independently in check-editor-checksum.py, tested
against the fast array sum over33bounded cases/seven actual images. No original
run was needed for this host-only oracle optimization. Body/coverage/components
reports distinguish instructions, inline data, alignment, CRT and unknown ranges.
No ordinal/forwarder/verbose/SEH-fault/SearchPath/fallback reachability is invented.
