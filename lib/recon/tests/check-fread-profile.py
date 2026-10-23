import hashlib,json,subprocess,sys,tempfile,time
from pathlib import Path
tests=Path(__file__).resolve().parent
record=json.loads((tests/'fread-profile-original.json').read_text())
binary=str(Path(sys.argv[1]).resolve())
root=Path.home()/'.ai-tmp/msvc2/recon-maintenance/fread-profile-native'
root.mkdir(parents=True,exist_ok=True)
directory=Path(tempfile.mkdtemp(prefix='check-',dir=root))
data=bytes((n*29+7)%256 for n in range(6762))
(directory/'input.bin').write_bytes(data);(directory/'short.bin').write_bytes(data[:-1])
start=time.monotonic();p=subprocess.run([binary],cwd=directory,capture_output=True)
assert p.returncode==0 and not p.stderr,(p.returncode,p.stdout,p.stderr)
assert hashlib.sha256(p.stdout).hexdigest()==record['files']['stdout.bin'],'FILE flags/capacity/pointer/count/physical position/result/errors mismatch'
outputs=[f'case-{n}-{name}.bin' for n in range(6) for name in ('output','buffer-before','buffer-after')]+['partial-output.bin','input.bin','short.bin']
for name in outputs:
    assert hashlib.sha256((directory/name).read_bytes()).hexdigest()==record['files'][name],name
assert (directory/'case-0-buffer-before.bin').read_bytes()==(directory/'case-0-buffer-after.bin').read_bytes()
assert (directory/'case-1-buffer-before.bin').read_bytes()==(directory/'case-1-buffer-after.bin').read_bytes()
assert (directory/'case-3-buffer-before.bin').read_bytes()==(directory/'case-3-buffer-after.bin').read_bytes()
print(f'canonical seven direct/buffered/remainder/partial-item/EOF/zero-count states, full caller buffers/tails/FILE buffers/input identity exact ({time.monotonic()-start:.3f}s)')
