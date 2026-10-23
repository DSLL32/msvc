"""Read-only proof of the selected native startup precision, not a fixture."""
import hashlib,json,re
from pathlib import Path
from x87_inputs import control_observation
BINARY=Path(__file__).resolve().parents[1]
BASE=BINARY/'evidence/startup-precision53'
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def verify():
 proof=json.loads((BASE/'validation.json').read_text())
 for name,digest in proof['evidence_sha256'].items():assert sha(BASE/name)==digest,'startup evidence changed: '+name
 config=json.loads((BINARY/'functions.json').read_text());original=json.loads((BASE/'original-receipt.json').read_text());before=json.loads((BASE/'native-before-checkpoint.json').read_text());after=json.loads((BASE/'native-after-checkpoint.json').read_text())
 assert original['original_sha256']==config['target']['sha256'] and original['terminal']=='_main' and original['accepted_count']==26
 assert proof['accepted_startup_prefix']==config['accepted'][:26] and config['accepted'][26]=='_main'
 actual=control_observation((BASE/'original-events.txt').read_text())
 assert actual==original['x87_control'] and actual['requested']==actual['actual']==0
 main=next(row for row in config['driver'] if row['name']=='_main')
 assert re.search(r'^exception 80000003 rva %08x base 00010000 first 1$'%(main['start']-config['target']['base']),(BASE/'original-events.txt').read_text(),re.M),'original main trap identity differs'
 assert before['function']==after['function']=='main'
 assert (before['control'],before['status'],before['tag'])==(0x37f,0,0xffff)
 assert (after['control'],after['status'],after['tag'])==(actual['control'],actual['status'],actual['tag'])==(0x27f,0,0xffff)
 assert '-DRECON_NATIVE_STARTUP_CONTROLFP_53 ' in (BINARY/'Makefile').read_text()
 assert sha(BINARY/proof['shared_pin']['manifest'])==proof['shared_pin']['manifest_sha256']
 assert proof['profile']=='RECON_NATIVE_STARTUP_CONTROLFP_53' and proof['no_application_entry_or_graph_change']
 assert proof['application_credit']==proof['source_function_credit']==0
 pin=json.loads((BINARY/'cases/shared-build-reference.json').read_text())
 assert pin['commit']==proof['shared_pin']['committed_reference'] and pin['tree']==proof['shared_pin']['committed_tree']
 assert proof['shared_pin']['storage']=='git-blobs' and 'archive' not in proof['shared_pin']
 old=proof['historical_archive'];assert sha(BINARY/old['archive'])==old['archive_sha256']
 fixtures=json.loads((BASE/'native-fixture-receipt.json').read_text());assert fixtures['status']=='pass' and len(fixtures['cases'])==6 and all(row['status']=='pass' for row in fixtures['cases'])
 return dict(status='pass',original_main_control=actual['control'],native_main_control=after['control'],status_word=after['status'],physical_tags=after['tag'],profile=proof['profile'],application_credit=0)
if __name__=='__main__':print(json.dumps(verify(),indent=2))
