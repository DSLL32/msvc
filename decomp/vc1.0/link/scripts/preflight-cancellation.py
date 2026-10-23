#!/usr/bin/env python3
"""Explicit original-only public CTRL_BREAK preflight. Never a fixture dependency."""
import hashlib,json,os,select,shutil,subprocess,tempfile,time
from pathlib import Path
from case_paths import SCRATCH
BINARY=Path(__file__).resolve().parents[1];BASE=BINARY/'cases/console-cancellation'

def main():
    directory=Path(tempfile.mkdtemp(prefix='console-cancellation-preflight-',dir=SCRATCH/'vc1.0/link'))
    rfd,wfd=os.pipe();xlog=(directory/'xvfb.log').open('wb')
    command=['Xvfb','-displayfd',str(wfd),'-screen','0','1024x768x24','-nolisten','tcp']
    start=time.perf_counter();server=subprocess.Popen(command,pass_fds=(wfd,),stdout=xlog,stderr=subprocess.STDOUT);os.close(wfd)
    record=dict(scope='Explicit original-only public API transport; owned private Xserver/helperconsole/childgroup. No sleeps, internalcallbackdispatch or sharedprocess signals.',directory=str(directory),xvfb=dict(command=command,pid=server.pid,executable_sha256=hashlib.sha256(Path(shutil.which('Xvfb')).read_bytes()).hexdigest()),runs=[])
    try:
        ready,_,_=select.select([rfd],[],[],10);assert ready,'Private Xserver readiness timeout'
        display=os.read(rfd,64).decode().strip();assert display.isdigit();record['xvfb'].update(display=':'+display,readiness_seconds=time.perf_counter()-start)
        env=dict(os.environ,DISPLAY=':'+display)
        samples=['0x26b99:analysis_cancel_register_before','0x26b9e:analysis_cancel_register_after','0x1c700:analysis_cancel_boundary','0x270f0:analysis_decoder_cancel_handler','0x3d670:analysis_decoder_cancel_cleanup']
        for iteration in [1,2]:
            cmd=['python3','scripts/capture-case.py','--spec',str(BASE/'preflight-spec.json'),'--mode','original']
            for item in samples:cmd+=['--sample',item]
            t=time.perf_counter();run=subprocess.run(cmd,cwd=BINARY,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=45);elapsed=time.perf_counter()-t;(directory/('run%d.log'%iteration)).write_bytes(run.stdout)
            entry=dict(command=cmd,returncode=run.returncode,seconds=elapsed)
            lines=run.stdout.decode(errors='replace').splitlines()
            captures=[Path(line) for line in lines if line.startswith(str(SCRATCH/'vc1.0/link/new-captures/console-cancellation'))]
            if captures:
                capture=captures[0];entry['capture_directory']=str(capture);receipt=json.loads((capture/'receipt.json').read_text());entry['receipt']=receipt
                output=BASE/'preflight'/str(iteration);output.parent.mkdir(parents=True,exist_ok=True);shutil.copytree(capture,output)
                events=[json.loads(line) for line in (capture/'events.jsonl').read_text().splitlines()]
                entry['transport_events']=[e for e in events if e['event'].startswith('cancel_')]
                success=receipt['summary']['failed']==0 and receipt['summary']['root_exit']==1 and len([e for e in entry['transport_events'] if e['event']=='cancel_callback_context' and e['site']==0x270f0 and e['first_stack_word']==1])==1
            else:success=False
            record['runs'].append(entry);record['status']='public-transport-pass' if success else 'HOLD-public-transport-failed'
            (BASE/'transport-preflight.json').write_text(json.dumps(record,indent=2)+'\n')
            print(record['status'],iteration,round(elapsed,3),flush=True)
            if not success:raise SystemExit(1)
    finally:
        os.close(rfd);server.terminate()
        try:server.wait(timeout=5)
        except subprocess.TimeoutExpired:server.kill();server.wait(timeout=5)
        xlog.close();record['xvfb']['exit']=server.returncode
        (BASE/'transport-preflight.json').write_text(json.dumps(record,indent=2)+'\n')
if __name__=='__main__':main()
