Truncated qualified-pointer name

scripts/build-undecorate-grammar-input.py pointer.obj '?cutqual@@3PA'
produces the frozen valid COFF input. Unlike cpp-template-status's bare P EOF,
this input consumes the A qualifier and reaches prepend_status in the original.
Two untouched-original-only captures agree exactly:332 stdout bytes, empty
stderr, exit0. No timestamp or string normalization. First fresh masked-original
trap3fe00/native GDB demangle_prepend_status agrees status2. Its full42-byte
cdecl wrapper retains Name(status) and Name-plus-Name calls; the final pair
matches the entire frozen output. All31 cached native cases pass afterward.

family-validation.json contains coverage/build/audit/timing results for the
completed C++ family. body-inventory.json separates71 demangler CFG bodies into
18899 instruction bytes,288 inline data and975 alignment; this is static body
accounting, not exercised branch coverage. The seven newly reached bodies add
955 instruction bytes,36 switch-table bytes and19 alignment bytes (1010 extents).

reachability.json records why the eighth nominated57-byte merge-error body
remains a named pending stub: all11 native call sites require suppression flags
absent from LINK's sole fixed0x800 wrapper call. The only native flags writer is
3d984. No flag forcing, hidden API call or false acceptance was used.

Use make -j4 check for native cached regression. Explicit original compiler and
static audit command:make -j4 audit. Fixture execution never runs Wine/originals.
See remaining.json for next public-input opportunities; no new case is started.
