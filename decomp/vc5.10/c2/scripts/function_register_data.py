"""Finite register-block data proof and authentic multi-arena snapshots."""
import json,re,struct
from state import BINARY,TARGET,sha


def verify(config):
    row=next((r for r in config['driver']if r['name']=='_new_register_block'and r['name']in config['accepted']+config.get('trial',[])),None)
    if row is None:return None
    raw=(BINARY/row['register_block_model']).read_bytes();assert sha(raw)==row['register_block_model_sha256'];value=json.loads(raw);assert value['target_sha256']==sha(TARGET.read_bytes())
    for name,digest in value['source_files'].items():assert sha((BINARY/name).read_bytes())==digest,'register-block source changed without contextual field proof'
    fields=json.loads((BINARY/'cases/analysis/function-records-analysis.json').read_text())['objects']
    tokens=['C2FunctionRegister','C2RegisterBlock']+[r['native']for r in fields if r['native']!='pending_debug_symbols']
    expression=r'\b(?:'+'|'.join(tokens)+r')\b'
    for path in (BINARY/'src').rglob('*'):
        if path.suffix not in ('.c','.h')or str(path.relative_to(BINARY))in value['source_files']:continue
        text=path.read_text()
        if path.name=='stubs.c':text=re.sub(r'^C2FunctionRegister \*__fastcall new_function_register\(unsigned char kind\)$','',text,flags=re.M)
        assert not re.search(expression,text),'unknown register field consumer or pointer escape requires contextual reconstruction'
    return {'model_sha256':sha(raw),'block_bytes':2572,'rows':32,'stride':80,'number_offset':28,'header_fields':[0,4,8],'unknown_fields':'No semantic/default credit; only real accepted allocator writer spans compared.','cold_growth_execution':'unvisited; exact86B owned and whole225B function audited'}


def arenas(directory):
    raw=(directory/'function-pools.bin').read_bytes();receipt=json.loads((directory/'receipt.json').read_text())
    assert sha(raw)==receipt['function_pools_sha256'],'function pool raw provenance changed'
    assert len(raw)==5*4104,'function pool snapshot extent differs'
    result={};order=[]
    for at in range(0,len(raw),4104):
        base,size=struct.unpack_from('<II',raw,at);assert size==4096 and base and base not in result
        assert all(base+4096<=other or other+4096<=base for other in result),'overlapping function pool snapshots'
        result[base]=raw[at+8:at+4104];order.append(base)
    assert [struct.unpack_from('<I',result[base])[0]for base in order]==order[1:]+[0],'function pool raw link/order differs'
    return order,result


def elf_bytes(binary,address,size):
    raw=binary.read_bytes();assert raw[:6]==b'\x7fELF\x01\x01';offset=struct.unpack_from('<I',raw,28)[0];stride,count=struct.unpack_from('<HH',raw,42)
    for n in range(count):
        kind,file,base,_,length,_,_,_=struct.unpack_from('<IIIIIIII',raw,offset+n*stride)
        if kind==1 and base<=address and address+size<=base+length:return raw[file+address-base:file+address-base+size]
    raise AssertionError('native directive literal is not backed by actual ELF data')


def compare_pools(original,native,config,expected,state,native_proof,proof):
    from globals_state import register_pool_state
    count=expected['function_register_blocks'];assert count==4
    evidence=[]
    for side,directory in [('original',original),('native',native)]:
        order,blocks=arenas(directory)
        def read(address,size):
            for base,data in blocks.items():
                if base<=address and address+size<=base+4096:return data[address-base:address-base+size]
            raise AssertionError('function pool snapshot read outside owned span')
        bases,value=register_pool_state(read,order[0],count,expected.get('register_sets',0),expected.get('function_register_nodes',0))
        assert value==state['globals_complete']['function_pools'],'actual function pool bytes differ from typed GDB view'
        oldest=bases[0];raw=bytearray(blocks[oldest]);wanted=bytes.fromhex(state['globals_complete']['directives']['pool_bytes'])
        assert raw[:8]==struct.pack('<II',0,4088)
        for offset,next_offset,text in [(8,20,b'LIBC\0'),(20,0,b'OLDNAMES\0')]:
            following,kind,string=struct.unpack_from('<III',raw,offset)
            assert following==(oldest+next_offset if next_offset else 0)and kind==4
            if side=='original':assert string==({8:0x49a658,20:0x497fdc}[offset])
            else:assert elf_bytes(native/'native.elf',string-int(native_proof['load_bias'],16),len(text))==text,'native raw directive string identity/content differs'
            normalized=struct.unpack_from('<I',wanted,offset+8)[0]
            struct.pack_into('<III',raw,offset,0x30000000+next_offset if next_offset else 0,4,normalized)
        for index in range(count):
            assert struct.unpack_from('<I',raw,32+4*index)[0]==bases[index+1]+8
            struct.pack_into('<I',raw,32+4*index,0x50000000+(index+1)*0x10000+8)
        assert raw==wanted,'oldest arena raw data/directory/own zeroed tail differs'
        evidence.append((order,blocks))
    order,blocks=evidence[1];symbols=proof.symbols(BINARY/'build/native/c2',data=True);root,size=symbols['c2_pools'];assert size==96
    wanted={'head':order[0],'root_address':root+12+int(native_proof['load_bias'],16),'arena_count':5}
    assert state['function_pool_state']==wanted
    observations=re.findall(r'^C2-FUNCTION-POOL-STATE (.*)$',(native/'gdb.stdout').read_text(),re.M)
    assert len(observations)==1 and json.loads(observations[0])==wanted
    if expected.get('register_add_input'):
        bases=list(reversed(evidence[1][0]));actual={'set':bases[-1]+2592,'number':0,'formals':[{'name':'set','type':'C2RegisterSet *'},{'name':'number','type':'unsigned int'}],'caller':'new_function_register','set_tracked_alias':True,'set_offset':2592,'number_row_alias':True,'row_pointer':bases[1]+20}
        assert state['register_add_input']==actual,'register-set actual ECX/EDX formals/owner aliases differ'
        observations=re.findall(r'^C2-REGISTER-ADD-INPUT (.*)$',(native/'gdb.stdout').read_text(),re.M)
        assert len(observations)==1 and json.loads(observations[0])==actual
        events=(original/'events.txt').read_text().rsplit('observe 0040189f ',1);assert len(events)==2
        original_bases=list(reversed(evidence[0][0]));wanted='register-add-input set=%08x number=0 row=%08x'%(original_bases[-1]+2592,original_bases[1]+20)
        assert re.findall(r'^register-add-input .*$',events[1],re.M)==[wanted]
        return evidence[0][1]
    if expected.get('register_node_input'):
        wanted={'kind':4,'formals':[{'name':'kind','type':'unsigned char'}],'caller':'initialize_function_records'}
        assert state['register_node_input']==wanted,'function-register actual CL formal differs'
        observations=re.findall(r'^C2-REGISTER-NODE-INPUT (.*)$',(native/'gdb.stdout').read_text(),re.M)
        assert len(observations)==1 and json.loads(observations[0])==wanted
        events=(original/'events.txt').read_text().rsplit('observe 00401c98 ',1);assert len(events)==2
        assert re.findall(r'^register-node-input kind=(\d+) no-edx-formal=1$',events[1],re.M)==['4']
        return evidence[0][1]
    wanted={'pool':1,'formals':[{'name':'pool','type':'int'}],'caller':'initialize_function_records'}
    assert state['register_set_input']==wanted,'new register-set actual fastcall ECX formal differs'
    observations=re.findall(r'^C2-REGISTER-SET-INPUT (.*)$',(native/'gdb.stdout').read_text(),re.M)
    assert len(observations)==1 and json.loads(observations[0])==wanted
    events=(original/'events.txt').read_text().rsplit('observe 00401863 ',1);assert len(events)==2
    assert re.findall(r'^register-set-input pool=(\d+) no-edx-formal=1$',events[1],re.M)==['1']
    return evidence[0][1]
