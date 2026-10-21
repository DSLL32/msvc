# AGENTS.md

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

While we must preserve the original function graph, you have full ownership of source layout, identifiers, libraries, headers, etc. Code in a style faithful to the reconstruction target.
Commit as you please, suggested is per function reconstruction or other substantial work.
