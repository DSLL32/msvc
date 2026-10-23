"""Actual queued debug descriptor formal, parent fields and cleanup ABI."""
from pathlib import Path
import copy,hashlib,importlib.util,json,struct,time
from pe_indexes import indexes
started=time.perf_counter();pe,canonical,config,left,right=indexes();row=next(r for r in config['driver']if r['name']=='@write_debug_symbol@4');path=Path(row['abi']['entry_observation']);receipt=json.loads(path.read_text());binding=json.loads(path.with_name('original-binding.json').read_text());sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
assert receipt['binding']==binding and all(sha(path.parent/n)==value for n,value in binding['files'].items())
def validate(r,metadata):
    assert r['matched']and r['function']==metadata['name']and r['original_va']==hex(metadata['start'])
    assert r['hidden_inputs']==metadata['abi']['hidden_inputs']==[]and r['callee_stack_cleanup']==metadata['abi']['callee_stack_cleanup']==0 and r['no_hidden_inputs_proof']
    assert len(r['formals'])==1 and [{k:p[k]for k in ['name','location','type']}for p in r['formals']]==metadata['abi']['parameters']==[dict(name='symbol',location='ECX',type='SymbolHeader pointer')]
    p=r['formals'][0];assert p['original']==r['original_aliases']['head_symbol']and p['native']==r['native']['head_symbol']==r['native']['pointer']and p['original']and p['native']
    assert r['descriptor_projection']==dict(kind=3,tag=31,flags=0,type=0,output_flags=0,identifier=118,offset=180,value=0,named=False)
    for raw in [r['original_descriptor_hex'],r['native']['descriptor_hex']]:
        b=bytes.fromhex(raw);assert len(b)==44 and b[12:16]==bytes([3,31,0,0])and b[32:36]==bytes(4)and struct.unpack_from('<I',b,16)[0]==0 and struct.unpack_from('<I',b,28)[0]==118 and struct.unpack_from('<I',b,4)[0]==180
    assert r['name_hex']==r['native']['name_hex']==''
    for state in [r['original_aliases'],r['native']]:assert state['cursor']and state['record']and state['allocation']==state['optimize']==0
    assert r['native']['stdout']=={k:v for k,v in r['original_stdout'].items()if k not in ['counter','cursor','value','position']}
    assert {k:r['native'][k]for k in ['column','cw','sw']}=={k:r['original_state'][k]for k in ['column','cw','sw']}
validate(receipt,row);rejected=[]
for label,change in [('missing formal',lambda r:r.update(formals=[])),('wrong formal register',lambda r:r['formals'][0].update(location='EDX')),('broken record alias',lambda r:r['native'].update(head_symbol=0)),('unknown hidden input',lambda r:r.update(hidden_inputs=['EDI?'])),('changed encountered tag',lambda r:r['descriptor_projection'].update(tag=0)),('wrong global override',lambda r:r['native'].update(allocation=4))]:
    bad=copy.deepcopy(receipt);change(bad)
    try:validate(bad,row)
    except(AssertionError,KeyError):rejected.append(label)
    else:raise AssertionError('Accepted bad debug descriptor input: '+label)
spec=importlib.util.spec_from_file_location('debug_symbol_abi_graph',Path('scripts/audit-graph.py'));g=importlib.util.module_from_spec(spec);spec.loader.exec_module(g);base=g.FunctionDecoder;decoders=[]
class Decoder(base):
    def __init__(self,*args):super().__init__(*args);self.visited={};decoders.append(self)
    def instruction(self,a):
        value=super().instruction(a)
        if value is not None:self.visited[a]=value
        return value
g.FunctionDecoder=Decoder;returns=[]
for image,records in [(pe,left),(canonical,right)]:
    record=next(r for r in records if r['name']==row['name']);g.edges(image,records,record);sites=[dict(site=hex(a),cleanup=int(op,0)if op else 0)for a,size,mn,op in decoders[-1].visited.values()if mn.split()[-1]=='ret'];assert sites and all(s['cleanup']==0 for s in sites);returns.append(sites)
Path('cases/debug-symbol-abi-validation.json').write_text(json.dumps(dict(formals=receipt['formals'],returns=returns,rejected=rejected,seconds=time.perf_counter()-started,scope='Actual first queued record identity, complete parent-read fields for encountered kind3, override/optimize/defined stdio/x87 and original/canonical CFG cleanup. This unnamed tag31 label is suppressed; no actual cold alias/leaf writer behavior claimed. Raw44-byte input retained without normalizing unrelated label instruction/hash pointers.'),indent=2)+'\n');print('1 actual descriptor formal,6 negatives and original/canonical cleanup pass')
