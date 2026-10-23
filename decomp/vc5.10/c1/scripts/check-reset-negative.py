#!/usr/bin/env python3
"""Focused real Reset writes, repeated-call inventory and CRT vector controls."""
import argparse,copy,importlib.util,json,re,shutil,struct,tempfile,time
from pathlib import Path
import arena_cycle_state,arena_state,scaler_state,crt_vectors
from allocation_state import symbols
from provenance import captured_native_binary

def reject(action,name):
    try:action()
    except(AssertionError,KeyError,ValueError,IndexError):return
    raise AssertionError('accepted '+name)

def main():
    parser=argparse.ArgumentParser();parser.add_argument('original',type=Path);parser.add_argument('native',type=Path);args=parser.parse_args();started=time.perf_counter()
    spec=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'));trial=importlib.util.module_from_spec(spec);spec.loader.exec_module(trial)
    trial.check(args.original,args.native)
    read=lambda name:json.loads((args.native/(name+'.json')).read_text())
    state=read('state');entries=read('arena-reset-entries');returns=read('arena-reset-returns');names=symbols(captured_native_binary(args.native));controls=[]
    for index in range(6):
        before=entries[index]['pre_seed'];base=before['bss']['arenas'][index]['base'];receiver=names['c1_bss'][0]+804+index*32;pc=names['_ZN13CompilerArena5ResetEv'][0]
        def check(entry,returned):return arena_cycle_state.reset_proof(before,entry,returned,index,receiver,True,pc)
        check(entries[index],returns[index])
        for field in ('cursor','committed_limit','committed_bytes','reset_value'):
            changed=copy.deepcopy(returns[index]);changed['compiler_state']['bss']['arenas'][index][field]^=1
            label='reset%d-wrong-%s'%(index,field);reject(lambda:check(entries[index],changed),label);controls.append(label)
        changed=copy.deepcopy(entries[index]);changed['ecx']+=32;label='reset%d-receiver'%index;reject(lambda:check(changed,returns[index]),label);controls.append(label)
        changed=copy.deepcopy(entries[index]);changed['formal_inventory']=[];label='reset%d-omitted-formal'%index;reject(lambda:check(changed,returns[index]),label);controls.append(label)
    index=0;before=entries[0]['pre_seed'];receiver=names['c1_bss'][0]+804;pc=names['_ZN13CompilerArena5ResetEv'][0]
    def check(entry,returned):return arena_cycle_state.reset_proof(before,entry,returned,0,receiver,True,pc)
    for name,path in [('descriptor',['bss','arenas',0,'descriptor']),('reservation',['bss','arenas',0,'reserve_bytes']),('base',['bss','arenas',0,'base']),('mapped-view',['bss','arenas',0,'mapped_view']),('other-arena',['bss','arenas',1,'base']),('snapshot',['snapshot','compiler_flags']),('initialized',['initialized','compiler_flags']),('persistent-context',['pdb_context','read_mode'])]:
        changed=copy.deepcopy(returns[0]);cursor=changed['compiler_state']
        for key in path[:-1]:cursor=cursor[key]
        cursor[path[-1]]^=1;reject(lambda:check(entries[0],changed),name);controls.append(name)
    for name,side,key,value in [('reset-cleanup','return','esp',0),('reset-return-pc','return','pc',0),('reset-index','entry','index',1),('reset-seed-omitted','entry','seeded',False)]:
        entry=copy.deepcopy(entries[0]);returned=copy.deepcopy(returns[0]);(entry if side=='entry'else returned)[key]=value
        reject(lambda:check(entry,returned),name);controls.append(name)
    changed=copy.deepcopy(returns[0]);changed['fp']['sw']^=1;reject(lambda:check(entries[0],changed),'reset-fpu');controls.append('reset-fpu')
    candidate=json.loads((args.original/'candidate.json').read_text());fields=json.loads((Path(__file__).resolve().parents[1]/candidate['field_map']).read_text());events=(args.original/'events.txt').read_text()
    module=int(re.findall(r'^module name=c1n32_l.dll actual=([0-9a-f]+)',events,re.M)[0],16);sequence=int(re.findall(r'^trap seq=(\d+)',events,re.M)[-1])
    first_member,member_sequence=arena_state.member_boundary(args.original,args.native,state,sequence,module)
    first_scaler,scaler_sequence=scaler_state.boundary(args.original,args.native,state,sequence,module)
    inventory_controls=[];namespace_controls=[];scaler_controls=[]
    with tempfile.TemporaryDirectory(prefix='reset-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1')as temp:
        n=Path(temp)/'native';shutil.copytree(args.native,n)
        def cycle():return arena_cycle_state.validate(args.original,n,fields,first_member,first_scaler,state,member_sequence,scaler_sequence,sequence,module,candidate,'restore')
        files={file:file.read_bytes()for file in n.iterdir()if file.is_file()}
        def restore():
            for file,data in files.items():
                if file.read_bytes()!=data:file.write_bytes(data)
        for table in ('arena-member-entry-observations','arena-member-returns','scaler-entries','scaler-returns','arena-api-entries','arena-api-returns','arena-reset-entries','arena-reset-returns'):
            restore();file=n/(table+'.json');rows=json.loads(file.read_text());rows.pop(2);file.write_text(json.dumps(rows));reject(cycle,'missing-'+table);inventory_controls.append('missing-'+table)
        for table in ('arena-member-entry-observations','arena-reset-entries'):
            restore();file=n/(table+'.json');rows=json.loads(file.read_text());rows[0],rows[1]=rows[1],rows[0];file.write_text(json.dumps(rows));reject(cycle,'reordered-'+table);inventory_controls.append('reordered-'+table)
        for filename,label in [('scaler-fpu.json','last-scaler-qword'),('scaler-returns.json','last-scaler-return')]:
            restore();file=n/filename;rows=json.loads(file.read_text())
            if filename=='scaler-fpu.json':rows[-1]['qword_bytes']='0000000000000000'
            else:rows[-1]['eax']^=1
            file.write_text(json.dumps(rows));reject(lambda:scaler_state.compare(args.original,n,5),label);scaler_controls.append(label)
        restore();getter=read('internal-getenv-entries')[0];get_return=read('internal-getenv-returns')[0]
        def namespace(before,after):return crt_vectors.validate(args.original,n,state['invocation'],before,after)
        namespace(getter,get_return)
        for label,path,value in [('unbound',['bound'],0),('conflated-argc',['argc'],11),('conflated-argv',['argv','address'],state['invocation']['args']['argv']),('wrong-env-count',['environment','count'],1),('wrong-env-owner',['environment','address'],0)]:
            changed=copy.deepcopy(getter);cursor=changed['crt']
            for key in path[:-1]:cursor=cursor[key]
            cursor[path[-1]]=value;reject(lambda:namespace(changed,get_return),label);namespace_controls.append(label)
        changed=copy.deepcopy(get_return);changed['crt']['environment']^=4;reject(lambda:namespace(getter,changed),'environment-mutated');namespace_controls.append('environment-mutated')
        for filename,offset,value,label in [('native-crt-argv.bin',0,0,'null-argv-owner'),('native-crt-environment.bin',4,0,'wrong-raw-env-count'),('native-crt-environment.bin',16,65,'changed-environment-byte'),('original-crt-environment.bin',16,65,'changed-original-input-copy')]:
            restore();file=n/filename;raw=bytearray(file.read_bytes())
            if offset in (0,4):struct.pack_into('<I',raw,offset,value)
            else:raw[offset]^=value
            file.write_bytes(raw);reject(lambda:namespace(getter,get_return),label);namespace_controls.append(label)
    print(json.dumps(dict(status='PASS',fresh_reset_state_controls=len(controls),fresh_inventory_controls=len(inventory_controls),fresh_namespace_controls=len(namespace_controls),fresh_repeated_scaler_controls=len(scaler_controls),rejected=controls+inventory_controls+namespace_controls+scaler_controls,seconds=time.perf_counter()-started,
        scope='Fresh complete restore positive, six actual nonzero Reset write/receiver/ABI proofs, repeated inventory and private CRT namespace mutations. Independent full normal positive separate; unchanged scaler/core campaigns reused with pinned provenance.'),indent=2))
if __name__=='__main__':main()
