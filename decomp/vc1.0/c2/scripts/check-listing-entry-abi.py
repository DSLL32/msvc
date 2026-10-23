"""Actual-case listing formal/known hidden-input completeness and negatives."""
from pathlib import Path
import copy,json,time
started=time.perf_counter();path=Path('cases/dbglist-bytes-abi/validation.json');receipt=json.loads(path.read_text())
def validate(r):
    assert r['matched']and len(r['original'])==len(r['formals'])==2
    first,second=r['formals'];assert first['function']=='@write_listing_bytes@12'and second['function']=='@write_listing_spaces@4'
    assert [p['name']for p in first['parameters']]==['offset','count','bytes']and [p['name']for p in second['parameters']]==['count']
    assert [p['location']for p in first['parameters']]==['ECX','EDX','entryESP+4']and second['parameters'][0]['location']=='ECX'
    assert first['hidden_inputs']==second['hidden_inputs']==[]and first['callee_stack_cleanup']==4 and second['callee_stack_cleanup']==0
    assert [p['type']for p in first['parameters']]==['unsigned32','signed32','input byte pointer']and second['parameters'][0]['type']=='signed32'
    assert first['parameters'][0]['value']==0 and first['parameters'][1]['value']==second['parameters'][0]['value']==1
    originals=r['original'];a=r['native_bytes_arguments'];assert originals[0]['args'][:2]==[a['offset'],a['count']]==[0,1]and len(originals[0]['args'])==3
    assert originals[0]['args'][2]==first['parameters'][2]['original']and a['pointer']==first['parameters'][2]['native']and a['pointer']and originals[0]['args'][2]
    assert originals[0]['extra']['bytes_hex']==a['bytes_hex']==first['parameters'][2]['defined_input_bytes']=='55'
    assert originals[1]['args']==[r['native_spaces_count']]==[1]
    for row,native,column in zip(originals,[r['native_bytes_state'],r['native_spaces_state']],[0,7]):
        assert row['state']==dict(column=column,cw=0x27f,sw=0,fastcall=1)
        assert {k:native[k]for k in ['column','cw','sw']}==dict(column=column,cw=0x27f,sw=0)
        assert native['stdout']=={k:v for k,v in row['stdout'].items()if k not in ['counter','cursor','value','position']}
def validate_metadata(config):
    records={r['name']:r for r in config['driver']}
    for formal in receipt['formals']:
        abi=records[formal['function']]['abi']
        assert abi['convention']=='fastcall'and abi['hidden_inputs']==formal['hidden_inputs']==[]
        assert abi['callee_stack_cleanup']==formal['callee_stack_cleanup']
        assert abi['parameters']==[{k:v for k,v in p.items()if k in ['name','location','type']}for p in formal['parameters']]
        assert abi['entry_observation']==str(path)
validate(receipt);config=json.loads(Path('functions.json').read_text());validate_metadata(config);rejected=[]
for label,change in [('empty formal metadata',lambda r:r['formals'][0].update(parameters=[])),('omitted stack byte pointer',lambda r:r['formals'][0]['parameters'].pop()),('wrong fastcall register',lambda r:r['formals'][0]['parameters'][1].update(location='EAX')),('unobserved hidden input',lambda r:r['formals'][0].update(hidden_inputs=['ESI unknown'])),('wrong callee stack cleanup',lambda r:r['formals'][0].update(callee_stack_cleanup=0)),('changed actual count',lambda r:r['native_bytes_arguments'].update(count=2)),('changed pointed input byte',lambda r:r['native_bytes_arguments'].update(bytes_hex='54')),('omitted next formal',lambda r:r['formals'][1].update(parameters=[])),('wrong actual column',lambda r:r['native_spaces_state'].update(column=8))]:
    bad=copy.deepcopy(receipt);change(bad)
    try:validate(bad)
    except(AssertionError,KeyError):rejected.append(label)
    else:raise AssertionError('Accepted bad ABI observation: '+label)
bad=copy.deepcopy(config);next(r for r in bad['driver']if r['name']=='@write_listing_bytes@12')['abi']['parameters']=[]
try:validate_metadata(bad)
except(AssertionError,KeyError):rejected.append('empty driver formal metadata')
else:raise AssertionError('Accepted empty driver ABI metadata')
instruction=json.loads(Path('cases/dbglist-instruction-abi/validation.json').read_text())
def validate_instruction(r):
    assert r['matched']and r['function']in ['@write_instruction_text@4','@write_special_instruction@4','@write_instruction_mnemonic@4']
    assert len(r['formals'])==1 and r['hidden_inputs']==[]and r['callee_stack_cleanup']==0
    f=r['formals'][0];assert {k:f[k]for k in ['name','location','type']}==dict(name='instruction',location='ECX',type='MachineInstruction pointer')
    a,b=r['original_words'],r['native_words'];assert a[2:]==b[2:]==[0xd40,1,1,5,0,0]
    assert f['original']and f['native']==r['native']['pointer']
    assert a[:2]==[f['original']+32,f['original']-32]and b[:2]==[f['native']+32,f['native']-32]
    assert r['original_state']==dict(column=24,cw=0x27f,sw=0,fastcall=1)
    assert {k:r['native'][k]for k in ['column','cw','sw']}==dict(column=24,cw=0x27f,sw=0)
    assert r['native']['stdout']=={k:v for k,v in r['original_stdout'].items()if k not in ['counter','cursor','value','position']}
validate_instruction(instruction)
instruction_receipts=[('cases/dbglist-instruction-abi/validation.json',instruction)]
for path in ['cases/dbglist-special-abi/validation.json','cases/dbglist-mnemonic-abi/validation.json']:
    entry=json.loads(Path(path).read_text());validate_instruction(entry);instruction_receipts.append((path,entry))
for path,entry in instruction_receipts:
    metadata=next(r for r in config['driver']if r['name']==entry['function'])
    if entry['function']in config['accepted']:
        abi=metadata['abi'];assert abi['convention']=='fastcall'and abi['hidden_inputs']==[]and abi['callee_stack_cleanup']==0
        assert abi['parameters']==[{k:v for k,v in entry['formals'][0].items()if k in ['name','location','type']}]
        assert abi['entry_observation']==path
for label,change in [('missing instruction formal',lambda r:r.update(formals=[])),('unobserved instruction register',lambda r:r.update(hidden_inputs=['EDX unknown'])),('changed instruction operand',lambda r:r['native_words'].__setitem__(5,4)),('broken instruction link identity',lambda r:r['native_words'].__setitem__(0,0))]:
    bad=copy.deepcopy(instruction);change(bad)
    try:validate_instruction(bad)
    except(AssertionError,KeyError):rejected.append(label)
    else:raise AssertionError('Accepted bad instruction ABI: '+label)
operand=json.loads(Path('cases/dbglist-operand-abi/validation.json').read_text())
def validate_operand(r):
    assert r['matched']and r['function']=='@write_instruction_operand@12'and r['hidden_inputs']==[]and r['callee_stack_cleanup']==4
    assert [p['name']for p in r['formals']]==['instruction','operand','bytes']and [p['location']for p in r['formals']]==['ECX','EDX','entryESP+4']
    f,o,w=r['formals'];n=r['native_operand'];assert f['original']and f['native']==r['native']['pointer']
    assert o['original']==f['original']+16 and o['native']==f['native']+16==n['pointer']
    assert o['original_offset_from_instruction']==o['native_offset_from_instruction']==16
    assert o['defined_input_hex']==n['operand_hex']=='0100000005000000'and n['bytes']==w['value']==4
    a,b=r['original_words'],r['native_words'];assert a[2:]==b[2:]==[0xd40,1,1,5,0,0]
    assert a[:2]==[f['original']+32,f['original']-32]and b[:2]==[f['native']+32,f['native']-32]
    assert r['original_state']==dict(column=32,cw=0x27f,sw=0,fastcall=1)
    assert {k:r['native'][k]for k in ['column','cw','sw']}==dict(column=32,cw=0x27f,sw=0)
    assert r['native']['stdout']=={k:v for k,v in r['original_stdout'].items()if k not in ['counter','cursor','value','position']}
validate_operand(operand)
for label,change in [('missing operand width formal',lambda r:r['formals'].pop()),('wrong operand pointer alias',lambda r:r['formals'][1].update(native_offset_from_instruction=24)),('wrong actual operand width',lambda r:r['native_operand'].update(bytes=8)),('unknown hidden operand input',lambda r:r.update(hidden_inputs=['ESI unknown']))]:
    bad=copy.deepcopy(operand);change(bad)
    try:validate_operand(bad)
    except(AssertionError,KeyError,ValueError):rejected.append(label)
    else:raise AssertionError('Accepted bad operand ABI: '+label)
Path('cases/listing-entry-abi-validation.json').write_text(json.dumps(dict(checked=[r['function']for r in receipt['formals']]+[r['function']for p,r in instruction_receipts]+[operand['function']],observed_formals=7+len(instruction_receipts),rejected=rejected,seconds=time.perf_counter()-started,scope='Actual original/native byte, spacing, instruction text/special/mnemonic and next operand entries. Complete known formal observations plus static no-hidden-input/RET evidence. First PUSH EBP scalar/adjacent-link and operand alias projection only; no arbitrary deep pointer equivalence. No Wine/compiler/native launch.'),indent=2)+'\n')
print(7+len(instruction_receipts),'actual ABI formals, known implicit state and',len(rejected),'rejections pass')
