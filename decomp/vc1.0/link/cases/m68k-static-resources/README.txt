Static symbols and explicit Mac resources

The explicit COFF input has a startup CODE resource, static helper, standalone
INIT resource, and data references. CTOT16/DTOC16/DTOC32 select near/static jump
entries; a type20 packed pointer also targets the static helper. INIT carries a
same-resource PC-relative type21 fixup. Resource overrides reserve CODE7/INIT9.
The frozen input, layout and deterministic fixture writer are pinned in manifest.
No input or output is run as Macintosh code.

Two original-only captures succeed with identical streams and full2MB output
after only the PE COFF timestamp. Fresh masked-original/native-GDB entries close
through eight C functions and the same complete terminal artifact. Resource-ID
recording preserves the original assignment-before-test behavior. The insertion
result retains its native scratch-address low byte; native callers ignore it.

Original bsearch callback370d0 has identical bytes to qsort callback37070; they
remain separate graph vertices. A rejected initial callback sample is retained.
Final artifact comparison also found an old layout_sections conditional error:
a standalone non-CODE resource without a loader needs no prologue, while C had
reserved40 bytes. The original21af7..21b4a branch established the correction.
All28 native cases pass after that fix. See validation.json and the family
receipt at ../m68k-dupcon/family-validation.json for timing, audits and evidence.
