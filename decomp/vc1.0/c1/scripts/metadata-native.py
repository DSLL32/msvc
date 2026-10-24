#!/usr/bin/env python3
"""Explicit native-only PCH metadata context capture; never refresh originals."""
import argparse,hashlib,json,os,subprocess,time
from pathlib import Path
from case_paths import native_work,windows
BINARY=Path(__file__).resolve().parents[1];ROOT=BINARY.parents[2];CASE=BINARY/'cases/pch-metadata-interface'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def prepare(directory):
    start=time.perf_counter();directory=Path(directory).resolve();directory.mkdir(parents=True,exist_ok=False);m=json.loads((CASE/'manifest.json').read_text());phase=m['phases']['context'];work,paths=native_work(directory,phase['canonical_working_directory'])
    assert sha(ROOT/phase['target'])==phase['sha256']
    for row in phase['inputs']:
        p=CASE/row['path'];assert sha(p)==row['sha256']and p.stat().st_size==row['bytes'];out=work/p.name;out.write_bytes(p.read_bytes());os.utime(out,ns=(row['mtime_ns'],row['mtime_ns']))
    for row in phase['toolchain_dependencies']:assert sha(ROOT/row['path'])==row['sha256']
    environment=dict(LC_ALL='C',TZ='PST8PDT',RECON_PROGRAM_PATH=windows(ROOT/phase['target']),MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(ROOT/'toolchains/1.0-nt/bin'),include=windows(ROOT/'toolchains/1.0-nt/include')));environment.update(paths)
    return directory,work,environment,time.perf_counter()-start
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);p.add_argument('--exe',type=Path,default=BINARY/'build/native/C13232.EXE');args=p.parse_args();start=time.perf_counter();directory,work,env,setup=prepare(args.directory);exe=args.exe.resolve();header=exe.read_bytes()[:20];assert header[:5]==b'\x7fELF\x01'and header[18:20]==b'\x03\x00'
    command=['gdb','--nx','--batch','-ex','set $metadata_directory = '+json.dumps(str(directory)),'-ex','source '+str(BINARY/'scripts/metadata-context-gdb.py'),str(exe)];began=time.perf_counter()
    with(directory/'gdb.log').open('wb')as output:result=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=output,stderr=subprocess.STDOUT,timeout=30)
    assert result.returncode==0,(directory/'gdb.log').read_text();state=json.loads((directory/'state.json').read_text());assert state['status']=='natural-context-proved';(directory/'stderr').write_bytes(b'');receipt=dict(status='pass',public_reachability=True,scope='Read-only actual finish_call_arguments owner/range proof; no helper invocation or new function credit.',executable_sha256=sha(exe),environment=env,work=str(work),command=command,setup_seconds=setup,execution_seconds=time.perf_counter()-began,total_seconds=time.perf_counter()-start);(directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print('native actual context passed',receipt['execution_seconds'])
