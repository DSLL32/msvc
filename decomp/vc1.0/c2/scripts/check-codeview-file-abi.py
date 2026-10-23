"""Actual db FILE formal/owner and defined stream inputs, with rejects."""
from pathlib import Path
import copy,hashlib,importlib.util,json,struct,time
from pe_indexes import indexes
started=time.perf_counter();pe,canonical,config,left,right=indexes();row=next(r for r in config['driver']if r['name']=='@read_codeview_initializers@4');path=Path(row['abi']['entry_observation']);receipt=json.loads(path.read_text());binding=json.loads(path.with_name('original-binding.json').read_text());sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest();assert receipt['binding']==binding and all(sha(path.parent/n)==v for n,v in binding['files'].items())
expected=dict(flags=1,count=0,fd=8,size=0,null=1,used=0,valid=0,position=0,osfile=1,file_bytes=55,file_type=1,temporary_null=1,buffer_hex='')
def validate(r,metadata):
    assert r['matched']and r['function']==metadata['name']and r['original_va']==hex(metadata['start'])and r['no_hidden_inputs_proof']
    assert r['hidden_inputs']==metadata['abi']['hidden_inputs']==[]and r['callee_stack_cleanup']==metadata['abi']['callee_stack_cleanup']==0
    assert [{k:p[k]for k in ['name','location','type']}for p in r['formals']]==metadata['abi']['parameters']==[dict(name='file',location='ECX',type='input FILE pointer')]
    formal=r['formals'][0];a=r['original_file'];b=r['native']['file'];assert formal['original']==a['stream']==a['owner']and formal['native']==b['stream']==b['owner']and formal['original']and formal['native']
    for f in [a,b]:
        assert {k:f[k]for k in expected}==expected
        values=struct.unpack('<8I',bytes.fromhex(f['file_hex']));assert len(values)==8 and values[:5]==(0,0,0,1,8)and values[6:]==(0,0)
    assert b['host_path'].endswith('/irdb')
    assert r['native']['stdout']=={k:v for k,v in r['original_stdout'].items()if k not in ['counter','cursor','value','position']}
    assert {k:r['native'][k]for k in ['column','cw','sw']}=={k:r['original_state'][k]for k in ['column','cw','sw']}
validate(receipt,row);rejected=[]
for label,change in [('missing FILE formal',lambda r:r.update(formals=[])),('wrong FILE register',lambda r:r['formals'][0].update(location='EDX')),('broken FILE ownership',lambda r:r['native']['file'].update(owner=0)),('wrong count',lambda r:r['native']['file'].update(count=1)),('text translation instead of binary',lambda r:r['native']['file'].update(osfile=0x81)),('wrong descriptor position',lambda r:r['native']['file'].update(position=1)),('wrong input size',lambda r:r['native']['file'].update(file_bytes=54)),('unknown hidden output pointer',lambda r:r.update(hidden_inputs=['out?'])),('invented buffered byte',lambda r:r['native']['file'].update(buffer_hex='00'))]:
    bad=copy.deepcopy(receipt);change(bad)
    try:validate(bad,row)
    except(AssertionError,KeyError):rejected.append(label)
    else:raise AssertionError('Accepted bad db FILE input: '+label)
spec=importlib.util.spec_from_file_location('codeview_file_abi_graph',Path('scripts/audit-graph.py'));g=importlib.util.module_from_spec(spec);spec.loader.exec_module(g);base=g.FunctionDecoder;decoders=[]
class Decoder(base):
    def __init__(self,*args):super().__init__(*args);self.visited={};decoders.append(self)
    def instruction(self,a):
        v=super().instruction(a)
        if v is not None:self.visited[a]=v
        return v
g.FunctionDecoder=Decoder;returns=[]
for image,records in [(pe,left),(canonical,right)]:
    record=next(r for r in records if r['name']==row['name']);g.edges(image,records,record);sites=[dict(site=hex(a),cleanup=int(op,0)if op else 0)for a,size,mn,op in decoders[-1].visited.values()if mn.split()[-1]=='ret'];assert sites and all(s['cleanup']==0 for s in sites);returns.append(sites)
Path('cases/codeview-file-abi-validation.json').write_text(json.dumps(dict(formals=receipt['formals'],returns=returns,defined_file=expected,rejected=rejected,seconds=time.perf_counter()-started,scope='Actual db FILE owner, defined stream/lowio/file position/size/type/temporary-null/buffer fields and original/canonical cleanup. Raw32-byte FILEs preserved; unused char-buffer contents and host pointer/handle values not equated. No injected target calls or CRT/error/parser behavior acceptance. Fixed input bytes remain manifest-hashed.'),indent=2)+'\n');print('1 actual db FILE formal/defined state and9 negatives/cleanup pass')
