#!/usr/bin/env python3
"""Explicit native external PDB units, using the real constructor and no Wine."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import time
from case_paths import native_work,windows

BINARY=Path(__file__).resolve().parents[1]
ROOT=BINARY.parents[2]
CASE=BINARY/'cases/legacy-pdb-object'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def prepare(unit,directory):
    start=time.perf_counter()
    directory=Path(directory).resolve();directory.mkdir(parents=True,exist_ok=False)
    manifest=json.loads((CASE/'manifest.json').read_text());phase=manifest['phases']['context']
    work,paths=native_work(directory,phase['canonical_working_directory'])
    assert sha(ROOT/phase['target'])==phase['sha256']
    for item in phase['inputs']:
        source=CASE/item['path'];assert sha(source)==item['sha256']
        (work/source.name).write_bytes(source.read_bytes())
    if unit in (0,1,4,5):
        item=manifest['direct_inputs'][0];source=CASE/item['path']
        assert sha(source)==item['sha256'] and source.stat().st_size==item['bytes']
        (work/'unit.pdb').write_bytes(source.read_bytes())
    for item in phase['toolchain_dependencies']:
        assert sha(ROOT/item['path'])==item['sha256']
    environment=dict(LC_ALL='C',TZ='PST8PDT',RECON_PROGRAM_PATH=windows(ROOT/phase['target']),
        MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(
            bin=windows(ROOT/'toolchains/1.0-nt/bin'),include=windows(ROOT/'toolchains/1.0-nt/include')))
    environment.update(paths)
    return directory,work,environment,time.perf_counter()-start


def run(unit,directory,executable=None):
    start=time.perf_counter()
    directory,work,environment,setup=prepare(unit,directory)
    executable=Path(executable or BINARY/'build/pdb-tests/C13232.EXE').resolve()
    header=executable.read_bytes()[:20]
    assert header[:5]==b'\x7fELF\x01' and header[18:20]==b'\x03\x00'
    command=[str(executable),str(unit),str(directory/'report')];began=time.perf_counter()
    with(directory/'stdout').open('wb')as output:
        result=subprocess.run(command,cwd=work,env=environment,stdin=subprocess.DEVNULL,
                              stdout=output,stderr=subprocess.STDOUT,timeout=30)
    (directory/'stderr').write_bytes(b'')
    receipt=dict(unit=unit,exit=result.returncode,setup_seconds=setup,
        execution_seconds=time.perf_counter()-began,total_seconds=time.perf_counter()-start,
        command=command,environment=environment,executable_sha256=sha(executable),work=str(work),
        public_reachability=False,policy='Owned native process only. Test-only actual constructor INT3/SIGTRAP context and controlled external stack; no original, Wine, debugger, or reference refresh.')
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    return receipt


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--unit',type=int,choices=range(6),required=True)
    parser.add_argument('--directory',type=Path,required=True)
    parser.add_argument('--executable',type=Path)
    options=parser.parse_args()
    print(json.dumps(run(options.unit,options.directory,options.executable),indent=2))


if __name__=='__main__':
    main()
