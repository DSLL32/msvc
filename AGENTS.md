# AGENTS.md

Use ~/.ai-tmp for scratch space, creating folders under here as required. Create your own wine prefix in this scratch space.

## Folder layout

 - toolchains/
   Original MSVC toolchains with one folder per version (e.g. 7.1-sp1, 1.0-nt)
 - decomp/
   Reconstructions, with one folder per project - if project consists of more than one binary, one subfolder for each under the project
   Within each binary folder, keep reconstructed source in src/ and scripts in
   scripts/. Keep the Makefile at the binary root; invoke its scripts from that
   root so existing case and build paths remain stable.
 - tooling/replay/
   Shared historical replay infrastructure, independent of a reconstruction project.

## Reconstruction of a binary

Functionally equivalent reconstruction preserving original function graph, individual function ABIs, and any exported functions' signature, name, and ordinal.
The original EXE will have its entire code segment masked with INT3 traps, and our reconstruction should also trap at the entry point. 

Then, for some case:
 - Invoke the modified EXE and our reconstruction in parallel with the case.
 - On trap: 
   * assert that they trap in the same logical place. If they don't, debug, fix, and rerun the case - we may have wrongly determined a function's extent in a previous reconstruction.
   * else, this is a new function to reconstruct. Determine its extent, reconstruct it (potentially replacing a prior stub function), then unmask the instructions + its .text data (e.g. jump tables or constants) in the original EXE. Calls to functions not yet reconstructed should have named stub functions whose bodies trap. Un-stub calls to the new function in prior reconstructions.
   * rerun the case.
 - On no trap: 
   * if case successful - producing identical output to the original - move on to another and repeat this process. Else, debug, fix, and rerun.

## Regression replay cadence

Use fast continuous historical replay for ordinary cases with captured inputs
and concrete outputs, through each binary's scripts/ adapter or Makefile replay
target. It checks recorded first-entry order and the declared terminal event;
retain the case's concrete output comparisons at the end. Keep the fresh paired
check above when accepting a newly reconstructed function.

Keep specialized historical cases (for example real Ctrl+C delivery, injected
state, or unsupported child-process modes) on their existing runners. Replay
those histories after a substantial batch of reconstruction, at a meaningful
checkpoint and at case completion, or earlier when a change directly affects
their behavior. Do not replay every convoluted historical case after each small
function addition. Keep explicit fresh-history and stop-at-frontier commands for
diagnosing divergence. Existing ABI/function-graph audits and relevant full
validation remain required; fast entry-order checks do not replace them.

Record elapsed fixture times when running validation, separating fast replay,
fresh reference, setup/build and output checks where measured. Label unsplit or
unmeasured components honestly rather than estimating them as stopwatch timings.

## Reconstruction source

Reconstruct program logic as readable, idiomatic (for the time period and platform) C, recovering meaningful function signatures, types, structures, and control flow. Do not use mechanically copied or generated assembly as the reconstructed implementation. Use inline assembly only where required to preserve behavior or an ABI that the original C toolchain cannot express, and document the reason. Disassemblers, decompilers, and generated assembly listings are permitted for analysis and verification; this restriction applies to reconstruction source.

Do not bulk copy data sections into the reconstruction in any format. Define data as it is encountered during reconstruction, placing it somewhere that makes sense in the current context.
Keep ordinary string literals at their point of use. Introduce named string objects only when sharing, mutation, or observable address identity requires them.
While we must preserve the original function graph, you have full ownership of source layout, identifiers, libraries, headers, etc. Code in a style faithful to the reconstruction target. Feel free to reorganize code or update function, type, and field names as your knowledge of the target grows during reconstruction.
Compile with the original toolchain that built the target. Create a GNU Makefile in the binary's folder to do the build - invoking the toolchain via wine - and ensure it supports parallelism (e.g. -j 2).
Commit as you please, suggested is per function reconstruction or other substantial work.

## Folder layout

In the binary's folder, put reconstructed source under src/, scripts under scripts/, and cases under cases/.
