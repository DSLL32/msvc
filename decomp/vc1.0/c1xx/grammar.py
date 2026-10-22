"""Generate the C++ LALR automaton from named grammar and precedence rules.

This module does not read the original image. Kernel sorting and queued closure
retain the original yacc state identities while the grammar remains readable.
"""
from collections import Counter, defaultdict, deque
from dataclasses import dataclass, field
from pathlib import Path


@dataclass
class Production:
    lhs: int
    rhs: tuple
    precedence: str | None = None
    label: str = ''


@dataclass
class Grammar:
    terminals: dict = field(default_factory=dict)
    nonterminals: dict = field(default_factory=dict)
    productions: list = field(default_factory=list)
    precedence: dict = field(default_factory=dict)
    recovery: list = field(default_factory=list)
    start: str = ''


def read_grammar(path):
    grammar = Grammar()
    pending = []
    precedence_level = 0
    occurrences = Counter()
    for line_number, line in enumerate(Path(path).read_text().splitlines(), 1):
        text = line.partition('#')[0].strip()
        if not text:
            continue
        words = text.split()
        if words[0] == '%start':
            grammar.start = words[1]
        elif words[0] == '%token':
            grammar.terminals[words[1]] = int(words[2])
        elif words[0] == '%nonterminal':
            grammar.nonterminals[words[1]] = int(words[2])
        elif words[0] in ('%left', '%right', '%nonassoc'):
            precedence_level += 1
            for name in words[1:]:
                grammar.precedence[name] = (precedence_level, words[0][1:])
        elif words[0] == '%recover':
            grammar.recovery.append(words[1:])
        else:
            lhs, separator, rhs = text.partition(':')
            assert separator and rhs.rstrip().endswith(';'), ('grammar syntax', line_number)
            symbols = rhs.rstrip()[:-1].split()
            precedence = None
            if '%prec' in symbols:
                position = symbols.index('%prec')
                assert position == len(symbols)-2, ('rule precedence', line_number)
                precedence = symbols[position+1]
                symbols = symbols[:position]
            lhs = lhs.strip()
            occurrences[lhs] += 1
            pending.append((lhs, symbols, precedence, '%s[%d]' % (lhs, occurrences[lhs])))
    grammar.productions = [Production(0, (-grammar.nonterminals[grammar.start],), label='$accept')]
    symbols = dict(grammar.terminals)
    symbols.update((name, -value) for name, value in grammar.nonterminals.items())
    for lhs, rhs, precedence, label in pending:
        grammar.productions.append(Production(grammar.nonterminals[lhs],
                                             tuple(symbols[name] for name in rhs), precedence, label))
    assert len(symbols) == len(grammar.terminals)+len(grammar.nonterminals)
    return grammar


@dataclass
class Automaton:
    grammar: Grammar
    kernels: list
    states: list
    transitions: dict
    actions: dict
    defaults: list
    exceptions: list
    needs_token: list
    goto_defaults: dict
    recoveries: list
    conflicts: int


def generate(grammar):
    productions = grammar.productions
    by_lhs = defaultdict(list)
    for rule, production in enumerate(productions):
        by_lhs[-production.lhs].append(rule)

    def closure(kernel):
        # Original yacc sorts a kernel before expanding nonterminals in
        # encounter order; sorting the completed closure changes state IDs.
        result = sorted(kernel)
        present = set(result)
        cursor = 0
        while cursor < len(result):
            rule, position = result[cursor]
            cursor += 1
            rhs = productions[rule].rhs
            if position < len(rhs) and rhs[position] < 0:
                for child in by_lhs[rhs[position]]:
                    item = child, 0
                    if item not in present:
                        present.add(item)
                        result.append(item)
        return result

    kernels = [((0, 0),)]
    identities = {frozenset(kernels[0]): 0}
    states, transitions = [], {}
    for state, kernel in enumerate(kernels):
        expanded = closure(kernel)
        states.append(frozenset(expanded))
        groups = {}
        for rule, position in expanded:
            rhs = productions[rule].rhs
            if position < len(rhs):
                groups.setdefault(rhs[position], []).append((rule, position+1))
        for symbol, advanced in groups.items():
            identity = frozenset(advanced)
            if identity not in identities:
                identities[identity] = len(kernels)
                kernels.append(tuple(advanced))
            transitions[state, symbol] = identities[identity]

    nullable, first = set(), defaultdict(int)
    changed = True
    while changed:
        changed = False
        for production in productions:
            symbol = -production.lhs
            terminals, empty = 0, True
            for child in production.rhs:
                if child >= 0:
                    terminals |= 1 << child
                    empty = False
                    break
                terminals |= first[child]
                if child not in nullable:
                    empty = False
                    break
            if terminals & ~first[symbol]:
                first[symbol] |= terminals
                changed = True
            if empty and symbol not in nullable:
                nullable.add(symbol)
                changed = True

    def tail_first(symbols):
        terminals = 0
        for symbol in symbols:
            if symbol >= 0:
                return terminals | (1 << symbol), False
            terminals |= first[symbol]
            if symbol not in nullable:
                return terminals, False
        return terminals, True

    propagate, spontaneous = defaultdict(set), defaultdict(dict)
    for state, items in enumerate(states):
        for rule, position in items:
            rhs = productions[rule].rhs
            if position == len(rhs):
                continue
            symbol = rhs[position]
            origin = state, rule, position
            propagate[origin].add((transitions[state, symbol], rule, position+1))
            if symbol < 0:
                terminals, empty = tail_first(rhs[position+1:])
                for child in by_lhs[symbol]:
                    target = state, child, 0
                    spontaneous[origin][target] = terminals
                    if empty:
                        propagate[origin].add(target)
    lookahead = defaultdict(int)
    lookahead[0, 0, 0] = 1 # EOF
    pending = deque([(0, 0, 0)])
    while pending:
        origin = pending.popleft()
        terminals = lookahead[origin]
        for target in propagate[origin]:
            if terminals & ~lookahead[target]:
                lookahead[target] |= terminals
                pending.append(target)
        for target, additions in spontaneous[origin].items():
            if additions & ~lookahead[target]:
                lookahead[target] |= additions
                pending.append(target)
    choices = defaultdict(set)
    for (state, symbol), target in transitions.items():
        if symbol >= 0:
            choices[state, symbol].add(('shift', target))
    for state, items in enumerate(states):
        for rule, position in items:
            if position != len(productions[rule].rhs):
                continue
            terminals = lookahead[state, rule, position]
            while terminals:
                bit = terminals & -terminals
                token = bit.bit_length()-1
                choices[state, token].add(('reduce', rule) if rule else ('accept', 0))
                terminals ^= bit

    precedence = {grammar.terminals[name]: value for name, value in grammar.precedence.items()}
    def production_precedence(rule):
        production = productions[rule]
        if production.precedence:
            return grammar.precedence[production.precedence]
        terminals = [symbol for symbol in production.rhs if symbol >= 0]
        return precedence.get(terminals[-1]) if terminals else None

    actions, conflicts = {}, 0
    for key, possibilities in choices.items():
        if len(possibilities) == 1:
            actions[key] = next(iter(possibilities))
            continue
        conflicts += 1
        reductions = sorted((choice for choice in possibilities if choice[0] == 'reduce'),
                            key=lambda choice: choice[1])
        shifts = [choice for choice in possibilities if choice[0] == 'shift']
        choice = reductions[0]
        if shifts:
            rule_precedence = production_precedence(choice[1])
            token_precedence = precedence.get(key[1])
            if not rule_precedence or not token_precedence or rule_precedence[0] < token_precedence[0]:
                choice = shifts[0]
            elif rule_precedence[0] == token_precedence[0]:
                if token_precedence[1] == 'right':
                    choice = shifts[0]
                elif token_precedence[1] == 'nonassoc':
                    choice = ('error', 0)
        actions[key] = choice

    defaults, exceptions, needs_token = [], [], []
    for state in range(len(states)):
        row = {token: action for (source, token), action in actions.items() if source == state}
        counts = Counter(rule for kind, rule in row.values() if kind == 'reduce')
        # The accepting state retains an error fallback. Other states use the
        # most frequent reduction, breaking ties in production order.
        default = max(counts, key=lambda rule: (counts[rule], -rule)) if counts else 0
        if any(kind == 'accept' for kind, rule in row.values()):
            default = 0
        exceptional = {token: action for token, action in row.items()
                       if action[0] != 'shift' and action != ('reduce', default)}
        defaults.append(default)
        exceptions.append(exceptional)
        needs_token.append(bool(exceptional) or any(kind == 'shift' for kind, rule in row.values()))
    goto_defaults = {}
    for nonterminal in grammar.nonterminals.values():
        counts = Counter(target for (source, symbol), target in transitions.items() if symbol == -nonterminal)
        goto_defaults[nonterminal] = max(counts, key=lambda target: (counts[target], -target))

    labels = {production.label: rule for rule, production in enumerate(productions)}
    recoveries = []
    for query, token_name, *flags in grammar.recovery:
        required, excluded = set(), set()
        for term in query.split(','):
            destination = excluded if term.startswith('!') else required
            term = term.lstrip('!')
            label, position = term.rsplit('@', 1)
            destination.add((labels[label], int(position)))
        candidates = [state for state, kernel in enumerate(kernels)
                      if required <= set(kernel) and not excluded.intersection(kernel)]
        assert len(candidates) == 1, ('recovery context is ambiguous', query, candidates)
        state, token = candidates[0], grammar.terminals[token_name]
        target = transitions[state, token]
        assert set(flags) <= {'deferred'}, ('unknown recovery flag', flags)
        recoveries.append((state, -token if 'deferred' in flags else token, target))
    recoveries.sort()
    return Automaton(grammar, kernels, states, transitions, actions, defaults,
                     exceptions, needs_token, goto_defaults, recoveries, conflicts)


def action_at(model, state, token):
    action = model.actions.get((state, token))
    if action and action[0] == 'shift':
        return action
    return model.exceptions[state].get(token, ('reduce', model.defaults[state])
                                       if model.defaults[state] else ('error', 0))


def goto_at(model, state, nonterminal):
    return model.transitions.get((state, -nonterminal), model.goto_defaults[nonterminal])
