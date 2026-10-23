# WCC stopped-loader prerequisite

This is one explicit original-reference plumbing capture, not a native fixture,
application acceptance or a request to reconstruct a function. The payload CALL
stays trapped. No CF instruction executes. Stop afterward for user review.

The reviewed inputs are immutable `binnt/WCC386.EXE` (SHA
`8fee7d6ab3f2c06a0a874135e3898386898679e7dd53993fcc508e296364767d`)
and `binb/WCC386.EXE` (SHA
`c3666de94f6fa6800f452dae8acf45505ecdb62f0ade2cc27cc86c2d9e8e2b6b`).
Use the shared prefix, fixed R:/S: mappings, raw argument `?`, empty regular stdin
and an empty case working directory. Preserve host environment privately at0600;
remove WCC386/WCC/INCLUDE and compiler-option environment overrides. The exact
scratch plan binds all scripts, broker source/PE/MAP and both original files.

1. Existing checked entry-trap transport stops the loader at observed imagebase
   +10010. It refuses TLS, unexpected threads/processes/exceptions. Consume only
   that INT3, preserving every unrelated register and flag.
2. Before continuation, compare all4608 BEGTEXT and512 DGROUP initialized bytes
   against the pinned loader, applying its137 HIGHLOW relocations at the observed
   imagebase (134 text,3 data). Exclude only OS-resolved .idata from file identity.
   This thin byte verifier does not discover a CFG or declare CF code ownership.
3. New RPC21 is callable only at that exact rewound entry, with TF clear and the
   actual loader's entry-JMP, Invoke and launch-CALL bytes verified. Install the
   existing checked/write1/flush/readback INT3 at imagebase+10763, then continue
   this fixed loader prefix once. This is the one CF-launch path: CALL10763 ->
   Invoke10015 -> CALL ESI. The first CALL is never executed here.
4. Accept only its same-process/thread first-chance breakpoint, actual EIP10764,
   TF clear, launch byte stillCC. New threads/processes/exceptions and after-entry
   DLL events remain HOLD. After prefix continuation the broker permanently
   rejects all resume, consume, ARM, STEP, trap writes and repeated-prefix RPCs;
   only the initial EVENT, inspection and owned termination are available.
5. Observe globals +30000(entry),+30004(load),+30008(base),+3000c(breakflag).
   Require load==base!=0, entry==load+2fe00 and no32-bit allocation wrap.
   Require actual ESIentry, EBXbase, EDXimagebase+10788, EDI=ESP+104. Read all five
   actual parameter DWORDs: program, command, environment, break pointer,
   copyright0. Require binb program path, raw command`?`, break pointer+3000c.
   Store the bounded complete double-NUL environment privately; public receipt
   contains only pointer/length/hash. No2002 seven-field structure is imposed.
6. Record VirtualQueryEx regions covering requested91000 bytes. Require committed
   readable pages; LocalAlloc's returned pointer need not equal the enclosing
   virtual allocation base. Read all8209c initialized bytes from the actual load
   address. Compare every byte against original CF image after all11801 DWORD
   additions in original stream order, modulo2^32 (8 groups; stream ends8209c).
   This includes every non-fixup byte. Preserve actual snapshot privately in
   scratch with hash/length in receipt; do not rebundle a toolchain binary in Git.
7. Recheck stopped full context, allocation globals and launchCC. Terminate only
   the owned child, require actual EXIT_PROCESS+signaled process and natural
   broker exit, retain raw launcher/target streams even on failure. Target stdout
   and stderr must be empty before the payload launch.

The fixed loader prefix uses ordinary synchronous KERNEL32 imports to read and
relocate the pinned file. Its SetConsoleCtrlHandler registers known *loader*
handler105c9, not a CF callback; no console/PTY/control events are supplied. Any
actual new thread/exception remains stopped. This is not a free-run permission for
unknown CF startup, CRT or application code. The CF header entry2fe00 is observed
but not entered; original main1a99b and native main remain unaccepted.

Budgets: one helper launch,55s helper deadline,2s independent owner lease,60s
Python channel deadline,10000 debug events, outer bounded cleanup. Snapshot uses
512-byte reads; parameters bounded64KiB each. No retries. Preserve any failed
receipt/streams and revise a fresh attempt only after review. Record setup/entry
identity, fixed prefix, initialized snapshot, relocation comparison and teardown
elapsed times separately. Existing four authored transport controls remain the
transport proof; no new generic suite is a prerequisite.

This document and sources are preparation only until the orchestrator releases
the exact frozen scratch command packet. No original loader run has occurred for
this new operation at preparation time.
