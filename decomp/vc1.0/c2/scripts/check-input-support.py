"""Cached actual public fgets/strchr proof and rejections, no Wine/compiler."""
from pathlib import Path
import copy,json,time
from crt_input import CONTRACTS,validate_observations,validate
started=time.perf_counter();records={r['name']:r for r in json.loads(Path('runtime.json').read_text())};observation=json.loads(Path('cases/dbglist-input-support/validation.json').read_text())
for name in CONTRACTS:validate_observations(name,observation)
checked=[]
for path in sorted(Path('cases').glob('dbglist-input-crt-*-evidence.json')):
    e=json.loads(path.read_text());validate(e,records[e['logical']],None);checked.append(e['logical'])
rejected=[]
def reject(label,check):
    try:check()
    except(AssertionError,KeyError):rejected.append(label)
    else:raise AssertionError('Accepted invalid public input proof: '+label)
def change_file(r,key,value):
    for i in range(1,4):r['original_observations'][i]['input'][key]=r['native_observations'][i][key]=value
changes=[('empty original inventory',lambda r:r.update(original_observations=[])),
 ('wrong read limit',lambda r:r['native_fgets_args'].__setitem__(1,255)),
 ('wrong returned fgets pointer',lambda r:r.update(native_fgets_result=0)),
 ('wrong returned search pointer',lambda r:r.update(native_strchr_result=r['native_strchr_result']+1)),
 ('wrong search character',lambda r:r['native_strchr_args'].__setitem__(1,13)),
 ('wrong source stream identity',lambda r:r['native_fgets_args'].__setitem__(2,0)),
 ('wrong descriptor',lambda r:change_file(r,'fd',8)),
 ('wrong physical position',lambda r:change_file(r,'position',27)),
 ('text mode substituted',lambda r:change_file(r,'osfile',129)),
 ('wrong initialized span',lambda r:change_file(r,'valid',1691)),
 ('wrong flags',lambda r:change_file(r,'flags',1)),
 ('wrong floating state',lambda r:change_file(r,'sw',0x400)),
 ('wrong source contents',lambda r:change_file(r,'file_buffer_hex','00'*1690)),
 ('wrong defined length',lambda r:r.update(defined_destination_bytes=27))]
for label,change in changes:
    bad=copy.deepcopy(observation);change(bad);reject(label,lambda r=bad:validate_observations('_fgets',r))
for side in ['original','native']:
    bad=copy.deepcopy(observation)
    row=bad['original_observations'][1]['input']if side=='original'else bad['native_observations'][1]
    data=bytearray.fromhex(row['destination_hex']);data[200]^=1;row['destination_hex']=data.hex()
    reject('changed '+side+' own unused destination tail',lambda r=bad:validate_observations('_fgets',r))
bad=copy.deepcopy(observation);data=bytearray.fromhex(bad['native_observations'][3]['destination_hex']);data[0]^=1;bad['native_observations'][3]['destination_hex']=data.hex()
reject('strchr modified input string',lambda:validate_observations('_strchr',bad))
reject('unknown helper',lambda:validate_observations('_unknown_input',observation))
if checked:
    e=json.loads(next(Path('cases').glob('dbglist-input-crt-*-evidence.json')).read_text());bad=copy.deepcopy(e);bad['identity']['linked_text_sha256']='0'*64
    reject('changed archive identity',lambda:validate(bad,records[bad['logical']],None))
    bad=copy.deepcopy(e);bad['native_boundary']='Invented native internal function match'
    reject('invented internal match',lambda:validate(bad,records[bad['logical']],None))
Path('cases/input-support-validation.json').write_text(json.dumps(dict(contracts=list(CONTRACTS),accepted=checked,rejected=rejected,seconds=time.perf_counter()-started,scope='Strict cached actual first binary fgets256/LF search and exact archive identities. No launches/compiler, no application credit or general EOF/error claim.'),indent=2)+'\n')
print(len(CONTRACTS),'observed public input contracts,',len(checked),'accepted proofs,',len(rejected),'rejections pass')
