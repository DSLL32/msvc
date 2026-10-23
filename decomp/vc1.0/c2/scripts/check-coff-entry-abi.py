"""Complete observed filename callback ABI and state; no process launches."""
from pathlib import Path
import copy,hashlib,json,time
started=time.perf_counter();path=Path('cases/dbglist-coff-pair-abi/validation.json');receipt=json.loads(path.read_text());binding=json.loads(Path('cases/dbglist-coff-pair-abi/original-binding.json').read_text())
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
assert all(sha(path.parent/name)==value for name,value in binding['files'].items())
assert receipt['config_sha256']==binding['config_sha256']and receipt['target_sha256']==binding['target_sha256']and receipt['manifest_sha256']==binding['manifest_sha256']
def validate(r):
    assert r['matched']and r['function']=='@coff_output_debug_pair@4'and r['observed_legacy_logical']=='_coff_output_debug_pair'and r['original_va']=='0x6d3b0'
    assert len(r['formals'])==1 and r['hidden_inputs']==[]and r['callee_stack_cleanup']==0 and r['no_hidden_inputs_proof']
    formal=r['formals'][0];assert {k:formal[k]for k in ['name','location','type']}==dict(name='filename',location='ECX',type='input char pointer')
    assert formal['defined_input_hex']==r['native']['filename_hex']==b'dbglist.c\0'.hex()
    assert formal['original']==r['original_coff']['source']==r['original_coff']['listing_source']and formal['original']
    assert formal['native']==r['native']['pointer']==r['native']['coff']['source']==r['native']['coff']['listing_source']and formal['native']
    for state in [r['original_coff'],r['native']['coff']]:assert {k:state[k]for k in ['emitted','codeview','line','pending']}==dict(emitted=1,codeview=1,line=0,pending=0)
    assert r['original_state']==dict(column=0,cw=0x27f,sw=0,fastcall=1)
    assert {k:r['native'][k]for k in ['column','cw','sw']}==dict(column=0,cw=0x27f,sw=0)
    assert r['native']['stdout']=={k:v for k,v in r['original_stdout'].items()if k not in ['counter','cursor','value','position']}
validate(receipt)
# The next stub is still unaccepted, but all four actual entry inputs must match.
entry_path=Path('cases/dbglist-begin-lines-abi/validation.json');entry=json.loads(entry_path.read_text())
entry_binding=json.loads(entry_path.with_name('original-binding.json').read_text())
assert all(sha(entry_path.parent/name)==value for name,value in entry_binding['files'].items())
def validate_next(r):
    assert r['matched']and r['function']=='@coff_begin_lines@16'and r['hidden_inputs']==[]and r['no_hidden_inputs_proof']and r['callee_stack_cleanup']==8
    expected=[('handle','ECX','unsigned32',10),('line','EDX','unsigned16',24),('bytes','entryESP+4','unsigned32',185),('offset','entryESP+8','unsigned32',0)]
    assert [(p['name'],p['location'],p['type'],p['value'])for p in r['formals']]==expected
    assert r['original_values']==r['native']['values']==[10,24,185,0]
    assert r['native']['stdout']=={k:v for k,v in r['original_stdout'].items()if k not in ['counter','cursor','value','position']}
    assert {k:r['native'][k]for k in ['column','cw','sw']}==dict(column=0,cw=0x27f,sw=0)
validate_next(entry)
config=json.loads(Path('functions.json').read_text())
def validate_metadata(cfg):
    row=next(r for r in cfg['driver']if r['name']==receipt['function']);abi=row['abi']
    assert row['start']==0x6d3b0 and row['end']==0x6d3ec and abi['convention']=='fastcall'and abi['hidden_inputs']==[]and abi['callee_stack_cleanup']==0
    assert abi['parameters']==[dict(name='filename',location='ECX',type='input char pointer')]and abi['entry_observation']==str(path)
validate_metadata(config);rejected=[]
for label,change in [('missing filename formal',lambda r:r.update(formals=[])),('wrong filename input register',lambda r:r['formals'][0].update(location='EDX')),('changed filename bytes',lambda r:r['native'].update(filename_hex='00')),('broken source alias',lambda r:r['native']['coff'].update(source=0)),('wrong emitted guard state',lambda r:r['native']['coff'].update(emitted=0)),('unknown hidden input',lambda r:r.update(hidden_inputs=['ESI unknown'])),('wrong callback stack cleanup',lambda r:r.update(callee_stack_cleanup=4))]:
    bad=copy.deepcopy(receipt);change(bad)
    try:validate(bad)
    except(AssertionError,KeyError):rejected.append(label)
    else:raise AssertionError('Accepted bad filename ABI: '+label)
bad=copy.deepcopy(config);next(r for r in bad['driver']if r['name']==receipt['function'])['abi']['parameters']=[]
try:validate_metadata(bad)
except(AssertionError,KeyError):rejected.append('empty driver filename metadata')
else:raise AssertionError('Accepted empty filename metadata')
for label,change in [('missing next formal',lambda r:r.update(formals=[])),('wrong next handle',lambda r:r['native']['values'].__setitem__(0,12)),('wrong next line width',lambda r:r['formals'][1].update(type='unsigned32')),('wrong next offset slot',lambda r:r['formals'][3].update(location='entryESP+4')),('unknown next hidden register',lambda r:r.update(hidden_inputs=['EDI unknown']))]:
    bad=copy.deepcopy(entry);change(bad)
    try:validate_next(bad)
    except(AssertionError,KeyError):rejected.append(label)
    else:raise AssertionError('Accepted bad begin-lines ABI: '+label)
Path('cases/coff-entry-abi-validation.json').write_text(json.dumps(dict(observed_formals=5,next_function=entry['function'],function=receipt['function'],rejected=rejected,seconds=time.perf_counter()-started,scope='Actual original/native filename, NUL bytes, source/listing aliases and known guard/stdio/x87 state. Original raw capture hashes remain immutable; legacy decoration correction preserves C identity/VA. Initial guard false, no cold-child execution claim. No Wine/compiler/native launch.'),indent=2)+'\n');print('5 actual filename/line formals, aliases/state and',len(rejected),'negatives pass')
