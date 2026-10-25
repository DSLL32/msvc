# VC5.10 LINK

102 application functions / 37,062 original code bytes are reconstructed;
13 shipped CRT/import boundaries add no application credit.
`binary.c` is the function and extent registry. The native harness aliases the
canonical driver `main` to `link_main` using a compilation flag.

`make -j4 check unit` runs the saved native invocations. Help is complete
(full DWORD status 1100); Win32 hello is partial at the genuine
`allocate_module_section_headers` INT3 (original RVA `0x62ef0`).
The hello capture retains both the final original output and the applicable
partial output. No fixture invokes Wine.

`make -j4 win32 audit` builds with the original 5.10 toolchain and checks the
full 115-vertex graph, imports and ownership, with return-cleanup checks
for the 102 application bodies and six CRT bodies. The seven import thunks
remain in the graph. Root policy files retain only live finite audit contracts;
shared harness commands are documented in the repository's `STANDARDIZATION.md`.

Reconstruction remains paused after this layout migration. The unaccepted
32-byte `allocate_module_section_headers` body (`0x462ef0..0x462f10`, stdcall
`void(OutputModule *)`, current input is the live module from hello.obj) and
its research/build snapshot are preserved at
`~/.ai-tmp/msvc2/vc5.10/link/standardization-wip-20261025-085012`.
The working source retains the last accepted real stub, not that WIP body.
Previous proofs and queued DLL source-only groundwork remain in Git history
at `bc043a3e6fbeb243d65ebc0b0bd0963674f63155`.
