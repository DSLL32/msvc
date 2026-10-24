Original-validated member registry/browser v2 frontend fixture.
Two exact source inputs; five IR outputs plus complete registry.sbr.
Original full CL/C1XX/C2/LINK and both generated-program runs exit0 silently.
The independent frontend reproduces the captured actual S: working directory,
including its -driver suffix, because the SBR header records it. No path bytes
are normalized. Original/native captures and native-only fixtures use that same
canonical cwd. Full baseline manifests/validation and ordinary scratch path/hash
provenance accompany the frontend inputs/outputs; no toolchain bundle.
The failed cross-model widening v1 remains separate unchanged evidence.
