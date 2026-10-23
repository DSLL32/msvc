#!/usr/bin/env python3
"""Native-only negative: unproved post-free queue lifetime must trap first."""
from pathlib import Path
import json,subprocess,tempfile,time
from state import BINARY

def check():
    started=time.perf_counter();scratch=Path.home()/'.ai-tmp/vc1.0-cvpack';scratch.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='nonempty-queue-guard-',dir=scratch)as folder:
        folder=Path(folder);result=folder/'result.json';commands=folder/'commands.gdb'
        commands.write_text('set pagination off\nset confirm off\nset startup-with-shell off\nset disable-randomization off\nbreak main\nrun\nset first_type_match = (TypeMatch *)1\npython\n'
            'import gdb,json\n'
            'try:gdb.execute("call finish_recursive_types()")\n'
            'except gdb.error as error:\n assert "signaled"in str(error),str(error)\n'
            'f=gdb.newest_frame()\n'
            'assert int(gdb.parse_and_eval("$_siginfo.si_signo"))==5 and f.name()=="finish_recursive_types"\n'
            'assert int(f.read_var("match"))==1\n'
            'pc=int(gdb.parse_and_eval("$pc"));assert bytes(gdb.selected_inferior().read_memory(pc-1,1))==b"\\xcc"\n'
            'json.dump(dict(signal=5,function=f.name(),queue=1),open('+repr(str(result))+',"w"))\nend\nquit\n')
        run=subprocess.run(['gdb','-q','-batch','-x',str(commands),str(BINARY/'build/native/CVPACK.EXE')],cwd=folder,capture_output=True)
        assert run.returncode==0,run.stdout.decode('latin1')+run.stderr.decode('latin1')
        assert json.loads(result.read_text())==dict(signal=5,function='finish_recursive_types',queue=1)
    print('rejected: native unsupported nonempty queue before post-free lifetime path (%.6fs)'%(time.perf_counter()-started))

if __name__=='__main__':check()
