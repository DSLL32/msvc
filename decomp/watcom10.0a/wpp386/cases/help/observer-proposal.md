# Next step: CF launch observation only

Review and implement through the common CF observer before any masked acceptance.
The approved two baseline invocations are complete; this proposal authorizes no
additional original invocation by itself.

1. Start the same binnt launcher, canonical cwd, bare `?`, controlled host
   environment and redirected regular handles under a Win32 Debug API observer.
   Capture the actual child command line, current directory, image path and
   process status. Use the PE load-event base and verify original stub identity.
2. Stop at stub RVA `0x10763`, immediately before `_InvokePgm`. Read the allocation
   base from stub RVA `0x30004` and verify it equals EBX, while ESI equals base +
   `0x664f4` and EDX equals stub-base + `0x10788`. Retain raw CONTEXT and parameter
   block addressed by EDI. The stub's pre-invoke ECX is not the eventual CX=CS;
   that selector is established inside `_InvokePgm`.
3. Read the full initialized payload and compare against the exact original file
   plus all decoded base relocations. Record allocation bounds/protection and
   owned pointers. Read pgmname, command tail, complete NUL-terminated environment
   block, breakflag address/value, copyright pointer and observed trailing field
   layout from actual memory. Do not infer the 2002 layout's additional fields
   exist in 1994 without proof.
4. Place one temporary breakpoint at base + `0x664f4`, retain the original byte,
   and stop at first payload entry. Record actual AH, CX, segment registers,
   stack/far-return frame and EDI ownership. Restore that exact byte, rewind EIP
   and continue the unmodified payload; this is an observer preflight, not a mask
   or an application-function reconstruction. Retain raw status/streams/artifacts
   and compare with the existing masters, with no new normalizations.
5. Produce a hash-bound receipt with exact restored breakpoint ranges and timing.
   No additional Wine environment helper, process, prefix or drive mapping.

The later application mask is a separate gate: it must cover every proved
executable application range and prevent any unclassified application code from
running silently. A known-function-only trap list does not satisfy that. Require
either a complete reviewed executable/data layout or the common observer's
reviewed fail-closed outbound-control-transfer mechanism. Apply masks only after
relocation at launch; never write INT3 bytes into on-disk DWORD relocation sites.
Keep typed data/relocation metadata intact; no blanket startup or CRT exemptions.

Once this observation works, collect actual startup/library provenance and use
the same transport for fresh paired frontiers. Native uses GCC i386 ABI; original
Win32 uses audited Watcom ABI. The comparison decodes each side's real formals
and checks semantic values/owners/effects. It does not compare register positions.
