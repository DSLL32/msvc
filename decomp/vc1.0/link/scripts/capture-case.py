#!/usr/bin/env python3
"""Explicit original-only new-case probe/capture, outside all native fixtures."""
import argparse
import fcntl
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import time
from binary import PE
from case_paths import ROOT,SCRATCH,PREFIX,PROGRAM,LIBRARY,windows
from indexes import imports

BINARY=Path(__file__).resolve().parents[1]
def sha(data):return hashlib.sha256(data).hexdigest()

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--spec',type=Path,required=True)
    ap.add_argument('--mode',choices=['original','masked'],required=True)
    ap.add_argument('--sample',action='append',default=[],help='Explicit analysis entry/restoration trap ADDRESS:NAME')
    options=ap.parse_args()
    spec=json.loads(options.spec.read_text())
    assert (PREFIX/'dosdevices/r:').resolve()==ROOT
    assert (PREFIX/'dosdevices/s:').resolve()==SCRATCH
    base=SCRATCH/'vc1.0/link'
    lease=(base/'capture.lock').open('a')
    with lease:
        fcntl.flock(lease,fcntl.LOCK_EX)
        work=base/spec['case']
        if work.exists():shutil.rmtree(work)
        work.mkdir(parents=True)
        for item in spec['inputs']:
            source=(BINARY/item['path']).resolve()
            data=source.read_bytes()
            assert sha(data)==item['sha256']
            (work/source.name).write_bytes(data)
        for name in spec.get('directories',[]):
            relative=Path(name)
            assert not relative.is_absolute() and '..' not in relative.parts
            (work/relative).mkdir(parents=True,exist_ok=True)
        captures=base/'new-captures'/spec['case']
        captures.mkdir(parents=True,exist_ok=True)
        capture=Path(tempfile.mkdtemp(prefix=options.mode+'-',dir=captures))
        pe=PE(ROOT/'toolchains/1.0-nt/bin/LINK.EXE')
        config=json.loads((BINARY/'functions.json').read_text())
        assert sha(pe.data)==config['target']['sha256']
        section=next(s for s in pe.sections if s['flags']&0x20)
        text=bytearray(pe.data[section['off']:section['off']+section['size']])
        if options.mode=='masked':
            text[:]=b'\xcc'*len(text)
            accepted=set(config['accepted'])
            records=config['driver']+json.loads((BINARY/'runtime.json').read_text())+imports(pe)
            for record in records:
                if record['name'] not in accepted:continue
                for start,end in [(record['start'],record['end'])]+[tuple(x) for x in record.get('shared',[])]:
                    index=start-pe.base-section['rva']
                    assert index>=0 and index+end-start<=len(text)
                    text[index:index+end-start]=pe.read(start,end-start)
        samples=[]
        for item in options.sample:
            address,name=item.split(':',1);address=int(address,0)
            assert name.startswith('analysis_') and name.replace('_','').isalnum()
            index=address-pe.base-section['rva']
            assert 0<=index<len(text)
            samples.append('%x %x 0 %s\n'%(address-pe.base,pe.read(address,1)[0],name))
            text[index]=0xcc
        (capture/'text.bin').write_bytes(struct.pack('<II',section['rva'],len(text))+text)
        (capture/'entries.txt').write_text(''.join(samples))
        environment=dict(os.environ,WINEPREFIX=str(PREFIX),WINEDEBUG='-all',WINEPATH=r'R:\toolchains\1.0-nt\bin',LIB=LIBRARY,TZ='PST8PDT')
        for key in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB','LINK_REPRO','COPYCMD']:environment.pop(key,None)
        environment['TMP']=environment['TEMP']=windows(work)
        for key,value in spec.get('environment',{}).items():environment[key]=value.replace('{lib}',LIBRARY)
        environment.pop('LINK_CAPTURE_TMP',None)
        environment.pop('LINK_CAPTURE_BREAK',None)
        if spec.get('public_console_break'):environment['LINK_CAPTURE_BREAK']='1'
        if spec.get('controlled_tmp'):environment['LINK_CAPTURE_TMP']=windows(work)
        command=['wine',windows(base/'tooling/capture-original.exe'),PROGRAM,subprocess.list2cmdline(spec['arguments']),windows(capture/'stdout.bin'),windows(capture/'stderr.bin'),windows(capture/'events.jsonl'),windows(work),windows(capture/'text.bin'),windows(capture/'entries.txt'),windows(capture/'before.dll'),windows(capture/'after.dll'),'-']
        if spec.get('external_copies'):
            plan=[str(len(spec['external_copies']))]
            for item in spec['external_copies']:
                assert item['source'] in [Path(x['path']).name for x in spec['inputs']]
                assert item['image'].lower()==r'c:\windows\system32\cmd.exe'
                assert sha((PREFIX/'drive_c/windows/system32/cmd.exe').read_bytes())==item['image_sha256']
                plan.extend([item['image'],item['command'],windows(work),
                             windows(work/item['source']),windows(work/item['destination'])])
            (capture/'copies.plan').write_text('\n'.join(plan)+'\n')
            command.append(windows(capture/'copies.plan'))
        start=time.perf_counter()
        result=subprocess.run(command,env=environment,cwd=work,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=45)
        elapsed=time.perf_counter()-start
        (capture/'host.stdout').write_bytes(result.stdout)
        (capture/'host.stderr').write_bytes(result.stderr)
        events=[json.loads(line) for line in (capture/'events.jsonl').read_text().splitlines()]
        for item in spec.get('outputs',[]):
            file=work/item['output_name']
            if file.exists():
                destination=capture/item['output_name']
                destination.parent.mkdir(parents=True,exist_ok=True)
                shutil.copyfile(file,destination)
        working_files={str(p.relative_to(work)):dict(bytes=p.stat().st_size,sha256=sha(p.read_bytes())) for p in sorted(work.rglob('*')) if p.is_file()}
        if options.mode=='original' and 'final_files' in spec:assert sorted(working_files)==spec['final_files'], ('Original final inventory',working_files,spec['final_files'])
        receipt=dict(mode=options.mode,analysis_samples=options.sample,helper_sha256=sha((base/'tooling/capture-original.exe').read_bytes()),helper_source_sha256=sha((BINARY/'src/capture-original.c').read_bytes()),working_files=working_files,case=spec['case'],helper_exit=result.returncode,summary=events[-1],events_sha256=sha((capture/'events.jsonl').read_bytes()),original_sha256=sha(pe.data),accepted_count=len(config['accepted']),accepted_sha256=sha(json.dumps(config['accepted']).encode()),environment={key:environment.get(key) for key in ['LIB','LINK32_DB','LINK_REPRO','TMP','TEMP']},command=command,seconds=elapsed)
        (capture/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
        print(capture,flush=True)
        print(json.dumps(receipt['summary']),flush=True)
if __name__=='__main__':main()
