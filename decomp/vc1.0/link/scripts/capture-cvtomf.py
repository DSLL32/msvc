#!/usr/bin/env python3
"""Explicit original-only external C6 converter preflight; never a fixture."""
import argparse,fcntl,hashlib,json,os,shutil,struct,subprocess,tempfile,time
from binary import PE
from indexes import imports
from pathlib import Path
from case_paths import ROOT,SCRATCH,PREFIX,PROGRAM,windows
BINARY=Path(__file__).resolve().parents[1]
CASE=BINARY/'cases/direct-cvtomf-codeview'
sha=lambda b:hashlib.sha256(b).hexdigest()
def inventory(directory):
    return {str(p.relative_to(directory)):dict(bytes=p.stat().st_size,sha256=sha(p.read_bytes())) for p in sorted(directory.rglob('*')) if p.is_file()}
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--explicit-original-preflight',action='store_true',required=True)
    ap.add_argument('--trace-new',action='store_true');ap.add_argument('--masked',action='store_true');ap.add_argument('--trial');ap.add_argument('--variant',choices=['with-lines','no-lines'],default='with-lines')
    args=ap.parse_args();assert not args.trial or args.masked;assert not args.trace_new or (args.masked and not args.trial)
    assert (PREFIX/'dosdevices/r:').resolve()==ROOT and (PREFIX/'dosdevices/s:').resolve()==SCRATCH
    input_case=CASE if args.variant=='with-lines' else CASE/'variants/no-lines'
    layout=json.loads((input_case/'inputs/layout.json').read_text());data=(input_case/'inputs/legacy.obj').read_bytes();assert sha(data)==layout['sha256']
    base=SCRATCH/'vc1.0/link';helper=base/'tooling/capture-cvtomf.exe'
    assert helper.stat().st_mtime_ns >= (BINARY/'src/capture-cvtomf.c').stat().st_mtime_ns, 'original controller is stale; explicit make capture-cvtomf must succeed first'
    with (base/'capture.lock').open('a') as lease:
        fcntl.flock(lease,fcntl.LOCK_EX)
        work=base/'direct-cvtomf-codeview';work.mkdir(exist_ok=True);assert not list(work.iterdir())
        parent=base/'new-captures/direct-cvtomf-codeview';parent.mkdir(parents=True,exist_ok=True)
        capture=Path(tempfile.mkdtemp(prefix=('no-lines-' if args.variant=='no-lines' else '')+('masked-' if args.masked else 'original-'),dir=parent))
        (work/'legacy.obj').write_bytes(data);before=inventory(work)
        command=['wine',windows(helper),PROGRAM,windows(capture/'stdout.bin'),windows(capture/'stderr.bin'),windows(capture/'events.jsonl'),windows(work)]
        config=json.loads((BINARY/'functions.json').read_text())
        if args.masked:
            pe=PE(ROOT/'toolchains/1.0-nt/bin/LINK.EXE');section=next(s for s in pe.sections if s['flags']&0x20);start=pe.base+section['rva'];text=bytearray(b'\xcc'*section['size'])
            for rec in config['driver']+json.loads((BINARY/'runtime.json').read_text())+imports(pe):
                if rec['name'] not in config['accepted'] and rec['name']!='_'+str(args.trial):continue
                for a,b in [(rec['start'],rec['end'])]+[tuple(x) for x in rec.get('shared',[])]:text[a-start:b-start]=pe.read(a,b-a)
            (capture/'text.bin').write_bytes(struct.pack('<II',section['rva'],len(text))+text);command.append(windows(capture/'text.bin'))
            if args.trial:command.append(hex(next(r['start'] for r in config['driver'] if r['name']=='_'+args.trial)))
        if args.trace_new:
            names=config['accepted'][965:];assert len(names)==18 and all(next(r for r in config['driver'] if r['name']==name)['source']=='src/omf-debug.c' for name in names)
            records=[next(r for r in config['driver'] if r['name']==name) for name in names]
            (capture/'trace-points.bin').write_bytes(struct.pack('<I',len(records))+b''.join(struct.pack('<I64s',r['start'],r['name'].encode()) for r in records));command.extend(['0',windows(capture/'trace-points.bin')])
        env=dict(os.environ,WINEPREFIX=str(PREFIX),WINEDEBUG='-all',TZ='PST8PDT')
        for key in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB','LINK_REPRO','LINK_CAPTURE_BREAK','LINK_CAPTURE_TMP']:env.pop(key,None)
        started=time.perf_counter();run=subprocess.run(command,cwd=work,env=env,capture_output=True,timeout=30);elapsed=time.perf_counter()-started
        (capture/'host.stdout').write_bytes(run.stdout);(capture/'host.stderr').write_bytes(run.stderr)
        events=[json.loads(line) for line in (capture/'events.jsonl').read_text().splitlines()]
        after=inventory(work);shutil.copytree(work,capture/'files')
        receipt=dict(trace_new=args.trace_new,variant=args.variant,masked=args.masked,trial=args.trial,accepted_count=len(config['accepted']),accepted_sha256=sha(json.dumps(config['accepted']).encode()),status='Explicit original external-driver capture; acceptance requires separate native matching',command=command,seconds=elapsed,helper_exit=run.returncode,summary=events[-1],cwd=windows(work),argv=['cvtomf','-g','-o','converted.obj','legacy.obj'],before=before,after=after,original_sha256=sha((ROOT/'toolchains/1.0-nt/bin/LINK.EXE').read_bytes()),helper_sha256=sha(helper.read_bytes()),helper_source_sha256=sha((BINARY/'src/capture-cvtomf.c').read_bytes()),input_layout_sha256=sha((input_case/'inputs/layout.json').read_bytes()),boundary='Explicit owned external cdecl caller and public setjmp context; never claimed public LINK CLI or original application caller',error_policy='Actual longjmp entry is recorded and held before unwind, never bypassed to continue conversion')
        receipt['capture_files']=inventory(capture)
        (capture/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
        print(capture);print(json.dumps(events[-1]));print(json.dumps(after))
        for p in work.iterdir():
            assert p.is_file();p.unlink()
        assert run.returncode==0 and events[-1]['failed']==0,receipt
if __name__=='__main__':main()
