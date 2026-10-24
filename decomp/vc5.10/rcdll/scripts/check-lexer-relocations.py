#!/usr/bin/env python3
"""Exact i386 linker binding residual for the frozen production-object proof.

No process execution, CFG discovery, replacement implementations or byte skips.
"""
import argparse,json,struct,time
from pathlib import Path
from protocol import sha
p=argparse.ArgumentParser();p.add_argument('--proof',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();started=time.perf_counter()
receipt=json.loads((a.proof/'receipt.json').read_text())
for name in ['lexer.o','production.elf','probe.elf']:assert sha(a.proof/name)==receipt['files'][name]
class Elf:
    def __init__(self,path):
        self.image=path.read_bytes();d=self.image;assert d[:6]==b'\x7fELF\x01\x01'
        at=struct.unpack_from('<I',d,32)[0];width,count,strings=struct.unpack_from('<HHH',d,46)
        self.sections=[struct.unpack_from('<10I',d,at+width*i)for i in range(count)];names=self.data(self.sections[strings])
        self.named={names[r[0]:names.index(b'\0',r[0])].decode():r for r in self.sections}
        table=self.named['.symtab'];names=self.data(self.sections[table[6]]);self.symbols=[]
        for at in range(table[4],table[4]+table[5],table[9]):
            name,value,size,info,other,section=struct.unpack_from('<IIIBBH',d,at)
            label=names[name:names.index(b'\0',name)].decode()
            if not label and info&15==3:label=next(k for k,v in self.named.items()if v==self.sections[section])
            self.symbols.append(dict(name=label,value=value,size=size,info=info,section=section))
    def data(self,section):return self.image[section[4]:section[4]+section[5]]
    def symbol(self,name):
        rows=[s for s in self.symbols if s['name']==name and s['section']!=0];assert len(rows)==1,name
        return rows[0]
    def span(self,address,size,readonly=False):
        rows=[s for s in self.sections if s[1]!=8 and s[2]&2 and s[3]<=address and address+size<=s[3]+s[5]];assert len(rows)==1
        section=rows[0]
        if readonly:assert not section[2]&1
        return self.image[section[4]+address-section[3]:section[4]+address-section[3]+size]
obj=Elf(a.proof/'lexer.o');text=obj.data(obj.named['.text']);table=obj.named['.rel.text'];relocations=[struct.unpack_from('<II',obj.image,at)for at in range(table[4],table[4]+table[5],8)]
ro_reloc=obj.named['.rel.rodata'];rodata=obj.data(obj.named['.rodata']);ro_rows=[struct.unpack_from('<II',obj.image,at)for at in range(ro_reloc[4],ro_reloc[4]+ro_reloc[5],8)]
assert len(ro_rows)*4==len(rodata)==176
def u32(value):return value&0xffffffff
def check_relocation(elf,at,info,override=None):
    function=elf.symbol('rc_next_token');base=function['value'];assert function['size']==len(text)==5194
    got=elf.symbol('_GLOBAL_OFFSET_TABLE_')['value'];symbol=obj.symbols[info>>8];kind=info&255
    addend=struct.unpack_from('<i',text,at)[0];P=base+at;actual=struct.unpack('<I',override or elf.span(P,4))[0]
    result=dict(offset=at,type=kind,symbol=symbol['name'],addend=addend,place=hex(P),linked_word=hex(actual))
    if kind in [4,10,43]or kind==9 and symbol['name']=='rc_next_token':
        S=elf.symbol(symbol['name'])['value']
        expected=u32(S+addend-P)if kind in [4,10]else u32(S+addend-got)
        assert actual==expected,'wrong named symbol/type/addend binding'
        result.update(symbol_address=hex(S),expected_word=hex(expected))
        if kind==43:
            before=text[at-2:at];after=elf.span(P-2,2)
            assert before[0]==0x8b and before[1]&0xc0==0x80 and after==b'\x8d'+before[1:],(hex(at),symbol['name'],before.hex(),after.hex())
            result['relaxation']='R_386_GOT32X exact MOV[base+disp32] to LEA[base+disp32], same actual ModRM/register, disp=S+A-GOT'
    elif kind==9 and symbol['name']=='.rodata':
        S=u32(got+actual-addend);data=elf.span(S,len(rodata),readonly=True)
        for offset,entry in ro_rows:
            assert entry&255==9 and obj.symbols[entry>>8]['info']&15==3
            owner=obj.sections[obj.symbols[entry>>8]['section']];assert owner==obj.named['.text']
            A=struct.unpack_from('<i',rodata,offset)[0];assert 0<=A<len(text)
            assert struct.unpack_from('<I',data,offset)[0]==u32(base+A-got),'wrong lexer jump-table target'
        result.update(symbol_address=hex(S),table_entries=len(ro_rows),table_binding='All44 R_386_GOTOFF .text+A entries equal this exact rc_next_token continuation minus GOT')
    elif kind==9 and symbol['name'].startswith('.L.str'):
        S=u32(got+actual-addend);section=obj.sections[symbol['section']];expected=obj.data(section)[symbol['value']:symbol['value']+symbol['size']]
        assert symbol['size']in [4,6]and elf.span(S,len(expected),readonly=True)==expected
        result.update(symbol_address=hex(S),literal_bytes=expected.hex(),literal_scope='Exact encountered UTF16 literal object; ordinary string merging permitted, no distinct address claim')
    else:raise AssertionError('unhandled actual object relocation')
    return result
images={name:Elf(a.proof/name)for name in ['production.elf','probe.elf']};bindings={name:[check_relocation(elf,at,info)for at,info in relocations]for name,elf in images.items()}
controls=[]
for symbol,wrong in [('rc_expanding_macro','rc_source_depth'),('rc_apstudio','rc_result'),('rc_read_identifier','rc_read_macro_identifier')]:
    at,info=next((at,info)for at,info in relocations if obj.symbols[info>>8]['name']==symbol)
    elf=images['production.elf'];P=elf.symbol('rc_next_token')['value']+at;A=struct.unpack_from('<i',text,at)[0];kind=info&255
    wrong_value=u32(elf.symbol(wrong)['value']+A-(P if kind==4 else elf.symbol('_GLOBAL_OFFSET_TABLE_')['value']))
    assert struct.pack('<I',wrong_value)!=elf.span(P,4)
    try:check_relocation(elf,at,info,struct.pack('<I',wrong_value))
    except AssertionError as error:assert str(error)=='wrong named symbol/type/addend binding'
    else:raise AssertionError('wrong linker identity passed')
    controls.append(dict(symbol=symbol,wrong_symbol=wrong,semantic_rejection='actual linked field fails original object symbol/type/addend formula'))
report=dict(status='pass',proof_receipt_sha256=sha(a.proof/'receipt.json'),bindings=bindings,negative_controls=controls,scope='Rechecks frozen object/ELFs only. Every actual349 text relocation is bound to exact symbol/type/addend on both sides; all44 local jump-table continuations and20 bytes of encountered merged UTF16 literal objects are checked. No blanket offset exclusion or target/probe execution.',checker_sha256=sha(Path(__file__)),seconds=time.perf_counter()-started)
a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(status='pass',relocations_per_elf=len(relocations),controls=len(controls),seconds=report['seconds'])))
