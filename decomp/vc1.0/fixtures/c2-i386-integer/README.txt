Explicit original-only capture; fixture execution is native-only.
Source, CL/C1/C2 transport, exact backend environment/IR and output are pinned
in manifest.json. capture-validation.json records independent original backend
verification and phase timings. Normalize only the COFF timestamp bytes4..7.
No reconstructed frontend or external dependency is used.
Original CL arguments: /nologo /c /O1 /G3 integer386.c
