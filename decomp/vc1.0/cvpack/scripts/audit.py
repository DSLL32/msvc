#!/usr/bin/env python3
"""Original compiler graph/ABI/extents, contextual data and runtime provenance."""
import argparse
import json
from pathlib import Path
import re
import subprocess
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from capstone.x86 import X86_OP_MEM,X86_OP_IMM
from binary import PE,u32
from graph import graph,names
from runtime import identify
from state import BINARY,ROOT,load

def validate_ownership(config,pe,runtime,context):
    """Every restored byte has one declared owner in file-backed original code."""
    def code_range(start,end):
        assert isinstance(start,int) and isinstance(end,int) and start<end,('invalid code range',start,end)
        owners=[s for s in pe.sections if s['flags']&0x20
                and pe.base+s['rva']<=start<end<=pe.base+s['rva']+min(s['size'],s['virtual_size'])
                and s['off']+end-pe.base-s['rva']<=len(pe.data)]
        assert len(owners)==1,('range is not original file-backed code',hex(start),hex(end))
    def overlaps(a,b):return a[0]<b[1] and b[0]<a[1]
    application=[]
    functions=config['functions']
    for record in functions:
        code_range(record['original'],record['original']+1)
        if record['status']=='stub':
            assert record['extent'] is None and not record.get('text_data'),('stub owns restored bytes',record['name'])
            continue
        start,end=record['extent']
        assert start==record['original'],('extent does not start at entry',record['name'])
        for kind,(a,b) in [('code',(start,end)),*[('text-data',interval)for interval in record.get('text_data',[])]]:
            code_range(a,b)
            interval=(a,b,record['name'],kind)
            for prior in application:
                assert not overlaps(interval,prior),('overlapping application ownership',interval,prior)
            for other in functions:
                if other['name']==record['name'] and kind=='code':continue
                assert not a<=other['original']<b,('application range contains another entry',interval,other['name'])
            application.append(interval)
    known={(r['name'],r['start'],r['end'])for r in runtime['functions']+runtime['imports']}
    aliases=[]
    for alias in config.get('runtime_aliases',[]):
        assert len(set(alias['names']))>=2 and alias.get('evidence') and (BINARY/alias['evidence']).is_file()
        assert all((name,alias['start'],alias['end'])in known for name in alias['names']),('unidentified runtime alias',alias)
        aliases.append(alias)
    support=[]
    for record in config.get('support_unmasked',[]):
        a,b=record['start'],record['end']
        code_range(a,b)
        assert (record['name'],a,b)in known,('unidentified runtime range',record)
        interval=(a,b,record['name'],'runtime')
        for prior in application:
            assert not overlaps(interval,prior),('runtime overlaps application ownership',interval,prior)
        for function in functions:
            assert not a<=function['original']<b,('runtime overlaps application entry',interval,function['name'])
        for item in context:
            assert not overlaps(interval,(item['original'],item['original']+item['size'])),('runtime overlaps contextual data',interval,item['name'])
        for prior in support:
            if not overlaps(interval,prior):continue
            assert any(a==prior[0]==alias['start'] and b==prior[1]==alias['end']
                       and record['name']!=prior[2] and {record['name'],prior[2]}<=set(alias['names'])
                       for alias in aliases),('overlapping runtime ownership without explicit aliases',interval,prior)
        support.append(interval)
    # Contextual data may live in .text; it must then be explicitly owned as
    # text-data, never implicitly exposed by an application code envelope.
    for item in context:
        interval=(item['original'],item['original']+item['size'])
        owners=[r for r in application if overlaps(interval,r)]
        if owners:
            assert len(owners)==1 and owners[0][3]=='text-data' and owners[0][0]<=interval[0]<interval[1]<=owners[0][1],('contextual data overlaps code or ambiguous ownership',item['name'],owners)

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

def seh_roots(pe,start,end,compiler_scaffolding=False):
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
    if compiler_scaffolding and roots:
        instructions=list(decoder.disasm(pe.read(start,end-start),start))
        by_address={ins.address:ins for ins in instructions}
        for root in list(roots):
            region=[ins for ins in instructions if ins.address>=root]
            for ins in region:
                if ins.mnemonic=='ret':
                    following=by_address.get(ins.address+ins.size)
                    if following and following.mnemonic=='jmp' and re.fullmatch(r'0x[0-9a-f]+',following.op_str):
                        target=int(following.op_str,16)
                        assert start<=target<end
                        roots.append(following.address)
                    break
                if ins.mnemonic=='jmp':
                    following=by_address.get(ins.address+ins.size)
                    if following and following.mnemonic=='mov' and re.fullmatch(r'dword ptr \[ebp - 0x[0-9a-f]+\], 0xffffffff',following.op_str):
                        # /Od emits a dead reset after a returning handler. Its
                        # exact slot/value must also be a normal scope reset.
                        assert sum(other.bytes==following.bytes for other in instructions)>=2
                        roots.append(following.address)
                    break
    return sorted(set(roots))

def audited_graph(pe,start,end,roots=(),data=(),switches=None,padding=()):
    result=graph(pe,start,end,data=data,roots=roots,switches=switches)
    decoder=Cs(CS_ARCH_X86,CS_MODE_32)
    covered=set()
    for address,instruction in result['instructions'].items():covered.update(range(address,address+instruction.size))
    for a,b in padding:
        # This compiler also uses ADD EAX,0 as a five-byte alignment filler.
        # It writes flags, so allow only an exact explicitly reviewed gap that
        # the complete CFG proves unreachable, never an executable instruction.
        assert start<=a<b<=end and b-a==5 and pe.read(a,5)==b'\x05\0\0\0\0'
        assert not any(byte in covered for byte in range(a,b)),'declared alignment is executable'
    cursor=start
    previous=None
    compiler_roots=[]
    while cursor<end:
        interval=next(((a,b) for a,b in data if a<=cursor<b),None)
        if interval:cursor=interval[1];continue
        item=list(decoder.disasm(pe.read(cursor,min(15,end-cursor)),cursor,count=1))
        assert item,'invalid/truncated whole-function instruction at %x'%cursor
        ins=item[0]
        assert cursor+ins.size<=end,'partial final instruction'
        if cursor not in covered:
            if previous and previous.mnemonic==ins.mnemonic=='jmp' and previous.op_str==ins.op_str and re.fullmatch(r'0x[0-9a-f]+',ins.op_str):
                assert int(ins.op_str,16) in result['instructions']
                compiler_roots.append(cursor)
            else:
                declared=(cursor,cursor+ins.size)in [tuple(r)for r in padding]
                assert declared or ins.bytes.hex() in ('90','8bff','8d4900','8d642400','8da42400000000','8d9b00000000','cc'),('unrooted code',hex(cursor),ins.mnemonic,ins.op_str)
        previous=ins
        cursor+=ins.size
    if compiler_roots:
        result=graph(pe,start,end,data=data,roots=[*roots,*compiler_roots],switches=switches)
    return result

def compiler_switches(pe,start,end,expected_counts):
    """Recover only compiler tables with the declared source switch bound."""
    if not expected_counts:return {},[]
    decoder=Cs(CS_ARCH_X86,CS_MODE_32)
    decoder.detail=True
    cursor=start
    recent=[]
    switches={}
    data=[]
    counts=[]
    while cursor<end:
        table=next(((a,b) for a,b in data if a<=cursor<b),None)
        if table:cursor=table[1];continue
        decoded=list(decoder.disasm(pe.read(cursor,min(15,end-cursor)),cursor,count=1))
        assert decoded,'invalid compiler instruction'
        ins=decoded[0]
        if ins.mnemonic=='jmp' and ins.operands[0].type==X86_OP_MEM:
            base=ins.operands[0].mem.disp&0xffffffff
            assert start<=base<end,'indirect jump is not a compiler text table'
            bound=None
            for index,prior in enumerate(recent):
                if prior.mnemonic=='cmp' and prior.operands[-1].type==X86_OP_IMM:
                    if any(branch.mnemonic=='ja' for branch in recent[index+1:]):
                        bound=prior.operands[-1].imm+1
            assert bound and 0<bound<=256,'no unsigned compiler table bound'
            assert base+bound*4<=end,'compiler table exceeds function extent'
            targets=[u32(pe.read(base+4*i,4),0) for i in range(bound)]
            assert all(start<=target<end for target in targets),'compiler switch target outside function'
            switches[ins.address]=targets
            data.append((base,base+4*bound))
            counts.append(bound)
        recent=(recent+[ins])[-8:]
        cursor+=ins.size
    assert counts==expected_counts,('compiler/source switch bounds differ',counts,expected_counts)
    return switches,data

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
    validate_ownership(config,original,runtime,json.loads((BINARY/'data.json').read_text()))
    from seams import approved
    approved(config,original,runtime)
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
        a=audited_graph(original,start,end,record.get('roots',()),record.get('text_data',()),switches,record.get('alignment_padding',()))
        rebuilt_start=linked[record['symbol']]
        rebuilt_end=next(address for address in addresses if address>rebuilt_start)
        roots=seh_roots(rebuilt,rebuilt_start,rebuilt_end,record.get('compiler_seh_scaffolding',False))
        roots+= [rebuilt_start+offset for offset in record.get('rebuilt_extra_roots',[])]
        rebuilt_switches,rebuilt_data=compiler_switches(rebuilt,rebuilt_start,rebuilt_end,record.get('compiler_switch_counts',[]))
        b=audited_graph(rebuilt,rebuilt_start,rebuilt_end,roots,rebuilt_data,rebuilt_switches)
        left,right=names(a['edges'],oi),names(b['edges'],ri)
        assert left==right,(record['name'],'original only',left-right,'rebuilt only',right-left)
        assert a['returns']==b['returns']=={record['stack_cleanup']},('ABI RET cleanup differs',record['name'],a['returns'],b['returns'])
        assert record['abi'],record['name']
        checked+=1
    for record in json.loads((BINARY/'data.json').read_text()):
        if record.get('kind')=='string-pointer-array':
            for index in range(record['size']//4):
                a=u32(original.read(record['original']+index*4,4),0)
                b=u32(rebuilt.read(linked[record['symbol']]+index*4,4),0)
                assert original.string(a)==rebuilt.string(b),('contextual string differs',record['name'],index)
        else:
            assert original.read(record['original'],record['size'])==rebuilt.read(linked[record['symbol']],record['size']),('initial data differs',record['name'])
    for record in config.get('support_unmasked',[]):
        found=[r for r in runtime['functions']+runtime['imports'] if r['start']==record['start'] and r['end']==record['end'] and r['name']==record['name']]
        assert found,('unproven CRT/import unmask',record)
        assert (BINARY/record['evidence']).is_file()
    print('%d complete original/compiler CFG/callee/ABI functions; %d contextual data objects; strict CRT/import identities verified.'%(checked,len(json.loads((BINARY/'data.json').read_text()))))

if __name__=='__main__':main()
