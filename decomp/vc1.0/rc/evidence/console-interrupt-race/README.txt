These exact failed logs are analysis evidence, never accepted outcomes.
The initial helper removed the lexer-entry breakpoint before resuming the
primary to allow Wine SIGINT dispatch. A callback CREATE_THREAD event did
not guarantee that the primary had stopped before resuming resource parsing.
Both original and reconstruction could therefore race normal stream closure.

original.proof retains 128 stack words and frame chain: callback argument 0,
OS-created thread 528, native _fcloseall -> fclose -> _close descriptor 4,
CloseHandle(0x44), NTDLL exception address 0x7bd00bae, STATUS_INVALID_HANDLE
0xc0000008 first chance. original.events retains the same second-chance fault;
the harness then terminates with 99. Reconstruction separately reproduced the
same fault on native callback thread 308; its raw proof/events are also kept.

The corrected helper keeps the lexer breakpoint installed while briefly
resuming for signal delivery. It suspends at either CREATE_THREAD or a repeated
lexer trap and handles an already queued trap without a second suspend count.
The primary cannot execute native parsing past the captured boundary. Ten
consecutive paired runs with the corrected barrier matched status/streams and
full cleanup. Final durable replay independently repeats the case.
