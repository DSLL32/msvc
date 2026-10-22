#!/usr/bin/env python3
"""Generate logical assembler LR rows solely from its named source grammar."""
import argparse
from pathlib import Path
from grammar import read_grammar, generate

BINARY = Path(__file__).resolve().parents[1]


def files():
    g = read_grammar(BINARY/'src/asm-grammar.y')
    m = generate(g)
    assert len(m.states) == 136 and len(g.productions) == 83
    tokens = {v:k for k,v in g.terminals.items()}
    nonterminals = {v:k for k,v in g.nonterminals.items()}
    header = '''/* Generated from asm-grammar.y; logical symbols and table declarations. */
#ifndef ASSEMBLER_GRAMMAR_H
#define ASSEMBLER_GRAMMAR_H
#include "parser.h"
#include <recon.h>

enum AsmToken {
'''
    header += ''.join('    %s = %d,\n'%(n,v) for n,v in g.terminals.items())
    header += '''};
extern ParserTransition asm_shifts[], asm_reductions[], asm_goto_rows[];
extern ParserState asm_states[136];
extern ParserGoto asm_gotos[24];
extern ParserProduction asm_productions[83];
#endif
'''
    shifts, reductions, states = [], [], []
    for q in range(len(m.states)):
        first_shift, first_reduce = len(shifts), len(reductions)
        shifts += sorted((t,d) for (state,t),(kind,d) in m.actions.items() if state==q and kind=='shift')
        reductions += sorted((t,(-1 if a[0]=='accept' else a[1])) for t,a in m.exceptions[q].items())
        states.append((m.defaults[q],int(m.needs_token[q]),first_shift,len(shifts)-first_shift,
                       first_reduce,len(reductions)-first_reduce))
    source = '''/* Generated from named assembler grammar and precedence declarations.
   These are logical shifts/reductions/gotos, not the original packed tables. */
#include "asm-grammar.h"
#include <recon.h>

ParserTransition asm_shifts[] = {
'''
    for q,row in enumerate(states):
        source += '    /* State %d. */\n'%q
        source += ''.join('    { %s, %d },\n'%(tokens[t],d) for t,d in shifts[row[2]:row[2]+row[3]])
    source += '};\n\nParserTransition asm_reductions[] = {\n'
    source += ''.join('    { %s, %d },\n'%(tokens[t],r) for t,r in reductions)
    source += '};\n\nParserState asm_states[136] = {\n'
    source += ''.join('    { %d, %d, %d, %d, %d, %d }, /* state %d */\n'%(*row,q) for q,row in enumerate(states))
    source += '};\n\nParserTransition asm_goto_rows[] = {\n'
    gotos, rows = [(0,0,0)], []
    for nt in range(1,24):
        transitions = sorted((q,d) for (q,s),d in m.transitions.items() if s==-nt)
        source += '    /* %s. */\n'%nonterminals[nt]
        source += ''.join('    { %d, %d },\n'%(q,d) for q,d in transitions)
        gotos.append((m.goto_defaults[nt],len(rows),len(transitions)))
        rows += transitions
    source += '};\n\nParserGoto asm_gotos[24] = {\n'
    source += ''.join('    { %d, %d, %d }, /* %s */\n'%(*row,nonterminals.get(nt,'unused')) for nt,row in enumerate(gotos))
    source += '};\n\nParserProduction asm_productions[83] = {\n    { 0, 0 }, /* augmented accept is handled by the automaton */\n'
    for rule,p in enumerate(g.productions[1:],1):
        rhs = ' '.join(tokens[t] if t>=0 else nonterminals[-t] for t in p.rhs)
        source += '    { %d, %d }, /* %d: %s : %s */\n'%(p.lhs,len(p.rhs),rule,nonterminals[p.lhs],rhs)
    source += '};\n'
    return {'src/asm-grammar.h':header,'src/asm-grammar.c':source}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    options = parser.parse_args()
    for name,text in files().items():
        path = BINARY/name
        if options.check:
            assert path.read_text() == text, 'regenerate '+name
        else:
            path.write_text(text)
