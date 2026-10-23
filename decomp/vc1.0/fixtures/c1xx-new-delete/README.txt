Original-validated C1XX follow-on case: c1xx-new-delete

Source and exact original driver arguments are in manifest.json. Raw child
applications/command lines/environment blocks and all phase inputs are retained.
Compiler phases have argv0 only and an explicit MSC_CMD_FLAGS environment.
Independent C1XX, C2 and LINK replay passed; the generated program exits0 with
empty streams. C1XX outputs are exact; only independently demonstrated four-byte
COFF/PE timestamps are permitted for downstream replay. Validation JSON records
actual differing offsets and split elapsed times. This is cached reference data;
no fixture executes Wine. Reference registration lives in C1XX scripts/.
