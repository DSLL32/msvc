"""Emit the build's logical assembler parser tables from the declarative C++ grammar.

No original image or analysis listing is read. The output intentionally uses
separate named rows instead of reproducing the executable's packed storage.
"""
from pathlib import Path
import sys
from grammar import generate, read_grammar


def tables(model):
    shifts, exceptions, states = [], [], []
    for state in range(len(model.states)):
        shift_row = sorted((symbol, target)
                           for (source, symbol), (kind, target) in model.actions.items()
                           if source == state and kind == 'shift')
        exception_row = []
        for token, (kind, target) in sorted(model.exceptions[state].items()):
            assert kind != 'shift'
            exception_row.append((token, -1 if kind == 'accept' else
                                  target if kind == 'reduce' else 0))
        states.append((len(shifts), len(shift_row), len(exceptions),
                       len(exception_row), int(model.needs_token[state]), model.defaults[state]))
        shifts.extend(shift_row)
        exceptions.extend(exception_row)
    gotos, goto_transitions = [(0, 0, 0)], []
    for nonterminal in range(1, len(model.grammar.nonterminals)+1):
        default = model.goto_defaults[nonterminal]
        row = sorted((state, target) for (state, symbol), target in model.transitions.items()
                     if symbol == -nonterminal and target != default)
        gotos.append((len(goto_transitions), len(row), default))
        goto_transitions.extend(row)
    productions = [(0, 0)] + [(production.lhs, len(production.rhs))
                            for production in model.grammar.productions[1:]]
    return (
        ('ParserState', 'asm_states', states),
        ('ParserTransition', 'asm_shifts', shifts),
        ('ParserTransition', 'asm_reductions', exceptions),
        ('ParserGoto', 'asm_gotos', gotos),
        ('ParserTransition', 'asm_goto_rows', goto_transitions),
        ('ParserProduction', 'asm_productions', productions),

    )


def emit(model):
    lines = ['/* Generated from asm-grammar.y; edit the grammar, not this file. */',
             '#include "asm-grammar.h"', '#include <recon.h>', '']
    for ctype, name, rows in tables(model):
        lines.append('%s %s[%d] = {' % (ctype, name, len(rows)))
        lines.extend('    { %s },' % ', '.join(map(str, row)) for row in rows)
        lines.extend(['};', ''])
    return '\n'.join(lines)


if __name__ == '__main__':
    here = Path(__file__).resolve().parent
    model = generate(read_grammar(here.parent / 'src/asm-grammar.y'))
    destination = Path(sys.argv[1])
    destination.write_text(emit(model))
    print('generated %d states, %d productions from asm-grammar.y' %
          (len(model.states), len(model.grammar.productions)-1))
