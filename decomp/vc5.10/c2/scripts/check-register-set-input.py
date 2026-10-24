#!/usr/bin/env python3
"""Exercise actual production model inputs after one full current-pair validation."""
import argparse,copy,importlib.util,json,re,shutil,tempfile,time
from pathlib import Path
from state import BINARY,sha
from compare import compare
import globals_state
from function_register_data import compare_pools

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for n in ('original','native','output'):p.add_argument('--'+n,type=Path,required=True)
    a=p.parse_args();start=time.monotonic();contexts=[];model=globals_state.complete_globals
    def observe(read,addresses,**kwargs):
        contexts.append((read,addresses,kwargs));return model(read,addresses,**kwargs)
    globals_state.complete_globals=observe
    try:positive=compare(a.original,a.native)
    finally:globals_state.complete_globals=model
    assert len(contexts)==1;read,addresses,kwargs=contexts[0];assert kwargs['register_blocks']==4 and kwargs['register_directory'] and kwargs['register_sets']==2;passed=[]
    config=json.loads((a.original/'configuration.json').read_text());expected=next(r for r in config['stubs']if r['name']=='@new_function_register@4');state=json.loads((a.native/'state.json').read_text());native_proof=positive['native_elf_proof'];spec=importlib.util.spec_from_file_location('register_block_proof',BINARY/'scripts/native-proof.py');proof=importlib.util.module_from_spec(spec);spec.loader.exec_module(proof)
    def reject(name,fn):
        t=time.monotonic()
        try:fn()
        except(AssertionError,KeyError,ValueError)as e:passed.append({'control':name,'reason':str(e),'seconds':time.monotonic()-t})
        else:raise AssertionError('mutation accepted: '+name)
    # Capture exact call arguments from the real compare; never rebuild a
    # simplified model or repeat the unchanged CFG audit for each data byte.
    globals_to_mutate=['function_register_blocks','next_function_register_block','end_function_register_blocks','next_function_register_number','first_function_register_block','last_function_register_block','tracked_register_block','secondary_register_block','plain_register_block','default_register_block','extra_register_block','free_plain_register_nodes','free_tracked_register_nodes','secondary_register_set','tracked_register_set','empty_function_register','free_register_set_head']
    pool_head=int.from_bytes(read(addresses['pools']+12,4),'little');bases,_=globals_state.register_pool_state(read,pool_head,4,2)
    cases=[('global '+name,addresses[name])for name in globals_to_mutate]
    cases += [('latest allocator cursor',addresses['pools']+16),('latest allocator remaining',addresses['pools']+20),('directory first pointer',bases[0]+32),('directory last used pointer',bases[0]+44),('directory first unwritten pointer',bases[0]+48),('directory old defined tail',bases[0]+4032)]
    for i,base in enumerate(bases[1:],1):
        cases.extend((('block%d '%i+label,base+offset)for label,offset in [('pool next',0),('capacity',4),('record next',8),('record cursor',12),('record end',16),('first row number',48),('last row number',48+31*80),('unknown row byte modified',25),('own zeroed arena tail',2580)]))
    for at in (2580,2592):
        for offset,label in [(0,'first pointer'),(4,'pool byte'),(5,'untouched padding'),(8,'lowstate'),(11,'upperstate')]:cases.append(('set%d '%at+label,bases[-1]+at+offset))
    for name,at in cases:
        touched=[0]
        def changed(address,size):
            data=read(address,size)
            if address<=at<address+size:
                raw=bytearray(data);raw[at-address]^=1;touched[0]+=1;return bytes(raw)
            return data
        reject(name,lambda:model(changed,addresses,**kwargs));assert touched[0]>0,'mutation did not alter actual model input: '+name
    # Use the actual raw snapshot checker for transport/ELF/root/formal tests.
    for label,field,key,value in [('kind formal value','register_node_input','kind',3),('kind formal missing','register_node_input','formals',[]),('kind formal wrong type','register_node_input','formals',[{'name':'kind','type':'int'}]),('wrong caller','register_node_input','caller','new_register_block'),('wrong pool root','function_pool_state','root_address',state['function_pool_state']['root_address']+12),('wrong active arena','function_pool_state','head',state['function_pool_state']['head']+4096),('wrong arena count','function_pool_state','arena_count',4)]:
        changed=copy.deepcopy(state);assert changed[field][key]!=value;changed[field][key]=value;reject(label,lambda v=changed:compare_pools(a.original,a.native,config,expected,v,native_proof,proof))
    with tempfile.TemporaryDirectory(prefix='block-raw-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as temp:
        root=Path(temp)
        for i,(side,index,label)in enumerate([('n',8+25,'native unknown initialized byte'),('o',8+2580,'original own zeroed tail'),('n',0,'native raw arena base'),('o',4,'original raw arena size'),('n',4*4104+8+16,'native old literal pointer')]):
            case=root/str(i)
            for key,source in [('o',a.original),('n',a.native)]:
                target=case/key;target.mkdir(parents=True)
                for name in ['receipt.json','function-pools.bin']+(['events.txt']if key=='o'else['gdb.stdout','native.elf']):shutil.copyfile(source/name,target/name)
            path=case/side/'function-pools.bin';raw=bytearray(path.read_bytes());raw[index]^=1;path.write_bytes(raw);path=case/side/'receipt.json';r=json.loads(path.read_text());r['function_pools_sha256']=sha(raw);path.write_text(json.dumps(r));reject(label,lambda:compare_pools(case/'o',case/'n',config,expected,state,native_proof,proof))
    a.output.write_text(json.dumps({'positive':positive,'count':len(passed),'controls':passed,'seconds':time.monotonic()-start,'checker_sha256':sha(Path(__file__).read_bytes()),'model_sha256':expected['state_model_sha256'],'production_invocation':{'model_path':'scripts/globals_state.py:complete_globals','model_source_sha256':sha((BINARY/'scripts/globals_state.py').read_bytes()),'read_path':'scripts/compare.py:compare/read','read_source_sha256':sha((BINARY/'scripts/compare.py').read_bytes()),'addresses':addresses,'kwargs':kwargs,'original_raw_hashes':dict(json.loads((a.original/'receipt.json').read_text())['globals_complete_hashes'],function_pools=sha((a.original/'function-pools.bin').read_bytes()))},'original_receipt_sha256':sha((a.original/'receipt.json').read_bytes()),'native_receipt_sha256':sha((a.native/'receipt.json').read_bytes()),'scope':'Full production pair/audit once, then exact captured production data-model read callback/addresses/keyword premises with targeted actual mutations. No private discovery/CFG/checker substitute; all mutated reads required to fire. Same raw five-arena, ELF and fastcall frontier checker used for raw/formal controls.'},indent=2)+'\n')
if __name__=='__main__':main()
