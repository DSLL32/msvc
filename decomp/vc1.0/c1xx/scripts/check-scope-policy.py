"""Verify the user-selected unsupported native cached-lookahead guard.

This focused rejection uses immutable valid-case inputs, injects only the
lookahead state in GDB, and requires the documented in-function INT3. It does
not claim malformed-input equivalence and never runs a Windows process.
"""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time
from case_paths import BINARY, CAPTURE_ROOT, ROOT, native_work, windows


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    started=time.monotonic()
    directory=Path(tempfile.mkdtemp(prefix='scope-native-policy-',dir=CAPTURE_ROOT))
    fixture=BINARY.parent/'fixtures/c1xx-copy-assignment'
    phase=json.loads((fixture/'manifest.json').read_text())['phases']['c1']
    executable=BINARY/'build/native-trace/C1XX3232.EXE'
    frozen={str(p):sha(p) for p in [executable,fixture/'manifest.json',BINARY/'functions.json',BINARY/'cases/native-traces.json']}
    work,environment=native_work(directory,phase['canonical_working_directory'])
    for item in phase['inputs']:
        path=fixture/item['path'];data=path.read_bytes()
        assert hashlib.sha256(data).hexdigest()==item['sha256']
        frozen[str(path)]=sha(path)
        (work/Path(item['path']).name).write_bytes(data)
    commands=['set pagination off','break begin_implicit_statement_scope',
        'run > '+str(directory/'stdout.bin')+' 2> '+str(directory/'stderr.bin'),
        'python','import gdb, json',
        'assert gdb.newest_frame().name() == "begin_implicit_statement_scope", "unexpected earlier stop"',
        'assert int(gdb.parse_and_eval("parser_lookahead")) == -1, "valid input did not establish the normal lookahead state"',
        'end','set var parser_lookahead = 2','disable 1','continue','python',
        'frame = gdb.newest_frame()',
        'assert frame.name() == "begin_implicit_statement_scope", "unsupported branch did not trap in scope entry"',
        'assert int(gdb.parse_and_eval("$_siginfo.si_signo")) == 5, "unsupported branch did not raise SIGTRAP"',
        'assert int(gdb.parse_and_eval("parser_lookahead")) == 2, "injected lookahead changed"',
        'pc = frame.pc()',
        'entry = int(gdb.parse_and_eval("(unsigned int)begin_implicit_statement_scope"))',
        'assert pc > entry and bytes(gdb.selected_inferior().read_memory(pc-1,1)) == b"\\xcc", "expected an interior inline INT3"',
        'result = {"status": "unsupported native path rejected", "function": frame.name(), "injected_lookahead": 2, "signal": 5, "pc_offset": pc-entry, "preceding_instruction": "cc"}',
        'open('+repr(str(directory/'observation.json'))+',"w").write(json.dumps(result,indent=2)+"\\n")','end']
    (directory/'commands.gdb').write_text('\n'.join(commands)+'\n')
    env={key:value for key,value in os.environ.items() if not key.startswith('RECON_')}
    for key in ['CL','_CL_','LINK','_LINK_']:env.pop(key,None)
    env.update(environment)
    env['TZ']='PST8PDT'
    env['MSC_CMD_FLAGS']=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(ROOT/'toolchains/1.0-nt/bin'),include=windows(ROOT/'toolchains/1.0-nt/include'))
    env['RECON_PROGRAM_PATH']=phase['command_line_template'].format(bin=windows(ROOT/'toolchains/1.0-nt/bin'))
    prepared=time.monotonic()
    result=subprocess.run(['gdb','-q','-batch','-x',str(directory/'commands.gdb'),'--args',str(executable)],cwd=work,env=env,stdin=subprocess.DEVNULL,capture_output=True,text=True,timeout=60)
    ran=time.monotonic();(directory/'gdb.log').write_text(result.stdout+result.stderr)
    assert result.returncode==0 and (directory/'observation.json').exists(),result.stdout+result.stderr
    assert all(sha(path)==value for path,value in frozen.items()),'frozen guard-test inputs changed'
    receipt=dict(status='pass',scope=__doc__,frozen=frozen,observation=json.loads((directory/'observation.json').read_text()),setup_seconds=prepared-started,debugger_seconds=ran-prepared,check_seconds=time.monotonic()-ran,total_seconds=time.monotonic()-started)
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(directory/'receipt.json')


if __name__=='__main__':main()
