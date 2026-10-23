#!/usr/bin/env python3
"""Explicit original-only direct IDE callback baseline/frontier capture."""
import argparse,fcntl,hashlib,json,os,struct,subprocess,tempfile,time
from pathlib import Path
from binary import PE
from case_paths import ROOT,SCRATCH,PREFIX,PROGRAM,windows
from indexes import imports
BINARY=Path(__file__).resolve().parents[1]
CASE=BINARY/'cases/ide-callback-protocol'
sha=lambda b:hashlib.sha256(b).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--mode',choices=['original','masked'],required=True);ap.add_argument('--trial',choices=['initialize_ide_api','emit_ide_message','read_ide_flags']);args=ap.parse_args();assert not args.trial or args.mode=='masked'
    proof=json.loads((CASE/'analysis/proof.json').read_text());dependency=json.loads((CASE/'analysis/compiled-dependency.json').read_text())
    pe=PE(ROOT/'toolchains/1.0-nt/bin/LINK.EXE');assert sha(pe.data)==proof['original_sha256']
    dll=Path(dependency['path']);assert sha(dll.read_bytes())==dependency['sha256']
    assert (PREFIX/'dosdevices/r:').resolve()==ROOT and (PREFIX/'dosdevices/s:').resolve()==SCRATCH
    base=SCRATCH/'vc1.0/link';config=json.loads((BINARY/'functions.json').read_text())
    assert 'import:LoadLibraryA' in config['accepted'],'external loader must use already verified support thunk'
    with (base/'capture.lock').open('a') as lease:
        fcntl.flock(lease,fcntl.LOCK_EX)
        work=base/'ide-callback-protocol';work.mkdir(exist_ok=True);assert list(work.iterdir())==[]
        parent=base/'new-captures/ide-callback-protocol';parent.mkdir(parents=True,exist_ok=True)
        capture=Path(tempfile.mkdtemp(prefix=args.mode+'-',dir=parent))
        s=next(s for s in pe.sections if s['flags']&0x20);start=pe.base+s['rva'];text=bytearray(pe.read(start,s['size']))
        if args.mode=='masked':
            text[:]=b'\xcc'*len(text)
            for rec in config['driver']+json.loads((BINARY/'runtime.json').read_text())+imports(pe):
                if rec['name'] not in config['accepted'] and rec['name']!='_'+str(args.trial):continue
                for a,b in [(rec['start'],rec['end'])]+[tuple(x) for x in rec.get('shared',[])]:text[a-start:b-start]=pe.read(a,b-a)
        (capture/'text.bin').write_bytes(struct.pack('<II',s['rva'],len(text))+text)
        (capture/'exports.txt').write_text(' '.join('%x'%dependency['callbacks'][n]['rva'] for n in ['GetIdeProbe','IdeProbeChannel','IdeProbeWrite','IdeProbeAssertion'])+'\n')
        helper=base/'tooling/capture-ide-protocol.exe'
        command=['wine',windows(helper),PROGRAM,windows(dll),windows(capture/'exports.txt'),windows(capture/'text.bin'),windows(capture/'stdout.bin'),windows(capture/'stderr.bin'),windows(capture/'events.jsonl'),windows(capture/'protocol.bin'),windows(work)]
        env=dict(os.environ,WINEPREFIX=str(PREFIX),WINEDEBUG='-all',TZ='PST8PDT')
        for key in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB','LINK_REPRO','LINK_CAPTURE_BREAK','LINK_CAPTURE_TMP']:env.pop(key,None)
        begin=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,capture_output=True,timeout=30);elapsed=time.perf_counter()-begin
        (capture/'host.stdout').write_bytes(r.stdout);(capture/'host.stderr').write_bytes(r.stderr)
        events=[json.loads(x) for x in (capture/'events.jsonl').read_text().splitlines()]
        assert list(work.iterdir())==[], 'unexpected target file effect'
        assert sha(dll.read_bytes())==dependency['sha256'],'external dependency changed during capture'
        receipt=dict(status='capture complete; semantic validation separate',mode=args.mode,trial=args.trial,command=command,seconds=elapsed,helper_exit=r.returncode,summary=events[-1],boundary='External direct IDE API caller plus compiled C callback DLL; no public CLI/callback application-vertex claim',original_sha256=sha(pe.data),helper_sha256=sha(helper.read_bytes()),helper_source_sha256=sha((BINARY/'src/capture-ide-protocol.c').read_bytes()),dependency_sha256=dependency['sha256'],proof_sha256=sha((CASE/'analysis/proof.json').read_bytes()),accepted_count=len(config['accepted']),accepted_sha256=sha(json.dumps(config['accepted']).encode()),environment={'_MSC_IDE_FLAGS':'alpha.obj,beta.lib','set_by':'owned helper SetEnvironmentVariable before child launch'},target_files=[])
        for p in capture.iterdir():
            if p.is_file():receipt[p.name]=dict(bytes=p.stat().st_size,sha256=sha(p.read_bytes()))
        (capture/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(capture);print(json.dumps(events[-1]));assert r.returncode==0 and events[-1]['failed']==0,receipt
if __name__=='__main__':main()
