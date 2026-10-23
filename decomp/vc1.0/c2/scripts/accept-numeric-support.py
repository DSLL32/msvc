"""Accept ONE reached original CRT body under the observed numeric printf seam.

Explicit reconstruction metadata action, never a fixture. No native internal
function or ABI match is invented; the public call is independently observed.
"""
from pathlib import Path
import argparse,hashlib,json
from crt_numeric import CONTRACTS,PUBLIC_BOUNDARY,identity,validate_observations
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--before',type=Path,required=True)
args=parser.parse_args()
before=json.loads(args.before.read_text())
config_path=Path('functions.json');config=json.loads(config_path.read_text());name=before['logical']
assert name in CONTRACTS and name not in config['accepted']
assert before['accepted']==len(config['accepted']) and before['config_sha256']==hashlib.sha256(config_path.read_bytes()).hexdigest()
cache=json.loads(Path('cases/native-traces.json').read_text())
assert cache['cases']['dbglist']['terminal']==name and cache['cases']['dbglist']['pending_runtime']
record=next(r for r in json.loads(Path('runtime.json').read_text())if r['name']==name)
observation_path=Path('cases/dbglist-numeric-support/validation.json')
observation=json.loads(observation_path.read_text());observed=validate_observations(name,observation)
evidence=dict(logical=name,before_config_sha256=before['config_sha256'],target_sha256=before['target_sha256'],identity=identity(record),contract=CONTRACTS[name],observation_path=str(observation_path),observation_sha256=hashlib.sha256(observation_path.read_bytes()).hexdigest(),internal_observations=observed,native_boundary=PUBLIC_BOUNDARY,source_case='dbglist numeric printf1.25, finite C locale, plain%g',limit='One observed finite public call and actual internal body inputs/outputs. No independent native internal ABI match or application credit. Full listing/object/PDB oracle remains mandatory.')
number=len(config['accepted'])+1;path=Path('cases/dbglist-numeric-crt-%d-evidence.json'%number)
assert not path.exists();path.write_text(json.dumps(evidence,indent=2)+'\n')
config['accepted'].append(name);config_path.write_text(json.dumps(config,indent=2)+'\n')
print(name,number,path)
