#!/usr/bin/env python3
"""Explicit original-only direct COMDAT diagnostic capture. Never a fixture."""
import argparse,fcntl,hashlib,json,os,struct,subprocess,tempfile,time
from pathlib import Path
from binary import PE
from indexes import imports
from case_paths import ROOT,SCRATCH,PREFIX,PROGRAM,windows
BINARY=Path(__file__).resolve().parents[1];CASE=BINARY/'cases/comdat-graph-diagnostics'
sha=lambda b:hashlib.sha256(b).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--mode',choices=['original','masked'],required=True);ap.add_argument('--trial');args=ap.parse_args();assert not args.trial or args.mode=='masked'
    proof=json.loads((CASE/'analysis/proof.json').read_text());pe=PE(ROOT/'toolchains/1.0-nt/bin/LINK.EXE');assert sha(pe.data)==proof['original_sha256']
    assert (PREFIX/'dosdevices/r:').resolve()==ROOT and (PREFIX/'dosdevices/s:').resolve()==SCRATCH
    payload=(CASE/proof['input']['path']).read_bytes();assert sha(payload)==proof['input']['sha256']
    config=json.loads((BINARY/'functions.json').read_text());base=SCRATCH/'vc1.0/link'
    with (base/'capture.lock').open('a') as lease:
        fcntl.flock(lease,fcntl.LOCK_EX);work=base/'comdat-graph-diagnostics';work.mkdir(exist_ok=True);assert list(work.iterdir())==[]
        parent=base/'new-captures/comdat-graph-diagnostics';parent.mkdir(parents=True,exist_ok=True);capture=Path(tempfile.mkdtemp(prefix=args.mode+'-',dir=parent))
        s=next(s for s in pe.sections if s['flags']&0x20);start=pe.base+s['rva'];text=bytearray(pe.read(start,s['size']))
        if args.mode=='masked':
            text[:]=b'\xcc'*len(text)
            for rec in config['driver']+json.loads((BINARY/'runtime.json').read_text())+imports(pe):
                if rec['name'] not in config['accepted'] and rec['name']!='_'+str(args.trial):continue
                for a,b in [(rec['start'],rec['end'])]+[tuple(x) for x in rec.get('shared',[])]:text[a-start:b-start]=pe.read(a,b-a)
        (capture/'text.bin').write_bytes(struct.pack('<II',s['rva'],len(text))+text)
        helper=base/'tooling/capture-comdat-diagnostics.exe';command=['wine',windows(helper),PROGRAM,windows(CASE/proof['input']['path']),windows(capture/'text.bin'),windows(capture/'stdout.bin'),windows(capture/'stderr.bin'),windows(capture/'events.jsonl'),windows(work)]
        env=dict(os.environ,WINEPREFIX=str(PREFIX),WINEDEBUG='-all',TZ='PST8PDT')
        for key in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB','LINK_REPRO','LINK_CAPTURE_BREAK','LINK_CAPTURE_TMP']:env.pop(key,None)
        start_time=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,capture_output=True,timeout=30);elapsed=time.perf_counter()-start_time
        (capture/'host.stdout').write_bytes(r.stdout);(capture/'host.stderr').write_bytes(r.stderr)
        events=[json.loads(x) for x in (capture/'events.jsonl').read_text().splitlines()];files=[str(x.relative_to(work)) for x in work.rglob('*') if x.is_file()]
        receipt=dict(mode=args.mode,trial=args.trial,command=command,seconds=elapsed,helper_exit=r.returncode,summary=events[-1],files=files,original_sha256=sha(pe.data),helper_sha256=sha(helper.read_bytes()),helper_source_sha256=sha((BINARY/'src/capture-comdat-diagnostics.c').read_bytes()),proof_sha256=sha((CASE/'analysis/proof.json').read_bytes()),input_sha256=sha(payload),accepted_count=len(config['accepted']),accepted_sha256=sha(json.dumps(config['accepted']).encode()),boundary='Explicit direct caller-owned fixed input; not public CLI; no global mode seed or output normalization')
        for p in capture.iterdir():
            if p.is_file():receipt[p.name]=dict(bytes=p.stat().st_size,sha256=sha(p.read_bytes()))
        (capture/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(capture);print(json.dumps(events[-1]));assert r.returncode==0 and events[-1]['failed']==0 and not files,receipt
if __name__=='__main__':main()
