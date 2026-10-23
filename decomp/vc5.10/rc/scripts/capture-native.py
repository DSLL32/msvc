#!/usr/bin/env python3
"""Native-only debugger capture; no original invocation or PE helper."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import time
from common import HERE,read,sha,source_pins,names,symbol
from reference_data import verify as verify_reference
from native_pc import functions

GDB=r'''
set pagination off
set confirm off
set disable-randomization off
BREAKPOINTS
run /help >OUTPUT 2>ERROR
python
import gdb,json
try:
 f=gdb.newest_frame()
 name=f.name()
 mapped=MAPPING
 if name not in mapped: raise RuntimeError('Unexpected native trap '+str(name))
 state={'frontier':mapped[name],'completion':False,'status':124}
 pc=int(gdb.parse_and_eval('$pc'));start=int(gdb.parse_and_eval('(void *)'+name));symbols=ELF_SYMBOLS
 if name in ['RC','recon_controlfp','recon_initterm']:
  assert pc==start,'Imported boundary did not stop at exact entry'
  state['pc_check']={'kind':'import-entry','symbol':name,'symbol_start':start,'pc':pc,'file_value':symbols[name]['value'],'file_size':symbols[name]['size']}
 else:
  signal=int(gdb.parse_and_eval('$_siginfo.si_signo'))
  opcode=gdb.selected_inferior().read_memory(pc-1,1).tobytes().hex()
  assert signal==5 and opcode=='cc' and start<=pc-1<start+symbols[name]['size'],'Not a named SIGTRAP INT3 stub'
  state['pc_check']={'kind':'int3','symbol':name,'symbol_start':start,'pc':pc,'offset':pc-start-1,'signal':signal,'opcode':opcode,'file_value':symbols[name]['value'],'file_size':symbols[name]['size']}

 def text(pointer):
  p=int(pointer);out=bytearray()
  for n in range(16384):
   byte=gdb.selected_inferior().read_memory(p+n,1).tobytes();out+=byte
   if byte==b'\0':return out.hex()
  raise RuntimeError('String too long')
 def vector(pointer):
  p=int(pointer);out=[]
  for n in range(4096):
   item=int.from_bytes(gdb.selected_inferior().read_memory(p+4*n,4).tobytes(),'little')
   if not item:return out
   out.append(text(item))
  raise RuntimeError('Vector too long')
 if name=='rc_main':
  state['arguments']={'argc':int(gdb.parse_and_eval('argc')),'argv':vector(gdb.parse_and_eval('argv')),'env':vector(gdb.parse_and_eval('environment'))}
 if STATE_CONTRACT:
  def integer(name):return int(gdb.parse_and_eval(name))&0xffffffff
  state['crt_state']={'initializers':[integer('crt_xc[0]'),integer('crt_xc[1]'),integer('crt_xi[0]'),integer('crt_xi[1]')], 'wildcard':integer('crt_wildcard'),'newmode':integer('crt_newmode'),'default_math_error':integer('crt_default_math_error'),'commit_mode':integer('crt_commit_mode'),'file_mode':integer('crt_file_mode'),'onexit_begin':integer('crt_onexit_begin'),'onexit_end':integer('crt_onexit_end'),'fdiv_adjust':integer('crt_fdiv_adjust')}
 if name in ['RC','recon_controlfp','recon_initterm']:
  esp=int(gdb.parse_and_eval('$esp'))
  words=[int.from_bytes(gdb.selected_inferior().read_memory(esp+n*4,4).tobytes(),'little') for n in range(7)]
  if name=='RC':
   assert words[3]==int(gdb.parse_and_eval('(void *)rc_diagnostic')),'Wrong callback pointer'
   state['arguments']={'window':words[1],'flags':words[2],'diagnostic':'_rc_diagnostic@12','resource':words[4],'argc':words[5],'argv':vector(words[6])}
  elif name=='recon_controlfp':state['arguments']={'value':words[1],'mask':words[2]}
  else:
   xc=int(gdb.parse_and_eval('(void *)crt_xc'));xi=int(gdb.parse_and_eval('(void *)crt_xi'))
   assert words[1] in [xc,xi] and words[2]==words[1]+4,'Wrong initializer range'
   value=int.from_bytes(gdb.selected_inferior().read_memory(words[1],4).tobytes(),'little')
   state['arguments']={'table':'xc' if words[1]==xc else 'xi','count':1,'entries':[value]}
 if name=='rc_diagnostic':
  state['arguments']={'window':int(gdb.parse_and_eval('window')),'code':int(gdb.parse_and_eval('code')),'message':text(gdb.parse_and_eval('message'))}
except gdb.error:
 status=int(gdb.parse_and_eval('$_exitcode'))
 if status!=0:raise RuntimeError('Native exited unsuccessfully '+str(status))
 state={'frontier':None,'completion':True,'status':status}
gdb.write('RC_RAW_STATE='+json.dumps(state,sort_keys=True)+'\n')
end
quit
'''

def capture(directory,candidate):
    directory=Path(directory);directory.mkdir(parents=True,exist_ok=False)
    settings=read(candidate);reference_pins=verify_reference(settings);
    compiled=read(HERE/'build/native/rc.inputs.json')
    assert compiled['binary_sha256']==sha(HERE/'build/native/rc'),'Missing/stale compiled-source provenance'
    for name,text in compiled['compiled_sources'].items():assert (HERE.parents[2]/name).read_text()==text,'Consumed source changed; rebuild native: '+name
    (directory/'compiled-sources.json').write_text(json.dumps(compiled,indent=2)+'\n')
    (directory/'candidate.json').write_bytes(Path(candidate).read_bytes())
    mapping={symbol(r['name']):r['name'] for r in settings['driver']+settings['support']}
    imports={'recon_controlfp':'import:_controlfp','recon_initterm':'import:_initterm','RC':'import:RC'}
    mapping.update(imports)
    stops='\n'.join('break *'+name for name,logical in imports.items() if logical not in settings['accepted'])
    script=GDB.replace('BREAKPOINTS',stops).replace('OUTPUT',str(directory/'stdout.bin')).replace('ERROR',str(directory/'stderr.bin')).replace('MAPPING',repr(mapping)).replace('STATE_CONTRACT',repr(bool(settings.get('state_contract')))).replace('ELF_SYMBOLS',repr({n:r for n,r in functions().items() if n in mapping}))
    (directory/'capture.gdb').write_text(script)
    (directory/'filter.txt').write_text(''.join(symbol(r['name'])+'\n' for r in settings['driver']))
    (directory/'seen.txt').write_text('')
    env={k:v for k,v in os.environ.items() if not k.startswith(('RECON_','WINE'))}
    start=time.monotonic()
    with (directory/'trace.txt').open('wb') as trace:
        env.update(RECON_PROGRAM_PATH=r'S:\vc5.10\rc\original\RC.EXE',RC_DEPENDENCY_RESPONSE=str(HERE/'cases/help/reference/rcdll-response.bin'),RC_DEPENDENCY_LOG=str(directory/'boundary.txt'),RECON_TRACE_FD=str(trace.fileno()),RECON_TRACE_FILTER=str(directory/'filter.txt'),RECON_TRACE_SEEN=str(directory/'seen.txt'))
        result=subprocess.run(['gdb','-q','-batch','-x',str(directory/'capture.gdb'),str(HERE/'build/native/rc')],cwd=directory,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,pass_fds=(trace.fileno(),),timeout=20)
    elapsed=time.monotonic()-start
    (directory/'gdb.stdout').write_bytes(result.stdout);(directory/'gdb.stderr').write_bytes(result.stderr)
    assert result.returncode==0,(result.stdout,result.stderr)
    receipt={'config_sha256':sha(candidate),'native_sha256':sha(HERE/'build/native/rc'),'rebuilt_sha256':sha(HERE/'build/rc.exe'),'map_sha256':sha(HERE/'build/rc.map'),'sources':source_pins(),'reference_pins':reference_pins,'elapsed_seconds':elapsed,'status':result.returncode,'files':{p.name:{'sha256':sha(p),'bytes':p.stat().st_size} for p in directory.iterdir() if p.is_file()}}
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    return receipt

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--candidate',type=Path,default=HERE/'functions.json');p.add_argument('directory',type=Path);a=p.parse_args();print(json.dumps(capture(a.directory,a.candidate),indent=2))
