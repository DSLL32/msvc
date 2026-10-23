# Type-rank initialization

Target C1XX.DLL SHA256 is recorded in functions.json and each immutable trial.
The sole candidate is `initialize_type_ranks`, RVA `38a6b..38ab6` (75 bytes),
cdecl `int(void)`. Its accepted caller `38a60..38a6b` stores the return at
`c4c78` and returns. The helper reads no incoming register or stack argument;
its inputs are the two initialized tables and the BSS output arrays below.
The next actual frontier is callback35, a cdecl(void) boundary with no incoming
reads. No export/callback or allocator is mocked in these trials.

## Actual records and consumers

Each encountered original record is an int code at +0 and BYTE rank at +4,
with stride8. The three intervening original padding bytes remain unmodeled.
The original PE bytes and each loaded original table establish these defaults:

| Table | Original address | Code/rank records, including terminator |
| --- | --- | --- |
| primitive | 104cd708 | (10h,1), (40h,2), (80h,3), (100h,4), (200h,5), (400h,6), (800h,7), (0,0) |
| complex | 104cd748 | (1,8), (2,9), (3,10), (4,11), (10h,12), (20h,13), (40h,14), (44h,15), (0,0) |

The candidate walks each table until code0. For primitive entries it performs
signed code>>5 and stores rank at c4cc8+index. For complex entries it stores at
c4c80+code. It returns1. Read-only consumer evidence at RVA1137e..113b2 selects
c4cc8 when a type's +4 pointer is NULL, extracting a masked type flag index;
otherwise it uses the pointed descriptor's first byte and c4c80. This grounds
rank roles without guessing the C type corresponding to each code.

The current output domains cover exactly the smallest contiguous byte arrays
containing all encountered writes: primitive[65], complex[69]. Their loaded
original defaults are verified zero. The later consumer can form larger
indices and is not accepted. It must remain a genuine frontier until any
additional domain is reconstructed. No bounds branch, guessed storage or
placeholder original gap has been inserted in target source.

## Projection and lifecycle

All17 records contribute85 semantic initialized bytes. Together with the
existing first-invocation int, the projection is89 bytes out of the original
8520-byte snapshot; the other8431 bytes remain UNMODELED. Native FrontendData
is140 bytes, including51 natural record-padding bytes. The original and native
padding values are not compared or claimed equivalent. Native field offsets,
record stride, sizes, source member expressions, original storage intervals,
snapshot offsets and nonaliasing are checked separately.

Original datafe first-copy and restore still transfer all8520 actual bytes,
whose complete raw images are retained. Native target code performs the same
ordered typed assignment for every modeled field, then clears every modeled
BSS field on restore. Its malloc request remains8520. The native BSS is892 bytes:
863 semantic bytes and29 natural padding bytes, including the two trailing
bytes introduced by the byte arrays. The current initialized projection is not
a claim of full original data or byte equivalence.

The unaccepted helper is the only candidate. Its source and the expanded
already-accepted initializer compile with the original VC5 compiler, and the
common Rust audit checks the actual same current Win32 code graph, extents,
RET cleanup, imports and references. No table of unencountered original data
is copied into source. Unknown semantic consumers, serialization, hashing,
comparison or escape of original unmodeled fields remain hard frontiers.

## Fresh execution proofs

`rank-helper-trials.json` records one natural candidate proof and two explicit
nonzero-state restore auxiliaries. The natural proof uses all actual defaults
and the genuine accepted allocator, demonstrating first-copy of every modeled
code/rank before the helper executes. Both auxiliaries use owned saved state;
no function body is mocked or additionally unmasked.

For restore, every current code/rank is replaced by distinct nonzero values,
including terminator codes. Saved records retain terminating code0 but have
rank+40 in all17 records, including the unused terminal rank. The saved first
primitive code becomes20h and first complex code5. This proves actual copied
code values affect the resulting indices rather than testing only constants.
The initializer restores saved codes/ranks before the candidate consumes them.
All134 output bytes are independently seeded nonzero (181/182), as is the
ready flag37; actual original and native reset them to zero before the helper.
Both compiler restore modes are exercised. The helper then builds exactly the
expected outputs, skips the nonzero rank of terminal records, and the caller
sets ready1. Previously modeled context/output/PDB/pool/token/signature state,
real43-entry callback order, hidden thiscall inputs and caller state remain
checked by their existing decoders.

The first diagnostic run omitted the new helper's restore-phase observer and
is not an acceptance proof. Controller classification was corrected and a fresh
normal pair numbered2 captured. A validator-only hyphen/underscore filename
correction reused those unchanged raw executions; both auxiliaries were then
captured fresh. No historical receipt or raw file was edited.
