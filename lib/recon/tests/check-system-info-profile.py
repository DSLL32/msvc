import hashlib,json,subprocess,sys,tempfile,os,struct
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from system_profile import encode_profile,encode_system_info_profile
v=json.loads((Path(__file__).parent/'system-profile-original.json').read_text())
system=bytes.fromhex(v['target_system']);memory=bytes.fromhex(v['target_memory'])
raw=(Path(__file__).parent/'system-info-profile-reference.json').read_bytes()
reference=hashlib.sha256(raw).hexdigest();identity=json.loads(raw)
assert hashlib.sha256(system).hexdigest()==identity['output_sha256']
binary=str(Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory() as tmp:
    path=Path(tmp)/'profile';good=encode_system_info_profile(reference,system);path.write_bytes(good)
    p=subprocess.run([binary,str(path),reference],capture_output=True)
    assert p.returncode==0 and p.stdout==system and not p.stderr,(p.returncode,p.stdout,p.stderr)
    p=subprocess.run([binary,str(path),reference,'unbound'],capture_output=True)
    assert p.returncode==0 and not p.stderr and len(p.stdout)==36,(p.returncode,p.stderr)
    assert struct.unpack_from('<I',p.stdout,4)[0]==os.sysconf('SC_PAGE_SIZE') and struct.unpack_from('<I',p.stdout,28)[0]==65536
    for mode in ('rebind','combined-load','memory-first','null','repeat','memory'):
        p=subprocess.run([binary,str(path),reference,mode],capture_output=True)
        assert p.returncode==125,(mode,p.returncode,p.stderr)
    controls=[('hash',good[:4]+b'0'*64+good[68:]),('magic',b'BAD!'+good[4:]),('short',good[:-1]),('trailing',good+b'x'),('page0',good[:72]+b'\0'*4+good[76:]),('page3',good[:72]+b'\3\0\0\0'+good[76:]),('combined',encode_profile(reference,system,memory))]
    for mode,data in controls:
        path.write_bytes(data);p=subprocess.run([binary,str(path),reference],capture_output=True)
        assert p.returncode==125,(mode,p.returncode,p.stderr)
    path.write_bytes(good)
    for sha in ('bad','A'*64):
        p=subprocess.run([binary,str(path),sha],capture_output=True)
        assert p.returncode==125,(sha,p.returncode,p.stderr)
for args in [('bad',system),('A'*64,system),(reference,system[:-1]),(reference,b'\0'*36),(reference,system[:4]+b'\3\0\0\0'+system[8:])]:
    try:encode_system_info_profile(*args)
    except ValueError:pass
    else:raise AssertionError('invalid encoder input accepted')
print('SYSTEM_INFO-only exact36 writes/caller tails/three errors/live x87;unboundhostdefault;15native+5encoder denials; no MEMORYSTATUS')
