"""Native-only rejection of valid public empty-I input's unsupported stack bug.

Use the existing allocation source with an empty include directory before its
valid directory. Require the exact interior guard; never launch an original.
"""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time
from case_paths import BINARY, ROOT, CAPTURE_ROOT, native_work, windows


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    started=time.perf_counter()
    directory=Path(tempfile.mkdtemp(prefix='include-native-policy-',dir=CAPTURE_ROOT))
    fixture=BINARY.parent/'fixtures/c1xx-new-delete'
    phase=json.loads((fixture/'manifest.json').read_text())['phases']['c1']
    executable=BINARY/'build/native-trace/C1XX3232.EXE'
    frozen={str(p):sha(p) for p in [executable,fixture/'manifest.json',BINARY/'functions.json',BINARY/'cases/native-traces.json']}
    work,drives=native_work(directory,phase['canonical_working_directory'])
    for item in phase['inputs']:
        path=fixture/item['path'];raw=path.read_bytes()
        assert len(raw)==item['bytes'] and hashlib.sha256(raw).hexdigest()==item['sha256']
        frozen[str(path)]=sha(path);(work/Path(item['path']).name).write_bytes(raw)
    commands=['set pagination off','break search_include_directories',
        'run > '+str(directory/'stdout.bin')+' 2> '+str(directory/'stderr.bin'),'python','import gdb,json',
        'assert gdb.newest_frame().name()=="search_include_directories"',
        'assert int(gdb.parse_and_eval("options.include_directories->count"))>=2',
        'assert int(gdb.parse_and_eval("*(char*)options.include_directories->items[0]"))==0',
        'end','disable 1','continue','python',
        'frame=gdb.newest_frame()',
        'assert frame.name()=="search_include_directories", "empty directory escaped the guarded function"',
        'assert int(gdb.parse_and_eval("$_siginfo.si_signo"))==5',
        'assert int(gdb.parse_and_eval("directory"))!=0 and int(gdb.parse_and_eval("*directory"))==0',
        'pc=frame.pc();entry=int(gdb.parse_and_eval("(unsigned int)search_include_directories"))',
        'assert pc>entry and bytes(gdb.selected_inferior().read_memory(pc-1,1))==b"\\xcc"',
        'result={"status":"valid public empty-I native path rejected","function":frame.name(),"signal":5,"pc_offset":pc-entry}',
        'open('+repr(str(directory/'observation.json'))+',"w").write(json.dumps(result,indent=2)+"\\n")','end']
    (directory/'commands.gdb').write_text('\n'.join(commands)+'\n')
    environment={k:v for k,v in os.environ.items() if not k.startswith('RECON_')}
    for key in ('CL','_CL_','LINK','_LINK_'):environment.pop(key,None)
    environment.update(drives,TZ='PST8PDT',
        MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(ROOT/'toolchains/1.0-nt/bin'),include=windows(ROOT/'toolchains/1.0-nt/include')),
        RECON_PROGRAM_PATH=phase['command_line_template'].format(bin=windows(ROOT/'toolchains/1.0-nt/bin')))
    environment['MSC_CMD_FLAGS']=environment['MSC_CMD_FLAGS'].replace('-I R:', '-I "" -I R:')
    prepared=time.perf_counter()
    result=subprocess.run(['gdb','-q','-batch','-x',str(directory/'commands.gdb'),'--args',str(executable)],cwd=work,env=environment,capture_output=True,text=True,timeout=60)
    ran=time.perf_counter();(directory/'gdb.log').write_text(result.stdout+result.stderr)
    assert result.returncode==0 and (directory/'observation.json').exists(),result.stdout+result.stderr
    assert all(sha(p)==v for p,v in frozen.items()),'frozen native policy inputs changed'
    report=dict(status='pass',frozen=frozen,observation=json.loads((directory/'observation.json').read_text()),setup_seconds=prepared-started,debugger_seconds=ran-prepared,check_seconds=time.perf_counter()-ran,total_seconds=time.perf_counter()-started)
    (directory/'receipt.json').write_text(json.dumps(report,indent=2)+'\n');print(directory/'receipt.json')


if __name__=='__main__':main()
