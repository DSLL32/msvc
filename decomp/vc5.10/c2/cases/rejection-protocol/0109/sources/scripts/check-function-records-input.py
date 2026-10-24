#!/usr/bin/env python3
"""Mutate actual noarg register-block frontier and allocated directory state."""
import argparse,ast,copy,json,re,shutil,tempfile,time
from pathlib import Path
from compare import compare
from state import sha

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('original','native','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();start=time.monotonic();positive=compare(a.original,a.native);passed=[]
    def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n')
    def mutate_native(root,change):
        path=root/'n/state.json';v=json.loads(path.read_text());before=json.dumps(v['globals_complete']['function_registers'],sort_keys=True);change(v['globals_complete']['function_registers']);assert before!=json.dumps(v['globals_complete']['function_registers'],sort_keys=True);write(path,v)
        path=root/'n/gdb.stdout';raw=path.read_text();assert len(re.findall('^C2-GLOBALS-COMPLETE ',raw,re.M))==1;path.write_text(re.sub(r'^C2-GLOBALS-COMPLETE .*$',lambda m:'C2-GLOBALS-COMPLETE '+json.dumps(v['globals_complete'],sort_keys=True),raw,flags=re.M));path=root/'n/receipt.json';r=json.loads(path.read_text());r['state_sha256']=sha((root/'n/state.json').read_bytes());r['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write(path,r)
    def mutate_original(root,name,index):
        path=root/'o'/name;raw=bytearray(path.read_bytes());raw[index]^=4;path.write_bytes(raw);path=root/'o/receipt.json';r=json.loads(path.read_text());r['globals_complete_hashes'][name]=sha(raw)
        if name=='unmodeled-original-data.bin':r['unmodeled_original_data']['sha256']=sha(raw)
        write(path,r)
    cases=[]
    for field in ['directory_offset','cursor_offset','end_offset','capacity','allocation_bytes']:
        cases.append(('native '+field,lambda root,key=field:mutate_native(root,lambda v:v.__setitem__(key,v[key]+4))))
    for field in ['next_function_register_number','first_function_register_block','last_function_register_block','tracked_register_block','secondary_register_set','empty_function_register','pending_debug_symbols']:
        cases.append(('native unexpected initialized '+field,lambda root,key=field:mutate_native(root,lambda v:v['pending'].__setitem__(key,1))))
    for address,name in [(0x48f88c,'directory base'),(0x48faa8,'directory cursor'),(0x48faa4,'directory end'),(0x48e030,'allocator cursor'),(0x48e034,'allocator remaining')]:
        cases.append(('original '+name,lambda root,at=address:mutate_original(root,'unmodeled-original-data.bin',at-0x48e000)))
    for index,name in [(32,'first directory pointer'),(4028,'last directory pointer'),(4032,'defined own unallocated tail')]:cases.append(('original '+name,lambda root,index=index:mutate_original(root,'directive-pool.bin',index)))
    with tempfile.TemporaryDirectory(prefix='function-records-input-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as temp:
        folder=Path(temp)
        for i,(name,change)in enumerate(cases):
            root=folder/str(i);shutil.copytree(a.original,root/'o');shutil.copytree(a.native,root/'n');change(root);t=time.monotonic()
            try:compare(root/'o',root/'n')
            except(AssertionError,KeyError,ValueError)as e:passed.append({'control':name,'reason':str(e),'seconds':time.monotonic()-t})
            else:raise AssertionError('register directory mutation accepted: '+name)
    # Exercise the exact generated assertion; dormant entries retain their own
    # old model hash, and still fail if they actually become the frontier.
    program=ast.parse((a.native/'gdb-state.py').read_text());nodes=[n for n in ast.walk(program)if isinstance(n,ast.Assert)and isinstance(n.msg,ast.Constant)and n.msg.value=='actual frontier state model identity differs'];assert len(nodes)==1
    assertion=compile(ast.Module(body=nodes,type_ignores=[]),'<captured-state-model-assert>','exec');config=json.loads((a.native/'configuration.json').read_text());current=next(r for r in config['stubs']if r['name']=='_new_register_block');exec(assertion,{'records':[current]})
    for label,record in [('actual path mismatch',dict(current,state_model='scripts/other.py')),('actual hash mismatch',dict(current,state_model_sha256='0'*64))]+[('dormant '+r['name'],r)for r in config['stubs']if r.get('state_policy')=='globals-complete'and r['name']!=current['name']]:
        try:exec(assertion,{'records':[record]})
        except AssertionError:passed.append({'control':label,'reason':'exact actual-frontier decoder assertion rejects'})
        else:raise AssertionError('stale decoder accepted '+label)
    a.output.write_text(json.dumps({'positive':positive,'count':len(passed),'controls':passed,'seconds':time.monotonic()-start,'checker_sha256':sha(Path(__file__).read_bytes()),'original_receipt_sha256':sha((a.original/'receipt.json').read_bytes()),'native_receipt_sha256':sha((a.native/'receipt.json').read_bytes()),'scope':'Actual noarg new_register_block/initialize_function_records caller and full typed1000-pointer4000B allocation in pool1. Current natural first-child boundary only; no claimed constructor execution or static-table unknown data.'},indent=2)+'\n')
if __name__=='__main__':main()
