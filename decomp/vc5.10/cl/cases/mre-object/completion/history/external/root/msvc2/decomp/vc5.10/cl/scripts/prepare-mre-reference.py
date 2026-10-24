#!/usr/bin/env python3
"""Explicit one-time owned native regression oracle from reviewed fresh MRE data."""
import argparse,hashlib,json,shutil
from pathlib import Path
from mre_jobs_packet import decode
from mre_native import encoded
from case_inputs import load
from state import vector,environment
P=Path(__file__).resolve().parents[1];sha=lambda b:hashlib.sha256(b).hexdigest()
p=argparse.ArgumentParser(description=__doc__);p.add_argument('original',type=Path);a=p.parse_args();original=a.original.resolve();packet=decode(original)
ref=P/'cases/mre-object/reference';assert not ref.exists(),'never silently replace a captured oracle';ref.mkdir()
files={}
def own(name,data):
 path=ref/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data);files[name]={'bytes':len(data),'sha256':sha(data)}
own('packet.json',encoded(packet));own('original-capture.raw',(original/'capture.json').read_bytes())
for n in('argv.bin','pgmptr.bin','environment.bin','config.json','stdout.bin','stderr.bin','trace.txt','case.idb'):
 own(n,(original/n).read_bytes())
for i in range(1,7):
 for side in('before','after'):
  phase='mre-%d-%s'%(i,side)
  for suffix in('crt-words.bin','mb-state.bin','ctype.bin','shared-scratch.bin','lowio-0.bin'):
   n='observer/'+phase+'-'+suffix;own(n,(original/n).read_bytes())
for n in('observer/mre-1-before-os-environment.bin',packet['intermediate_files'][0]['owned_contents'],packet['intermediate_files'][-1]['owned_contents']):own(n,(original/n).read_bytes())
own('resources.bin',(P/'build/resources.bin').read_bytes())
contract={'schema':1,'mode':'CL_MRE_JOBS_RESPONSE_V1','packet_sha256':sha(encoded(packet)),'files':files,'preparation':{'original_capture_sha256':sha((original/'capture.json').read_bytes()),'decoders':packet['decoder_sources'],'preparer_sha256':sha(Path(__file__).read_bytes())},'scope':'Cached native regression only; authenticated raw fresh decoder mandatory for acceptance. No original or golden packet regeneration during fixtures.'}
(ref/'cached-contract.json').write_bytes(encoded(contract));print('prepared',len(files),'owned files; packet',contract['packet_sha256'])

case=load('mre-object');config=json.loads((ref/'config.json').read_bytes())
manifest={'streams':{n:sha((ref/(n+'.bin')).read_bytes())for n in('stdout','stderr')},'status':0,'target_sha256':config['target']['sha256'],'source_capture_sha256':packet['capture_sha256']}
(ref/'manifest.json').write_bytes(encoded(manifest))
master={'case':'mre-object','arguments':case['arguments'],'inputs':case['inputs'],'argv':vector(ref/'argv.bin')[1],'environment':environment(vector(ref/'environment.bin')[1]),'entry_profile':'mre-object-canonical-msvcrt-v1','program_path':(ref/'pgmptr.bin').read_bytes()[:-1].decode('latin1'),'required_accepted':config['accepted'],'temporary_name_boundary':packet['entry_and_mktemp'],'streams':manifest['streams'],'trace_sha256':sha((ref/'trace.txt').read_bytes()),'final_idb_sha256':sha((ref/'case.idb').read_bytes()),'native_resource_provider_sha256':sha((ref/'resources.bin').read_bytes()),'cached_contract_sha256':sha((ref/'cached-contract.json').read_bytes()),'source_capture_sha256':packet['capture_sha256'],'packet_sha256':sha((ref/'packet.json').read_bytes()),'scope':'Cached native regression only; fresh acceptance must independently decode full raw original and GDB records.'}
(ref/'fixture.json').write_bytes(encoded(master))
