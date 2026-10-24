"""Explicit, bounded validation reuse after native read-observer line drift.

Raw original receipts and snapshots remain immutable. This does not execute or
recapture the original, and is never an implicit fixture fallback.
"""
import argparse,copy,json,tempfile
from pathlib import Path
from state import BINARY,ROOT,sha,mask
from binary import PE

def validate_native_fields(before,after):
    assert len(before['sites'])==len(after['sites'])
    expected=copy.deepcopy(before)
    for a,b in zip(expected['sites'],after['sites']):a['native_line']=b['native_line']
    # These hashes bind native consumer source, never original observation code.
    expected['source_sha256']=after['source_sha256']
    assert set(expected.get('additional_sources',{}))==set(after.get('additional_sources',{}))
    expected['additional_sources']=after.get('additional_sources',{})
    assert expected==after and before!=after,'rebind changes original read-observation policy'


def validate(original,config,current,native,native_receipt,sidecar):
    sidecar=Path(sidecar);root=sidecar.parent;p=json.loads(sidecar.read_text())
    assert p['schema']==1 and p['kind']in('native-read-source-line-rebind','cvpack142-host-driver-hash-only')
    for name,digest in p['files'].items():assert sha(root/name)==digest,'rebind immutable proof changed'
    raw=json.loads((original/'receipt.json').read_text());old=json.loads((original/'configuration.json').read_text())
    assert sha(original/'receipt.json')==p['original_receipt_sha256'],'rebind original receipt changed'
    assert sha(original/'configuration.json')==raw['configuration_sha256']==p['old_configuration_sha256'],'rebind original snapshot changed'
    assert sha(current)==p['new_configuration_sha256']==native_receipt['configuration_sha256']==sha(native/'configuration.json'),'rebind stale native/current snapshot'
    assert json.loads((native/'configuration.json').read_text())==config,'rebind native snapshot contents differ'
    expected=copy.deepcopy(old)
    if p['kind']=='cvpack142-host-driver-hash-only':
        # One approved derivation: host launch support changed, original run did not.
        before=json.loads((root/'old-native-driver-proof.json').read_text());after=json.loads((root/'new-native-driver-proof.json').read_text())
        oldhash='e13a9ccacf0405b8e5e683b491f18e04d921e9e2b9554f506ed6e614d7c2f60d'
        newhash='3e978ff1b9bb3cba2a3ec1ac5fecd2e706fbb53161eb091fc4ba1ac8f9397b05'
        assert p['native_driver_hashes']==[oldhash,newhash] and before['sources']['scripts/check-native.py']==oldhash and after['sources']['scripts/check-native.py']==newhash==sha(BINARY/'scripts/check-native.py'),'unapproved native driver derivation'
        changed=copy.deepcopy(before);changed['sources']['scripts/check-native.py']=newhash
        assert changed==after,'host derivation changes scalar observation policy'
        assert sha(root/'old-native-driver-proof.json')==old['scalar_block_observations']['sha256']
        assert sha(root/'new-native-driver-proof.json')==config['scalar_block_observations']['sha256']==sha(BINARY/config['scalar_block_observations']['path'])
        expected['scalar_block_observations']['sha256']=config['scalar_block_observations']['sha256']
        assert raw['entry_progress_only'] and raw['candidates']==['build_type_offsets'],'host derivation restricted to actual142 entry flow'
    else:
        expected['read_observations']['evidence_sha256']=config['read_observations']['evidence_sha256']
        before=json.loads((root/'old-read-proof.json').read_text());after=json.loads((root/'new-read-proof.json').read_text())
        assert sha(root/'old-read-proof.json')==old['read_observations']['evidence_sha256']
        assert sha(root/'new-read-proof.json')==config['read_observations']['evidence_sha256']==sha(BINARY/config['read_observations']['evidence'])
        validate_native_fields(before,after)
        assert sha(BINARY/after['source'])==after['source_sha256']
        for path,digest in after.get('additional_sources',{}).items():assert sha(BINARY/path)==digest,'rebind native source changed'
    assert expected==config and old!=config,'rebind unrelated configuration change'
    for path,digest in p['current_files'].items():assert sha(BINARY/path)==digest,'rebind current dependency changed'
    assert sha(BINARY/'build/debug-run.exe')==raw['debugger_sha256']==p['controller_sha256'],'rebind original controller changed'
    assert sha(original/'CVPACK.EXE')==raw['executable_sha256']==p['masked_sha256'],'rebind original mask changed'
    assert sha(BINARY/'build/native/CVPACK.EXE')==native_receipt['executable_sha256']==p['native_executable_sha256'],'rebind native executable changed'
    assert raw['discover_runtime']and not raw['unmasked']and not raw.get('guard_probe')and not raw.get('stop_at'),'unsupported rebind controller mode'
    target=ROOT/config['target']['path'];pe=PE(target)
    assert sha(target)==raw['original_sha256']==config['target']['sha256'],'rebind original target changed'
    runtime=json.loads((BINARY/'runtime.json').read_text())
    application=[r for r in config['functions']if r['status']=='accepted'or r['name']in raw['candidates']]
    ranges={(r['start'],r['end'])for r in runtime['functions']+runtime['imports']}|{tuple(r['extent'])for r in application}
    plan=''.join('%x %x %x\n'%(a,b,pe.offset(a))for a,b in sorted(ranges))
    assert (original/'runtime-plan').read_text()==plan,'rebind original runtime plan differs'
    with tempfile.TemporaryDirectory(dir=Path.home()/'.ai-tmp/vc1.0-cvpack',prefix='rebind-mask-')as d:
        path=Path(d)/'image';mask(config,pe,path,raw['candidates']);image=bytearray(path.read_bytes())
        for r in application:image[pe.offset(r['original'])]=0xcc
        for r in (after['sites']if p['kind']=='native-read-source-line-rebind'else []):
            image[pe.offset(r['call'])]=0xcc;image[pe.offset(r['returned'])]=0xcc
        assert bytes(image)==(original/'CVPACK.EXE').read_bytes(),'rebind regenerated masked image differs'
    case=json.loads((BINARY/'cases'/raw['case']/'case.json').read_text())
    assert raw['command'][3]==' '.join(case['argv']),'rebind case invocation changed'
    for item in case['inputs']:
        assert sha(ROOT/item['path'])==item['sha256']
        assert (ROOT/item['path']).read_bytes()==(original/'work'/item['name']).read_bytes(),'rebind case input changed'
    assert p['checker_sha256']==sha(__file__),'rebind checker changed'
    return p

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original',required=True,type=Path);parser.add_argument('--native',required=True,type=Path)
    parser.add_argument('--old-proof',required=True,type=Path);parser.add_argument('--output',required=True,type=Path)
    a=parser.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    raw=json.loads((a.original/'receipt.json').read_text());native=json.loads((a.native.parent/'receipt.json').read_text())[0]
    config=json.loads((BINARY/'functions.json').read_text())
    (a.output/'old-read-proof.json').write_bytes(a.old_proof.read_bytes())
    (a.output/'new-read-proof.json').write_bytes((BINARY/config['read_observations']['evidence']).read_bytes())
    p=dict(schema=1,kind='native-read-source-line-rebind',original_receipt_sha256=sha(a.original/'receipt.json'),
        old_configuration_sha256=sha(a.original/'configuration.json'),new_configuration_sha256=sha(BINARY/'functions.json'),
        controller_sha256=raw['debugger_sha256'],masked_sha256=raw['executable_sha256'],native_executable_sha256=native['executable_sha256'],
        checker_sha256=sha(__file__),files={n:sha(a.output/n)for n in ('old-read-proof.json','new-read-proof.json')},
        current_files={n:sha(BINARY/n)for n in ('data.json','walks.json','runtime.json','runtime-support.json','scripts/capture-original.py','scripts/debug-run.c','scripts/accept.py','scripts/check-native.py','scripts/native-debug.py')})
    sidecar=a.output/'rebind.json';sidecar.write_text(json.dumps(p,indent=2)+'\n')
    validate(a.original,config,BINARY/'functions.json',a.native,native,sidecar)
    print('Original raw capture retained; exact regenerated mask and bounded native-only rebind verified:',sidecar)

if __name__=='__main__':main()
