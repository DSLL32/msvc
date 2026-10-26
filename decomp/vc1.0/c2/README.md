# VC1.0 C2 reconstruction

Checkpoint 1371 completes the protected streaming floating-filter case. There
are 1256 reconstructed App functions and five unchanged historical handwritten
CRT overrides. App code occupies 341445/399693 original code-section bytes
(85.4268%); adding 916 distinct owned text-data bytes gives an accepted App
extent union of 342361/399693 (85.6560%). These are clipped byte-extent unions,
not branch coverage. The five CRT overrides contribute 1776 bytes separately
and receive zero App credit. The available App, classified CRT and import union is
368735/399693 (92.2546%). A reliable non-CRT denominator remains unavailable;
105 pending App rows and 30958 bytes outside that available union still need
investigation. No only-CRT remainder claim is made.

`binary.c` is the sole registry. All 20 cached IO cases complete and 126 narrow
units pass. Fixtures retain actual frontend IR, the single MSC_CMD_FLAGS child
transport, stable S cwd, explicit first-entry seeds and complete object/listing/
PDB oracles. Only established COFF timestamp bytes 4..8 are normalized; OMF bytes
are exact. Native fixtures do not run Wine.

The guarded-filter source uses untouched CL `/O2 /Ob0 /G5 /Oi /Z7`. It combines
call-free nested floating loops, noncommutative branches, live stored values,
volatile protected accesses and nested SEH cleanup. Read-only shared DIRECT
observation retained the actual five C1 channels and C2 invocation before CL
started the backend. That support capture does not claim parent CL completion;
an ordinary untouched CL baseline and independent standalone C2 both succeed.
Source 4169B has no headers; actual ex/sy/in/gl/db inputs are 4369/996/6/179/285B.
The fixture keeps the observed 001284 prefix and exact child environment/cwd.
The complete 5273B standalone object matches CL and native reconstruction with
only the established timestamp normalization. Normalized SHA256:
`3dd1a0a38ed67f9d259c662d0aa739450c533583c1f97eb80870fe5309bc49d2`.
Streams are empty, status 0 and the object is the sole generated output.

Seven whole App functions add 2024 code bytes: Pentium floating-store grouping
(822), successor-use transfer (207), inheritance eligibility (241), operand-range
checking (258), diamond register lifetime (334), protected floating context (108)
and protected-region skipping (54). Three child functions were discovered from
real call edges. Fresh masked-original/native evidence reaches all seven in
order and then completes. Existing callees retain individual ABIs; all new
scoped RET contracts pass. No shared shim, CRT body or assembly bridge was added.

The first real store-grouping entry takes its unchanged exit; separate cold
controls cover six opcode relationships, reorder identities and the original
partial-mutation exit. Actual successor-use, operand, range, diamond and protected
block fields/owner aliases are checked by typed native entry hooks against
bounded original observations. Cold controls check inclusive capacity, both use
chains, constant/register exclusion, alias ranges, LEA omission, register
read/overwrite decisions and protected marker/cursor behavior. No branch-merge
or scope-unwind reach is inferred just from the source's feature list. Existing
diagnostic child traps and unsupported OMF cases remain explicit.

Use `make -j4 native win32` for native and original-toolchain builds. During a
case use current-case replay, necessary builds and focused checks; complete
regression/audit and commits follow case completion. The completed-case
`make -j4 audit check unit` passes 1261 paired graphs, 1261 accepted source/runtime
extents on each side and 64 scoped RET contracts. Global RET comparison remains
unresolved for finish_debugging's indirect tail; other checks are not waived.
The independently changed byte 100 object expectation rejects on its artifact
hash. No output comparison was relaxed.

The case-end build/audit/full-fixture command took 5.440s, with phases unsplit.
Cached filter IO took 36.163ms: setup 12.973, execution 19.133, checks 4.057.
Its 13-unit command took 71.528ms: setup 9.482, execution 58.264, checks 3.783.
Fresh original completion took 0.552s; its paired native command 0.072s
(setup 14.782/execution20.150/checks5.121ms internally). Across all measured
attempts, native/canonical/case build commands took60.054s, registry/runtime
preparation 9.861s, original W3 commands 10.244s, untouched CL baseline 1.689s,
original reference/transport captures 16.274s, seven focused graph/RET audits
0.398s, native replay 0.498s, units 0.487s, GDB diagnostic 0.191s and oracle-negative
or failed authoring commands0.052s. Totals include failed/repeated attempts and
can overlap; they are not additive task elapsed time. Analysis/editing, observer
compilation and fixture authoring/static checks were not separately measured.
Full raw output, timing and completion summaries remain under
`~/.ai-tmp/msvc2/vc1.0/c2/guarded-filter/`.

Failed test/build/authoring attempts are retained there: an undefined register
constant spelling; a cold LEA ID and missing symbol owner; an incorrect test
maximum-depth assertion; and assumptions that captured outputs had inline bytes
or that successful runner workspaces persisted. The target build and actual
fresh pairs were not made to reproduce these test-glue assumptions.

Earlier completed cases include the C++ ledger's 7810B object/18463B listing,
the text-record assembler's 3102B object and the SEH packet's 3723B object.
Original eight-byte name POD returns use per-object `-freg-struct-return` for
the App owner and narrow harness access; case objects inherit binary flags.
`src/` contains App reconstruction, `crt/runtime.c` the five older justified CRT
overrides and `harness/` test/native glue. Generic tooling and shared shims remain
outside this binary. The OMF alignment 8 missing-return boundary retains recovered
EAX/ECX inputs on its native symbol; other nonstandard alignments and OMF public
kinds outside 1/3/4 still trap. Migration's historical unaccepted SEH snapshot and
invocation remain under `~/.ai-tmp/msvc2/vc1.0/c2/standardization/before/` and
`paused-wip.patch`; superseded proof trees remain in Git history.
