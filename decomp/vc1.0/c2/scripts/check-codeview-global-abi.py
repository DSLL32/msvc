"""Actual global S_UDT descriptor ownership/consumed fields and cleanup."""
from pathlib import Path
import copy,hashlib,importlib.util,json,struct,time
from pe_indexes import indexes
started=time.perf_counter();pe,canonical,config,left,right=indexes();sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def validate(r,metadata):
    assert r['matched']and r['function']==metadata['name']and r['original_va']==hex(metadata['start'])and r['no_hidden_inputs_proof']
    assert r['hidden_inputs']==metadata['abi']['hidden_inputs']==[]and r['callee_stack_cleanup']==metadata['abi']['callee_stack_cleanup']==0
    assert [{k:p[k]for k in ['name','location','type']}for p in r['formals']]==metadata['abi']['parameters']==[dict(name='symbol',location='ECX',type='SymbolHeader pointer')]
    p=r['formals'][0];assert p['original']==r['original_owner']['member']and p['native']==r['native']['pointer']==r['native']['member']and p['original']and p['native']
    for owner in [r['original_owner'],r['native']]:assert owner['bucket']==expected['identifier']&63 and owner['index']==0
    assert r['descriptor_projection']==expected
    for raw in [r['original_descriptor_hex'],r['native']['descriptor_hex']]:
        b=bytes.fromhex(raw);assert len(b)==44 and b[12]==expected['kind']and struct.unpack_from('<I',b,28)[0]==expected['identifier']and struct.unpack_from('<H',b,32)[0]==expected['type'] and struct.unpack_from('<I',b,16)[0]
    if name=='@write_codeview_local_data@4':
        for raw in [r['original_descriptor_hex'],r['native']['descriptor_hex']]:
            b=bytes.fromhex(raw);assert struct.unpack_from('<I',b,4)[0]==expected['offset']and struct.unpack_from('<H',b,14)[0]==expected['flags']
        for owner in [r['original_owner'],r['native']]:assert owner['coff']==1 and owner['byte_first']==0
    assert r['name_hex']==r['native']['name_hex']==name_bytes.hex()
    assert r['native']['stdout']=={k:v for k,v in r['original_stdout'].items()if k not in ['counter','cursor','value','position']}
    assert {k:r['native'][k]for k in ['column','cw','sw']}=={k:r['original_state'][k]for k in ['column','cw','sw']}
spec=importlib.util.spec_from_file_location('codeview_global_abi_graph',Path('scripts/audit-graph.py'));g=importlib.util.module_from_spec(spec);spec.loader.exec_module(g);base=g.FunctionDecoder;decoders=[]
class Decoder(base):
    def __init__(self,*args):super().__init__(*args);self.visited={};decoders.append(self)
    def instruction(self,a):
        v=super().instruction(a)
        if v is not None:self.visited[a]=v
        return v
g.FunctionDecoder=Decoder;checked=[];rejected=[]
profiles=[('@write_codeview_typedef@4',dict(kind=11,identifier=105,type=4117,named=True),b'_DebugValue\0'),('@write_codeview_local_data@4',dict(kind=1,identifier=113,type=4109,named=True,offset=0,flags=2432),b'_adjustments\0')]
for name,expected,name_bytes in profiles:
    row=next(r for r in config['driver']if r['name']==name);path=Path(row['abi']['entry_observation']);receipt=json.loads(path.read_text());binding=json.loads(path.with_name('original-binding.json').read_text());assert receipt['binding']==binding and all(sha(path.parent/n)==v for n,v in binding['files'].items())
    validate(receipt,row)
    for label,change in [('missing global pointer formal',lambda r:r.update(formals=[])),('wrong input register',lambda r:r['formals'][0].update(location='EDX')),('broken global member identity',lambda r:r['native'].update(member=0)),('wrong hash bucket',lambda r:r['native'].update(bucket=(expected['identifier']&63)^1)),('wrong typeWORD',lambda r:r['descriptor_projection'].update(type=0)),('changed type name',lambda r:r.update(name_hex='00')),('unknown global hidden input',lambda r:r.update(hidden_inputs=['stack?']))]:
        bad=copy.deepcopy(receipt);change(bad)
        try:validate(bad,row)
        except(AssertionError,KeyError):rejected.append(label)
        else:raise AssertionError('Accepted bad global descriptor ABI: '+label)
    returns=[]
    for image,records in [(pe,left),(canonical,right)]:
        record=next(r for r in records if r['name']==row['name']);g.edges(image,records,record);sites=[dict(site=hex(a),cleanup=int(op,0)if op else 0)for a,size,mn,op in decoders[-1].visited.values()if mn.split()[-1]=='ret'];assert sites and all(s['cleanup']==0 for s in sites);returns.append(sites)
    checked.append(dict(function=name,formals=receipt['formals'],returns=returns))
Path('cases/codeview-global-abi-validation.json').write_text(json.dumps(dict(checked=checked,rejected=rejected,seconds=time.perf_counter()-started,scope='Actual typedef descriptor in closed global_scope hash bucket41/member0; consumed typeWORD/name and defined identifier/kind, stdio/x87 observed. Raw44 bytes retained. Unused value/offset/attributes/padding are not normalized or asserted equal; no function-scope alias invented. No child behavior/failure acceptance.'),indent=2)+'\n');print(len(checked),'actual global descriptors/member/type and',len(rejected),'negatives/cleanup pass')
