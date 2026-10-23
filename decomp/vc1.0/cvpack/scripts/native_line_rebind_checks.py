"""Bounded source-line rebind scope and immutable capture rejection controls."""
import copy,json,tempfile,shutil
from pathlib import Path
from state import BINARY,sha
from native_line_rebind import validate,validate_native_fields

def check(original,native,sidecar,configuration=None):
    original,native,sidecar=map(Path,(original,native,sidecar))
    current=Path(configuration)if configuration else BINARY/'functions.json'
    config=json.loads(current.read_text());receipt=json.loads((native.parent/'receipt.json').read_text())[0]
    validate(original,config,current,native,receipt,sidecar)
    def deny(label,action,reason):
        try:action()
        except (AssertionError,FileNotFoundError)as e:
            assert reason in str(e),(label,e);print('rejected:',label);return
        raise AssertionError('accepted rebind '+label)
    before=json.loads((sidecar.parent/'old-read-proof.json').read_text());after=json.loads((sidecar.parent/'new-read-proof.json').read_text())
    for label,mutator in [('original call site',lambda x:x['sites'][0].__setitem__('call',x['sites'][0]['call']+1)),
             ('original return site',lambda x:x['sites'][0].__setitem__('returned',x['sites'][0]['returned']+1)),
             ('native owner',lambda x:x['sites'][0].__setitem__('owner','other')),
             ('read size',lambda x:x['sites'][0].__setitem__('size',65))]:
        bad=copy.deepcopy(after);mutator(bad)
        deny('rebind changes '+label,lambda:validate_native_fields(before,bad),'rebind changes original read-observation policy')
    deny('rebind both stale configs',lambda:validate(original,json.loads((original/'configuration.json').read_text()),current,native,receipt,sidecar),'rebind native snapshot contents differ')
    bad=copy.deepcopy(config);bad['functions'][0]['original']+=1
    deny('rebind unrelated function address',lambda:validate(original,bad,current,native,receipt,sidecar),'')
    with tempfile.TemporaryDirectory(prefix='rebind-negatives-',dir=Path.home()/'.ai-tmp/vc1.0-cvpack')as directory:
        work=Path(directory)/'original';work.mkdir()
        # Copy only immutable bytes inspected before the rejected condition.
        for name in ('receipt.json','configuration.json','CVPACK.EXE','runtime-plan'):shutil.copyfile(original/name,work/name)
        p=work/'CVPACK.EXE';raw=p.read_bytes();p.write_bytes(bytes([raw[0]^1])+raw[1:])
        deny('rebind masked image mutation',lambda:validate(work,config,current,native,receipt,sidecar),'rebind original mask changed');p.write_bytes(raw)
        p=work/'receipt.json';raw=p.read_bytes();r=json.loads(raw);r['debugger_sha256']='0'*64;p.write_text(json.dumps(r))
        deny('rebind controller receipt mutation',lambda:validate(work,config,current,native,receipt,sidecar),'rebind original receipt changed');p.write_bytes(raw)
        p=work/'runtime-plan';raw=p.read_bytes();p.write_bytes(raw+b'17000 17001 0\n')
        deny('rebind runtime plan mutation',lambda:validate(work,config,current,native,receipt,sidecar),'rebind original runtime plan differs');p.write_bytes(raw)
        folder=work/'work';folder.mkdir()
        source=original/'work/hello-debug.dll';p=folder/'hello-debug.dll';raw=source.read_bytes();p.write_bytes(bytes([raw[0]^1])+raw[1:])
        deny('rebind case input mutation',lambda:validate(work,config,current,native,receipt,sidecar),'rebind case input changed')
        p=work/'configuration.json';p.unlink()
        deny('rebind missing old raw snapshot',lambda:validate(work,config,current,native,receipt,sidecar),'configuration.json')
