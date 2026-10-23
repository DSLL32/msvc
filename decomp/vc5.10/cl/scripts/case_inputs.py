"""Authored CL case inputs; no outputs or original execution at import time."""
import hashlib
import json
from pathlib import Path
import re

PROJECT=Path(__file__).resolve().parents[1]


def target_status(status):
    assert type(status)is int and 0<=status<=0xffffffff and status not in (0x7001,0x7003),'invalid/reserved target status'
    return status


def load(name):
    plan=json.loads((PROJECT/'cases/plan.json').read_text())
    assert name in plan['authorized_queue'],'case is not authorized'
    themes=[item for item in plan['themes'] if item['id']==name]
    assert len(themes)==1,'unknown/ambiguous case'
    theme=themes[0]
    inputs=[]
    if theme['source']:
        path=PROJECT/'cases'/theme['source']
        inputs.append({'name':path.name,'source':str(path.relative_to(PROJECT)),
            'bytes':path.stat().st_size,'sha256':hashlib.sha256(path.read_bytes()).hexdigest()})
    result={'case':name,'arguments':theme['arguments'],'inputs':inputs}
    if 'expected_status'in theme:
        result['expected_status']=target_status(theme['expected_status'])
    return result


def copy_inputs(case,directory):
    for item in case['inputs']:
        assert Path(item['name']).name==item['name']
        raw=(PROJECT/item['source']).read_bytes()
        assert len(raw)==item['bytes'] and hashlib.sha256(raw).hexdigest()==item['sha256']
        path=Path(directory)/item['name']
        assert not path.exists(),'case input collides with capture control file'
        path.write_bytes(raw)


def dependencies(name):
    path=PROJECT/'cases'/name/'dependencies.json'
    return json.loads(path.read_text())if path.exists()else None


def copy_dependencies(proof,directory):
    root=PROJECT.parents[2]
    for item in proof['files']:
        assert Path(item['name']).name==item['name']
        raw=(root/item['source']).read_bytes()
        assert len(raw)==item['bytes'] and hashlib.sha256(raw).hexdigest()==item['sha256']
        path=Path(directory)/item['name'];assert not path.exists()
        path.write_bytes(raw)
    (Path(directory)/'dependency-inputs.json').write_text(json.dumps(proof,indent=2)+'\n')


def verify_dependencies(directory):
    directory=Path(directory);proof=json.loads((directory/'dependency-inputs.json').read_text())
    assert proof==dependencies(proof['case']),'unknown compiler dependency input set'
    for item in proof['files']:
        path=directory/item['name']
        assert path.stat().st_size==item['bytes'] and hashlib.sha256(path.read_bytes()).hexdigest()==item['sha256']
    return proof


def prepare_c_drive(original,directory):
    """Reproduce only observed existing C: parents, using owned empty directories.

    Canonical _access error2 proves the searched leaf is absent with an existing
    parent. This never reads Wine's prefix or invents missing Windows files.
    """
    original=Path(original);directory=Path(directory)
    capture=json.loads((original/'capture.json').read_text())
    if capture['case']!='minimal-c':return None
    def raw(name):
        value=(original/name).read_bytes();item=capture['files'][name]
        assert len(value)==item['bytes'] and hashlib.sha256(value).hexdigest()==item['sha256']
        return value
    calls=re.findall(rb'^call sequence=(\d+) .* mode=4$',raw('access-events.txt'),re.M)
    parents=set()
    from mktemp_state import original_states
    for number in calls:
        number=number.decode('ascii');name='access-'+number
        path=raw(name+'-path.bin')
        if not path.startswith(b'C:\\'):continue
        assert path.endswith(b'\0') and b'\0' not in path[:-1]
        assert b'return sequence='+number.encode()+b' result=ffffffff\n' in raw('access-events.txt')
        for suffix in ('teb','ptd','tls-index'):raw('mktemp-'+name+'-after-'+suffix+'.bin')
        state=original_states(original,[name+'-after'],'access-events.txt')[name+'-after']
        assert state=={'errno':2,'doserrno':2,'last_error':2},'unproved C: parent/lookup semantics'
        parent=path[:-1].decode('ascii').rsplit('\\',1)[0]
        parts=parent[3:].split('\\')
        assert parts and all(part and part not in ('.','..') and '/' not in part for part in parts)
        parents.add(parent)
    assert parents,'missing original C: search observations'
    root=directory/'drive-c';root.mkdir()
    for parent in sorted(parents):
        (root/Path(*parent[3:].split('\\'))).mkdir(parents=True,exist_ok=True)
    ledger={'kind':'observed _access missing leaves under existing C: parents',
            'source_capture_sha256':hashlib.sha256((original/'capture.json').read_bytes()).hexdigest(),
            'dos_parents':sorted(parents),'native_root':str(root)}
    (directory/'native-filesystem.json').write_text(json.dumps(ledger,indent=2)+'\n')
    return root
