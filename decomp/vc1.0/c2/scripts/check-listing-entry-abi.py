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
    assert r['matched']and r['function']in ['@write_instruction_text@4','@write_special_instruction@4','@write_instruction_mnemonic@4','@write_instruction_comment@4']
    assert len(r['formals'])==1 and r['hidden_inputs']==[]and r['callee_stack_cleanup']==0
    f=r['formals'][0];assert {k:f[k]for k in ['name','location','type']}==dict(name='instruction',location='ECX',type='MachineInstruction pointer')
    a,b=r['original_words'],r['native_words'];assert a[2:]==b[2:]==[0xd40,1,1,5,0,0]
    assert f['original']and f['native']==r['native']['pointer']
    assert a[:2]==[f['original']+32,f['original']-32]and b[:2]==[f['native']+32,f['native']-32]
    column=35 if r['function']=='@write_instruction_comment@4'else 24
    assert r['original_state']==dict(column=column,cw=0x27f,sw=0,fastcall=1)
    assert {k:r['native'][k]for k in ['column','cw','sw']}==dict(column=column,cw=0x27f,sw=0)
    assert r['native']['stdout']=={k:v for k,v in r['original_stdout'].items()if k not in ['counter','cursor','value','position']}
validate_instruction(instruction)
instruction_receipts=[('cases/dbglist-instruction-abi/validation.json',instruction)]
for path in ['cases/dbglist-special-abi/validation.json','cases/dbglist-mnemonic-abi/validation.json','cases/dbglist-comment-abi/validation.json']:
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
    assert r['matched']and r['function']in ['@write_instruction_operand@12','@write_operand_prefix@12']and r['hidden_inputs']==[]and r['callee_stack_cleanup']==4
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
prefix=json.loads(Path('cases/dbglist-prefix-abi/validation.json').read_text());validate_operand(prefix)
for path,entry in [('cases/dbglist-operand-abi/validation.json',operand),('cases/dbglist-prefix-abi/validation.json',prefix)]:
    if entry['function']in config['accepted']:
        abi=next(r for r in config['driver']if r['name']==entry['function'])['abi']
        assert abi['convention']=='fastcall'and abi['hidden_inputs']==[]and abi['callee_stack_cleanup']==4
        assert abi['parameters']==[{k:v for k,v in formal.items()if k in ['name','location','type']}for formal in entry['formals']]
        assert abi['entry_observation']==path
for label,change in [('missing operand width formal',lambda r:r['formals'].pop()),('wrong operand pointer alias',lambda r:r['formals'][1].update(native_offset_from_instruction=24)),('wrong actual operand width',lambda r:r['native_operand'].update(bytes=8)),('unknown hidden operand input',lambda r:r.update(hidden_inputs=['ESI unknown']))]:
    bad=copy.deepcopy(operand);change(bad)
    try:validate_operand(bad)
    except(AssertionError,KeyError,ValueError):rejected.append(label)
    else:raise AssertionError('Accepted bad operand ABI: '+label)
register=json.loads(Path('cases/dbglist-register-abi/validation.json').read_text())
def validate_register(r):
    assert r['matched']and r['function']=='@machine_register_name@8'and r['hidden_inputs']==[]and r['callee_stack_cleanup']==0 and r['no_hidden_inputs_proof']
    assert r['formals']==[dict(name='reg',location='ECX',type='unsigned32',value=5),dict(name='bytes',location='EDX',type='unsigned32',value=4)]
    assert r['original_values']==r['native']['values']==[5,4]
    assert r['original_state']==dict(column=32,cw=0x27f,sw=0,fastcall=1)
    assert {k:r['native'][k]for k in ['column','cw','sw']}==dict(column=32,cw=0x27f,sw=0)
    assert r['native']['stdout']=={k:v for k,v in r['original_stdout'].items()if k not in ['counter','cursor','value','position']}
validate_register(register)
abi=next(r for r in config['driver']if r['name']==register['function'])['abi']
assert abi['convention']=='fastcall'and abi['hidden_inputs']==[]and abi['callee_stack_cleanup']==0
assert abi['parameters']==[{k:v for k,v in formal.items()if k in ['name','location','type']}for formal in register['formals']]
assert abi['entry_observation']=='cases/dbglist-register-abi/validation.json'
for label,change in [('missing register formal',lambda r:r['formals'].pop()),('changed register width input',lambda r:r['native']['values'].__setitem__(1,2)),('missing register hidden-input proof',lambda r:r.update(no_hidden_inputs_proof=''))]:
    bad=copy.deepcopy(register);change(bad)
    try:validate_register(bad)
    except(AssertionError,KeyError):rejected.append(label)
    else:raise AssertionError('Accepted bad register ABI: '+label)
comment_prefix=json.loads(Path('cases/dbglist-comment-prefix-abi/validation.json').read_text())
def validate_comment_prefix(r):
    assert r['matched']and r['function']=='@write_listing_comment@4'and r['hidden_inputs']==[]and r['callee_stack_cleanup']==0 and r['no_hidden_inputs_proof']
    assert r['formals']==[dict(name='first',location='ECX',type='signed32',value=1)]and r['original_values']==r['native']['values']==[1]
    assert r['original_state']==dict(column=39,cw=0x27f,sw=0,fastcall=1)
    assert {k:r['native'][k]for k in ['column','cw','sw']}==dict(column=39,cw=0x27f,sw=0)
    assert r['native']['stdout']=={k:v for k,v in r['original_stdout'].items()if k not in ['counter','cursor','value','position']}
validate_comment_prefix(comment_prefix)
abi=next(r for r in config['driver']if r['name']==comment_prefix['function'])['abi']
assert abi['convention']=='fastcall'and abi['hidden_inputs']==[]and abi['callee_stack_cleanup']==0
assert abi['parameters']==[{k:v for k,v in formal.items()if k in ['name','location','type']}for formal in comment_prefix['formals']]
assert abi['entry_observation']=='cases/dbglist-comment-prefix-abi/validation.json'
for label,change in [('missing first-comment formal',lambda r:r.update(formals=[])),('changed actual first-comment input',lambda r:r['native']['values'].__setitem__(0,0))]:
    bad=copy.deepcopy(comment_prefix);change(bad)
    try:validate_comment_prefix(bad)
    except(AssertionError,KeyError):rejected.append(label)
    else:raise AssertionError('Accepted bad comment-prefix ABI: '+label)
data_type=json.loads(Path('cases/dbglist-data-type-abi/validation.json').read_text())
def validate_data_type(r):
    assert r['matched']and r['function']=='@listing_data_type@4'and r['hidden_inputs']==[]and r['callee_stack_cleanup']==0 and r['no_hidden_inputs_proof']
    assert r['formals']==[dict(name='bytes',location='ECX',type='signed32',value=4)]and r['original_values']==r['native']['values']==[4]
    assert r['original_state']==dict(column=37,cw=0x27f,sw=0,fastcall=1)
    assert {k:r['native'][k]for k in ['column','cw','sw']}==dict(column=37,cw=0x27f,sw=0)
    assert r['native']['stdout']=={k:v for k,v in r['original_stdout'].items()if k not in ['counter','cursor','value','position']}
validate_data_type(data_type)
abi=next(r for r in config['driver']if r['name']==data_type['function'])['abi']
assert abi['convention']=='fastcall'and abi['hidden_inputs']==[]and abi['callee_stack_cleanup']==0
assert abi['parameters']==[{k:v for k,v in formal.items()if k in ['name','location','type']}for formal in data_type['formals']]
assert abi['entry_observation']=='cases/dbglist-data-type-abi/validation.json'
for label,change in [('missing data-type formal',lambda r:r.update(formals=[])),('changed actual data-type width',lambda r:r['native']['values'].__setitem__(0,2))]:
    bad=copy.deepcopy(data_type);change(bad)
    try:validate_data_type(bad)
    except(AssertionError,KeyError):rejected.append(label)
    else:raise AssertionError('Accepted bad data-type ABI: '+label)
displacement=json.loads(Path('cases/dbglist-displacement-abi/validation.json').read_text())
def validate_displacement(r):
    assert r['matched']and r['function']=='@write_listing_displacement@8'and r['hidden_inputs']==[]and r['callee_stack_cleanup']==0 and r['no_hidden_inputs_proof']
    assert r['formals']==[dict(name='displacement',location='ECX',type='signed32',value=0),dict(name='hint',location='EDX',type='signed32',value=0)]and r['original_values']==r['native']['values']==[0,0]
    assert r['original_state']==dict(column=57,cw=0x27f,sw=0,fastcall=1)
    assert {k:r['native'][k]for k in ['column','cw','sw']}==dict(column=57,cw=0x27f,sw=0)
    assert r['native']['stdout']=={k:v for k,v in r['original_stdout'].items()if k not in ['counter','cursor','value','position']}
validate_displacement(displacement)
if displacement['function']in config['accepted']:
    abi=next(r for r in config['driver']if r['name']==displacement['function'])['abi']
    assert abi['convention']=='fastcall'and abi['hidden_inputs']==[]and abi['callee_stack_cleanup']==0
    assert abi['parameters']==[{k:v for k,v in formal.items()if k in ['name','location','type']}for formal in displacement['formals']]
    assert abi['entry_observation']=='cases/dbglist-displacement-abi/validation.json'
for label,change in [('missing displacement hint formal',lambda r:r['formals'].pop()),('changed actual displacement hint',lambda r:r['native']['values'].__setitem__(1,1))]:
    bad=copy.deepcopy(displacement);change(bad)
    try:validate_displacement(bad)
    except(AssertionError,KeyError):rejected.append(label)
    else:raise AssertionError('Accepted bad displacement ABI: '+label)
suffix_path=Path('cases/dbglist-size-suffix-abi/validation.json');suffix=json.loads(suffix_path.read_text())
import hashlib
binding=json.loads(suffix_path.with_name('original-binding.json').read_text())
assert all(hashlib.sha256((suffix_path.parent/n).read_bytes()).hexdigest()==v for n,v in binding['files'].items())
def validate_suffix(r):
    assert r['matched']and r['function']=='@listing_size_suffix@4'and r['hidden_inputs']==[]and r['callee_stack_cleanup']==0 and r['no_hidden_inputs_proof']
    assert r['formals']==[dict(name='bytes',location='ECX',type='unsigned32',value=4)]and r['original_values']==r['native']['values']==[4]
    assert r['original_state']==dict(column=32,cw=0x27f,sw=0,fastcall=1)
    assert {k:r['native'][k]for k in ['column','cw','sw']}==dict(column=32,cw=0x27f,sw=0)
    assert r['native']['stdout']=={k:v for k,v in r['original_stdout'].items()if k not in ['counter','cursor','value','position']}
validate_suffix(suffix);abi=next(r for r in config['driver']if r['name']==suffix['function'])['abi'];assert abi['parameters']==[dict(name='bytes',location='ECX',type='unsigned32')]and abi['hidden_inputs']==[]and abi['callee_stack_cleanup']==0 and abi['entry_observation']==str(suffix_path)
for label,change in [('missing suffix formal',lambda r:r.update(formals=[])),('wrong actual suffix size',lambda r:r['native']['values'].__setitem__(0,2)),('unknown hidden suffix input',lambda r:r.update(hidden_inputs=['EDX?']))]:
    bad=copy.deepcopy(suffix);change(bad)
    try:validate_suffix(bad)
    except(AssertionError,KeyError):rejected.append(label)
    else:raise AssertionError('Accepted bad size-suffix ABI: '+label)
Path('cases/listing-entry-abi-validation.json').write_text(json.dumps(dict(checked=[r['function']for r in receipt['formals']]+[r['function']for p,r in instruction_receipts]+[operand['function'],prefix['function'],register['function'],comment_prefix['function'],data_type['function'],displacement['function'],suffix['function']],observed_formals=17+len(instruction_receipts),rejected=rejected,seconds=time.perf_counter()-started,scope='Actual original/native listing-family formals, known state and static no-hidden-input/RET evidence. First encountered inputs only, including next displacement0/hint0; no arbitrary deep pointer or branch equivalence. No Wine/compiler/native launch.'),indent=2)+'\n')
print(17+len(instruction_receipts),'actual ABI formals, known implicit state and',len(rejected),'rejections pass')
