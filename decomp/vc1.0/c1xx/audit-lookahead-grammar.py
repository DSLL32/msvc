"""Verify the generated declaration-lookahead grammar against its original driver."""
from pathlib import Path
import struct
from binary import PE
from grammar import read_grammar, generate, action_at, goto_at
from importlib.util import spec_from_file_location, module_from_spec

HERE = Path(__file__).resolve().parent
original = PE(HERE.parents[2] / 'toolchains/1.0-nt/bin/C1XX3232.EXE')
model = generate(read_grammar(HERE / 'lookahead.bnf'))
specification = spec_from_file_location('generate_parser', HERE / 'generate-lookahead.py')
generator = module_from_spec(specification)
specification.loader.exec_module(generator)
generated = {name: rows for ctype, name, rows in generator.tables(model)}


def generated_action(state, token):
    first, count, first_exception, exception_count, needs_token, rule = \
        generated['lookahead_states'][state]
    for symbol, destination in generated['lookahead_shifts'][first:first+count]:
        if symbol == token:
            return 'shift', destination
    for symbol, destination in generated['lookahead_exceptions'][first_exception:first_exception+exception_count]:
        if symbol == token:
            rule = destination
            break
    return ('accept', 0) if rule < 0 else ('reduce', rule) if rule else ('error', 0)


def generated_goto(state, nonterminal):
    first, count, default = generated['lookahead_gotos'][nonterminal]
    for source, destination in generated['lookahead_goto_transitions'][first:first+count]:
        if source == state:
            return destination
    return default

def shorts(address, count):
    return struct.unpack('<' + 'h'*count, original.read(address, count*2))

# Bounds are established by the driver's immediate limits and the adjacent
# table objects; trailing alignment bytes are excluded from semantic counts.
actions = shorts(0xa6918, 1408)
shift_bias = shorts(0xa7418, 279)
goto_bias = shorts(0xa7648, 52)
rule_lhs = original.read(0xa76b0, 170)
rule_lengths = original.read(0xa7760, 170)
incoming = shorts(0xa7810, 279)
defaults = shorts(0xa7a40, 279)
assert len(model.states) == 279
assert len(model.grammar.productions) == 170
for rule, production in enumerate(model.grammar.productions[1:], 1):
    assert (production.lhs, len(production.rhs)) == (rule_lhs[rule], rule_lengths[rule]), \
        ('production identity differs', rule, production.label)


def original_action(state, token):
    index = shift_bias[state] + token
    if shift_bias[state] > -1000 and 0 <= index < len(actions):
        target = actions[index]
        if incoming[target] == token:
            return 'shift', target
    rule = defaults[state]
    if rule == -2:
        offset = original.read(0xa67d8 + state, 1)[0]
        cursor = 0xa6718 + offset*2
        while True:
            lookahead, rule = shorts(cursor, 2)
            if lookahead == token or lookahead < 0:
                break
            cursor += 4
    if rule == -1:
        return 'accept', 0
    return ('reduce', rule) if rule else ('error', 0)


for state, kernel in enumerate(model.kernels):
    labels = {model.grammar.productions[rule].rhs[position-1]
              for rule, position in kernel if position}
    assert (labels == {incoming[state]}) if state else not labels, ('state identity differs', state)
    assert model.needs_token[state] == (shift_bias[state] > -1000 or defaults[state] == -2), \
        ('lexer timing differs', state)
    assert generated['lookahead_states'][state][4] == model.needs_token[state]
    for token in range(257):
        assert action_at(model, state, token) == original_action(state, token), \
            ('parser action differs', state, token)
        assert generated_action(state, token) == original_action(state, token), \
            ('generated C parser row differs', state, token)
    for nonterminal in model.grammar.nonterminals.values():
        index = goto_bias[nonterminal] + state + 1
        target = actions[index] if index < len(actions) else -1
        if target < 0 or incoming[target] != -nonterminal:
            target = actions[goto_bias[nonterminal]]
        assert goto_at(model, state, nonterminal) == target, ('goto differs', state, nonterminal)
        assert generated_goto(state, nonterminal) == target, ('generated C goto differs', state, nonterminal)

assert generated['lookahead_productions'][1:] == list(zip(rule_lhs[1:], rule_lengths[1:]))
assert model.conflicts == 332
assert not model.recoveries
print('169 lookahead productions regenerate all 279 original state identities.')
print('71703 actions, 14229 gotos and lexer timing match the original.')
print('332 precedence conflicts; logical C actions, gotos and productions match.')

# Audit the complete semantic switch, including actions not visited by the
# starting case. The source combines equivalent actions, retaining every rule.
import re
switch_targets = struct.unpack('<165I', original.read(0x75a54, 165*4))
native_action_rules = {rule for rule, target in enumerate(switch_targets, 1)
                       if target != 0x75240}
semantic_source = (HERE / 'lookahead-parser.c').read_text().split('switch (rule) {', 1)[1]
source_action_rules = set(map(int, re.findall(r'case (\d+):', semantic_source)))
assert source_action_rules == native_action_rules, 'semantic action coverage differs'
print('%d non-default semantic rules retain source actions.' % len(native_action_rules))
