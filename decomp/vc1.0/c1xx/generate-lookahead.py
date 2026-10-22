"""Emit logical C rows from the declaration-lookahead grammar."""
from pathlib import Path
from importlib.util import spec_from_file_location, module_from_spec
import sys
from grammar import read_grammar, generate

HERE = Path(__file__).resolve().parent
spec = spec_from_file_location('generate_parser', HERE / 'generate-parser.py')
shared = module_from_spec(spec)
spec.loader.exec_module(shared)


def tables(model):
    return tuple((ctype, name.replace('parser_', 'lookahead_'), rows)
                 for ctype, name, rows in shared.tables(model) if ctype != 'ParserRecovery')


def emit(model):
    lines = ['/* Generated from lookahead.bnf; edit that grammar. */',
             '#include "lookahead-grammar.h"', '']
    for ctype, name, rows in tables(model):
        lines.append('%s %s[%d] = {' % (ctype, name, len(rows)))
        lines.extend('    { %s },' % ', '.join(map(str, row)) for row in rows)
        lines.extend(['};', ''])
    return '\n'.join(lines)


if __name__ == '__main__':
    model = generate(read_grammar(HERE / 'lookahead.bnf'))
    Path(sys.argv[1]).write_text(emit(model))
    print('generated %d lookahead states, %d productions' %
          (len(model.states), len(model.grammar.productions)-1))
