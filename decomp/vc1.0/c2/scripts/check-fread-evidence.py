"""Reject changed observations/relocations in cached fread seam evidence."""
from copy import deepcopy
import json
from pathlib import Path
import tempfile
from importlib.machinery import SourceFileLoader
trace = SourceFileLoader('native_traces',str(Path(__file__).with_name('native-traces.py'))).load_module()

before = json.loads(Path('cases/floating-original-942.json').read_text())
evidence = json.loads(Path('cases/floating-crt-fread.json').read_text())
record = evidence['original']['identity']['record']
passed = []
with tempfile.TemporaryDirectory(prefix='fread-reject-',dir=Path.home()/'.ai-tmp/vc1.0-c2') as scratch:
    p = Path(scratch)/'evidence.json'
    def check(label, edit=None):
        value = deepcopy(evidence)
        if edit: edit(value)
        p.write_text(json.dumps(value))
        try:
            trace.check_runtime_evidence(p,'_fread',before,record)
        except AssertionError:
            assert edit is not None, label
        else:
            assert edit is None, 'accepted changed '+label
        passed.append(label)
    check('unchanged complete observation')
    check('changed destination',lambda e:e['observed_native'].update(bytes_hex='00'*12))
    check('changed result',lambda e:e['observed_native'].update(result=0))
    check('changed size',lambda e:e['observed_native'].update(size=11))
    check('changed CRT target',lambda e:e['original']['identity']['relocations'][0].update(target=0))
    check('changed linked body',lambda e:e['original']['identity'].update(linked_text_sha256='00'*32))
    check('changed caller use',lambda e:e.update(caller_result_use='unchecked'))
Path('cases/floating-fread-evidence-validation.json').write_text(json.dumps(dict(passed=passed,executes_wine=False),indent=2)+'\n')
print(len(passed),'fread evidence checks pass')
