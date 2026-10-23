"""Record ONE reached original CRT body under actual listing-input observations."""
from pathlib import Path
import argparse,hashlib,json
from crt_input import CONTRACTS,PUBLIC_BOUNDARY,identity,validate_observations
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--before',type=Path,required=True);args=parser.parse_args()
before=json.loads(args.before.read_text());path=Path('functions.json');config=json.loads(path.read_text());name=before['logical']
assert name in CONTRACTS and name not in config['accepted']and before['accepted']==len(config['accepted'])and before['config_sha256']==hashlib.sha256(path.read_bytes()).hexdigest()
cache=json.loads(Path('cases/native-traces.json').read_text());assert cache['cases']['dbglist']['terminal']==name and cache['cases']['dbglist']['pending_runtime']
record=next(r for r in json.loads(Path('runtime.json').read_text())if r['name']==name);observation_path=Path('cases/dbglist-input-support/validation.json');observation=json.loads(observation_path.read_text())
evidence=dict(logical=name,before_config_sha256=before['config_sha256'],target_sha256=before['target_sha256'],identity=identity(record),observations=validate_observations(name,observation),observation_path=str(observation_path),observation_sha256=hashlib.sha256(observation_path.read_bytes()).hexdigest(),contract=CONTRACTS[name],native_boundary=PUBLIC_BOUNDARY,source_case='dbglist first binary listing-source fgets256/strchrLF',limit='Actual first positive binary line read and LF search, no universal EOF/error/locale equivalence. Full final object/listing/PDB oracle mandatory.')
n=len(config['accepted'])+1;target=Path('cases/dbglist-input-crt-%d-evidence.json'%n);assert not target.exists();target.write_text(json.dumps(evidence,indent=2)+'\n');config['accepted'].append(name);path.write_text(json.dumps(config,indent=2)+'\n');print(name,n,target)
