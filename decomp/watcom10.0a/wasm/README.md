# WASM 10.0a

Target: `toolchains/watcom10.0a/binb/WASM.EXE`, executed for original
captures through its corresponding `binnt/WASM.EXE` loader. Initial proposed
case: `/?`, with redirected output to avoid interactive help pagination.

`make -j4 inspect` only reads and validates the two original files. `make -j4
native original` builds the named `main` INT3 stub with GCC i386/shared shims
and the original Watcom compiler/NT libraries. Both builds pass; native GDB
stops at SIGTRAP in `main`. These are compilability checks, with no application
acceptance or FILE compatibility claim. Fixtures remain gated on CF masking
and service-boundary review.

The first approved unmasked `/?` baseline reached its 30-second timeout with
empty stdout/stderr and no tool artifacts. `cases/help/baseline-timeout.json`
retains the exact command, host environment, identities, inventory and timing.
This failed transport capture supplies no help-output oracle; no repeat or
environment change has been made to that baseline. `scripts/capture-help-baseline.py` is an
explicit one-shot capture utility and refuses to overwrite its evidence folder.

The subsequent approved debugger-only capture independently proves WASM's
resource reader changes GS from the entry selector `0x63` to flat DS `0x2b`,
leaves it changed on return, and reaches WriteFile in that state. All four
instruction before/after pairs are captured for a caller-owned 128-byte stack
buffer. The target stalls in the GetFileType/NtQueryVolumeInformationFile path;
no access-violation claim is made. The helper records the owned child's exit
after termination. `scripts/check-gs-diagnostic.py` checks the cached proof
without invoking Wine. See `cases/help/gs-diagnostic/`.

`cases/help/gs-patch-proposal.json` proposes four fixed-size substitutions in a
new `.patched/binb/WASM.EXE`, with an unchanged `.patched/binnt/WASM.EXE` loader
copy. The approved copies have been created with their exact proposed hashes;
`cases/help/gs-copy-creation.json` records their provenance. They have not run.
A new reference capture must
guard **every** resource-reader invocation's far selector against actual DS;
the first observed reader call does not prove future callers. The changed
`.patched` module/payload/program-name paths are new explicit case inputs,
not normalized or relabeled versions of the failed baseline.

The embedded CF header is at file offset `0x50`, the MZ header size in
paragraphs. Its initialized image occupies file `[0x2200, 0x2a482)`, with entry
RVA `0x1da16`, requested allocation `0x2f000`, and relocation stream at RVA
`0x26bae`. The stream has three groups and 2,915 DWORD base relocations; it
ends exactly at initialized-image end `0x28282`. The loader at PE VA
`0x41018c` decodes an initial high/low 16-bit address followed by cumulative
16-bit deltas for each group, adding the allocated image base at each site.
These are CF relocations, separate from the PE stub's own relocations.

Use payload-relative RVAs as logical identities. CF initialized storage mixes
instructions, data and relocation material: the startup clears BSS
`[0x26bb0, 0x27dc4)`, overlapping relocation storage after relocation has
finished. A blanket `.text` interpretation of the CF image would be wrong.

Static source correlations, all provisional pending original entry evidence:

| Payload RVA | Candidate identity | Evidence |
| --- | --- | --- |
| `0x1da16` | CRT CF startup | Captures loader startup structure, gateway CS:EDX, stack and system byte; clears BSS and initializes CRT |
| `0x1fca2` | CRT main bridge | Loads argc/argv from globals into EAX/EDX, calls `0xcb28`, then exits |
| `0xcb28..0xcb5c` | `main` | Main init, argument init, assembler body, optional stats, message cleanup and error-count return match later `main.c` |
| `0xcb60..0xcb73` | `usage_msg` | Loads usage message ID 2000, calls `PrintfUsage`, exits 1 |
| `0xcb74..0xcba8` | `trademark` | Quiet flag, null-terminated banner pointer array, printf loop, count result |
| `0x1dae9..0x1daf1` | CRT gateway call wrapper | Calls far pointer at payload `0x26800`, preserving EBP |

The loader's actual invoke sequence at PE RVA `0x10015` pushes CS, copies CS
into BX, sets AH to 2, clears ECX and calls ESI. Consequently the CF startup
receives AH=2, ECX=0, gateway selector in BX's low WORD and offset in EDX,
plus the startup structure in EDI. Full EBX is not an allocation-base formal.
The synthetic far return occupies eight stack bytes; only the selector's low
WORD is meaningful, and its upper stack padding must not be treated as input.

The exact later source is in
`/root/.ai-tmp/watcom10/open-watcom-initial-6b9cb443`, commit
`6b9cb4438959d845a2b4dd4bf19f7908ecd608e9`, notably
`bld/wasm/c/main.c` and `bld/wasm/c/wasmmsg.c`. It is a reference, not proof
that the 1994 binary has the same functions or behavior. Its resource reader
reopens the executable using `_cmdname`; retain the full original payload and
resource tail during captures.

Application function ABIs and graph remain in scope. The user permits replacing
the DOS-register/far-call service implementation with normal Win32/CRT calls
through shared `lib/recon/`, after proving each selected service's contract.
That permission does not exempt unknown helpers, handwritten CRT bodies, or
application callbacks. By explicit user decision, the native build may use
ordinary GCC i386 ABIs. The original Watcom Win32 build must retain and audit
the original Watcom ABIs, including EAX/EDX/EBX/ECX register formals. Native
observers decode the native compiler's actual semantic arguments and pointer
ownership; raw native/original register slots need not match. No per-function
veneers or OMF-to-ELF transport are required.

The proposed original command, capture environment, masking checkpoints and
native build recipe are recorded in `cases/help/execution-plan.json`. CF code
extent discovery and Watcom CRT/FILE compatibility remain review gates.
