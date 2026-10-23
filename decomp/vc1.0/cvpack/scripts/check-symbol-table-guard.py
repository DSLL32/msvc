#!/usr/bin/env python3
"""Direct bounded native guard test; no original or Wine process is involved."""
import argparse,json,os,subprocess,tempfile,time
from pathlib import Path
from state import BINARY,ROOT,sha

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    args.output.mkdir(exist_ok=False,parents=True);timings={}
    def run(name,argv):
        start=time.monotonic();r=subprocess.run(argv,cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);timings[name]=time.monotonic()-start
        (args.output/(name+'.log')).write_bytes(r.stdout);assert r.returncode==0,r.stdout.decode('latin1')
    renamed=args.output/'main.o';executable=args.output/'guard'
    run('prepare',['objcopy','--redefine-sym','main=cvpack_original_main',str(BINARY/'build/native/main.o'),str(renamed)])
    objects=[p for p in sorted((BINARY/'build/native').glob('*.o'))if p.name!='main.o']
    shims=sorted((ROOT/'lib/recon/src').glob('*.c'))
    run('compile',['clang','-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-O0','-g','-DRECON_NATIVE_TEST','-Isrc','-I../../../lib/recon/include','-I../../../lib/recon/native','scripts/symbol-table-guard.c',str(renamed),*map(str,objects),*map(str,shims),'-o',str(executable)])
    commands=args.output/'commands.gdb';commands.write_text('''set pagination off
set confirm off
run
python
import gdb
frame=gdb.newest_frame()
assert frame.name()=="pack_new_types", frame.name()
assert int(frame.read_var("index"))==32
assert int(frame.read_var("kind"))==65535
assert int(frame.read_var("conversion"))==int(gdb.parse_and_eval("&symbol_conversions[32]"))
pc=int(gdb.parse_and_eval("$pc"))
assert bytes(gdb.selected_inferior().read_memory(pc-1,1))==b"\\xcc"
assert gdb.parse_and_eval("$_siginfo.si_signo")==5
print("Unknown symbol kind stopped before callback read: index32, kindFFFF, owning pack_new_types frame")
end
quit
''')
    environment=os.environ.copy();environment.pop('RECON_TRACE_FD',None)
    start=time.monotonic();r=subprocess.run(['gdb','-q','-batch','-x',str(commands),str(executable)],cwd=BINARY,env=environment,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);timings['execution_and_checks']=time.monotonic()-start
    (args.output/'gdb.log').write_bytes(r.stdout);assert r.returncode==0,r.stdout.decode('latin1')
    assert b'Unknown symbol kind stopped before callback read'in r.stdout
    receipt=dict(kind='native-only bounded unsupported-kind guard',native_sha256=sha(BINARY/'build/native/CVPACK.EXE'),source_sha256=sha(BINARY/'src/types.c'),control_sha256=sha(BINARY/'scripts/symbol-table-guard.c'),seconds=timings)
    (args.output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
