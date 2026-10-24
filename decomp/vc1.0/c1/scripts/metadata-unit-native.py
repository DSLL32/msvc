#!/usr/bin/env python3
"""Explicit native-only fixed metadata unit; no original refresh."""
import argparse,importlib.util,json,subprocess,time
from pathlib import Path
BINARY=Path(__file__).resolve().parents[1];CASE=BINARY/'cases/pch-metadata-interface'
spec=importlib.util.spec_from_file_location('metadata_context_native',Path(__file__).with_name('metadata-native.py'));context=importlib.util.module_from_spec(spec);spec.loader.exec_module(context)
def run(unit,directory,executable=None,debug=False,entry_only=False,seen_count=1108,frontier=None):
    start=time.perf_counter();assert 0<=unit<=9;assert frontier is None or(unit==3 and debug and entry_only and frontier=='minimum');directory,work,env,setup=context.prepare(directory);m=json.loads((CASE/'manifest.json').read_text());plan=m['direct_plan'][unit]
    executable=Path(executable or BINARY/('build/metadata-tests/C13232.EXE'if debug else'build/metadata-tests-trace/C13232.EXE')).resolve();header=executable.read_bytes()[:20];assert header[:5]==b'\x7fELF\x01'and header[18:20]==b'\x03\x00'
    config=json.loads((BINARY/'functions.json').read_text());cache=json.loads((BINARY/'cases/native-traces.json').read_text());definitions=cache['application_definitions'];assert 0<=seen_count<=len(config['accepted']);prefix=set(config['accepted'][:seen_count]);assert'mark_pch_ir_node'in definitions
    (directory/'filter').write_text(''.join(n+'\n'for n in definitions));(directory/'seen').write_text(''.join(n+'\n'for n,r in definitions.items()if r['logical']in prefix))
    with(directory/'entries').open('wb')as trace:
        env.update(RECON_TRACE_FD=str(trace.fileno()),RECON_TRACE_FILTER=str(directory/'filter'),RECON_TRACE_SEEN=str(directory/'seen'))
        settings=dict(unit=unit,directory=str(directory),function='smaller_size'if frontier else plan['function'][1:].split('@')[0],entry_only=entry_only,frontier=frontier,arguments=4 if unit==9 else 0 if unit==8 else 2 if frontier or unit==5 else 3 if unit in[3,4]else 1,pop=8 if frontier else 0 if unit in[5,8,9]else 12 if unit in[3,4]else 4);(directory/'settings.json').write_text(json.dumps(settings,indent=2)+'\n')
        command=[str(executable),str(unit),str(directory/'report')]
        if debug:command=['gdb','--nx','--batch','-ex','set $metadata_settings = '+json.dumps(str(directory/'settings.json')),'-ex','source '+str(BINARY/'scripts/metadata-unit-gdb.py'),str(executable)]
        setup=time.perf_counter()-start;began=time.perf_counter()
        with(directory/('gdb.log'if debug else'stdout')).open('wb')as output:
            child=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=output,stderr=subprocess.STDOUT,pass_fds=(trace.fileno(),),timeout=30)
    execution=time.perf_counter()-began;exit=child.returncode
    if debug:
        assert child.returncode==0,(directory/'gdb.log').read_text();state=json.loads((directory/'state.json').read_text());exit=state.get('exit')
    (directory/'stderr').write_bytes(b'');receipt=dict(unit=unit,exit=exit,setup_seconds=setup,execution_seconds=execution,total_seconds=time.perf_counter()-start,command=command,environment=env,executable_sha256=context.sha(executable),work=str(work),public_reachability=False,policy='Owned native i386 process; fixed real completed-call owners. No Wine/reference refresh. GDB only for fresh acceptance when explicitly requested.')
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');return receipt
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--unit',type=int,choices=range(10),required=True);p.add_argument('--directory',type=Path,required=True);p.add_argument('--exe',type=Path);p.add_argument('--debug',action='store_true');p.add_argument('--entry-only',action='store_true');p.add_argument('--seen-count',type=int,default=1108);p.add_argument('--frontier',choices=['minimum']);a=p.parse_args();assert not a.entry_only or a.debug;r=run(a.unit,a.directory,a.exe,a.debug,a.entry_only,a.seen_count,a.frontier);print('Metadata unit',a.unit,'exit',r['exit'],'execution',r['execution_seconds'],'seconds')
