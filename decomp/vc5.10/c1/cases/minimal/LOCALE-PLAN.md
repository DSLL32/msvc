# Isolated locale support capture

The checksum trial stops natively at the actual `recon_setlocale` entry. The
same original capture separately observes canonical MSVCRT `setlocale(2, "")`
returning `English_United States.1252`. That original-only suffix is not a
native locale-success proof. The existing CL profile combines other compiler
effects, including multibyte codepage changes, and cannot supply this API's
isolated transition by assumption.

Before supporting the reached call, extend the existing original-only entry and
return observer with the narrow CRT locale schema already used by CL:

- Resolve exported `__mb_cur_max`, `__lc_codepage`, `__lc_handle`, and `_pctype`
  against this same actual canonical provider. Re-prove the internal mb-codepage
  slot/table offsets using its own provider bytes; CL's schema is an analysis
  seed. Capture all six locale handles, 257 MB bytes, and 257 ctype WORDs,
  including EOF and the exposed-pointer-plus-one relationship.
- Record static versus newly allocated table storage and retain the complete old
  ctype table at return and the next C1 stub. Preserve the returned string's full
  NUL-terminated bytes, observed allocation identity, and lifetime to that stub.
  Do not infer a general heap layout or repeated-call contract.
- Record target-thread errno, doserrno, LastError, the existing full FloatSave,
  input/caller/stack, current CRT vectors, and unchanged complete C1 regions.
  All reads are observational; no child-side query calls or target-state seeds.

This requires new normal/restore original observations with the same candidate
body and mask but the extended helper, followed by native-only support proof
against those profiles. Existing raw references stay immutable. It is additional
evidence for a newly reached CRT contract, not a recapture for bookkeeping or
additional application credit. Shared behavior belongs to the recon maintainer;
unknown locales, inputs, provider identities and state shapes remain denied.
