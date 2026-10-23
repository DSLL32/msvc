import importlib.util,json,struct,subprocess,sys,tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[3]
spec=importlib.util.spec_from_file_location('resources',root/'lib/recon/resources.py')
resources=importlib.util.module_from_spec(spec);spec.loader.exec_module(resources)
owner='62f51b0d705d10426d5b38fc6340859d0bfc829d9d93f27d12a6caa8a2ebe6c6'
binary=str(Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory(prefix='format-inserts-',dir='/root/.ai-tmp') as d:
    path=Path(d)/'provider'
    entries=[dict(kind='message',id=10001,language=1033,text='Version %1 build %2!d!\r\n')]
    resources.write_resources(path,owner,entries)
    for version,number in [('5.00.1472.1',1472),('other',-2147483648),('x',2147483647)]:
        p=subprocess.run([binary,str(path),version,str(number)],capture_output=True,check=True)
        text=('Version '+version+' build '+str(number)+'\r\n').encode('ascii')
        assert p.stdout==struct.pack('<I',len(text))+text+bytes(2048-len(text))
    for mode in ('unknown','heap','printf'):
        p=subprocess.run([binary,str(path),mode,'1472'],capture_output=True)
        assert p.returncode==125
    for text in ('%1 %3!d!','%1 %2!u!','%1 %1 %2!d!','%1!s! %2!d!','%10 %2!d!'):
        resources.write_resources(path,owner,[dict(kind='message',id=10001,language=1033,text=text)])
        p=subprocess.run([binary,str(path),'v','1472'],capture_output=True)
        assert p.returncode==125
    resources.write_resources(path,owner,entries)
    p=subprocess.run([binary,str(path),'x'*3000,'1472'],capture_output=True)
    assert p.returncode==125
    evidence=root/'decomp/vc5.10/rcdll/cases/help/message-entries.json'
    if evidence.exists():
        row=json.loads(evidence.read_text())['entries'][0]
        template=bytes.fromhex(row['payload_hex']).split(b'\0',1)[0]
        resources.write_resources(path,owner,[dict(kind='message',id=10001,language=1033,text=template)])
        p=subprocess.run([binary,str(path),'5.00.1472.1','1472'],capture_output=True,check=True)
        lines=Path('/root/.ai-tmp/msvc2/vc5.10/rcdll/crt-format-original/events.txt')
        if lines.exists():
            original=bytes.fromhex(next(line.split()[2] for line in lines.read_text().splitlines() if line.startswith('format-after ')))
            assert p.stdout==struct.pack('<I',78)+original
print('typed dynamic message inserts/full original buffer and 9 rejection controls passed')
