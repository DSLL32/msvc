# Internal switch scanner

scan_internal_switches owns79 hot bytes128b1..12900 and84 cold bytes30459..304ad.
Its argv loop skips argv[0] and carries the separate parser's mode result across
arguments. It then scans MSC_CMD_FLAGS with a fresh mode and a514-byte token
buffer through a separate token reader. The string literal stays at getenv's
point of use. Both parser/token-reader functions remain named INT3 entries.

The first genuine parser frontier12900 has actual argument argv[1] ("-il") and
unsigned mode0. Fresh normal and labeled reset captures prove raw original
argument pointer/return, native GDB formal inventory and argument bytes including
NUL, and exact invocation pointer identity. An independently recorded native
preprologue scanner entry matches the original128b1 trap; allocator/main
continuation proofs end there, before the scanner's own execution. Scanner entry
to parser changes no original data/BSS/snapshot/plain globals or currently
modeled native state. Real CALL sites/returns and source extents are checked.

No global field or new shared shim is needed. The environment/token branch has
not executed in this first-parser proof; it is reconstructed in source with its
callee boundaries preserved, and will be exercised after argv processing.
Focused scanner ABI/entry/state controls are fresh. The unchanged generic and
allocation controls are explicitly reused with frozen checker/contract/raw
hashes; their full current positive state/API checks still run for both pairs.
Native fixtures remain cached and Wine-free; minimal compilation is incomplete.
