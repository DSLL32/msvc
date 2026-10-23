Storage, pragma and runtime initialization checksum
==================================================

One original-validated /Z7 /O2 /G5 /W3 C source combines thread-local scalar
and record initializers, block statics, parameter-dependent automatic record/
bitfield values, nested named pack(push/pop), alloc_text and explicit intrinsic
selection after function(abs). The original-linked program checks state,
layouts and integer results and exits0. Actual CL transport and complete raw
frontend files are frozen before reconstruction. No normalization; fixture
execution is native only. Explicit original captures are separate.

Complete at905 accepted entries,778 source functions; eighteen new functions
add2177 frontend extent bytes. Total227839/277234 (82.182921%), not branch
coverage. All26 native phases,11 oracle/dependency negatives and six strict
graph/ABI/mask checks pass. Original/native raw output/entry proofs and measured
including failed attempts are in validation.json and evidence/.
