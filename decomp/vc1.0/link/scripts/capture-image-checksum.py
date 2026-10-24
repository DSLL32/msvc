#!/usr/bin/env python3
"""Explicit original-only direct image-checksum capture, never a fixture."""
import argparse,fcntl,hashlib,json,os,shutil,struct,subprocess,tempfile,time
from pathlib import Path
from binary import PE
from indexes import imports
from case_paths import ROOT,SCRATCH,PREFIX,PROGRAM,windows
BINARY=Path(__file__).resolve().parents[1];CASE=BINARY/'cases/direct-image-checksum'
sha=lambda b:hashlib.sha256(b).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--mode',choices=['original','masked'],required=True);ap.add_argument('--trial');ap.add_argument('--support',action='append',default=[]);ap.add_argument('--candidate',action='append',default=[]);args=ap.parse_args();assert not args.trial or args.mode=='masked'
    plan=json.loads((CASE/'plan.json').read_text());payload=CASE/'inputs/client.exe';assert sha(payload.read_bytes())==plan['input_sha256']
    pe=PE(ROOT/'toolchains/1.0-nt/bin/LINK.EXE');config=json.loads((BINARY/'functions.json').read_text());assert sha(pe.data)==config['target']['sha256']
    assert (PREFIX/'dosdevices/r:').resolve()==ROOT and (PREFIX/'dosdevices/s:').resolve()==SCRATCH
    base=SCRATCH/'vc1.0/link';parent=base/'new-captures/direct-image-checksum';parent.mkdir(parents=True,exist_ok=True)
    with (base/'capture.lock').open('a') as lease:
        fcntl.flock(lease,fcntl.LOCK_EX);work=base/'direct-image-checksum';work.mkdir(exist_ok=True);assert not list(work.iterdir()),work
        capture=Path(tempfile.mkdtemp(prefix=args.mode+'-',dir=parent));s=next(s for s in pe.sections if s['flags']&0x20);start=pe.base+s['rva'];text=bytearray(pe.read(start,s['size']))
        if args.mode=='masked':
            text[:]=b'\xcc'*len(text)
            for rec in config['driver']+json.loads((BINARY/'runtime.json').read_text())+imports(pe):
                if rec['name'] not in config['accepted'] and rec['name']!='_'+str(args.trial) and rec['name'] not in args.support and rec['name'] not in args.candidate:continue
                for a,b in [(rec['start'],rec['end'])]+[tuple(x) for x in rec.get('shared',[])]:text[a-start:b-start]=pe.read(a,b-a)
        shutil.copyfile(payload,work/'client.exe');(capture/'stages').mkdir()
        (capture/'text.bin').write_bytes(struct.pack('<II',s['rva'],len(text))+text)
        helper=base/'tooling/capture-image-checksum.exe';source=BINARY/'src/capture-image-checksum.c';assert helper.stat().st_mtime>=source.stat().st_mtime,'stale capture helper'
        command=['wine',windows(helper),PROGRAM,windows(capture/'text.bin'),windows(capture/'stdout.bin'),windows(capture/'stderr.bin'),windows(capture/'events.jsonl'),windows(work),windows(capture/'stages')]
        env=dict(os.environ,WINEPREFIX=str(PREFIX),WINEDEBUG='-all',TZ='PST8PDT')
        for key in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB','LINK_REPRO','LINK_CAPTURE_BREAK','LINK_CAPTURE_TMP']:env.pop(key,None)
        t=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,capture_output=True,timeout=40);elapsed=time.perf_counter()-t
        (capture/'host.stdout').write_bytes(r.stdout);(capture/'host.stderr').write_bytes(r.stderr)
        shutil.copyfile(source,capture/'controller.c')
        events=[json.loads(x) for x in (capture/'events.jsonl').read_text().splitlines()];files=[];(capture/'files').mkdir()
        for p in work.rglob('*'):
            if p.is_file():
                relative=p.relative_to(work);destination=capture/'files'/relative;destination.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,destination);files.append(dict(path=str(relative),bytes=p.stat().st_size,sha256=sha(p.read_bytes())))
        receipt=dict(mode=args.mode,trial=args.trial,support=args.support,candidate=args.candidate,command=command,seconds=elapsed,helper_exit=r.returncode,summary=events[-1],files=files,original_sha256=sha(pe.data),helper_sha256=sha(helper.read_bytes()),helper_source_sha256=sha(source.read_bytes()),accepted_count=len(config['accepted']),accepted_sha256=sha(json.dumps(config['accepted']).encode()),canonical_cwd=windows(work),scope='Explicit external direct support caller; actual mapped-image API setup, no private-global seeds, no public CLI claim')
        for p in capture.iterdir():
            if p.is_file():receipt[p.name]=dict(bytes=p.stat().st_size,sha256=sha(p.read_bytes()))
        (capture/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(capture,flush=True);print(json.dumps(events[-1]),flush=True)
        # Every attempted artifact has already been preserved, including failure.
        for p in work.iterdir():
            assert p.is_file();p.unlink()
        assert r.returncode==0 and events[-1]['failed']==0,receipt
if __name__=='__main__':main()
