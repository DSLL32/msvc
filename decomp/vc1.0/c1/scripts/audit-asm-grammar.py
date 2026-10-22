#!/usr/bin/env python3
"""Audit complete assembler actions/defaults/timing/gotos against original PE data."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from binary import PE
from grammar import read_grammar, generate, action_at, goto_at
from images import publics

BINARY = Path(__file__).resolve().parents[1]
ROOT = BINARY.parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--model-only', action='store_true')
    options = parser.parse_args()
    config = json.loads((BINARY/'functions.json').read_text())
    original = PE(ROOT/'toolchains/1.0-nt/bin/C13232.EXE')
    assert hashlib.sha256(original.data).hexdigest() == config['target']['sha256']
    word = lambda a:struct.unpack('<h',original.read(a,2))[0]
    g = read_grammar(BINARY/'src/asm-grammar.y')
    m = generate(g)
    assert len(m.states) == 136
    rebuilt = None
    if not options.model_only:
        rebuilt = PE(BINARY/'build/c1.exe')
        names = publics()
    checked = 0
    for state in range(136):
        base = word(0x70c60+2*state)
        default = word(0x70f60+2*state)
        timing = base > -1000 or default == -2
        assert m.needs_token[state] == timing, ('lookahead timing',state)
        if default >= 0:
            assert m.defaults[state] == default, ('default reduction',state)
        if rebuilt:
            rule,needs,fs,ns,fe,ne = struct.unpack('<hBx4H',rebuilt.read(names['_asm_states']+12*state,12))
            shifts = dict(struct.unpack('<Hh',rebuilt.read(names['_asm_shifts']+4*i,4)) for i in range(fs,fs+ns))
            exceptions = dict(struct.unpack('<Hh',rebuilt.read(names['_asm_reductions']+4*i,4)) for i in range(fe,fe+ne))
            assert rule==m.defaults[state] and bool(needs)==timing
        for token in list(range(50))+[256]:
            target = -1
            if base > -1000 and 0 <= base+token < 260:
                candidate = word(0x70a58+2*(base+token))
                if word(0x70e50+2*candidate) == token:
                    target = candidate
            if target >= 0:
                expected = ('shift',target)
            else:
                reduction = default
                if reduction == -2:
                    a = 0x710b8+2*original.read(0x710e8+state,1)[0]
                    while True:
                        item,reduction = struct.unpack('<hh',original.read(a,4))
                        if item < 0 or item==token:
                            break
                        a += 4
                expected = ('reduce',reduction) if reduction>0 else (('accept',0) if reduction<0 else ('error',0))
            assert action_at(m,state,token)==expected, ('model action',state,token,expected,action_at(m,state,token))
            if rebuilt:
                r = exceptions.get(token,rule)
                actual = ('shift',shifts[token]) if token in shifts else (('reduce',r) if r>0 else (('accept',0) if r<0 else ('error',0)))
                assert actual==expected, ('emitted action',state,token)
            checked += 1
    for rule in range(83):
        expected = tuple(original.read(0x70da0+rule,1)+original.read(0x70df8+rule,1))
        p = g.productions[rule]
        actual = (p.lhs,len(p.rhs)) if rule else (0,0)
        assert expected==actual, ('production',rule)
        if rebuilt:
            assert tuple(rebuilt.read(names['_asm_productions']+2*rule,2))==expected
    gotos = 0
    for nt in range(1,24):
        base = word(0x70d70+2*nt)
        if rebuilt:
            fallback,first,count = struct.unpack('<hHH',rebuilt.read(names['_asm_gotos']+6*nt,6))
            row = dict(struct.unpack('<Hh',rebuilt.read(names['_asm_goto_rows']+4*i,4)) for i in range(first,first+count))
        for state in range(136):
            index = base+state+1
            target = word(0x70a58+2*index) if 0<=index<260 else -1
            if target<0 or word(0x70e50+2*target)!=-nt:
                target = word(0x70a58+2*base)
            assert goto_at(m,state,nt)==target, ('model goto',state,nt)
            if rebuilt:
                assert row.get(state,fallback)==target, ('emitted goto',state,nt)
            gotos += 1
    if rebuilt:
        token_classes = rebuilt.read(names['_asm_token_classes'],153)
        assert token_classes == bytes(value & 63 for value in original.read(0x6d92f,153)), 'assembler lexical classes'
        assert rebuilt.read(names['_asm_opcode_aliases'],196) == original.read(0x72cb4+230*2,196), 'assembler opcode aliases'
        for index in range(330):
            old_text = struct.unpack('<I',original.read(0x71cb0+4*index,4))[0]
            old_id = struct.unpack('<h',original.read(0x72218+4*index,2))[0]
            new_text,new_id = struct.unpack('<Ih',rebuilt.read(names['_asm_mnemonics']+8*index,6))
            assert original.string(old_text)==rebuilt.string(new_text), ('mnemonic spelling/order',index)
            assert old_id==new_id, ('mnemonic ID',index)
        for letter in range(95,123):
            assert original.read(0x7211a+letter*2,4)==rebuilt.read(names['_asm_mnemonic_groups']+(letter-95)*4,4), ('mnemonic group',letter)
        for index in range(76):
            old_text = struct.unpack('<I',original.read(0x71278+4*index,4))[0]
            old_id = struct.unpack('<h',original.read(0x713e8+4*index,2))[0]
            new_text,new_id = struct.unpack('<Ih',rebuilt.read(names['_asm_keywords']+8*index,6))
            assert original.string(old_text)==rebuilt.string(new_text), ('operand keyword spelling/order',index)
            assert old_id==new_id, ('operand keyword ID',index)
        for letter in range(95,123):
            assert original.read(0x712ea+letter*2,4)==rebuilt.read(names['_asm_keyword_groups']+(letter-95)*4,4), ('operand keyword group',letter)
        print('76 register/operator/qualifier rows and 28 groups match.')
        assert original.read(0x731e0,92)==rebuilt.read(names['_asm_register_properties'],92), 'register properties'
        print('46 register encodings, widths and addressing properties match.')
        assert original.read(0x73168,120)==rebuilt.read(names['_asm_operand_class_rules'],120), 'operand class rules'
        for flags in range(32):
            mode = original.read(0x73250+flags,1)[0]
            category = original.read(0x73240+mode,1)[0]
            assert rebuilt.read(names['_asm_address_rules']+flags*2,2)==bytes([mode,category]), ('address classification',flags)
        print('60 operand-class rules and 32 address flag/mode/category mappings match.')
        maximum_operand = 0
        for mode in range(21):
            old_text = struct.unpack('<I',original.read(0x732e0+4*mode,4))[0]
            new_text = struct.unpack('<I',rebuilt.read(names['_asm_operand_formats']+4*mode,4))[0]
            assert original.string(old_text)==rebuilt.string(new_text), ('operand format',mode)
            maximum_operand=max(maximum_operand,sum({'s':4,'d':4,'o':4,'v':4}.get(c,1 if c in 'mrbigx' else 0) for c in original.string(old_text)))
        assert maximum_operand==13 and 3+3*(maximum_operand+12)==78 and 78<=127<128
        print('21 logical operand serialization formats match; maximum instruction78 bytes fits128-byte workspace.')
        for opcode in range(230):
            assert original.read(0x72740+8*opcode,5)==rebuilt.read(names['_asm_instruction_properties']+8*opcode,5), ('instruction properties',opcode)
        print('230 instruction operand classes and processor masks match.')
        print('153 assembler lexical classes, 98 instruction aliases and 330 mnemonic rows/28 groups match.')
    print('136 assembler states: %d complete actions/errors, 136 lookahead timings/defaults, 83 production records and %d gotos match%s.' % (checked,gotos,' (model)'if options.model_only else ' (model and emitted C)'))


if __name__ == '__main__':
    main()
