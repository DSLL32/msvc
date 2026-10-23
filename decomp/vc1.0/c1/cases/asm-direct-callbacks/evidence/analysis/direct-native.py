#!/usr/bin/env python3
"""Native-only explicit direct support callback observation at a real marker."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time
from case_paths import native_work, windows
BINARY=Path(__file__).resolve().parents[1]
ROOT=BINARY.parents[2]
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--function',required=True,choices=['report_asm_expected_token','asm_recovery_value'])
    parser.add_argument('--arguments',required=True,nargs='+',type=lambda v:int(v,0))
    parser.add_argument('--directory',required=True,type=Path)
    parser.add_argument('--body',action='store_true')
    parser.add_argument('--exe',type=Path,default=BINARY/'build/native/C13232.EXE')
    options=parser.parse_args();start=time.perf_counter();directory=options.directory.resolve();directory.mkdir(parents=True,exist_ok=False)
    case=BINARY/'cases/asm-direct-callbacks';manifest=json.loads((case/'manifest.json').read_text());phase=manifest['phases']['c1']
    work,paths=native_work(directory,phase['canonical_working_directory'])
    for item in phase['inputs']:
        data=(case/item['path']).read_bytes();assert hashlib.sha256(data).hexdigest()==item['sha256'];(work/Path(item['path']).name).write_bytes(data)
    for item in phase['toolchain_dependencies']:
        assert hashlib.sha256((ROOT/item['path']).read_bytes()).hexdigest()==item['sha256']
    env=dict(LC_ALL='C',TZ='PST8PDT',RECON_PROGRAM_PATH=windows(ROOT/phase['target']),MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(ROOT/'toolchains/1.0-nt/bin'),include=windows(ROOT/'toolchains/1.0-nt/include')));env.update(paths)
    assert phase.get('stdio',{}).get('stderr_to_stdout'), 'This direct workflow requires the captured CL stream alias'
    (directory/'stderr').write_bytes(b'')
    config=json.loads((BINARY/'functions.json').read_text());formals=['actual','expected']if options.function=='report_asm_expected_token'else['token']
    assert len(formals)==len(options.arguments) and all(0<=v<=0xffffffff for v in options.arguments)
    settings=dict(directory=str(directory),function=options.function,arguments=options.arguments,formals=formals,body=options.body,buffered_streams=[r['name'][1:]for r in config['buffered_streams']]);(directory/'settings.json').write_text(json.dumps(settings,indent=2)+'\n')
    command=['gdb','--nx','--batch','-ex','set $direct_settings = '+json.dumps(str(directory/'settings.json')),'-ex','source '+str(BINARY/'scripts/direct-gdb.py'),str(options.exe.resolve())]
    setup=time.perf_counter()-start;run=time.perf_counter();result=subprocess.run(command,cwd=work,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);execution=time.perf_counter()-run;(directory/'gdb.log').write_bytes(result.stdout)
    assert result.returncode==0,(directory/'gdb.log').read_text();state=json.loads((directory/'state.json').read_text());receipt=dict(status='pass',public_reachability=False,function=options.function,arguments=options.arguments,body=options.body,executable_sha256=hashlib.sha256(options.exe.read_bytes()).hexdigest(),setup_seconds=setup,execution_seconds=execution,total_seconds=time.perf_counter()-start,state=state,environment=env);(directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print('Direct native',options.function,options.arguments,state['status'],execution)
if __name__=='__main__':main()
