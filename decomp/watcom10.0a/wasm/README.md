# WASM 10.0a

Target: `toolchains/watcom10.0a/binb/WASM.EXE`, executed for original
captures through its corresponding `binnt/WASM.EXE` loader. Initial proposed
case: `/?`, with redirected output to avoid interactive help pagination.

`make -j4 inspect` only reads and validates the two original files. Native,
original-compiler and fixture targets deliberately fail until the common CF
transport and service-boundary review is complete. There is no accepted code or
captured behavior yet.

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
