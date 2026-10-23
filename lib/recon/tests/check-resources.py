import importlib.util,json,struct,subprocess,sys,tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[3]
spec=importlib.util.spec_from_file_location('resources',root/'lib/recon/resources.py')
resources=importlib.util.module_from_spec(spec);spec.loader.exec_module(resources)
owner='62f51b0d705d10426d5b38fc6340859d0bfc829d9d93f27d12a6caa8a2ebe6c6'
entries=[dict(kind='string',id=8008,language=1033,text='A %s\r\n'),dict(kind='message',id=10001,language=1033,text='Literal %1 %2!d!\r\n')]
binary=str(Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory(prefix='resources-',dir='/root/.ai-tmp') as d:
    path=Path(d)/'provider'; resources.write_resources(path,owner,entries)
    for entry in entries:
        p=subprocess.run([binary,str(path),entry['kind'],str(entry['id']),'4096'],capture_output=True,check=True)
        text=entry['text'].encode('ascii')
        assert p.stdout==struct.pack('<I',len(text))+text+b'\0'+b'\xa5'*(4095-len(text))
    for kind,id,capacity in [('string',42,4096),('message',42,4096),('string',8008,1),('badflags',10001,4096),('badlang',10001,4096),('badmodule',8008,4096)]:
        p=subprocess.run([binary,str(path),kind,str(id),str(capacity)],capture_output=True)
        assert p.returncode==125,p
    baseline=path.read_bytes()
    for bad in [baseline[:10],baseline+b'x',baseline[:8]+b'0'*64+baseline[72:],baseline[:72]+struct.pack('<IIII',9,1,1033,0)]:
        path.write_bytes(bad)
        p=subprocess.run([binary,str(path),'string','8008','4096'],capture_output=True)
        assert p.returncode==125
    path.write_bytes(baseline)
    # Compare actual captured RCDLL full buffer, not only a normalized prefix.
    evidence=root/'decomp/vc5.10/rcdll/cases/help/message-entries.json'
    if evidence.exists():
        rows=json.loads(evidence.read_text())['entries']
        for row in rows:
            text=bytes.fromhex(row['payload_hex']).split(b'\0',1)[0]
            resources.write_resources(path,owner,[dict(kind='message',id=row['id'],language=1033,text=text)])
            p=subprocess.run([binary,str(path),'message',str(row['id']),'4096'],capture_output=True,check=True)
            expected=text+b'\0'+b'\xa5'*(4095-len(text))
            assert p.stdout==struct.pack('<I',len(text))+expected
            for language in (0,1033):
                for sentinel in (0,305419896):
                    original=Path('/root/.ai-tmp/msvc2/vc5.10/rcdll/message-api-reference')/('message-%s-%s-%s.bin'%(row['id'],language,sentinel))
                    if original.exists():
                        assert original.read_bytes()==expected
print('module-bound resource buffers, 10 rejection controls and encountered message entries passed')
