"""Narrow typed caller-local identity for reviewed output-buffer callsites."""
import hashlib,re,json
from state import BINARY

def validate(item,pe):
    if item.get('register'):assert item['register']=='edx','unsupported typed output register'
    evidence=BINARY/item['evidence']
    assert hashlib.sha256(evidence.read_bytes()).hexdigest()==item['evidence_sha256'],'changed stack-output proof'
    proof=json.loads(evidence.read_text())
    assert 0<proof['size']<=1024 and proof['offset']==0
    for part in ('callsite','frame',*(['wrapper']if 'wrapper'in proof else [])):
        r=proof[part];assert hashlib.sha256(pe.read(r['start'],r['end']-r['start'])).hexdigest()==r['sha256'],'changed stack-output instruction evidence'
    local_offset=proof['esp_offset']-proof['frame'].get('entry_bias',0)
    assert 0<=local_offset and proof['size']+local_offset<=proof['frame']['allocation'],'buffer exceeds reviewed stack frame'
    return proof

def compare(item,descriptions,words,parameters,events,pe):
    proof=validate(item,pe)
    for r in proof['return_slots']:assert words[r['slot']]==r['address'],'unknown stack-output caller'
    esp=int(re.search(r'^registers .* esp ([0-9a-f]+) ',events,re.M)[1],16)
    value=words[item['register']]if item.get('register')else words[item['slot']];actual=parameters[item['name']]
    assert value==esp+proof['esp_offset'],'output pointer differs from original caller local'
    if 'size_argument'in item:
        count=next(r for r in descriptions if r['name']==item['size_argument'])
        assert words[count['slot']]==parameters[count['name']]==proof['size'],'output count differs from declared local'
    assert actual['address']==actual['local_address']+proof['offset'],'native raw output address differs from local base'
    assert not actual['null'] and actual['local']==dict(owner=proof['owner'],name=proof['local'],size=proof['size'],offset=proof['offset']),'native output pointer differs from typed caller local'
    label='input'if proof.get('mode')=='input'else 'output'
    match=re.search(r'^%s-buffer %08x ([0-9a-f]+)$'%(label,value),events,re.M)
    assert match and len(bytes.fromhex(match[1]))==proof['size']
    assert len(bytes.fromhex(actual['prebytes']))==proof['size']
    if 'initial_bytes'in proof:
        assert match[1]==actual['prebytes']==proof['initial_bytes'],'initialized stack output local differs'
    defined=None
    if proof.get('mode')=='input':
        old=bytes.fromhex(match[1]);new=bytes.fromhex(actual['prebytes'])
        if 'defined_bytes'in proof:
            defined=actual_length=proof['defined_bytes'];assert defined==proof['size'],'fixed input definition exceeds typed local'
        else:
            r=proof['defined_length'];defined=int.from_bytes(old[r['offset']:r['offset']+r['width']],'little')+r['bias']
            actual_length=int.from_bytes(new[r['offset']:r['offset']+r['width']],'little')+r['bias']
        assert defined==actual_length and 4<=defined<=proof['size'],'invalid defined stack input length'
        assert old[:defined]==new[:defined],'defined stack input bytes differ'
    return dict(defined_input_bytes=defined,original_address=value,native_address=actual['address'],typed_local=actual['local'],original_undefined_bytes=match[1],native_undefined_bytes=actual['prebytes'])
