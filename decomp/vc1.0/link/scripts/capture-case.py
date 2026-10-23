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
        (capture/'text.bin').write_bytes(struct.pack('<II',section['rva'],len(text))+text)
        (capture/'entries.txt').write_bytes(b'')
        environment=dict(os.environ,WINEPREFIX=str(PREFIX),WINEDEBUG='-all',WINEPATH=r'R:\toolchains\1.0-nt\bin',LIB=LIBRARY,TZ='PST8PDT')
        for key in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB']:environment.pop(key,None)
        for key,value in spec.get('environment',{}).items():environment[key]=value.replace('{lib}',LIBRARY)
        environment['TMP']=environment['TEMP']=windows(work)
        command=['wine',windows(base/'tooling/capture-original.exe'),PROGRAM,subprocess.list2cmdline(spec['arguments']),windows(capture/'stdout.bin'),windows(capture/'stderr.bin'),windows(capture/'events.jsonl'),windows(work),windows(capture/'text.bin'),windows(capture/'entries.txt'),windows(capture/'before.dll'),windows(capture/'after.dll'),'-']
        start=time.perf_counter()
        result=subprocess.run(command,env=environment,cwd=work,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=45)
        elapsed=time.perf_counter()-start
        (capture/'host.stdout').write_bytes(result.stdout)
        (capture/'host.stderr').write_bytes(result.stderr)
        events=[json.loads(line) for line in (capture/'events.jsonl').read_text().splitlines()]
        for item in spec.get('outputs',[]):
            file=work/item['output_name']
            if file.exists():shutil.copyfile(file,capture/item['output_name'])
        receipt=dict(mode=options.mode,case=spec['case'],helper_exit=result.returncode,summary=events[-1],events_sha256=sha((capture/'events.jsonl').read_bytes()),original_sha256=sha(pe.data),accepted_count=len(config['accepted']),accepted_sha256=sha(json.dumps(config['accepted']).encode()),environment={key:environment.get(key) for key in ['LIB','LINK32_DB','TMP','TEMP']},command=command,seconds=elapsed)
        (capture/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
        print(capture,flush=True)
        print(json.dumps(receipt['summary']),flush=True)
if __name__=='__main__':main()
