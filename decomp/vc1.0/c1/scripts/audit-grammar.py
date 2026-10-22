#!/usr/bin/env python3
"""Check the logical parser tables against the original compressed automaton.

Only verification reads the original image. Reconstruction uses its own
logical shifts, reductions, gotos, and production records.
"""
from pathlib import Path
import re
import struct
from compare import indexes, publics

original, rebuilt, _, _, _ = indexes()
names = publics()

def word(image, address):
    return struct.unpack('<h', image.read(address, 2))[0]

def byte(image, address):
    return image.read(address, 1)[0]

def original_action(state, token):
    base = word(original, 0x6ff00 + 2 * state)
    if base > -1000:
        index = base + token
        if 0 <= index < 1219:
            destination = word(original, 0x6f578 + 2 * index)
            if word(original, 0x70478 + 2 * destination) == token:
                return 'shift', destination
    rule = word(original, 0x70768 + 2 * state)
    if rule == -2:
        address = 0x6b308 + 2 * byte(original, 0x6b368 + state)
        while True:
            exception, rule = struct.unpack('<hh', original.read(address, 4))
            if exception < 0 or exception == token:
                break
            address += 4
    return 'reduce', rule

used_rules = set()
terminals = list(range(153)) + [256]
for state in range(376):
    rule, needs_token, first, count, first_exception, exception_count = struct.unpack(
        '<hBx4H', rebuilt.read(names['_parser_states'] + 12 * state, 12))
    expected_timing = (word(original, 0x6ff00 + 2 * state) > -1000 or
                       word(original, 0x70768 + 2 * state) == -2)
    assert needs_token == expected_timing, ('lookahead timing', state)
    shifts = dict(struct.unpack('<Hh', rebuilt.read(names['_parser_shifts'] + 4 * i, 4))
                  for i in range(first, first + count))
    exceptions = dict(struct.unpack('<Hh', rebuilt.read(names['_parser_exceptions'] + 4 * i, 4))
                      for i in range(first_exception, first_exception + exception_count))
    for token in terminals:
        actual = ('shift', shifts[token]) if token in shifts else ('reduce', exceptions.get(token, rule))
        expected = original_action(state, token)
        assert actual == expected, ('parser action', state, token, expected, actual)
        if expected[0] == 'reduce' and expected[1] > 0:
            used_rules.add(expected[1])

# Derive the required bound from all actions, rather than an assumed rule count.
production_count = int(re.search(r'#define PARSER_PRODUCTION_COUNT (\d+)',
                                 Path('src/parser.h').read_text())[1])
assert max(used_rules) < production_count, 'parser action references an absent production'
nonterminals = set()
for rule in range(production_count):
    actual = tuple(rebuilt.read(names['_parser_productions'] + 2 * rule, 2))
    expected = byte(original, 0x702a8 + rule), byte(original, 0x70390 + rule)
    assert actual == expected, ('production', rule, expected, actual)
    if rule:
        nonterminals.add(actual[0])

for nonterminal in nonterminals:
    base = word(original, 0x701f0 + 2 * nonterminal)
    default, first, count = struct.unpack('<hHH', rebuilt.read(
        names['_parser_gotos'] + 6 * nonterminal, 6))
    transitions = dict(struct.unpack('<Hh', rebuilt.read(
        names['_parser_goto_transitions'] + 4 * i, 4)) for i in range(first, first + count))
    for state in range(376):
        index = base + state + 1
        destination = word(original, 0x6f578 + 2 * index) if index < 1219 else -1
        if index < 1219 and word(original, 0x70478 + 2 * destination) == -nonterminal:
            expected = destination
        else:
            expected = word(original, 0x6f578 + 2 * base)
        actual = transitions.get(state, default)
        assert actual == expected, ('goto', nonterminal, state, expected, actual)

print('376 states: %d actions/timings, %d productions, and %d gotos match.' %
      (376 * len(terminals), production_count, 376 * len(nonterminals)))
