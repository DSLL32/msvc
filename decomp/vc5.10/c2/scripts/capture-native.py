#!/usr/bin/env python3
"""Linux/GDB-only frontier capture. No original transport imports."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import shutil
import time
from state import BINARY, TARGET, load, sha, source_hashes, trial_configuration, audit_input_hashes

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args()
    begin=time.monotonic();base,_,_=load()
    base_snapshot=(BINARY/'functions.json').read_bytes();config,snapshot=trial_configuration(base_snapshot)
    output=args.output.resolve();output.mkdir(parents=True,exist_ok=False)
    (output/'base_configuration.json').write_bytes(base_snapshot);(output/'configuration.json').write_bytes(snapshot)
    executable=BINARY/'build/native/c2'
    prefix_program="""import gdb,json
trace=[]
class Entered(gdb.Breakpoint):
    def __init__(self,symbol,logical):
        self.logical=logical
        super().__init__(symbol,internal=True)
    def stop(self):
        if self.logical not in trace:
            trace.append(self.logical)
            gdb.write('C2-SOURCE-ENTRY %%s 0x%%x\\n'%%(self.logical,int(gdb.parse_and_eval('$pc'))))
        return False
for record in %r:
    Entered(record.get('native',record['name'].lstrip('_')),record['name'])
""" % [r for r in config['driver']+config['support']if r['name']in config['accepted']and r.get('kind')!='import']
    (output/'gdb-prefix.py').write_text(prefix_program)
    program="""import gdb,json
from pathlib import Path
frame=gdb.newest_frame()
info={'function':frame.name(),'pc':int(gdb.parse_and_eval('$pc')),'signal':gdb.execute('info program',to_string=True),'backtrace':gdb.execute('bt',to_string=True),'trace':trace,'globals':{}}
pid=gdb.selected_inferior().pid
actual_args=Path('/proc/%%d/cmdline'%%pid).read_bytes().rstrip(b'\\0').split(b'\\0')
actual_env=Path('/proc/%%d/environ'%%pid).read_bytes().split(b'\\0')
Path(%r).write_text(Path('/proc/%%d/maps'%%pid).read_text())
gdb.execute('info registers eip')
gdb.execute('info symbol $pc')
gdb.execute('x/1ib $pc-1')
gdb.execute('info program')
info['arguments']=[v.decode()for v in actual_args[1:]]
flags=[v[14:].decode()for v in actual_env if v.startswith(b'MSC_CMD_FLAGS=')]
info['MSC_CMD_FLAGS']=flags[0]if flags else None
records=[r for r in %r if r['native']==frame.name()]
if len(records)==1:
    for view in records[0].get('state',[]):
        info['globals'][view['original']]=int(gdb.parse_and_eval(view['native'])) & 0xffffffff
    if records[0].get('state_policy')in ('command-line-state','environment-flags'):
        ptr=gdb.parse_and_eval('c2_argv');values=[];i=0
        while int(ptr[i]):
            values.append(ptr[i].string());i+=1
            assert i<4096
        info['command_state']={'argc':int(gdb.parse_and_eval('c2_argc')),'argv':values}
        if records[0]['state_policy']=='environment-flags':
            info['flags']=None if int(gdb.parse_and_eval('flags'))==0 else gdb.parse_and_eval('flags').string()
    if records[0].get('state_policy')=='exit-callback':
        info['callback']=int(gdb.parse_and_eval('callback'))
        gdb.write('C2-CALLBACK 0x%%x\\n'%%info['callback'])
    if records[0].get('state_policy')=='main-arguments-and-initial-state':
        def vector(expression):
            pointer=gdb.parse_and_eval(expression);result=[];i=0
            while int(pointer[i]):
                result.append(pointer[i].string());i+=1
                assert i<4096
            return result
        info['main']={'argc':int(gdb.parse_and_eval('argc')),'argv':vector('argv'),'environment':vector('environment'),
                      'initenv_alias':int(gdb.parse_and_eval('recon_crt_initenv == environment')),
                      'x87_control':int(gdb.parse_and_eval('$fctrl'))}
open(%r,'w').write(json.dumps(info,indent=2)+'\\n')
""" % (str(output/'native-maps.txt'),config['stubs'],str(output/'state.json'))
    (output/'gdb-state.py').write_text(program)
    command=['/usr/bin/gdb','-nx','-batch','-ex','set pagination off','-ex','set confirm off',
             '-ex','source '+str(output/'gdb-prefix.py'),
             '-ex','run /? > '+str(output/'stdout.bin')+' 2> '+str(output/'stderr.bin'),
             '-ex','source '+str(output/'gdb-state.py'),str(executable)]
    env=dict(os.environ);env.pop('MSC_CMD_FLAGS',None)
    environment_file=BINARY/'cases/unsupported-help-diagnostic/environment.bin'
    manifest=json.loads((environment_file.parent/'case.json').read_text())
    assert sha(environment_file.read_bytes())==manifest['environment']['sha256'],'frozen environment changed'
    env['C2_ENVIRONMENT_FILE']=str(environment_file)
    start=time.monotonic();result=subprocess.run(command,cwd=output,env=env,capture_output=True,timeout=30)
    execution=time.monotonic()-start
    (output/'gdb.stdout').write_bytes(result.stdout);(output/'gdb.stderr').write_bytes(result.stderr)
    provenance=BINARY/'build/native/input-provenance.json'
    build_inputs=json.loads(provenance.read_text())
    assert build_inputs['binary_sha256']==sha(executable.read_bytes()),'native build provenance does not match executable'
    shutil.copyfile(provenance,output/'compiled-inputs.json')
    shutil.copytree(build_inputs['snapshot'],output/'compiled-inputs')
    receipt=dict(kind='native-linux-gdb',command=command,candidates=config['trial'],base_configuration_sha256=sha(base_snapshot),configuration_sha256=sha(snapshot),
                 target_sha256=sha(TARGET.read_bytes()),native_sha256=sha(executable.read_bytes()),
                 rebuilt_sha256=sha((BINARY/'build/c2.exe').read_bytes()),
                 audit_inputs=audit_input_hashes(),source_hashes=source_hashes(),state_sha256=sha((output/'state.json').read_bytes())if(output/'state.json').exists()else None,
                 provenance_sha256=sha((output/'compiled-inputs.json').read_bytes()),
                 gdb_inputs={name:sha((output/name).read_bytes())for name in ('gdb-prefix.py','gdb-state.py')},
                 gdb_outputs={name:sha((output/name).read_bytes())for name in ('gdb.stdout','gdb.stderr','native-maps.txt')},
                 environment={'MSC_CMD_FLAGS':None},helper_status=result.returncode,
                 seconds={'execution':execution,'total':time.monotonic()-begin})
    for stream in ('stdout','stderr'):
        path=output/(stream+'.bin');receipt[stream]={'bytes':path.stat().st_size,'sha256':sha(path.read_bytes())}
    (output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    assert result.returncode==0 and (output/'state.json').is_file(),result.stderr
    assert base_snapshot==(BINARY/'functions.json').read_bytes()
    print((output/'state.json').read_text())
    print(output/'receipt.json')

if __name__=='__main__':main()
