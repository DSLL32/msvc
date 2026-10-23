Public EDIT rejection/file-failure checkpoint946

Three ordinary public phases exercise valid-image rejection and file failure:
  -edit /rebase:base=0x500000 fixed.dll ->54/LNK1151
  -edit /bind:path=. client.exe ->53/LNK1150, provider has no exports
  -edit /section:.text,R missing.exe ->7/LNK1104

Inputs are independently original-built ordinary i386 DLLs and a previously
frozen importing client. /FIXED is a separate valid setup with RELOCS_STRIPPED
and zero relocation directory. The original resource image's empty8-byte
relocation block was correctly recognized as present; the initial no-reloc
assumption failed static validation before EDIT executed. See the preserved
initial-static-correction.json. All setup/captures are explicit original-only
tools, never fixture dependencies. Twice untouched originals agree exactly on
status/streams and the full file inventory: fixed.dll/client.exe/provider.dll
are unchanged, missing phase creates nothing. No output normalization is used.

Whole readable C replacements:
  editor_rebase_error287a0..287e1:65instruction bytes
  editor_bind_error28750..28791:65instruction bytes
  file_failure26b10..26b33:35instruction bytes
Separate shipped CRT __snprintf52f20..52f90:103instruction bytes+9alignment, zero
application credit; exact original/canonical library identity and graph retained.
Only the actual bounded public formatting contract is claimed natively.

Fresh masked-original/native matching observes every incoming cdecl slot and
actual strings/callers. Snprintf additionally proves its output is the actual
caller-owned340-byte local message, capacity340, one consumed variadic DWORD,
return53/text/NUL/errno0. Unwritten output tail is not compared or claimed.
acceptance/snprintf/observer-annotation.json corrects erroneous scope prose in
its immutable raw receipt; the actual owner/extent fields were already present.
The initial GDB observer template syntax error was a failed analysis tool run,
never a promoted frontier. Three explicit native GDB completion runs check full
original streams/status/inventories separately from ordinary native fixtures.

All71 cached native cases/753application entries pass, using no Wine/debugger
or automatic reference recapture. Final rebuild7.596s, external suite6.307s;
summed setup2.097/ELF.796/oracle3.046/tracechecks.008s. New rebase/bind/missing
phases take.064/.119/.008s including setup/oracles; exact individual values are
in final-run.json. Canonical -j4 compile+audits26.596s, graph negatives18.104s.
785CFG graphs,770callback sets,490data definitions and41ECX routines pass.
23host-only ABI/output ownership/omission/caller controls and existing native,
CRT, edge, resource and cross-import guard controls pass. See checks/audits.json.

Actual shared C/H working-tree source hashes and scratch snapshots are bound in
final-run.json; this held window included coherent broker WIP with its new opt-in
profile disabled for LINK. It is not falsely described as a committed-only build.
Raw final native receipt remains under the stated scratch path with its SHA.

Extent coverage247710/281634=87.9546%,946frontiers. The partial instruction
inventory separately has182670/190690known target bytes,28112/37088CRT bytes,
258/348import bytes. Remaining16838unclassified bytes are neither CRT nor proven
padding. Contextual inline data1936 and alignment3063 are separate. Known OMF
closure7367B includes unavailable internal-g/mode2debug paths, not a public
reachability promise. next-proposal.json targets a coherent ordinary OMF workflow
and awaits root review; no extra case began during this checkpoint.
