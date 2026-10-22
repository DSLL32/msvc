Inline assembler frontend case (active reconstruction)
=======================================================

The source uses C parameter/local operands, x86 registers, arithmetic, an
immediate comparison and a conditional forward label. The original phase uses
the minimal-C compiler arguments and an explicit MSC_CMD_FLAGS-only environment.
All source/catalog dependencies, streams and complete four-file output oracle
are hashed in manifest.json. An untouched original exits0 with empty streams;
its raw outputs are irex1100,irsy107,irin6,irgl75. No normalization is permitted.

Current checkpoint:728 accepted original entries,625 reconstructed source
functions. The first698 belong to prior completed cases. The new30 entries
are29 frontend functions plus the known original tolower library extent.
Original/native executions match at31 consecutive frontiers,698 through728,
including exact six pending FILE buffers, visible files and streams. The current
named trap is find_label,original0x2c500,while resolving jle asm_done. This case
is intentionally not registered as a completed exit0 fixture yet. Its source,
full original oracle,current terminal and ordered acceptance evidence are
already durable here. The eight completed cases remain the normal native suite.

The82-rule assembler grammar is declarative source,src/asm-grammar.y. Ordinary
LALR generation plus seven MASM precedence groups regenerates all136 states.
The independent original-table audit verifies all6936 actions,including errors
and default reductions,all136 lookahead timings,83 production rows and3128
gotos,both against the model and the linked original-compiler C tables. Runtime
code uses logical rows and has no access to the original PE or packed tables.

Instruction/register dictionaries and operand rules are named contextual data.
Audits compare330 mnemonic rows,76 operand keywords,46 register properties,
98 aliases,230 instruction class/CPU properties,60 operand-class masks,32
address classifications,153 lexer classes and21 serialization formats. The
counted assembler instruction workspace is128 bytes ending before the original
target_cpu_mask. Its longest legal encoding is78 bytes; the former one-byte
encountered prefix is now owned and named by this reached producer.

Explicit development capture/debug commands are the same as the SEH case:
  python3 scripts/capture-frontier.py --case cases/inline-asm --debugger PATH --directory SCRATCH
  python3 scripts/debug-frontier.py --case cases/inline-asm --reference CAPTURE --directory SCRATCH
Only the explicit original capture runs Wine. Native GDB and ordinary fixture
checks never invoke it. Native CRT observations use explicit shim breakpoints
where needed; there is no invented application implementation of tolower.

validation.json records the checkpoint build,audits,old-case regressions and
measured timing totals. Raw evidence preserves original CRLF/trailing blanks.
The active queue continues with label handling,then browser/preprocessor output.
