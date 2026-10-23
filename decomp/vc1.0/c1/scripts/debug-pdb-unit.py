#!/usr/bin/env python3
"""Explicit native GDB method-entry/return proof, outside cached fixtures."""
import argparse
import importlib.util
import json
from pathlib import Path
import shlex
import subprocess
import time

HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('pdb_native',HERE/'pdb-native.py')
runner=importlib.util.module_from_spec(spec);spec.loader.exec_module(runner)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--unit',type=int,choices=range(6),required=True)
    parser.add_argument('--directory',type=Path,required=True)
    options=parser.parse_args()
    directory,work,environment,setup=runner.prepare(options.unit,options.directory)
    output=directory/'report';stdout=directory/'stdout';state=directory/'state.json'
    script='''set pagination off
set confirm off
set disable-randomization off
python
import gdb,json,struct
calls=[]
u=lambda expression:int(gdb.parse_and_eval(expression))&0xffffffff
read=lambda address,count:bytes(gdb.selected_inferior().read_memory(address,count))
class Returned(gdb.Breakpoint):
 def __init__(self,entry):
  super().__init__('*'+str(entry['return_address']),temporary=True)
  self.entry=entry
 def stop(self):
  entry=self.entry
  after={name:u('$'+name)for name in ['esp','ebx','esi','edi','ebp','fctrl','eax']}
  assert after['esp']==entry['registers']['esp']+4+entry['callee_pop']
  for name in ['ebx','esi','edi','ebp','fctrl']:assert after[name]==entry['registers'][name]
  entry['returned_registers']=after
  entry['object_after']=read(entry['registers']['ecx'],116).hex()
  return False
class Entry(gdb.Breakpoint):
 def __init__(self,name,pop):
  super().__init__('*'+name)
  self.name=name;self.pop=pop
 def stop(self):
  registers={name:u('$'+name)for name in ['eip','esp','ecx','edx','ebx','esi','edi','ebp','fctrl']}
  frame=struct.unpack('<IIII',read(registers['esp'],16))
  entry=dict(function=self.name,registers=registers,return_address=frame[0],callee_pop=self.pop,
             object_before=read(registers['ecx'],116).hex())
  if self.pop:
   entry.update(path_pointer=frame[1],path=read(frame[1],9).hex(),mode_pointer=frame[2],
                mode=read(frame[2],3).hex(),signature=frame[3])
   assert entry['path']==b'unit.pdb\\0'.hex() and entry['signature']==0x31415926
  calls.append(entry);Returned(entry)
  return False
Entry('open_program_database_object',12)
Entry('reload_program_database',0)
end
run %(unit)d %(report)s >%(stdout)s 2>&1
python
assert u('$eip')==u('(unsigned)create_program_database')+1
end
signal SIGTRAP
python
assert [entry['function']for entry in calls]==%(expected)r
assert all('returned_registers'in entry for entry in calls)
open(%(state)r,'w').write(json.dumps(calls,indent=2)+'\\n')
end
'''%dict(unit=options.unit,report=shlex.quote(str(output)),stdout=shlex.quote(str(stdout)),
         state=str(state),expected=['open_program_database_object']+(['reload_program_database']if options.unit>=4 else[]))
    commands=directory/'commands.gdb';commands.write_text(script)
    executable=runner.BINARY/'build/pdb-tests/C13232.EXE';start=time.perf_counter()
    result=subprocess.run(['gdb','-q','-batch','-x',str(commands),str(executable)],
        cwd=work,env=environment,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30)
    (directory/'gdb.log').write_bytes(result.stdout)
    receipt=dict(unit=options.unit,exit=result.returncode,setup_seconds=setup,
        execution_seconds=time.perf_counter()-start,executable_sha256=runner.sha(executable),
        scope='Explicit native method acceptance only; actual pre-prologue registers/stack, exact return/nonvolatiles and own x87 control preservation. Not a fixture dependency.')
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    assert result.returncode==0 and state.exists(),result.stdout.decode('latin1')
    print(json.dumps(receipt,indent=2))


if __name__=='__main__':
    main()
