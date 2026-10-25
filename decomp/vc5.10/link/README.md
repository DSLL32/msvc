# VC5.10 LINK

108 application functions / 38,621 original code bytes are reconstructed;
13 shipped CRT/import boundaries add no application credit.
`binary.c` is the function and extent registry. The native harness aliases the
canonical driver `main` to `link_main` using a compilation flag.
Application bodies and original constructor registration remain in `src/`;
`harness/startup.c` owns native setup, invocation and initializer wiring.
`crt/crt-imports.h` only preserves LINK's direct `_iob` import declaration
against the newer SDK macro. It implements no CRT routine and earns no credit;
the original build links the shipped MSVCRT library.

`make -j4 check unit` runs the saved native invocations. Help is complete
(full DWORD status 1100); Win32 hello is partial at the genuine
`add_input_section` INT3 (original RVA `0x5ea70`).
The hello capture retains both the final original output and the applicable
partial output. No fixture invokes Wine.

`make -j4 win32 audit` builds with the original 5.10 toolchain and checks the
full 121-vertex graph, imports and ownership, with return-cleanup checks
for the 108 application bodies and six CRT bodies. The seven import thunks
remain in the graph. Root policy files retain only live finite audit contracts;
shared harness commands are documented in the repository's `STANDARDIZATION.md`.

The pre-migration allocator draft and research remain at
`~/.ai-tmp/msvc2/vc5.10/link/standardization-wip-20261025-085012`.
The accepted allocator uses module offset `0x60`, distinct from copied headers
at `0x48`; a native live-state check covers this difference. The hello workload
now uses stable `T:\vc5.10\link\0001-hello-win32`; its old S: evidence and
final master remain historical references. Continue this EXE case before the
approved DLL workload.
