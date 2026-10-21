# AGENTS.md

Use ~/.ai-tmp for scratch space, creating folders under here as required. Create your own wine prefix in this scratch space.

## Folder layout

 - toolchains/
   Original MSVC toolchains with one folder per version (e.g. 7.1-sp1, 1.0-nt)
 - decomp/
   Reconstructions, with one folder per project - if project consists of more than one binary, one subfolder for each under the project

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

Do not bulk copy data sections into the reconstruction in any format. Define data as it is encountered during reconstruction, placing it somewhere that makes sense in the current context.
Keep ordinary string literals at their point of use. Introduce named string objects only when sharing, mutation, or observable address identity requires them.
While we must preserve the original function graph, you have full ownership of source layout, identifiers, libraries, headers, etc. Code in a style faithful to the reconstruction target. Feel free to reorganize code or update function, type, and field names as your knowledge of the target grows during reconstruction.
Compile with the original toolchain that built the target. Create a GNU Makefile in the binary's folder to do the build - invoking the toolchain via wine - and ensure it supports parallelism (e.g. -j 2).
Commit as you please, suggested is per function reconstruction or other substantial work.
