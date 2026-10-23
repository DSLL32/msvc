#!/usr/bin/env python3
"""Explicit original-only callback baseline/frontier capture. Never a fixture."""
import argparse,fcntl,hashlib,json,os,shutil,struct,subprocess,tempfile,time
from pathlib import Path
from binary import PE
from case_paths import ROOT,SCRATCH,PREFIX,PROGRAM,windows
from indexes import imports
BINARY=Path(__file__).resolve().parents[1]
CASE=BINARY/'cases/legacy-resource-diagnostics'
sha=lambda data:hashlib.sha256(data).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--function',choices=['strings','menu','dialog'],required=True)
    ap.add_argument('--mode',choices=['original','masked'],required=True)
    ap.add_argument('--trial',action='store_true',help='Explicit pending candidate body; not an acceptance')
    args=ap.parse_args()
    assert not args.trial or args.mode=='masked'
    proof=json.loads((CASE/'analysis/provenance.json').read_text())
    item=next(x for x in proof['payloads'] if x['label']==args.function)
    pe=PE(ROOT/'toolchains/1.0-nt/bin/LINK.EXE')
    assert sha(pe.data)==proof['original_sha256']
    assert (PREFIX/'dosdevices/r:').resolve()==ROOT
    assert (PREFIX/'dosdevices/s:').resolve()==SCRATCH
    assert sha((CASE/'inputs'/f'{args.function}.bin').read_bytes())==item['sha256']
    config=json.loads((BINARY/'functions.json').read_text())
    base=SCRATCH/'vc1.0/link';base.mkdir(parents=True,exist_ok=True)
    with (base/'capture.lock').open('a') as lease:
        fcntl.flock(lease,fcntl.LOCK_EX)
        work=base/'legacy-resource-diagnostics';work.mkdir(exist_ok=True)
        assert list(work.iterdir())==[], 'External callback test work directory must be empty'
        parent=base/'new-captures/legacy-resource-diagnostics';parent.mkdir(parents=True,exist_ok=True)
        capture=Path(tempfile.mkdtemp(prefix=args.function+'-'+args.mode+'-',dir=parent))
        text_section=next(s for s in pe.sections if s['flags']&0x20)
        start=pe.base+text_section['rva'];text=bytearray(pe.read(start,text_section['size']))
        if args.mode=='masked':
            text[:]=b'\xcc'*len(text)
            for rec in config['driver']+json.loads((BINARY/'runtime.json').read_text())+imports(pe):
                if rec['name'] not in config['accepted'] and not (args.trial and rec['name']=='_display_legacy_'+args.function):continue
                for a,b in [(rec['start'],rec['end'])]+[tuple(s) for s in rec.get('shared',[])]:
                    assert start<=a<b<=start+len(text)
                    text[a-start:b-start]=pe.read(a,b-a)
        (capture/'text.bin').write_bytes(struct.pack('<II',text_section['rva'],len(text))+text)
        helper=base/'tooling/capture-legacy-diagnostics.exe'
        command=['wine',windows(helper),PROGRAM,windows(CASE/'inputs'/f'{args.function}.bin'),str(item['type']),hex(item['function_va']),windows(capture/'text.bin'),windows(capture/'stdout.bin'),windows(capture/'stderr.bin'),windows(capture/'events.jsonl'),windows(work)]
        environment=dict(os.environ,WINEPREFIX=str(PREFIX),WINEDEBUG='-all',TZ='PST8PDT')
        for key in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB','LINK_REPRO','LINK_CAPTURE_BREAK','LINK_CAPTURE_TMP']:environment.pop(key,None)
        begin=time.perf_counter()
        result=subprocess.run(command,cwd=work,env=environment,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=30)
        elapsed=time.perf_counter()-begin
        (capture/'host.stdout').write_bytes(result.stdout);(capture/'host.stderr').write_bytes(result.stderr)
        events=[json.loads(x) for x in (capture/'events.jsonl').read_text().splitlines()]
        files={str(p.relative_to(work)):dict(bytes=p.stat().st_size,sha256=sha(p.read_bytes())) for p in work.rglob('*') if p.is_file()}
        assert files=={},files
        receipt=dict(mode=args.mode,trial=args.trial,function=args.function,boundary='External minimal cdecl code test; not public CLI or original application caller',command=command,helper_exit=result.returncode,seconds=elapsed,original_sha256=sha(pe.data),helper_sha256=sha(helper.read_bytes()),helper_source_sha256=sha((BINARY/'src/capture-legacy-diagnostics.c').read_bytes()),provenance_sha256=sha((CASE/'analysis/provenance.json').read_bytes()),input_sha256=item['sha256'],accepted_count=len(config['accepted']),accepted_sha256=sha(json.dumps(config['accepted']).encode()),files=files,summary=events[-1])
        for name in ['stdout.bin','stderr.bin','events.jsonl','text.bin']:
            data=(capture/name).read_bytes();receipt[name]=dict(bytes=len(data),sha256=sha(data))
        (capture/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
        print(capture);print(json.dumps(receipt['summary']))
        assert result.returncode==0 and receipt['summary']['failed']==0,receipt
if __name__=='__main__':main()
