"""Explicit native i386 GDB frontier check; never executes a reference binary."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import resource
import shutil
import subprocess
import tempfile
import time
from case_paths import C2_SCRATCH, ROOT, native_work, windows

BINARY = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile',required=True)
    parser.add_argument('--reference',type=Path,required=True)
    args = parser.parse_args()
    resource.setrlimit(resource.RLIMIT_CORE,(0,0))
    started = time.perf_counter()
    config = json.loads((BINARY/'functions.json').read_text())
    reference = json.loads(args.reference.read_text())
    assert reference['profile'] == args.profile
    assert reference['config_sha256'] == digest(BINARY/'functions.json')
    assert not reference['logical'].startswith('exit:'), 'Complete cases use the native trace/stream/status/object check'
    case = config['cases'][args.profile]
    manifest = BINARY/case['fixture_manifest']
    assert digest(manifest) == case['fixture_manifest_sha256'] == reference['manifest_sha256']
    phase = json.loads(manifest.read_text())['phases']['c2']
    directory = Path(tempfile.mkdtemp(prefix=args.profile+'-gdb-',dir=C2_SCRATCH))
    work, environment = native_work(directory,phase['canonical_working_directory'])
    for item in phase['inputs']:
        source = manifest.parent/item['path']
        assert digest(source) == item['sha256']
        shutil.copyfile(source,work/source.name)
    environment.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',
        MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(ROOT/'toolchains/1.0-nt/bin')),
        RECON_PROGRAM_PATH=windows(ROOT/'toolchains/1.0-nt/bin/C23232.EXE'))
    name = re.sub(r'@\d+$','',reference['logical'][1:])
    assert re.fullmatch('[A-Za-z_][A-Za-z_0-9]*',name)
    executable = BINARY/'build/native/C23232.EXE'
    command = ['gdb','-q','--batch','-ex','set pagination off','-ex','run','-ex','bt 7',
        '-ex','printf "C2_FRONTIER_PC=%#x ENTRY=%#x\\n",$pc,&'+name,str(executable)]
    setup_seconds = time.perf_counter()-started
    run_started = time.perf_counter()
    result = subprocess.run(command,cwd=work,env=environment,stdin=subprocess.DEVNULL,
        stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30)
    execution_seconds = time.perf_counter()-run_started
    (directory/'gdb.log').write_bytes(result.stdout)
    matched = b'SIGTRAP' in result.stdout and ('in '+name+' (').encode() in result.stdout
    if not matched and b'SIGTRAP' in result.stdout:
        naked = '__declspec(naked) void '+name+'(void)' in (BINARY/'src/stubs.c').read_text()
        site = re.search(rb'C2_FRONTIER_PC=(0x[0-9a-f]+) ENTRY=(0x[0-9a-f]+)',result.stdout)
        matched = bool(naked and site and int(site[1],16)==int(site[2],16)+1)
    receipt = dict(logical=reference['logical'],matched=matched,executable_sha256=digest(executable),
        config_sha256=digest(BINARY/'functions.json'),canonical_working_directory=phase['canonical_working_directory'],
        setup_seconds=setup_seconds,execution_seconds=execution_seconds,seconds=time.perf_counter()-started,
        log_sha256=hashlib.sha256(result.stdout).hexdigest(),directory=str(directory))
    args.reference.with_name(args.reference.stem+'-native.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(result.stdout.decode('utf8',errors='replace'))
    print(receipt)
    assert matched, 'Native frontier differs; see '+str(directory/'gdb.log')


if __name__=='__main__':
    main()
