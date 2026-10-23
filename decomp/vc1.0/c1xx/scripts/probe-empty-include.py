"""One bounded original-only public -I empty-string bug diagnostic.

Not a fixture or an equivalence claim. Original code is unchanged except for
entry/read/return debugger breakpoints, each restored before its instruction.
"""
import fcntl
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import time
from binary import PE
from case_paths import BINARY,CAPTURE_ROOT,SCRATCH,canonical_parts,windows


def main():
    directory=Path(tempfile.mkdtemp(prefix='empty-include-probe-',dir=CAPTURE_ROOT))
    started=time.perf_counter()
    with (directory/'build.log').open('w') as log:
        subprocess.run(['make','-j4','build/reference-empty-include-observer.exe'],cwd=BINARY,stdout=log,stderr=subprocess.STDOUT,check=True)
    build_seconds=time.perf_counter()-started
    spec=importlib.util.spec_from_file_location('c1xx_empty_tool',BINARY/'scripts/wine-tool.py')
    tool=importlib.util.module_from_spec(spec);spec.loader.exec_module(tool)
    image=PE(tool.TC/'bin/C1XX3232.EXE');data=bytearray(image.data)
    assert data[image.offset(0x24290)]==0x83
    data[image.offset(0x24290)]=0xcc
    executable=directory/'entry-original.exe';executable.write_bytes(data)
    fixture=BINARY/'../fixtures/c1xx-new-delete';phase=json.loads((fixture/'manifest.json').read_text())['phases']['c1']
    canonical='S:\\vc1.0\\c1xx\\empty-include-probe'
    flags=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(tool.TC/'bin'),include=windows(tool.TC/'include'))
    flags=flags.replace('-I R:', '-I "" -I R:')
    program=phase['command_line_template'].format(bin=windows(tool.TC/'bin'))
    lock=(CAPTURE_ROOT/'capture.lock').open('a');fcntl.flock(lock,fcntl.LOCK_EX)
    start=time.perf_counter();work=SCRATCH.joinpath(*canonical_parts(canonical));work.mkdir(exist_ok=True)
    for p in work.iterdir():
        assert p.is_file() and not p.is_symlink();p.unlink()
    for item in phase['inputs']:shutil.copy2(fixture/item['path'],work/Path(item['path']).name)
    command=['wine',windows(BINARY/'build/reference-empty-include-observer.exe'),windows(executable),'',windows(directory/'stdout.bin'),windows(directory/'stderr.bin'),windows(directory/'events.txt'),canonical,'MSC_CMD_FLAGS='+flags,program]
    env=tool.environment();env['C1XX_EMPTY_INCLUDE_LOG']=windows(directory/'observation.txt')
    setup_seconds=time.perf_counter()-start;start=time.perf_counter()
    result=subprocess.run(command,env=env,cwd=BINARY,stdin=subprocess.DEVNULL,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,timeout=90)
    seconds=time.perf_counter()-start
    shutil.copytree(work,directory/'case');fcntl.flock(lock,fcntl.LOCK_UN);lock.close()
    receipt=dict(scope='bounded original diagnostic only; explicit empty-I before valid toolchain include directory',command=command,observer_status=result.returncode,build_seconds=build_seconds,setup_seconds=setup_seconds,original_seconds=seconds)
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(directory);assert result.returncode==0
    print((directory/'observation.txt').read_text())


if __name__=='__main__':main()
