#!/usr/bin/env python3
"""Four fixed native units borrowing the actual missing-include fatal context."""
import argparse,hashlib,importlib.util,json,os,subprocess,sys,time
from pathlib import Path
from case_paths import native_work,windows
BINARY=Path(__file__).resolve().parents[1];ROOT=BINARY.parents[2];CASE=BINARY/'cases/nested-input-resources'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def run(unit,directory,executable=None,debug=False,entry_only=False,seen_count=1104):
    start=time.perf_counter();directory=Path(directory).resolve();directory.mkdir(parents=True,exist_ok=False)
    m=json.loads((CASE/'manifest.json').read_text());plan=m['direct_plan'][unit];phase=m['phases'][plan['phase']]
    work,paths=native_work(directory,phase['canonical_working_directory'])
    assert sha(ROOT/phase['target'])==phase['sha256']
    for row in phase['inputs']:
        p=CASE/row['path'];assert sha(p)==row['sha256']and p.stat().st_size==row['bytes'];target=work/p.name;target.write_bytes(p.read_bytes());os.utime(target,ns=(row['mtime_ns'],row['mtime_ns']))
    for row in phase['toolchain_dependencies']:assert sha(ROOT/row['path'])==row['sha256']
    env=dict(LC_ALL='C',TZ='PST8PDT',RECON_PROGRAM_PATH=windows(ROOT/phase['target']),MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(ROOT/'toolchains/1.0-nt/bin'),include=windows(ROOT/'toolchains/1.0-nt/include')));env.update(paths)
    executable=Path(executable or BINARY/('build/resource-tests/C13232.EXE'if debug else'build/resource-tests-trace/C13232.EXE')).resolve();header=executable.read_bytes()[:20];assert header[:5]==b'\x7fELF\x01'and header[18:20]==b'\x03\x00'
    config=json.loads((BINARY/'functions.json').read_text());cache=json.loads((BINARY/'cases/native-traces.json').read_text());definitions=cache['application_definitions'];assert 0<=seen_count<=len(config['accepted']);prefix=set(config['accepted'][:seen_count])
    (directory/'filter').write_text(''.join(n+'\n'for n in definitions));(directory/'seen').write_text(''.join(n+'\n'for n,r in definitions.items()if r['logical']in prefix));trace=(directory/'entries').open('wb');env.update(RECON_TRACE_FD=str(trace.fileno()),RECON_TRACE_FILTER=str(directory/'filter'),RECON_TRACE_SEEN=str(directory/'seen'))
    spec=importlib.util.spec_from_file_location('terminal_status',ROOT/'lib/recon/status.py');status=importlib.util.module_from_spec(spec);spec.loader.exec_module(status)
    with status.TerminalStatus()as terminal:
        env.update(terminal.environment());settings=dict(unit=unit,directory=str(directory),function=plan['function'][1:].split('@')[0],entry_only=entry_only,arguments=[1,0,5,1][unit],pop=[4,0,0,0][unit]);(directory/'settings.json').write_text(json.dumps(settings,indent=2)+'\n')
        command=[str(executable),str(unit),str(directory/'report')]
        if debug:command=['gdb','--nx','--batch','-ex','set $resource_settings = '+json.dumps(str(directory/'settings.json')),'-ex','source '+str(BINARY/'scripts/resource-gdb.py'),str(executable)]
        setup=time.perf_counter()-start;began=time.perf_counter()
        with(directory/('gdb.log'if debug else'stdout')).open('wb')as output:
            child=subprocess.Popen(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=output,stderr=subprocess.STDOUT,pass_fds=(trace.fileno(),terminal.fd));child.wait(timeout=30)
        execution=time.perf_counter()-began;trace.close();logical=None;posix=child.returncode;pid=child.pid
        if debug:
            assert child.returncode==0,(directory/'gdb.log').read_text();state=json.loads((directory/'state.json').read_text());pid=state['inferior_pid'];posix=state.get('exit')
        if not entry_only and unit>=2:logical=terminal.read(pid,posix)
    (directory/'stderr').write_bytes(b'')
    receipt=dict(unit=unit,exit=posix,full_terminal_status=logical,setup_seconds=setup,execution_seconds=execution,total_seconds=time.perf_counter()-start,command=command,environment=env,executable_sha256=sha(executable),work=str(work),public_reachability=False,policy='Owned native i386 process. Fixed genuine missing-include context; no Wine/original/reference refresh. GDB only for fresh acceptance when explicitly requested.')
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');return receipt
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--unit',type=int,choices=range(4),required=True);p.add_argument('--directory',type=Path,required=True);p.add_argument('--exe',type=Path);p.add_argument('--debug',action='store_true');p.add_argument('--entry-only',action='store_true');p.add_argument('--seen-count',type=int,default=1104);a=p.parse_args();assert not a.entry_only or a.debug;print(json.dumps(run(a.unit,a.directory,a.exe,a.debug,a.entry_only,a.seen_count),indent=2))
