"""Actual queued CodeView leaf input identities and consumed state."""
from pathlib import Path
import copy,hashlib,importlib.util,json,struct,time
from pe_indexes import indexes
started=time.perf_counter();pe,canonical,config,left,right=indexes();sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def validate(r,metadata):
    assert r['matched']and r['function']==metadata['name']and r['original_va']==hex(metadata['start'])and r['no_hidden_inputs_proof']
    assert r['hidden_inputs']==metadata['abi']['hidden_inputs']==[]and r['callee_stack_cleanup']==metadata['abi']['callee_stack_cleanup']==0
    assert [{k:p[k]for k in ['name','location','type']}for p in r['formals']]==metadata['abi']['parameters']==[dict(name='symbol',location='ECX',type='SymbolHeader pointer')]
    p=r['formals'][0];assert p['original']==r['original_aliases']['record_symbol']and p['native']==r['native']['record_symbol']==r['native']['pointer']and p['original']and p['native']
    assert r['descriptor_projection']==expected
    for raw in [r['original_descriptor_hex'],r['native']['descriptor_hex']]:
        b=bytes.fromhex(raw);assert len(b)==44 and b[12:16]==struct.pack('<BBH',expected['kind'],expected['tag'],expected['flags'])and b[32:36]==struct.pack('<HH',expected['type'],expected['output_flags'])and struct.unpack_from('<I',b,4)[0]==expected['offset'] and struct.unpack_from('<I',b,28)[0]==expected['identifier'] and struct.unpack_from('<I',b,16)[0]
    if 'attributes'in expected:
        for raw in [r['original_descriptor_hex'],r['native']['descriptor_hex']]:assert struct.unpack_from('<I',bytes.fromhex(raw),36)[0]==expected['attributes']
    assert r['name_hex']==r['native']['name_hex']==name_bytes.hex()
    for s in [r['original_aliases'],r['native']]:assert s['cursor']and s['record']and s['record_index']==record_index and s['allocation']==allocation and s['optimize']==s['byte_first']==0 and s['coff']==1
    assert r['function_flags']==r['native']['function_flags']==9
    assert r['native']['stdout']=={k:v for k,v in r['original_stdout'].items()if k not in ['counter','cursor','value','position']}
    assert {k:r['native'][k]for k in ['column','cw','sw']}=={k:r['original_state'][k]for k in ['column','cw','sw']}
spec=importlib.util.spec_from_file_location('codeview_leaf_abi_graph',Path('scripts/audit-graph.py'));g=importlib.util.module_from_spec(spec);spec.loader.exec_module(g);base=g.FunctionDecoder;decoders=[]
class Decoder(base):
    def __init__(self,*args):super().__init__(*args);self.visited={};decoders.append(self)
    def instruction(self,a):
        v=super().instruction(a)
        if v is not None:self.visited[a]=v
        return v
g.FunctionDecoder=Decoder;checked=[];rejected=[]
profiles=[('@write_codeview_label@4',dict(kind=3,tag=0,flags=0,type=0,output_flags=7,identifier=121,offset=28,value=0,named=True),b'$mixed_nonzero\0',1,0),('@write_codeview_frame_variable@4',dict(kind=1,tag=0,flags=2304,type=4097,output_flags=7,identifier=115,offset=8,value=4,named=True,attributes=16947),b'_record\0',0,4)]
for name,expected,name_bytes,record_index,allocation in profiles:
    row=next(r for r in config['driver']if r['name']==name);path=Path(row['abi']['entry_observation']);r=json.loads(path.read_text());binding=json.loads(path.with_name('original-binding.json').read_text());assert r['binding']==binding and all(sha(path.parent/n)==v for n,v in binding['files'].items())
    validate(r,row)
    for label,change in [('missing formal',lambda r:r.update(formals=[])),('wrong register',lambda r:r['formals'][0].update(location='EDX')),('broken list member',lambda r:r['native'].update(record_symbol=0)),('wrong list position',lambda r:r['native'].update(record_index=record_index+1)),('changed label offset',lambda r:r['descriptor_projection'].update(offset=0)),('unknown hidden pointer',lambda r:r.update(hidden_inputs=['stack out?'])),('changed COFF mode',lambda r:r['native'].update(coff=0))]:
        bad=copy.deepcopy(r);change(bad)
        try:validate(bad,row)
        except(AssertionError,KeyError):rejected.append(label)
        else:raise AssertionError('Accepted bad leaf ABI: '+label)
    returns=[]
    for image,records in [(pe,left),(canonical,right)]:
        record=next(r for r in records if r['name']==row['name']);g.edges(image,records,record);sites=[dict(site=hex(a),cleanup=int(op,0)if op else 0)for a,size,mn,op in decoders[-1].visited.values()if mn.split()[-1]=='ret'];assert sites and all(s['cleanup']==0 for s in sites);returns.append(sites)
    checked.append(dict(function=name,formals=r['formals'],returns=returns))
for zero_name,zero_start,zero_end in [('_begin_debug_function',0x69210,0x69249),('_finish_debug_module',0x69180,0x6920a),('_write_codeview_compile_record',0x69bd0,0x69c56)]:
    row=next(r for r in config['driver']if r['name']==zero_name);path=Path(row['abi']['entry_observation']);zero=json.loads(path.read_text());binding=json.loads(path.with_name('original-binding.json').read_text())
    assert all(sha(path.parent/n)==v for n,v in binding['files'].items())
    def validate_zero(r,metadata):
        assert metadata['name']==zero_name and metadata['start']==zero_start and metadata['end']==zero_end
        assert metadata['abi']['parameters']==[]and metadata['abi']['hidden_inputs']==[]and metadata['abi']['zero_input_proof']
        assert r['function']==metadata['name']and r['matched']and r['formals']==[]and r['hidden_inputs']==[]and r['callee_stack_cleanup']==metadata['abi']['callee_stack_cleanup']==0 and r['no_hidden_inputs_proof']
        assert r['original_raw_values']==r['original_values']==r['native']['values']==[]
        assert r['native']['stdout']=={k:v for k,v in r['original_stdout'].items()if k not in ['counter','cursor','value','position']}
        assert {k:r['native'][k]for k in ['column','cw','sw']}=={k:r['original_state'][k]for k in ['column','cw','sw']}
    validate_zero(zero,row);returns=[]
    for image,records in [(pe,left),(canonical,right)]:
        record=next(r for r in records if r['name']==row['name']);g.edges(image,records,record);sites=[dict(site=hex(a),cleanup=int(op,0)if op else 0)for a,size,mn,op in decoders[-1].visited.values()if mn.split()[-1]=='ret'];assert sites and all(s['cleanup']==0 for s in sites);returns.append(sites)
    checked.append(dict(function=row['name'],formals=[],returns=returns,zero_input_proof=row['abi']['zero_input_proof']))
    for label,change in [('invented zero-formal input',lambda r:r.update(formals=[dict(name='unknown')])),('unknown zero-formal register',lambda r:r.update(hidden_inputs=['ECX?'])),('missing zero-formal proof',lambda r:r.update(no_hidden_inputs_proof=''))]:
        bad=copy.deepcopy(zero);change(bad)
        try:validate_zero(bad,row)
        except(AssertionError,KeyError):rejected.append(label)
        else:raise AssertionError('Accepted incomplete zero-input proof: '+label)
Path('cases/codeview-leaf-abi-validation.json').write_text(json.dumps(dict(checked=checked,rejected=rejected,seconds=time.perf_counter()-started,scope='Actual named inline-asm label at current scope list index1, not an invented head alias. Defined44-byte prefix projection/name/COFF+byte-block/function flags/stdio/x87 and original/canonical cleanup. Unrelated hash/instruction addresses remain raw; no general deep alias or unvisited leaf claim.'),indent=2)+'\n');print(len(checked),'actual leaf pointer/list identities and',len(rejected),'negatives/cleanup pass')
