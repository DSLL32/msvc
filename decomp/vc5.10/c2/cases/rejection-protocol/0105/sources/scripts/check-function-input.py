#!/usr/bin/env python3
"""Actual noarg input frontier and explicit controller function-selection state."""
import argparse,json,re,shutil,tempfile,time
from pathlib import Path
from compare import compare
from state import sha

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('original','native','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();start=time.monotonic();positive=compare(a.original,a.native);passed=[]
    def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n')
    def native(root,field,change):
        path=root/'n/state.json';v=json.loads(path.read_text());before=json.dumps(v[field],sort_keys=True);change(v[field]);assert before!=json.dumps(v[field],sort_keys=True);write(path,v)
        prefix={'function_input':'C2-FUNCTION-INPUT','globals_complete':'C2-GLOBALS-COMPLETE'}[field];path=root/'n/gdb.stdout';text=path.read_text();assert len(re.findall('^'+prefix+' ',text,re.M))==1;path.write_text(re.sub('^'+prefix+r' .*$',lambda m:prefix+' '+json.dumps(v[field],sort_keys=True),text,flags=re.M))
        path=root/'n/receipt.json';r=json.loads(path.read_text());r['state_sha256']=sha((root/'n/state.json').read_bytes());r['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write(path,r)
    def original(root,file,index):
        path=root/'o'/file;raw=bytearray(path.read_bytes());raw[index]^=1;path.write_bytes(raw);path=root/'o/receipt.json';r=json.loads(path.read_text());r['globals_complete_hashes'][file]=sha(raw)
        if file=='unmodeled-original-data.bin':r['unmodeled_original_data']['sha256']=sha(raw)
        write(path,r)
    def event(root,pattern,replacement):
        path=root/'o/events.txt';raw=path.read_text();changed=re.sub(pattern,replacement,raw,flags=re.M);assert changed!=raw;path.write_text(changed);path=root/'o/receipt.json';r=json.loads(path.read_text());r['events_sha256']=sha((root/'o/events.txt').read_bytes());write(path,r)
    cases=[('noarg input invents formal',lambda r:native(r,'function_input',lambda v:v.__setitem__('formals',[{'name':'symbol','type':'C2Symbol *'}]))),
        ('caller noarg formal changed',lambda r:native(r,'function_input',lambda v:v.__setitem__('caller_formals',[{'name':'record','type':'void *'}]))),
        ('wrong actual controller',lambda r:native(r,'function_input',lambda v:v.__setitem__('caller','initialize_backend'))),
        ('selected first alias lost',lambda r:native(r,'function_input',lambda v:v.__setitem__('selected_first_alias',False))),
        ('selected serial changed',lambda r:native(r,'function_input',lambda v:v.__setitem__('selected_serial',92))),
        ('consumed flag not set',lambda r:native(r,'function_input',lambda v:v.__setitem__('selected_flags',40))),
        ('pending tail alias lost',lambda r:native(r,'function_input',lambda v:v.__setitem__('pending_next_alias',False))),
        ('volatile pass limit changed',lambda r:native(r,'function_input',lambda v:v.__setitem__('volatile_limit',64))),
        ('complete selection owner changed',lambda r:native(r,'globals_complete',lambda v:v['function_selection'].__setitem__('offset',776))),
        ('complete pending list retains selected head',lambda r:native(r,'globals_complete',lambda v:v['pending'].insert(0,[548,1132]))),
        ('original selection pointer changed',lambda r:original(r,'unmodeled-original-data.bin',0x495f1c-0x48e000)),
        ('original pending root changed',lambda r:original(r,'unmodeled-original-data.bin',0x4973d4-0x48e000)),
        ('original volatile limit changed',lambda r:original(r,'unmodeled-original-data.bin',0x499734-0x48e000)),
        ('original consumed flag changed',lambda r:original(r,'pool-block.bin',548+106)),
        ('original selected owner event changed',lambda r:event(r,r'^(function-input selected=)[0-9a-f]+',r'\g<1>00000000')),
        ('original actual controller flags changed',lambda r:event(r,r'^(function-input .* flags=)[0-9a-f]+',r'\g<1>00000028'))]
    with tempfile.TemporaryDirectory(prefix='function-input-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        for i,(name,change)in enumerate(cases):
            root=Path(folder)/str(i);shutil.copytree(a.original,root/'o');shutil.copytree(a.native,root/'n');change(root)
            try:compare(root/'o',root/'n')
            except(AssertionError,KeyError,ValueError)as e:passed.append({'control':name,'reason':str(e)})
            else:raise AssertionError('function selection mutation accepted: '+name)
    write(a.output,{'positive':positive,'count':len(passed),'rejected':passed,'seconds':time.monotonic()-start,'checker_sha256':sha(Path(__file__).read_bytes()),'original_receipt_sha256':sha((a.original/'receipt.json').read_bytes()),'native_receipt_sha256':sha((a.native/'receipt.json').read_bytes()),'scope':'Actual noarg begin_function_input and noarg compile_functions caller. Realcontroller selects mix_unsigned85, flags28->2a, retains next/pendingtail776, setsvolatilelimit128. Full4096defined arena, pointer aliases and output state compared; selected symbol is modeled by logical arena offset, never raw cross-process address equality.'})
if __name__=='__main__':main()
