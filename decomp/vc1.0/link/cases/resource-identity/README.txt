Named Win32 resource identity and duplicate-language diagnostics
==============================================================

Root approved this single family in cases/mac-loader-map/next-proposal.json.
The fixture-only generator build-resource-identity.py writes valid ordinary
Win32 RES records: named type SAMPLE, named resource Café, languages0409/0411,
and distinct English/Japanese byte payloads. The duplicate changes only the
second language to0409. Existing original-backed minimal-C test.obj supplies
main; /nodefaultlib /entry:main /subsystem:console avoid unrelated libraries.

Two untouched-original preflights per variant succeeded/repeated: positive
exit0 with a2MB resource PE; duplicate exit26 with exact Café diagnostic and
no identity.exe. reference/ holds these first uncontrolled-TMP runs. Their
working-directory inventory is scoped to the case directory; it does NOT
assert absence of files in the original CRT's separate C: temporary directory.
Raw independent evidence in tempnam/uncontrolled-path proves that directory
contained prior tmpN files and that the CRT searched for a missing candidate.
Those old paths/evidence are historical, not active native fixture inputs.

Controlled external input (separately named reference variant)
-------------------------------------------------------------

Root approved a per-child TMP/TEMP override, only in the capture helper's
process environment before CreateProcess. LINK_CAPTURE_TMP must equal the
verified existing per-case working directory. No prefix/server/global drive
mapping changes occur. controlled-*-spec.json and controlled-reference/ retain
new repeated originals, complete outputs and raw streams. Native phase TMP
uses that identical S: namespace and private drive mapping.

The actual original tempnam body prioritizes TMP, validates that directory,
starts its static counter at1, increments to2, and checks the candidate tmp2
with _access. Empty initial directory means this candidate is missing; the
actual _access returns-1 and leaves errno2. No candidate name is seeded or
normalized. The duplicate exits with a real zero-byte tmp2 artifact, which is
included in its complete expected artifact/inventory. The positive removes
its temporary file and leaves only identity.exe plus the two input files.

The initial native GDB sample correctly preserved resource longjmp value-1,
buffer identity and continuation, but showed errno0. It is deliberately saved
as FAILED evidence in tempnam/native-failed. Its last writer was traced to
shared recon_tempnam/recon_mktemp (0->0), whereas original tempnam directly
uses _access (0->2). The shared maintainer corrected this separately in7181ec69; no error reseed,
stream exclusion, path normalization or candidate constant was introduced.

ABI/graph acceptance
--------------------

The three accepted diagnostic helpers have complete original/native ABI
observations in acceptance/: resource_error_prefix has no formal/hidden
register input; resource_name_to_ansi observes all three argument slots,
counted UTF16 source ownership/content and caller-owned1024-byte output
extent; resource_unicode_to_ansi_byte observes its actual WORD/DWORD argument.
The first logical-only ANSI/byte observations are superseded by the typed
receipts, not silently relabeled. Their original scratch receipts remain in
frontiers/ and will be inventoried at final closure.

The129-WORD translation table at8069e..807a0 is the actual shared readonly
ANSI translation data; the old32-WORD special slice was an interior view of
this same object. Forward conversion still references the same special
entries; reverse conversion scans the full proven original129-WORD table.
The original-data audit checks the full table and named ownership.

Fatal_message is reconstructed as readable C over the complete187-byte
original body and is now accepted after matching live state and complete
output/file closure. Its observed ABI consumes file, message and exactly one
variadic machine word. Its nine original/native named call edges already
match, including _unlink and normal exit. No SEH unwind work is claimed.

Original resource longjmp proof binds buffer5c590/32bytes, insert_resource
caller4320b, convert_resource_input continuation21085, actual valueffffffff,
identical current/saved SEH registration and scope0==0. Native registered
context must resume with signed-1; value coercion to+1 would fail. A real
filter/finalizer/global-unwind requirement would hold this case for policy.

The matcher now validates object-local public callee identity as well as
relative relocation targets. This removes false _getc/_unlink wrapper aliases
without dropping the independently matched actual bodies. Neither wrapper
was previously accepted; newly reached _unlink is accepted as verified CRT
support, including its independent _remove callee identity/ABI and actual
filename/result/live-error proof. It confers no application code credit.

Final checkpoint942
-------------------

Four whole C functions add342 actual original instruction bytes, no new inline
code/data or alignment. The129-WORD readonly translation table is contextual
data and is not counted as application instructions. One16-byte shipped _unlink
CRT extent is separate support. Total247433/281634 original .text bytes is
87.8562%. Partial known-body inventory:182505/190625 target instructions,
28009/37088 CRT instructions,258/348 import thunk bytes;1936 known inline-data
and3063 known-alignment bytes are separate. Remaining16912 unclassified bytes
are NOT labeled CRT. OMF internal-g closure7367B remains honestly pending.

Actual shared tempnam profile libc100 models TMP-first directory validation,
counter/prefix state and direct _access result/errors. native_tempnam.py binds
activation to complete independent original packets and the exact canonical TMP
input. Native GDB observes counter1->2/prefix0->3, actual allocated path and all
live error fields. No candidate constant is supplied to the implementation.

There is a separate, precisely limited ambient LastError discrepancy: original
successful CloseHandle leaves stale1224 after ignored mapped SetEndOfFile
failure, while current native success APIs leave0. The failed full-ambient
assertion is retained at longjmp/native-failed-ambient. Original execution after
successful _unlink arms the target's GetLastError import thunk with INT3: no
reader occurs through actual exit/callback cleanup and exit26. Native GDB's
post-unlink reader guard is likewise empty. Root approved this actual def-use
limit, not ambient1224 equivalence or a broad success-API assumption. Actual
unlink input/result0/live errno2/DOS2, complete streams/status and every file
effect remain exact. Precise provider probe evidence was sent to the maintainer.

All68 cached native fixtures/750 application entries pass5.696s external,
5.409s internal; build7.303s is separate. Setup1.946s/direct ELF0.687s/output
oracles2.722s/trace checks0.007s, trace preparation separately recorded. New
error case0.014s and success0.070s, each direct ELF about6ms. Native fixture
execution launches neither Wine, an original, a PE, nor a debugger. The GDB
acceptance proof is separate and timed in timing.json. Only four structurally
identified PE/resource clock fields normalize; all other PE and empty temporary
bytes are exact. Independent tree checks verify Café, both languages and payloads.

Original -j4 compiler build plus781 CFG/call/RET/reference/ownership/extent graphs,
767 callback sets,490 contextual data definitions and41ECX ABI checks pass.
Graph mutation controls reject wrong app/runtime targets, RET cleanup and split
multi-byte runtime endpoints. ABI controls reject missing/wrong argument/caller
observations; resource recovery controls reject +1 coercion, missing context,
wrong buffer/continuation, required unwind and a later ambient-error reader.

Historical uncontrolled and controlled acceptance tags stay distinct. The final
controlled duplicate's freshly masked original captures reproduce their combined
contiguous four-app-entry segment; frontier_cases explicitly lists both tags,
without relabeling prior acceptance. The success case is a zero-new-entry segment
with full output and no unexpected app entries. All histories are cached.

validation.json, audits.json, checks.json, timing.json and components.json give
current evidence and limits. The next root-approved SINGLE workflow is public
EDIT valid-image rejection/file failure (next-proposal.json); it starts only
after this resource checkpoint commit. No forced OMF internal-g is authorized.

LastError attribution clarification: resource-identity/last-error-attribution.json
records an unattributed dead ambient discrepancy. Original CloseHandle preservation
is observed; no native CloseHandle1224-to0 transition was proved. Current shared
CloseHandle already preserves successful-call LastError. No provider fix is claimed.
