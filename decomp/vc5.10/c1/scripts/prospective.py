"""Bounded raw-only wrapper-to-member observation; never an acceptance audit."""
import hashlib,json
from pathlib import Path
PROJECT=Path(__file__).resolve().parents[1]
ROOT=PROJECT.parents[2]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def scope(candidate):
 durable=json.loads((PROJECT/'functions.json').read_text())
 config=json.loads((PROJECT/candidate['audit_config']).read_text())
 assert len(durable['driver'])==49 and len(candidate['functions'])==50
 assert config['driver'][:-1]==durable['driver']
 assert config['accepted']==durable['accepted']+['_initialize_state_1279c']
 assert config['driver'][-1]==dict(name='_initialize_state_1279c',start=0x1061279c,end=0x106127a6,role='application')
 assert candidate['functions'][-1]['name']=='_initialize_state_1279c'
 assert candidate['functions'][-1]['chunks']==[[0x1279c,0x127a6]]
 assert candidate['expected_frontier']['rva']==0x11e2a
 assert candidate['expected_frontier']['calling_convention']=='thiscall-tail'
 assert not any(a<=0x11e2a<b for f in candidate['functions'] for a,b in f['chunks'])
 runtime=json.loads((PROJECT/'runtime.json').read_text())
 text=json.dumps(runtime)
 assert 'CompilerPchState::Initialize' in text
 pending=[r for r in runtime if r.get('name')=='CompilerPchState::Initialize']
 assert len(pending)==1 and pending[0]['pending_extent']
 assert '__asm { int 3 }' in (PROJECT/'src/pch-state.cpp').read_text()
 return dict(kind='prospective-tail-chain-input',wrapper_rva=0x1279c,member_rva=0x11e2a,promotion=False,
             graph_ret_ownership_status='unresolved pending tail member; no acceptance or waiver')
def binding(candidate):
 result=scope(candidate)
 paths=[PROJECT/'functions.json',PROJECT/'cases/minimal/candidate.json',PROJECT/candidate['audit_config'],
        PROJECT/candidate['field_map'],PROJECT/'runtime.json',PROJECT/'runtime-rebuilt.json',
        PROJECT/'build/c1.dll',PROJECT/'build/c1.map',PROJECT/'build/native/c1-host',PROJECT/'build/dll-capture.exe',
        ROOT/'toolchains/5.10/bin/C1.DLL',ROOT/'toolchains/5.10/bin/MSVCRT.DLL']
 paths+=sorted((PROJECT/'src').glob('*'))+sorted((PROJECT/'scripts').glob('*.py'))+[PROJECT/'scripts/dll-capture.c']
 result['inputs']={str(path.relative_to(ROOT)):sha(path) for path in paths if path.is_file()}
 return result
def freeze(output,candidate):
 (output/'prospective-binding.json').write_text(json.dumps(binding(candidate),indent=2)+'\n')
def check(directory,candidate):
 record=json.loads((directory/'prospective-binding.json').read_text())
 assert record==binding(candidate),'changed prospective execution/contract inputs'
 assert not (directory/'audit.json').exists() and not (directory/'audit-binding.json').exists()
 return record
