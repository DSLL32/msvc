"""Cached numeric public-seam proofs and meaningful rejection checks; no Wine."""
from pathlib import Path
import copy,json,time
from crt_numeric import CONTRACTS,identity,validate,validate_observations
started=time.perf_counter()
observation=json.loads(Path('cases/dbglist-numeric-support/validation.json').read_text())
records={r['name']:r for r in json.loads(Path('runtime.json').read_text())}
for name in CONTRACTS:validate_observations(name,observation)
checked=[]
for path in sorted(Path('cases').glob('dbglist-numeric-crt-*-evidence.json')):
    evidence=json.loads(path.read_text());validate(evidence,records[evidence['logical']],None);checked.append(evidence['logical'])
rejected=[]
def reject(label,check):
    try:check()
    except (AssertionError,KeyError):rejected.append(label)
    else:raise AssertionError('Accepted invalid numeric proof: '+label)
changes=[
 ('public double bits',lambda r:r['public_args'].update(double_hex='00'*8)),
 ('public format',lambda r:r['public_args'].update(format_hex=b'; %Lg\0'.hex())),
 ('native return',lambda r:r.update(public_native_result=5)),
 ('native floating precision',lambda r:r['public_native_floating'][0].update(cw=0x37f)),
 ('native floating status',lambda r:r['public_native_floating'][1].update(sw=0x400)),
 ('both public buffers altered',lambda r:[x[-1].update(buffer_hex='00')for x in [r['public_native_file'],r['public_original_file']]]),
 ('descriptor position',lambda r:[x[-1].update(position=1)for x in [r['public_native_file'],r['public_original_file']]]),
 ('empty internal inventory',lambda r:r.update(observations=[])),
 ('conversion text',lambda r:next(x for x in r['observations']if x['name']=='__cfltcvt'and x['stage']=='return')['extra'].update(output_hex=b'1.25001\0'.hex())),
 ('conversion return identity',lambda r:next(x for x in r['observations']if x['name']=='__cfltcvt'and x['stage']=='return').update(eax=0))]
for label,change in changes:
    changed=copy.deepcopy(observation);change(changed)
    reject(label,lambda r=changed:validate_observations('__cfltcvt',r))
for name,field in [('___addl','sum_hex'),('___add_12','left_hex'),('___shl_12','extended_hex'),('___shr_12','extended_hex'),('___dtold','extended_hex'),('___ld12mul','left_hex'),('___multtenpow12','extended_hex')]:
    changed=copy.deepcopy(observation)
    value=next(x for x in changed['observations']if x['name']==name and x['stage']=='return')['extra']
    value[field]='ff'*len(bytes.fromhex(value[field]))
    reject('incorrect '+name+' result',lambda r=changed,n=name:validate_observations(n,r))
reject('unknown internal helper',lambda:validate_observations('__unrecovered_numeric',observation))
if checked:
    path=next(Path('cases').glob('dbglist-numeric-crt-*-evidence.json'));evidence=json.loads(path.read_text())
    bad=copy.deepcopy(evidence);bad['identity']['linked_text_sha256']='0'*64
    reject('changed linked archive identity',lambda:validate(bad,records[bad['logical']],None))
    bad=copy.deepcopy(evidence);bad['native_boundary']='Independent native internal ABI match'
    reject('fabricated native internal ABI match',lambda:validate(bad,records[bad['logical']],None))
Path('cases/numeric-support-validation.json').write_text(json.dumps(dict(observed_contracts=list(CONTRACTS),accepted_proofs=checked,rejected=rejected,seconds=time.perf_counter()-started,scope='Actual finite1.25 plain%g public seam, original internal arguments/results/defined buffers; no native internal ABI match or application credit, no launch/compiler'),indent=2)+'\n')
print(len(CONTRACTS),'observed numeric contracts,',len(checked),'accepted proofs,',len(rejected),'rejections pass')
