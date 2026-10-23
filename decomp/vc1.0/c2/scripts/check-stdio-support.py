"""Native-only validation/rejections for observed CRT/public printf seam records."""
from pathlib import Path
import copy,hashlib,json,time
from crt_stdio import CONTRACTS,validate,validate_observations
started=time.perf_counter();records={r['name']:r for r in json.loads(Path('runtime.json').read_text())};checked=[]
for path in sorted(Path('cases').glob('dbglist-crt-*-evidence.json')):
    evidence=json.loads(path.read_text());name=evidence['logical']
    validate(evidence,records[name],None);checked.append(name)
observation=json.loads(Path('cases/dbglist-stdio-support/validation.json').read_text());assert checked
rejected=[]
def rejection(label,check):
    try:check()
    except (AssertionError,KeyError):rejected.append(label)
    else:raise AssertionError('Accepted bad proof: '+label)
for label,change in [
 ('changed native buffer',lambda r:r['public_native_file'][-1].update(buffer_hex='00')),
 ('changed both buffers',lambda r:[x[-1].update(buffer_hex='00')for x in [r['public_native_file'],r['public_original_file']]]),
 ('wrong descriptor position',lambda r:[x[-1].update(position=24)for x in [r['public_native_file'],r['public_original_file']]]),
 ('wrong FILE mode',lambda r:[x[-1].update(flags=2)for x in [r['public_native_file'],r['public_original_file']]]),
 ('wrong public return',lambda r:r.update(public_result=23)),
 ('empty internal inventory',lambda r:r.update(observations=[])),
 ('wrong internal return',lambda r:[x.update(eax=1)for x in r['observations']if x['name']=='__stbuf'and x['stage']=='return'])]:
    changed=copy.deepcopy(observation);change(changed);rejection(label,lambda r=changed:validate_observations('__stbuf',r))
rejection('unknown internal helper',lambda:validate_observations('__unrecovered_stdio',observation))
e=json.loads(Path('cases/dbglist-crt-1155-evidence.json').read_text());bad=copy.deepcopy(e);bad['identity']['linked_text_sha256']='0'*64
rejection('changed linked archive identity',lambda:validate(bad,records[bad['logical']],None))
bad=copy.deepcopy(e);bad['native_boundary']='Independent native internal ABI match'
rejection('fabricated native internal ABI claim',lambda:validate(bad,records[bad['logical']],None))
receipt=dict(checked=checked,rejected=rejected,seconds=time.perf_counter()-started,
 scope='Strict cached observations and exact original archive identities, no Wine/native launch/compiler; no application credit for CRT support')
Path('cases/stdio-support-validation.json').write_text(json.dumps(receipt,indent=2)+'\n');print(len(checked),'CRT proofs and',len(rejected),'rejections pass')
