#!/usr/bin/env python3
"""Original compiler graph/ABI/extents, contextual data and runtime provenance."""
import argparse
import json
from pathlib import Path
import re
import subprocess
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from binary import PE,u32
from graph import graph,names
from runtime import identify
from state import BINARY,ROOT,load

def publics():
    result={}
    for line in (BINARY/'build/cvpack.map').read_text().splitlines():
        match=re.match(r'\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',line)
        if match:result[match[1]]=int(match[2],16)
    return result

def identity_table(runtime,functions,linked=None):
    identities={}
    by_address={}
    for record in runtime['functions']+runtime['imports']:
        by_address.setdefault(record['start'],set()).add(record['name'])
    for address,aliases in by_address.items():identities[address]='|'.join(sorted(aliases))
    for record in functions:
        address=linked[record['symbol']] if linked else record['original']
        identities[address]=record['name']
    return identities

def seh_roots(pe,start,end):
    """Decode the compiler's SEH scope table, not arbitrary data as code roots."""
    roots=[]
    decoder=Cs(CS_ARCH_X86,CS_MODE_32)
    for ins in decoder.disasm(pe.read(start,min(end-start,96)),start):
        if ins.mnemonic!='mov' or not ins.op_str.startswith('dword ptr [ebp - '):continue
        literal=re.search(r', (0x[0-9a-f]+)$',ins.op_str)
        if not literal:continue
        table=int(literal[1],16)
        try:
            section=next(s for s in pe.sections if s['rva']<=table-pe.base<s['rva']+s['virtual_size'])
        except StopIteration:continue
        if section['name']!='.rdata':continue
        for index in range(16):
            previous,filter,handler=(u32(pe.read(table+index*12+off,4),0) for off in (0,4,8))
            if previous!=0xffffffff and previous>=index:break
            valid=False
            for address in (filter,handler):
                if start<=address<end:roots.append(address);valid=True
            if not valid:break
    return sorted(set(roots))

def audited_graph(pe,start,end,roots=(),data=(),switches=None):
    result=graph(pe,start,end,data=data,roots=roots,switches=switches)
    decoder=Cs(CS_ARCH_X86,CS_MODE_32)
    covered=set()
    for address,instruction in result['instructions'].items():covered.update(range(address,address+instruction.size))
    cursor=start
    while cursor<end:
        interval=next(((a,b) for a,b in data if a<=cursor<b),None)
        if interval:cursor=interval[1];continue
        item=list(decoder.disasm(pe.read(cursor,min(15,end-cursor)),cursor,count=1))
        assert item,'invalid/truncated whole-function instruction at %x'%cursor
        ins=item[0]
        assert cursor+ins.size<=end,'partial final instruction'
        if cursor not in covered:
            assert ins.bytes.hex() in ('90','8bff','8d4900','8d642400','8da42400000000','8d9b00000000','cc'),('unrooted code',hex(cursor),ins.mnemonic,ins.op_str)
        cursor+=ins.size
    return result

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--canonical-only',action='store_true')
    parser.add_argument('--rebuilt',type=Path,default=BINARY/'build/CVPACK.EXE')
    args=parser.parse_args()
    config,original=load()
    linked=publics()
    rebuilt=PE(args.rebuilt)
    assert original.base+original.entry==config['target']['entry']
    functions=config['functions']
    assert len({r['name'] for r in functions})==len(functions)
    assert len({r['original'] for r in functions})==len(functions)
    runtime=identify(ROOT/config['target']['path'],{'_main':config['target']['application_main']})
    cached=json.loads((BINARY/'runtime.json').read_text())
    assert runtime==cached,'strict runtime identities changed'
    rebuilt_runtime=identify(args.rebuilt,{'_main':linked['_main']})
    oi=identity_table(runtime,functions)
    ri=identity_table(rebuilt_runtime,functions,linked)
    if not args.canonical_only:
        objects=[BINARY/'build/native'/(p.stem+'.o') for p in sorted((BINARY/'src').glob('*.c')) if p.name!='debug-run.c']
        source_symbols=[]
        for line in subprocess.check_output(['nm','--defined-only','--format=posix',*map(str,objects)],text=True).splitlines():
            parts=line.split()
            if len(parts)>=2 and parts[1] in ('T','t'):source_symbols.append(parts[0])
        assert len(source_symbols)==len(set(source_symbols))
        assert set(source_symbols)=={r['native'] for r in functions},source_symbols
    text_end=max(rebuilt.base+s['rva']+s['virtual_size'] for s in rebuilt.sections if s['flags']&0x20)
    addresses=sorted(set(linked.values())|{text_end})
    checked=0
    for record in functions:
        assert record['status'] in ('stub','candidate','accepted')
        assert record['kind']=='application'
        source=BINARY/record['source']
        text=source.read_text()
        assert '#include <recon.h>' in text
        if record['status']=='stub':
            assert re.search(r'\b'+re.escape(record['native'])+r'\s*\([^{}]*\)\s*\{\s*__asm\s*\{\s*int 3\s*\}',text),'unaccepted function must trap: '+record['name']
            continue
        if record['status']=='accepted':
            assert record['evidence'] and (BINARY/record['evidence']).is_file()
        assert record['extent'][0]==record['original']
        start,end=record['extent']
        switches={int(a):targets for a,targets in record.get('switches',{}).items()}
        for table in record.get('switch_tables',[]):
            actual=[u32(original.read(table['start']+i*4,4),0) for i in range(len(table['targets']))]
            assert actual==table['targets'],'switch targets differ'
        a=audited_graph(original,start,end,record.get('roots',()),record.get('text_data',()),switches)
        rebuilt_start=linked[record['symbol']]
        rebuilt_end=next(address for address in addresses if address>rebuilt_start)
        roots=seh_roots(rebuilt,rebuilt_start,rebuilt_end)
        roots+= [rebuilt_start+offset for offset in record.get('rebuilt_extra_roots',[])]
        rebuilt_switches={int(address):targets for address,targets in record.get('rebuilt_switches',{}).items()}
        b=audited_graph(rebuilt,rebuilt_start,rebuilt_end,roots,record.get('rebuilt_text_data',()),rebuilt_switches)
        left,right=names(a['edges'],oi),names(b['edges'],ri)
        assert left==right,(record['name'],'original only',left-right,'rebuilt only',right-left)
        assert a['returns']==b['returns']=={record['stack_cleanup']},('ABI RET cleanup differs',record['name'],a['returns'],b['returns'])
        assert record['abi'],record['name']
        checked+=1
    for record in json.loads((BINARY/'data.json').read_text()):
        assert original.read(record['original'],record['size'])==rebuilt.read(linked[record['symbol']],record['size']),('initial data differs',record['name'])
    for record in config.get('support_unmasked',[]):
        found=[r for r in runtime['functions']+runtime['imports'] if r['start']==record['start'] and r['end']==record['end'] and r['name']==record['name']]
        assert found,('unproven CRT/import unmask',record)
        assert (BINARY/record['evidence']).is_file()
    print('%d complete original/compiler CFG/callee/ABI functions; %d contextual data objects; strict CRT/import identities verified.'%(checked,len(json.loads((BINARY/'data.json').read_text()))))

if __name__=='__main__':main()
