"""Reviewed97 genuine entry progression; explicitly excludes later heap equality."""
import json,re,hashlib
from state import BINARY
from symbol_references import validate

def validate_body(config,pe):
    p=validate(config,pe)['progression']
    assert p['original_frontier']==0x25450 and p['native_frontier']=='begin_symbol_block'and p['scope_depth']==p['suppressed_depth']==0 and p['unobserved_later_heap']is True
    assert (p['module'],p['module_index'],p['directory_offset'],p['input_size'],p['input_capacity'],p['input_cursor'])==(10,9,4754,43,4746,4)
    assert(p['callback_kind'],p['callback_length'],p['callback_index'])==(0x206,35,20)
    body=p['body'];assert(body['start'],body['end'])==(0x25450,0x25471)and hashlib.sha256(pe.read(0x25450,0x21)).hexdigest()==body['sha256'],'next-body proof changed'
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    instructions={r.address:r for r in Cs(CS_ARCH_X86,CS_MODE_32).disasm(pe.read(0x25450,0x21),0x25450)}
    path=[instructions[a]for a in(0x25450,0x25456,0x2545d,0x25470)]
    assert [(r.mnemonic,r.op_str)for r in path]==[('inc','dword ptr [0x335ec]'),('cmp','dword ptr [0x335e8], 0'),('je','0x25470'),('ret','')],'zero-suppression path is not the complete no-heap path'
    record=next(r for r in config['functions']if r['name']=='begin_symbol_block');assert record['parameters']==[] and record['abi']=='void begin_symbol_block(void)','next-stub ABI differs'
    return p

def compare(config,pe,events,native,trace):
    p=validate_body(config,pe)
    traps=re.findall(r'^exception ([0-9a-f]+) va ([0-9a-f]+) ',events,re.M);assert traps==[('80000003','00025450')],'progression did not reach genuine next masked entry'
    assert native['signal']==5 and native['frames'][0]=='begin_symbol_block'and native['parameters']=={},'native progression next named stub/ABI differs'
    assert native['scope_depth']==native['suppressed_depth']==0 and native['later_heap_equality']is False,'next-stub consumed scalars or observation scope differs'
    memory={}
    for a,raw in re.findall(r'^memory ([0-9a-f]+) ([0-9a-f]+)$',events,re.M):memory.update((int(a,16)+i,v)for i,v in enumerate(bytes.fromhex(raw)))
    def read(address):return int.from_bytes(bytes(memory[address+i]for i in range(4)),'little')
    assert read(0x335ec)==read(0x335e8)==0,'original next-stub consumed state differs'
    rows=[line.split()[1:]for line in events.splitlines()if line.startswith('progress-input ')];assert len(rows)==1
    words=rows[0];assert words[::2]==['module','module_address','input','cursor','capacity','directory','offset','size','cvbase','read_offset','output','output_cursor','bytes']
    hexfields=('module_address','input','cursor','directory','output','output_cursor');old={k:v if k=='bytes'else int(v,16 if k in hexfields else 10)for k,v in zip(words[::2],words[1::2])}
    for row in(old,native):
        assert row['module']==10 and row['offset']==4754 and row['size']==43 and row['capacity']==4746 and row['cursor']==row['input']+4 and row['output_cursor']==row['output']+4 and row['read_offset']==4797,'actual next callback ownership/input extent differs'
        data=bytes.fromhex(row['bytes']);master=(BINARY/'cases/hello-debug-dll/input/hello-debug.dll').read_bytes();assert len(data)==43 and data==master[row['cvbase']+4754:row['cvbase']+4797],'next callback input not exact frozen DLL subsection'
        assert data[:8]==bytes.fromhex('0100000021000602'),'next callback kind/header differs'
    assert old['bytes']==native['bytes']and old['cvbase']==native['cvbase'],'next callback source differs'
    assert read(0x3305c)==old['input']and read(0x335e4)==old['cursor']and read(0x33084)==4746 and read(0x33050)==old['module_address']and read(0x335f0)==old['output_cursor'],'original context does not bind actual globals'
    assert native['callback']==dict(kind=0x206,length=35,index=20),'next callback native selection differs'
    apps={r['original']:r for r in config['functions']};entries=[apps[int(a,16)]['native']for a in re.findall(r'^runtime ([0-9a-f]+) [0-9a-f]+$',events,re.M)if int(a,16)in apps]+['begin_symbol_block']
    assert entries==trace and 'create_symbol_reference'in entries,'complete genuine application entry projection differs'
    return dict(next='begin_symbol_block',module=10,scope_depth=0,suppressed_depth=0,heap_reads_on_actual_next_path=0,entry_trace=entries,original_context=old,native_context=native,later_heap_equality=False,classification='actual-control-flow-and-next-consumed-state-only')
