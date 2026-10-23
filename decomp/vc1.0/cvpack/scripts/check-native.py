#!/usr/bin/env python3
"""Native-only fixtures against explicitly frozen original boundaries/results."""
import argparse
import json
import os
import resource
import shlex
from pathlib import Path
import subprocess
import time
from state import BINARY, ROOT, load, case, prepare, sha

def run_case(name, directory, debug=False):
    start = time.perf_counter()
    config, original = load()
    record = case(name)
    work = directory / 'work'
    prepare(record, work)
    stdout, stderr = directory/'stdout', directory/'stderr'
    report = directory/'gdb.json'
    script = directory/'commands.gdb'
    executable = BINARY / 'build/native/CVPACK.EXE'
    expected = record['current']
    canonical='Z:'+str(ROOT/config['target']['path']).replace('/','\\')
    script.write_text('set pagination off\nset confirm off\nset startup-with-shell on\n'
        'set disable-randomization off\nset args '+' '.join(map(shlex.quote,record['argv']))+'\n'
        'break main\nrun '+' '.join(map(shlex.quote,record['argv']))+' > '+shlex.quote(str(directory/'debug-stdout'))+' 2> '+shlex.quote(str(directory/'debug-stderr'))+'\n'
        'set argv[0] = recon_pgmptr\ncontinue\npython\n'
        'import gdb,json\n'
        'frames=[]\nf=gdb.newest_frame()\n'
        'while f:\n frames.append(f.name()); f=f.older()\n'
        'f=gdb.newest_frame()\n'
        'while f and f.name()!= "main": f=f.older()\n'
        'if f: f.select()\n'
        'signal=gdb.parse_and_eval("$_siginfo.si_signo")\n'
        'argc=int(gdb.parse_and_eval("argc"))\n'
        'argv=[gdb.parse_and_eval("argv[%d]"%i).string() for i in range(argc)]\n'
        'fields=["_ptr","_cnt","_base","_flag","_file","_charbuf","_bufsiz","_tmpfname"]\n'
        'stream={n:int(gdb.parse_and_eval("recon_iob[1]."+n)) for n in fields}\n'
        'state={"feedback":int(gdb.parse_and_eval("feedback")),"stdout_osfile":int(gdb.parse_and_eval("recon_osfile[1]"))}\n'
        'json.dump(dict(signal=int(signal),frames=frames,argc=argc,argv=argv,stdout=stream,state=state),open('+repr(str(report))+',"w"))\n'
        'end\nquit\n')
    env = os.environ.copy()
    for variable in list(env):
        if variable.startswith(('WINE','RECON_')) or variable in ('CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','_MSC_IDE_FLAGS','LINK32_DB'):
            env.pop(variable)
    env.update(LC_ALL='C',TZ='PST8PDT',RECON_PROGRAM_PATH=canonical,
               LIB='Z:'+str(ROOT/'toolchains/1.0-nt/lib').replace('/','\\'))
    symbols=directory/'trace-filter'
    symbols.write_text(''.join(r['native']+'\n' for r in config['functions']))
    seen=directory/'trace-seen'
    seen.write_bytes(b'')
    trace=directory/'trace'
    setup = time.perf_counter()-start
    run = time.perf_counter()
    def no_core():
        resource.setrlimit(resource.RLIMIT_CORE,(0,0))
    with trace.open('wb') as log,stdout.open('wb') as out,stderr.open('wb') as err:
        env.update(RECON_TRACE_FD=str(log.fileno()),RECON_TRACE_FILTER=str(symbols),
                   RECON_TRACE_SEEN=str(seen))
        child=subprocess.run([canonical,*record['argv']],executable=str(executable),cwd=work,env=env,
            stdout=out,stderr=err,stdin=subprocess.DEVNULL,timeout=10,
            pass_fds=(log.fileno(),),preexec_fn=no_core)
    execution = time.perf_counter()-run
    debug_start=time.perf_counter()
    if debug:
        for variable in list(env):
            if variable.startswith('RECON_TRACE_'): env.pop(variable)
        # A debugger rerun starts from an independent copy of every input.
        debug_work=directory/'debug-work'
        prepare(record,debug_work)
        # GDB records frames/state; direct execution above owns fixture checks.
        debugger=subprocess.run(['gdb','-q','-batch','-x',str(script),str(executable)],
            cwd=debug_work,env=env,stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=30)
        (directory/'gdb.stdout').write_bytes(debugger.stdout)
        (directory/'gdb.stderr').write_bytes(debugger.stderr)
        assert debugger.returncode==0,debugger.stdout.decode('latin1')+debugger.stderr.decode('latin1')
    debugging=time.perf_counter()-debug_start if debug else 0
    check = time.perf_counter()
    assert expected['terminal']=='trap', 'completed fixture support will be enabled with first completed case'
    assert child.returncode==-5,child.returncode
    entries=trace.read_text().splitlines()
    assert entries and entries[-1]==expected['native_function'],entries
    application={r['native'] for r in config['functions'] if r['kind']=='application'}
    assert [n for n in entries if n in application]==expected.get('application_trace',[]),entries
    if debug:
        actual=json.loads(report.read_text())
        assert actual['signal']==5,actual
        assert expected['native_function'] in actual['frames'],actual
    for stream in ('stdout','stderr'):
        reference = BINARY / 'cases' / name / expected[stream]
        assert (directory/stream).read_bytes()==reference.read_bytes(), stream+' mismatch'
    for item in record.get('inputs',[]):
        assert sha(work/item['name'])==item['sha256'], 'unexpected artifact mutation before frontier'
    return dict(case=name,terminal=expected['terminal'],function=expected['native_function'],
        executable_sha256=sha(executable),configuration_sha256=sha(BINARY/'functions.json'),
        seconds=dict(setup=setup,execution=execution,debugging=debugging,
                     checks=time.perf_counter()-check,total=time.perf_counter()-start))

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case')
    parser.add_argument('--output',type=Path)
    parser.add_argument('--debug',action='store_true')
    args=parser.parse_args()
    directory=args.output or Path.home()/'.ai-tmp/vc1.0-cvpack'/('native-'+str(time.time_ns()))
    directory.mkdir(parents=True,exist_ok=False)
    names=[args.case] if args.case else [r['name'] for r in json.loads((BINARY/'cases/queue.json').read_text())['cases'] if r['status']!='queued']
    receipts=[]
    for name in names:
        case_directory=directory/name
        case_directory.mkdir()
        receipts.append(run_case(name,case_directory,args.debug))
    (directory/'receipt.json').write_text(json.dumps(receipts,indent=2)+'\n')
    print(json.dumps(receipts,indent=2))
    print(directory/'receipt.json')

if __name__=='__main__':
    main()
