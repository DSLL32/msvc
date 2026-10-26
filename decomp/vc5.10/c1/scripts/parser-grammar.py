#!/usr/bin/env python3
"""Decode this C1 parser's yacc compression into logical grammar relations.

The input is the authenticated VC5.10 C frontend, not another compiler's
grammar. Image positions and compression indexes stay in this generator and
optional scratch provenance; generated C contains only grammar identities.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct


IMAGE_SHA256 = "7f12a4a889c5a0277f12c391ca462657dc81ef7be769faf629331bd117983d5d"
STATE_COUNT = 384
PRODUCTION_COUNT = 230
NONTERMINAL_COUNT = 92
TOKEN_KIND_COUNT = 245
ACTION_COUNT = 1198
ERROR_SYMBOL = 256

# These names follow the current keyword declarations. Shared classes without
# an established semantic name deliberately retain their grammar numbers.
SYMBOL_NAMES = {
    -1: "PARSER_NO_TOKEN", 44: "PARSER_BASED",
    98: "PARSER_BREAK", 99: "PARSER_CASE", 102: "PARSER_CONTINUE",
    103: "PARSER_DEFAULT", 105: "PARSER_DO", 107: "PARSER_ELSE",
    111: "PARSER_FOR", 113: "PARSER_GOTO", 114: "PARSER_IF",
    120: "PARSER_CALLING_CONVENTION", 125: "PARSER_RETURN",
    129: "PARSER_SIZEOF", 132: "PARSER_SWITCH", 135: "PARSER_WHILE",
    167: "PARSER_TRY", 168: "PARSER_EXCEPT", 169: "PARSER_FINALLY",
    180: "PARSER_ALIGNOF", 181: "PARSER_LEAVE", 256: "PARSER_ERROR",
}


def symbol_name(symbol):
    return SYMBOL_NAMES.get(symbol, "PARSER_CLASS_%d" % symbol)


def signed_short(value):
    return (value + 32768) % 65536 - 32768


class Grammar:
    def __init__(self, image):
        self.image = image
        if hashlib.sha256(image).hexdigest() != IMAGE_SHA256:
            raise ValueError("input is not the qualified VC5.10 C1 image")
        self.reads = []
        self.action = self.read(0x61330, "h", ACTION_COUNT)
        self.shift_base = self.read(0x61C90, "h", STATE_COUNT)
        self.goto_base = self.read(0x61F90, "h", NONTERMINAL_COUNT)
        self.lhs = self.read(0x62048, "B", PRODUCTION_COUNT + 1)
        self.length = self.read(0x62130, "B", PRODUCTION_COUNT + 1)
        self.check = self.read(0x62218, "h", STATE_COUNT)
        self.default = self.read(0x62518, "h", STATE_COUNT)
        self.token_classes = self.read(0x63C78, "B", TOKEN_KIND_COUNT)
        if any(not 0 <= value < STATE_COUNT for value in self.action):
            raise ValueError("action destination outside state domain")
        if any(not 0 <= value < NONTERMINAL_COUNT for value in self.lhs):
            raise ValueError("production LHS outside nonterminal domain")
        if any(not -2 <= value <= PRODUCTION_COUNT for value in self.default):
            raise ValueError("default production outside grammar domain")
        self.terminals = sorted(set(self.token_classes) | {-1, ERROR_SYMBOL})
        self.shifts, self.states, self.exceptions = [], [], []
        self.shift_provenance, self.exception_provenance = [], []
        for state in range(STATE_COUNT):
            first_shift = len(self.shifts)
            # Recovery also searches an ERROR shift, including in a state
            # whose ordinary action is an unconditional reduction.
            for symbol in self.terminals:
                destination = self.shift(state, symbol)
                if destination is not None:
                    self.shifts.append((symbol, destination))
                    self.shift_provenance.append({
                        "state": state, "terminal": symbol,
                        "destination": destination,
                        "action_index": signed_short(self.shift_base[state] + symbol),
                    })
            first_exception = len(self.exceptions)
            default = self.default[state]
            if default == -2:
                # Only these seven state-specific bytes are read by Run;
                # this is not a 384-byte snapshot of the offset storage.
                offset = self.read(0x61238 + state, "B", 1)[0]
                for pair in range(16):
                    symbol, production = self.read(0x611F8 + offset * 2 + pair * 4, "h", 2)
                    if not -1 <= production <= PRODUCTION_COUNT:
                        raise ValueError("exception production outside grammar domain")
                    self.exception_provenance.append({
                        "state": state, "symbol": symbol,
                        "production": production, "pair_index": offset // 2 + pair,
                    })
                    if symbol < 0:
                        default = production
                        break
                    if symbol not in self.terminals:
                        raise ValueError("exception terminal outside lexical domain")
                    self.exceptions.append((symbol, production))
                else:
                    raise ValueError("unterminated exception list")
            self.states.append((default, int(self.shift_base[state] > -1000),
                                first_shift, len(self.shifts) - first_shift,
                                first_exception, len(self.exceptions) - first_exception))
        self.gotos, self.goto_transitions, self.goto_provenance = [], [], []
        for nonterminal in range(NONTERMINAL_COUNT):
            base = self.goto_base[nonterminal]
            if not 0 <= base < ACTION_COUNT:
                raise ValueError("default goto index outside action domain")
            default = self.action[base]
            first = len(self.goto_transitions)
            for state in range(STATE_COUNT):
                index = signed_short(base + state + 1)
                if index < 0:
                    raise ValueError("negative goto index needs separate qualification")
                if index < ACTION_COUNT and self.check[self.action[index]] == -nonterminal:
                    destination = self.action[index]
                    if destination != default:
                        self.goto_transitions.append((state, destination))
                        self.goto_provenance.append({
                            "nonterminal": nonterminal, "state": state,
                            "destination": destination, "action_index": index,
                        })
            self.gotos.append((default, first, len(self.goto_transitions) - first))
        self.recoveries = []
        for row in range(47):
            state = self.read(0x62818 + row * 6, "h", 1)[0]
            if state == -1000:
                break
            symbol, destination = self.read(0x6281A + row * 6, "h", 2)
            if not 0 <= state < STATE_COUNT or not 0 <= destination < STATE_COUNT:
                raise ValueError("recovery state outside state domain")
            if abs(symbol) not in self.terminals:
                raise ValueError("recovery terminal outside lexical domain")
            if self.recoveries and state <= self.recoveries[-1][0]:
                raise ValueError("recovery rows are not strictly ordered")
            self.recoveries.append((state, symbol, destination))
        else:
            raise ValueError("unterminated recovery list")

    def read(self, rva, kind, count):
        size = struct.calcsize("<" + kind) * count
        # Fixed authenticated .rdatafe section. No other image data is input.
        if not 0x60000 <= rva or rva + size > 0x684A1:
            raise ValueError("grammar read outside qualified data section")
        offset = rva - 0x60000 + 0x52200
        self.reads.append({"rva": rva, "bytes": size})
        return struct.unpack_from("<%d%s" % (count, kind), self.image, offset)

    def shift(self, state, symbol):
        index = signed_short(self.shift_base[state] + symbol)
        if 0 <= index < ACTION_COUNT:
            destination = self.action[index]
            if self.check[destination] == symbol:
                return destination
        return None

    def verify(self):
        # One-time decoding verification over every modeled relation. The
        # emitted logical representation must reproduce the authentic lookup,
        # including each fallback and the signed 16-bit index arithmetic.
        for state, row in enumerate(self.states):
            logical = dict(self.shifts[row[2]:row[2] + row[3]])
            exceptions = dict(self.exceptions[row[4]:row[4] + row[5]])
            for terminal in self.terminals:
                if logical.get(terminal) != self.shift(state, terminal):
                    raise ValueError("logical shift differs from original lookup")
                # Independently walk the original exception encoding instead
                # of deriving expected reductions from the decoded rows.
                expected = self.default[state]
                if expected == -2:
                    offset = self.image[0x61238 + state - 0x60000 + 0x52200]
                    cursor = 0x611F8 + offset * 2 - 0x60000 + 0x52200
                    while True:
                        symbol, expected = struct.unpack_from("<hh", self.image, cursor)
                        if symbol < 0 or symbol == terminal:
                            break
                        cursor += 4
                if exceptions.get(terminal, row[0]) != expected:
                    raise ValueError("logical reduction differs from original lookup")
        for nonterminal, row in enumerate(self.gotos):
            logical = dict(self.goto_transitions[row[1]:row[1] + row[2]])
            for state in range(STATE_COUNT):
                index = signed_short(self.goto_base[nonterminal] + state + 1)
                expected = self.action[self.goto_base[nonterminal]]
                if index < ACTION_COUNT and self.check[self.action[index]] == -nonterminal:
                    expected = self.action[index]
                if logical.get(state, row[0]) != expected:
                    raise ValueError("logical goto differs from original lookup")

    def header(self):
        lines = ["#ifndef C1_PARSER_GRAMMAR_H", "#define C1_PARSER_GRAMMAR_H",
                 '#include "c1.h"', "#include <recon.h>", "",
                 "/* Logical grammar relations; unknown classes retain their IDs. */",
                 "enum ParserTerminal {"]
        lines += ["    %s = %d," % (symbol_name(value), value) for value in self.terminals]
        lines += ["};", "struct ParserTransition { short symbol, destination; };",
                  "struct ParserState {", "    short default_production;",
                  "    BYTE needs_token;", "    WORD first_shift, shift_count;",
                  "    WORD first_exception, exception_count;", "};",
                  "struct ParserGoto { short default_state; WORD first_transition, transition_count; };",
                  "struct ParserProduction { BYTE nonterminal, length; };",
                  "struct ParserRecovery { short state, terminal, destination; };", "",
                  "#define PARSER_STATE_COUNT 384", "#define PARSER_PRODUCTION_COUNT 230",
                  "#define PARSER_ACTION_CASE_COUNT 228", "#define PARSER_NONTERMINAL_COUNT 92",
                  "#define PARSER_TOKEN_KIND_COUNT 245",
                  "extern const BYTE parser_token_classes[PARSER_TOKEN_KIND_COUNT];",
                  "extern const ParserTransition parser_shifts[];",
                  "extern const ParserTransition parser_exceptions[];",
                  "extern const ParserState parser_states[PARSER_STATE_COUNT];",
                  "extern const ParserGoto parser_gotos[PARSER_NONTERMINAL_COUNT];",
                  "extern const ParserTransition parser_goto_transitions[];",
                  "extern const ParserProduction parser_productions[PARSER_PRODUCTION_COUNT + 1];",
                  "extern const ParserRecovery parser_recoveries[%d];" % len(self.recoveries),
                  "#endif", ""]
        return "\n".join(lines)

    def source(self):
        lines = ["/* Target-derived C grammar: logical relations, not yacc compression.",
                 " * Regenerate with scripts/parser-grammar.py. State, production and",
                 " * unknown nonterminal identities remain numbered until recovered. */",
                 '#include "parser-grammar.h"', "#include <recon.h>", "",
                 "const BYTE parser_token_classes[PARSER_TOKEN_KIND_COUNT] = {"]
        lines += ["    %s, /* token kind %d */" % (symbol_name(value), kind)
                  for kind, value in enumerate(self.token_classes)]
        lines += ["};", "", "const ParserTransition parser_shifts[] = {"]
        for state, row in enumerate(self.states):
            if row[3]:
                lines.append("    /* State %d. */" % state)
                lines += ["    { %s, %d }," % (symbol_name(symbol), destination)
                          for symbol, destination in self.shifts[row[2]:row[2] + row[3]]]
        lines += ["};", "", "const ParserTransition parser_exceptions[] = {"]
        lines += ["    { %s, %d }," % (symbol_name(symbol), production)
                  for symbol, production in self.exceptions]
        lines += ["};", "", "const ParserState parser_states[PARSER_STATE_COUNT] = {"]
        lines += ["    { %d, %d, %d, %d, %d, %d }, /* state %d */" % (*row, state)
                  for state, row in enumerate(self.states)]
        lines += ["};", "", "const ParserGoto parser_gotos[PARSER_NONTERMINAL_COUNT] = {"]
        lines += ["    { %d, %d, %d }, /* nonterminal %d */" % (*row, nonterminal)
                  for nonterminal, row in enumerate(self.gotos)]
        lines += ["};", "", "const ParserTransition parser_goto_transitions[] = {"]
        for nonterminal, row in enumerate(self.gotos):
            if row[2]:
                lines.append("    /* Nonterminal %d: source state, destination. */" % nonterminal)
                lines += ["    { %d, %d }," % relation
                          for relation in self.goto_transitions[row[1]:row[1] + row[2]]]
        lines += ["};", "", "const ParserProduction parser_productions[PARSER_PRODUCTION_COUNT + 1] = {"]
        lines += ["    { %d, %d }, /* production %d */" % (self.lhs[rule], self.length[rule], rule)
                  for rule in range(PRODUCTION_COUNT + 1)]
        lines += ["};", "", "const ParserRecovery parser_recoveries[%d] = {" % len(self.recoveries)]
        for state, symbol, destination in self.recoveries:
            terminal = symbol_name(abs(symbol))
            if symbol < 0:
                terminal = "-" + terminal
            lines.append("    { %d, %s, %d }," % (state, terminal, destination))
        return "\n".join(lines + ["};", ""])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", type=Path)
    parser.add_argument("--header", required=True, type=Path)
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--provenance", type=Path, help="optional scratch-only lookup provenance")
    args = parser.parse_args()
    grammar = Grammar(args.image.read_bytes())
    grammar.verify()
    args.header.write_text(grammar.header())
    args.source.write_text(grammar.source())
    if args.provenance:
        args.provenance.write_text(json.dumps({
            "image_sha256": IMAGE_SHA256, "reads": grammar.reads,
            "shifts": grammar.shift_provenance, "gotos": grammar.goto_provenance,
            "exceptions": grammar.exception_provenance,
            "states": STATE_COUNT, "productions": PRODUCTION_COUNT,
            "semantic_action_cases": 228, "recovery_rows": len(grammar.recoveries),
        }, indent=2) + "\n")
    print("decoded %d shifts, %d goto exceptions, %d reduction exceptions, %d recovery rows" %
          (len(grammar.shifts), len(grammar.goto_transitions), len(grammar.exceptions),
           len(grammar.recoveries)))


if __name__ == "__main__":
    main()
