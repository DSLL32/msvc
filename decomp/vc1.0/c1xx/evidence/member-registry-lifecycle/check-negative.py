"""Focused native-only negatives for the reached pre-output checkpoint."""
from pathlib import Path
import copy
import hashlib
import importlib.util
import json
import shutil
import sys
import tempfile
import time

BINARY = Path(__file__).resolve().parents[2]
sys.path.insert(0,str(BINARY/'scripts'))
from checkpoint_buffers import original_buffers
from case_paths import CAPTURE_ROOT, paths

started = time.perf_counter()
checks = []
for name,raw,count,expected in [
    ('explicit-stream-states','buffer 0 unopened\nbuffer 1 0 \nbuffer 2 1 aa\n',3,[None,'','aa']),
    ('missing-stream','buffer 0 unopened\n',2,None),
    ('duplicate-index','buffer 0 unopened\nbuffer 0 0 \n',2,None),
    ('unreadable-stream','buffer 0 unreadable-file\n',1,None),
    ('short-buffer-read','buffer 0 2 aa\n',1,None),
    ('invalid-buffer','buffer 0 invalid\n',1,None),
]:
    try: actual = original_buffers(raw,count)
    except AssertionError as error:
        assert expected is None, str(error)
        checks.append(dict(test=name,result='held',reason=str(error)))
    else:
        assert actual == expected and expected is not None
        checks.append(dict(test=name,result='verified',actual=actual))

spec=importlib.util.spec_from_file_location('native_lifecycle',BINARY/'scripts/check-native.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
config=json.loads((BINARY/'functions.json').read_text())
case,fixture,reference,frontiers=paths(config,'member_registry_browser')
original=json.loads((reference/'manifest.json').read_text())
assert original['outputs']==[] and len(original['phase_output_presence'])==6
assert all(not row['present'] for row in original['phase_output_presence'])
assert len(original['visible_files'])==2
scratch=Path(tempfile.mkdtemp(prefix='registry-lifecycle-negatives-',dir=CAPTURE_ROOT))
real_run=module.subprocess.run
executable=BINARY/'build/native/C1XX3232.EXE'
seed_receipt=BINARY/'cases/member_registry_browser-seed/native-fixture-receipt.json'
seed=json.loads(seed_receipt.read_text())
assert seed['status']=='pass' and any(row['case']=='member_registry_browser' and row['status']=='pass' for row in seed['cases'])
checks.append(dict(test='native-control',result='verified',receipt=str(seed_receipt.relative_to(BINARY))))
for name in ('missing-presence-row','unexpected-output-presence','unknown-visible-file','changed-input-bytes'):
    directory=scratch/name;directory.mkdir()
    target=directory/'reference';shutil.copytree(reference,target)
    metadata=copy.deepcopy(original)
    if name=='missing-presence-row':metadata['phase_output_presence'].pop()
    if name=='unexpected-output-presence':metadata['phase_output_presence'][0]['present']=True
    (target/'manifest.json').write_text(json.dumps(metadata,indent=2)+'\n')
    module.paths=lambda config,case_name:(case,fixture,target,frontiers)
    def launch(*args,**kwargs):
        result=real_run(*args,**kwargs)
        if kwargs.get('executable')==str(executable):
            work=Path(kwargs['cwd'])
            if name=='unknown-visible-file':(work/'unexpected.file').write_bytes(b'unknown')
            elif name=='changed-input-bytes':(work/'registry.h').write_bytes(b'changed after execution')
        return result
    module.subprocess.run=launch
    try: module.run(executable,directory/'native',None,{},'member_registry_browser')
    except AssertionError as error:
        checks.append(dict(test=name,result='held',reason=str(error)))
    else:
        raise AssertionError(name+' unexpectedly passed')
    finally:module.subprocess.run=real_run
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
report=dict(schema=1,scope=__doc__,checks=checks,reference_manifest=str(reference/'manifest.json'),reference_manifest_sha256=sha(reference/'manifest.json'),configuration_sha256=sha(BINARY/'functions.json'),native_executable_sha256=sha(executable),native_control_receipt_sha256=sha(seed_receipt),checker_sha256=sha(Path(__file__)),production_files={str(p.relative_to(BINARY)):sha(p) for p in [BINARY/'scripts/checkpoint_buffers.py',BINARY/'scripts/check-native.py',BINARY/'scripts/seed-case.py',BINARY/'scripts/advance-frontier.py']},elapsed_seconds=time.perf_counter()-started,scratch=str(scratch),original_invocations=0,production_cache_changes=False)
Path(__file__).with_name('negative-receipt.json').write_text(json.dumps(report,indent=2)+'\n')
print(Path(__file__).with_name('negative-receipt.json'))
