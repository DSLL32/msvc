# Checksum tables (accepted)

`initialize_checksums` owns RVA `392a8..39394` (236 bytes). Its three
256-byte BSS tables at `be970`, `bea70`, and `beb70` are consumed by the
16-bit checksum loop at `2bf2b..2bfa6`. The encountered polynomial taps at
`d5c58` are declared beside their use. No surrounding data is represented.

The typed projection adds exactly 768 semantic BSS bytes. Native BSS is now
2312 bytes: 2281 semantic bytes and 31 bytes of natural padding. Initialized
projection remains 1977 semantic bytes plus 51 bytes of natural padding;
6543 bytes of the original 8520-byte internal snapshot remain unmodeled.

The first original trial reached the real next target helper `39394` in
24.312 seconds. Its immutable directory is
`~/.ai-tmp/msvc2/vc5.10/c1xx/minimal-checksum-locale-original-1`, receipt
SHA256 `ebdbb204d8af623e8e00defcad03832006960fa7f4c88ec22317b76c0c638568`.
Original and native builds, whole-candidate graph audit, and normal/nonzero
paired executions pass. The durable ledger contains exactly one new 236-byte
application function; shared CRT selection receives no application credit.

The intervening canonical CRT call is `setlocale(LC_CTYPE, "")`, resolved
through actual IAT `c914c` to authenticated MSVCRT RVA `5729`. The original
changes LC codepage 0 to 1252, handle[2] to `409`, and `_pctype` from its
static table to another mapped table. It returns the persistent string
`English_United States.1252`. Internal MB flag and MB codepage both remain
zero; MB_CUR_MAX remains one, and the complete MB table remains unchanged.
This is distinct from CL's combined locale/codepage transition.

`scripts/check-isolated-locale.py` revalidates all 10772 captured artifact
hashes, canonical hFile identities, actual call ABI, full old/current tables,
raw TEB/PTD errors, complete 112-byte FP preservation, and unchanged original
live/saved/BSS state through the next target frontier. Private ordered CRT
argv/environment and Invoke vectors remain hash-bound private artifacts;
they are not printed or committed. Mapped regions establish readable ranges,
not heap object extents. The checker grants no native success or acceptance.

Thirteen semantic denials pass in 22.017 seconds. Mutated temporary copies
have their transport hashes rebound, so the tests exercise the semantic
guards rather than merely detecting raw-file changes. The original files
are unchanged. The shared maintainer independently revalidated the packet.

Native probes collect actual checksum entry/return, complete 768-byte output,
callee ABI and neighbor/snapshot guards, followed by the actual locale
entry/return and next target entry. The declared restore auxiliary seeds
all 768 bytes before BSS clear and again at the real checksum entry. Its
separate pre-seed handoff observation preserves earlier function proofs;
no caller argument, code, or helper result is mocked.

The native normal run takes 6.026 seconds under GDB; the restore auxiliary
takes 6.966 seconds versus 25.530 seconds for its original. Twenty-two fresh
checksum/native API denials pass in 29.824 seconds, alongside the thirteen
original API controls and three helper/source/mode controls (0.114 seconds).
The cached native lifecycle fixture takes 13.60 ms including build/setup/checks,
with 2.20 ms spent executing the ELF. These are separate from GDB observation
and original-capture times.

Native builds explicitly select the canonical MSVCRT 5.10 CRT profile. The
first denied-profile attempt is retained. The reviewed isolated selection is
enabled with this case's own original receipt before DLL entry; only the actual
target API call changes locale state. Before/after/next guards verify separate
MB flag/codepage zero, full tables, persistent old/current table and result
identities, ordered private vectors, errors, and architectural FP state.

The normal and restore helpers retain separate frozen binaries/source hashes.
Their exact source delta adds only the pre-seed BSS observation inside the
restore-only guard. It does not change the retained normal execution.
`checksum-trial.json` records both proofs, controls, source identities and timing.
`python3 scripts/check-checksum-history.py` revalidates them without executing
either binary. The next helper, `initialize_multibyte_categories` at `39394`,
remains a named trap.
