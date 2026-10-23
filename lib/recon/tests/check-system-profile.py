import hashlib,json,subprocess,sys,tempfile
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from system_profile import encode_profile
binary=str(Path(sys.argv[1]).resolve())
raw=(Path(__file__).parent/'system-profile-original.json').read_bytes();v=json.loads(raw)
reference=hashlib.sha256(raw).hexdigest()
system=bytes.fromhex(v['target_system']);memory=bytes.fromhex(v['target_memory'])
with tempfile.TemporaryDirectory() as tmp:
    path=Path(tmp)/'profile';good=encode_profile(reference,system,memory);path.write_bytes(good)
    p=subprocess.run([binary,str(path),reference],capture_output=True)
    assert p.returncode==0 and p.stdout==system+memory and not p.stderr,(p.returncode,p.stderr)
    modes=['unbound','rebind','wrong-order','null','extra-system','extra-memory']
    for mode in modes:
        p=subprocess.run([binary,str(path),reference,mode],capture_output=True)
        assert p.returncode==125,(mode,p.returncode,p.stderr)
    controls=[('hash',good[:4]+b'0'*64+good[68:]),('magic',b'BAD!'+good[4:]),('short',good[:-1]),('trailing',good+b'x'),('page',good[:72]+b'\0'*4+good[76:]),('length',good[:104]+b'\0'*4+good[108:])]
    for mode,data in controls:
        path.write_bytes(data);p=subprocess.run([binary,str(path),reference],capture_output=True)
        assert p.returncode==125,(mode,p.returncode,p.stderr)
for args in [('bad',system,memory),(reference,system[:-1],memory),(reference,system,b'\0'*32)]:
    try: encode_profile(*args)
    except ValueError: pass
    else: raise AssertionError('invalid encoder input accepted')
print('captured system profile exact68 bytes, 12 native controls and3encoder controls passed')
