Inline assembler frontend case (active reconstruction)
=======================================================

The source uses C parameter/local operands, x86 registers, arithmetic, an
immediate comparison and a conditional forward label. The original phase uses
the minimal-C compiler arguments and an explicit MSC_CMD_FLAGS-only environment.
All source/catalog dependencies, streams and complete four-file output oracle
are hashed in manifest.json. An untouched original exits0 with empty streams;
its raw outputs are irex1100,irsy107,irin6,irgl75. No normalization is permitted.

Completed at732 accepted original entries and629 reconstructed source
functions. The new34 entries are33 frontend functions plus the known original
tolower library extent. Original/native executions match at35 consecutive
observations,698 through732,including all pending FILE buffers,visible files,
streams and successful termination. Two untouched originals reproduce the raw
full oracle exactly. This is the ninth normal native trace/output fixture:
  make -j4 check CASE=inline-asm

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

validation.json records both verified checkpoints,builds,audits,regressions and
measured timing totals. Raw evidence preserves original CRLF/trailing blanks.
The active queue continues with browser/preprocessor output.
