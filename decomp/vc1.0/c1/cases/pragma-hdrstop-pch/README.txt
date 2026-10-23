Public pragma-hdrstop PCH create/reuse
======================================
Bare /Yc and /Yu with supported #pragma hdrstop("hdrstop.pch"), /FR /Z7
and a calibration checksum form one workflow. Both bare and filename pragma
syntax variants pass original compile/link/runtime0 before freezing. Actual
CL child transport, complete dependencies/mtimes and two repeated raw output
sets are retained. Every IR/browser/PCH byte is exact; PCH38101bytes.

The producer writes PCH before two named packpush/pop/push cycles, naturally
reusing a retired entry without serializing that free-chain state. Consumer
uses actual native PCH/browser producer outputs. Complete native producer IR
contract is checked before exposing the frozen original C2producer.obj; no
backend is invoked or represented as reconstructed. Source and object input
bytes remain checked immutable, as in untouched original execution.

Two whole frontend functions add218bytes. Three exact original LIBC support
entries add261CRTbytes, with independent strdup actualinput/return/NULcopy/
RET proof; no CRTinternal applicationgraph claim. clear_packing actual output
is all12zero bytes with one cdecloutputpointer and ESPdelta4.

Root-approved immutable23file shared input pin was used for reconstruction
after a moving-source guard held one build. All objects were forced to refresh
for pin entry and again for final live-root compilation. Readonly manifests
and actual compiled source hashes are retained. Final48public/15directnative
units,74public rejections/7directrejections+one undefinedword control and
originalcompiler/Rust/ABI/state/grammar checks pass. validation.json records
failures and separate measured timings honestly.

Total extent239800/277234=86.497327%; publiccase239614 and direct-only186.
This is byte-extent union, not branchcoverage or completion.
