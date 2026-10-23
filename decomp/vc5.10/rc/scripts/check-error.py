#!/usr/bin/env python3
"""Native RC1110/percent-escaping fixture; optional one-time native GDB evidence."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time
from common import HERE,read,sha,symbol,verify_compiled_sources
from error_reference import CASE,REFERENCE,ARGUMENTS,ABSENT,verify

GDB=r'''
set pagination off
set confirm off
set disable-randomization off
start /r missing%source.rc >OUTPUT 2>ERROR
python
import gdb,json
proof={'allocations':[],'escaped':[]}
class Returned(gdb.FinishBreakpoint):
 def __init__(self,frame,entry):
  super().__init__(frame,internal=True);self.entry=entry
 def stop(self):
  self.entry['returned_nonnull']=bool(int(gdb.parse_and_eval('$eax')))
  return False
class Allocation(gdb.Breakpoint):
 def stop(self):
  frame=gdb.newest_frame();caller=frame.older()
  if caller and caller.name()=='rc_diagnostic':
   entry={'bytes':int(gdb.parse_and_eval('*(unsigned int *)($esp+4)')),'caller':caller.name()}
   proof['allocations'].append(entry);Returned(frame,entry)
  return False
class Escaped(gdb.Breakpoint):
 def stop(self):
  value=gdb.parse_and_eval('escaped')
  data=value.string().encode('ascii')
  proof['escaped'].append({'bytes':data.hex(),'percent_count':data.count(b'%')})
  return False
Allocation('malloc',internal=True)
Escaped('src/rc.c:42',internal=True)
end
continue
python
assert int(gdb.parse_and_eval('$_exitcode'))==1,'Wrong native error-case exit'
gdb.write('RC_PERCENT_PROOF='+json.dumps(proof,sort_keys=True)+'\n')
end
quit
'''

def expected_log(observed):
    result='RC 0 0 3\n'
    for path in ABSENT:result+='absent '+path+'\n'
    for index,row in enumerate(observed['callbacks']):result+='callback %u 0 0 0\n'%index
    for index in range(3):result+='argv-after %u %u 0\n'%(index,index)
    return result+'return 1\n'

def verify_absence_tree(directory):
    assert list(directory.iterdir())==[],'Unknown input in absence-case directory'


def check(record=None,debug=False):
    started=time.perf_counter();manifest,observed=verify();settings=read(HERE/'functions.json')
    assert {row['name'] for row in settings['driver']}<=set(settings['accepted'])
    compiled=read(HERE/'build/native/rc.inputs.json');verify_compiled_sources(compiled)
    native_pin=sha(HERE/'build/native/rc');build_pin=sha(HERE/'build/native/rc.inputs.json')
    with tempfile.TemporaryDirectory(prefix='vc510-rc-error-',dir=str(Path.home()/'.ai-tmp')) as temporary:
        temporary=Path(temporary);directory=temporary/'case';controls=temporary/'controls';directory.mkdir();controls.mkdir()
        verify_absence_tree(directory)
        (controls/'filter').write_text(''.join(symbol(row['name'])+'\n' for row in settings['driver']))
        (controls/'seen').write_text('')
        environment={key:value for key,value in os.environ.items() if not key.startswith(('RECON_','WINE','RC_'))}
        environment.update(RC_DEPENDENCY_RESPONSE=str(REFERENCE/'rcdll-response.bin'),RC_DEPENDENCY_LOG=str(controls/'boundary.log'),RC_DEPENDENCY_CWD=observed['cwd'],RECON_PROGRAM_PATH=observed['before'][0]['text'])
        setup=time.perf_counter()-started
        with (controls/'trace').open('wb') as trace:
            environment.update(RECON_TRACE_FD=str(trace.fileno()),RECON_TRACE_FILTER=str(controls/'filter'),RECON_TRACE_SEEN=str(controls/'seen'))
            execution=time.perf_counter()
            if debug:
                script=GDB.replace('OUTPUT',str(controls/'stdout.bin')).replace('ERROR',str(controls/'stderr.bin'))
                (controls/'capture.gdb').write_text(script)
                run=subprocess.run(['gdb','-q','-batch','-x',str(controls/'capture.gdb'),str(HERE/'build/native/rc')],cwd=directory,env=environment,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,pass_fds=(trace.fileno(),),timeout=20)
                assert run.returncode==0,(run.stdout,run.stderr)
                (controls/'gdb.stdout').write_bytes(run.stdout);(controls/'gdb.stderr').write_bytes(run.stderr)
                import re
                lines=re.findall(rb'^RC_PERCENT_PROOF=(\{[^\n]*\})$',run.stdout,re.M);assert len(lines)==1
                proof=json.loads(lines[0]);assert proof['allocations']==[{'bytes':60,'caller':'rc_diagnostic','returned_nonnull':True}],proof
                expected=bytes.fromhex(observed['callbacks'][1]['after']).split(b'\0')[0].replace(b'%',b'%%')
                assert proof['escaped']==[{'bytes':expected.hex(),'percent_count':2}],proof
                status=1;stdout=(controls/'stdout.bin').read_bytes();stderr=(controls/'stderr.bin').read_bytes()
            else:
                run=subprocess.run([str(HERE/'build/native/rc')]+ARGUMENTS,cwd=directory,env=environment,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,pass_fds=(trace.fileno(),),timeout=10)
                status,stdout,stderr=run.returncode,run.stdout,run.stderr
            execution=time.perf_counter()-execution
        verification=time.perf_counter()
        assert status==1 and stdout==(REFERENCE/'stdout.bin').read_bytes() and stderr==(REFERENCE/'stderr.bin').read_bytes(),(status,stdout,stderr)
        actual_trace=(controls/'trace').read_text().splitlines();assert actual_trace==[symbol(name) for name in observed['trace']]
        assert (controls/'boundary.log').read_text()==expected_log(observed),'Callback mutable-buffer/argv/status effects differ'
        assert list(directory.iterdir())==[],'Error created an unobserved artifact'
        assert sha(HERE/'build/native/rc')==native_pin and sha(HERE/'build/native/rc.inputs.json')==build_pin,'Native changed during fixture'
        verification=time.perf_counter()-verification
        if record is not None:
            record=Path(record);assert record.is_relative_to(Path.home()/'.ai-tmp');record.mkdir(parents=True,exist_ok=False)
            for name in ['trace','boundary.log','capture.gdb','gdb.stdout','gdb.stderr']:
                if (controls/name).exists():shutil.copy2(controls/name,record/name)
            (record/'stdout.bin').write_bytes(stdout);(record/'stderr.bin').write_bytes(stderr)
            shutil.copy2(HERE/'build/native/rc',record/'native.elf');shutil.copy2(HERE/'build/native/rc.inputs.json',record/'compiled-sources.json')
    result={'status':'pass','case':'missing-percent-source','status_code':status,'stdout_bytes':len(stdout),'stderr_bytes':len(stderr),'callbacks':3,'artifacts':0,'native_trace':actual_trace,'setup_seconds':setup,'execution_seconds':execution,'verification_seconds':verification,'native_sha256':native_pin,'native_compiled_receipt_sha256':build_pin,'case_manifest_sha256':sha(REFERENCE/'manifest.json'),'target_functions_added':0,'DLL_coverage_claimed':False,'wine_invocations':0}
    if debug:result['actual_native_allocation_and_printf_evidence']=proof
    if record is not None:
        result['files']={path.name:{'bytes':path.stat().st_size,'sha256':sha(path)} for path in record.iterdir() if path.is_file()};(record/'receipt.json').write_text(json.dumps(result,indent=2)+'\n')
    return result

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--record',type=Path);parser.add_argument('--gdb',action='store_true');args=parser.parse_args();print(json.dumps(check(args.record,args.gdb),indent=2))
