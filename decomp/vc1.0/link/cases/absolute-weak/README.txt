COFF weak/lazy/alias fallbacks and /absolutesfrom

Original CL /nologo /c /Od /G3 compiles sources/weak.c and values.c. Executable
instruction bytes stay unchanged. scripts/build-absolute-weak-input.py explicitly
adds one COFF auxiliary record to each undefined function, with default index
pointing to compiled _default_target and characteristics1/2/3. It remaps symbol
indexes in relocations. These are authored COFF metadata edits, not a claim that
CL emitted weak records. Original WINNT.H documents weak class105/TagIndex and
characteristics1/2; native process_weak_external independently handles alias3.
The companion external becomes section-1,value0x1234. No code payload is used as
reconstruction implementation.

Two original successful links match streams/PE/map beyond identified timestamps.
All three weak names bind to _default_target in the original map. values.obj is
also an ordinary input, so this case's /absolutesfrom undefined list is empty.
absolute-weak-unmatched independently exercises a nonempty, nonmatching reader.
Four whole C functions close this family:process_weak_external,
resolve_weak_externals,sort_externals and read_absolute_symbols.

The new path corrected an older External tail alias. data-layout.json and C
assertions recover56-byte allocation, Mac section40/jump44, member50/fallback52;
WORD48 has no encountered reader and contains no copied initial bytes. All old
Mac/import outputs pass after separation. Undefined list nodes are typed8-byte
ExternalLink cells, not an External** array. Native one-shot list-tail behavior
and absolute reader assignment order are preserved.

Two semantic-defect probes remain REJECTED, separate from fixture acceptance:
- capture/rejected-matching-undefined: native reader overwrites the undefined
  backlink union with0x1234 before set_external_defined, then crashes25e99.
- capture/rejected-forced-relocated: /force with an unresolved DIR32 relocation
  serializes a process pointer; full artifacts differ across launches/native.
  Exact old refs, actual artifacts and failed diagnostics remain untouched.
No process pointer or code-byte normalization is permitted. The reader also
updates the remaining head after any match, rather than its matching scan node;
this recovered defect is retained and is not claimed successfully exercised.

family-validation.json contains seven total new FPO/absolute bodies,35 native
cases/600 app entries, canonical compilability/static audits and split timing.
All fixtures invoke ELF and cached refs only. Original capture/compiler diagnostics
run explicitly outside fixtures. User-authorized shared-prefix migration follows
this reconstruction checkpoint before any new reconstruction or idle prompt.
