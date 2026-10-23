"""Actual COFF scalar formals and CFG RET cleanup, with hostile metadata tests."""
from pathlib import Path
import copy,hashlib,importlib.util,json,time
from pe_indexes import indexes
started=time.perf_counter();pe,canonical,config,left,right=indexes()
spec=importlib.util.spec_from_file_location('coff_scalar_graph',Path('scripts/audit-graph.py'));g=importlib.util.module_from_spec(spec);spec.loader.exec_module(g);base=g.FunctionDecoder;decoders=[]
class Decoder(base):
    def __init__(self,*args):super().__init__(*args);self.visited={};decoders.append(self)
    def instruction(self,a):
        value=super().instruction(a)
        if value is not None:self.visited[a]=value
        return value
g.FunctionDecoder=Decoder
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def validate(receipt,metadata):
    abi=metadata['abi'];forms=receipt['formals'];assert forms and receipt['matched']and receipt['no_hidden_inputs_proof']
    assert receipt['function']in [metadata['name'],abi.get('observed_legacy_logical')]
    assert receipt['hidden_inputs']==abi['hidden_inputs']==[]and receipt['callee_stack_cleanup']==abi['callee_stack_cleanup']
    assert [{k:p[k]for k in ['name','location','type']}for p in forms]==abi['parameters']
    assert [p['value']for p in forms]==receipt['original_values']==receipt['native']['values']
    for i,p in enumerate(forms):
        assert p['location']==('ECX'if i==0 else'EDX'if i==1 else'entryESP+%d'%(4*(i-1)))
        bits=16 if p['type'].endswith('16')else 32;v=receipt['original_raw_values'][i]&((1<<bits)-1)
        if p['type'].startswith('signed')and v>=(1<<(bits-1)):v-=1<<bits
        assert p['value']==v
checked=[];rejected=[]
for name in ['@coff_begin_lines@16','@coff_output_debug_record@8','@coff_record_line@8','@coff_line_marker@4','@coff_end_lines@8']:
    row=next(r for r in config['driver']if r['name']==name);path=Path(row['abi']['entry_observation']);receipt=json.loads(path.read_text());binding=json.loads(path.with_name('original-binding.json').read_text())
    assert all(sha(path.parent/n)==digest for n,digest in binding['files'].items())
    assert receipt['config_sha256']==binding['config_sha256']and receipt['target_sha256']==binding['target_sha256']
    validate(receipt,row);returns=[]
    for image,records in [(pe,left),(canonical,right)]:
        r=next(r for r in records if r['name']==name);g.edges(image,records,r)
        sites=[dict(site=hex(a),cleanup=int(op,0)if op else 0)for a,size,mn,op in decoders[-1].visited.values()if mn.split()[-1]=='ret']
        assert sites and all(s['cleanup']==row['abi']['callee_stack_cleanup']for s in sites);returns.append(sites)
    checked.append(dict(function=name,formals=receipt['formals'],returns=returns))
    for label,change in [('missing formal',lambda r:r.update(formals=[])),('unknown hidden register',lambda r:r.update(hidden_inputs=['ESI?'])),('wrong input value',lambda r:r['native']['values'].__setitem__(0,0xdeadbeef)),('wrong input register',lambda r:r['formals'][0].update(location='EDX')),('wrong width',lambda r:r['formals'][0].update(type='signed32')),('wrong cleanup',lambda r:r.update(callee_stack_cleanup=99))]:
        bad=copy.deepcopy(receipt);change(bad)
        try:validate(bad,row)
        except(AssertionError,KeyError):rejected.append(name+': '+label)
        else:raise AssertionError('Accepted bad actual COFF ABI: '+label)
# The CodeView callback consumes one pointer, not an address-valued scalar.
row=next(r for r in config['driver']if r['name']=='@end_debug_function@4');path=Path(row['abi']['entry_observation']);receipt=json.loads(path.read_text());binding=json.loads(path.with_name('original-binding.json').read_text())
assert binding==receipt['binding']and all(sha(path.parent/n)==digest for n,digest in binding['files'].items())
def validate_pointer(r):
    assert r['matched']and r['function']==row['name']and r['original_va']=='0x69250'and r['observed_legacy_logical']==row['abi']['observed_legacy_logical']
    assert r['hidden_inputs']==[]and r['callee_stack_cleanup']==0 and r['no_hidden_inputs_proof']
    assert [{k:p[k]for k in ['name','location','type']}for p in r['formals']]==row['abi']['parameters']
    formal=r['formals'][0];assert formal['original']==r['original_aliases']['current_symbol']and formal['native']==r['native']['current_symbol']==r['native']['pointer']and formal['original']and formal['native']
    projection=r['symbol_projection'];assert projection==dict(kind=4,storage=0x50,attributes=24657,identifier=117,offset=0,code_bytes=185,prologue=9,epilogue=180)
    import struct
    for data in [bytes.fromhex(r['original_symbol_hex']),bytes.fromhex(r['native']['symbol_hex'])]:
        assert len(data)==84 and data[12]==4 and struct.unpack_from('<I',data,36)[0]==projection['attributes']
    for data,pointer,scope in [(r['original_function_hex'],formal['original'],r['original_aliases']['scope']),(r['native']['function_hex'],formal['native'],r['native']['scope'])]:
        raw=bytes.fromhex(data);assert len(raw)==44 and struct.unpack_from('<I',raw,4)[0]==pointer and struct.unpack_from('<I',raw,36)[0]==scope and scope
    assert r['name_hex']==r['native']['symbol_name_hex']==b'_transform_debug_record\0'.hex()
    assert r['native']['stdout']=={k:v for k,v in r['original_stdout'].items()if k not in ['counter','cursor','value','position']}
    assert {k:r['native'][k]for k in ['column','cw','sw']}=={k:r['original_state'][k]for k in ['column','cw','sw']}
validate_pointer(receipt);returns=[]
for image,records in [(pe,left),(canonical,right)]:
    record=next(r for r in records if r['name']==row['name']);g.edges(image,records,record);sites=[dict(site=hex(a),cleanup=int(op,0)if op else 0)for a,size,mn,op in decoders[-1].visited.values()if mn.split()[-1]=='ret'];assert sites and all(s['cleanup']==0 for s in sites);returns.append(sites)
checked.append(dict(function=row['name'],formals=receipt['formals'],returns=returns))
for label,change in [('missing pointer formal',lambda r:r.update(formals=[])),('broken symbol alias',lambda r:r['native'].update(current_symbol=0)),('changed parent storage',lambda r:r['symbol_projection'].update(storage=0x30)),('unknown pointer input',lambda r:r.update(hidden_inputs=['EDX unknown'])),('wrong symbol name',lambda r:r.update(name_hex='00'))]:
    bad=copy.deepcopy(receipt);change(bad)
    try:validate_pointer(bad)
    except(AssertionError,KeyError,IndexError):rejected.append(row['name']+': '+label)
    else:raise AssertionError('Accepted bad pointer ABI: '+label)
Path('cases/coff-scalar-abi-validation.json').write_text(json.dumps(dict(checked=checked,rejected=rejected,seconds=time.perf_counter()-started,scope='All actual scalar formals, no-hidden-input proof, immutable original raw binding and original/canonical CFG RET cleanup. Legacy logical spelling only allowed by explicit same-VA callback correction metadata. No blanket unvisited input equivalence.'),indent=2)+'\n');print(sum(len(r['formals'])for r in checked),'actual scalar formals and',len(rejected),'rejections pass')
