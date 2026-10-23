"""Linux-only real DLL-pass frontier capture, called by capture-native.py."""
import json
import os
from pathlib import Path
import subprocess
import time


def capture(project,output,binary,trial):
    for item,digest in trial['case_inputs'].items():
        import hashlib
        source=project/item
        if hashlib.sha256(source.read_bytes()).hexdigest()!=digest:raise ValueError('case input changed')
        (output/source.name).write_bytes(source.read_bytes())
    cfg=json.loads((output/'functions.json').read_text())
    names=[]
    for row in cfg['driver']:
        name=row.get('native_symbol') or row['name'].lstrip('_').split('@')[0]
        names.append(name)
    (output/'filter.txt').write_text(''.join(name+'\n' for name in names))
    (output/'seen.txt').write_text('')
    environment=os.environ.copy()
    contract=json.loads((output/'contract.json').read_text())
    environment.update(contract['consumed_environment_seed'])
    environment['C1XX_INVOKE_ARGV']=str(output/'invoke-argv.bin')
    environment['RECON_DRIVE_R']=str(project.parents[2])
    wrapper=output/'exec-wrapper.py'
    wrapper.write_text('#!/usr/bin/python3\nimport os,sys\nfd=os.open('+repr(str(output/'trace.txt'))+',os.O_WRONLY|os.O_CREAT|os.O_TRUNC,0o600)\nos.set_inheritable(fd,True)\nos.environ.update(RECON_TRACE_FD=str(fd),RECON_TRACE_FILTER='+repr(str(output/'filter.txt'))+',RECON_TRACE_SEEN='+repr(str(output/'seen.txt'))+')\nos.execv(sys.argv[1],sys.argv[1:])\n')
    wrapper.chmod(0o755)
    collector=output/'collect.gdb'
    collector.write_text('''python
import gdb,json,struct
frame=gdb.newest_frame()
value={'symbol':frame.name(),'pc':int(gdb.parse_and_eval('$pc')),
       'control_word':int(gdb.parse_and_eval('$fctrl')),'args':{}}
block=frame.block()
while block is not None and not block.is_global and not block.is_static:
    for symbol in block:
        if symbol.is_argument: value['args'][symbol.name]=int(frame.read_var(symbol))
    block=block.superblock
value['target_state']={name:int(gdb.parse_and_eval('(unsigned int)'+name)) for name in
    ('process_attached','raw_dll_entry','dll_adjust_fdiv','dll_module','onexit_begin','onexit_end')}
value['target_state']['last_error']=int(gdb.parse_and_eval('last_error'))
try:value['target_state']['compiler_mode']=int(gdb.parse_and_eval('compiler_mode'))
except gdb.error:pass
value['target_state']['initializer_words']=[int(gdb.parse_and_eval('(unsigned int)crt_initializers[%d]'%n)) for n in range(2)]
value['target_state']['onexit_first']=int(gdb.parse_and_eval('(unsigned int)*onexit_begin')) if value['target_state']['onexit_begin'] else None
invocation=frame
while invocation is not None and invocation.name()!='InvokeCompilerPass':invocation=invocation.older()
if invocation is None:raise RuntimeError('no actual Invoke caller frame')
argc=int(invocation.read_var('argc'));argv=invocation.read_var('argv');mode=int(invocation.read_var('mode'))
raw=struct.pack('<II',int(argv),argc);strings=[]
for n in range(argc):
    pointer=int(argv[n]);text=argv[n].string(encoding='latin1').encode('latin1')+b'\\0'
    raw+=struct.pack('<II',pointer,len(text))+text;strings.append(text[:-1].decode('latin1'))
open(@@ARGV@@,'wb').write(raw)
value['invocation']={'argc':argc,'mode':mode,'argv_address':int(argv),'argv':strings}
host=frame
while host is not None and host.name()!='main':host=host.older()
if host is None:raise RuntimeError('no actual native DLL host frame')
spec=host.read_var('spec')
value['registration']={key:spec[key].string() for key in ('name','path','sha256')}
value['registration']['flags']=int(spec['flags']);value['registered_token']=int(host.read_var('module'))
open(@@STATE@@,'w').write(json.dumps(value,indent=2)+'\\n')
end
'''.replace('@@ARGV@@',repr(str(output/'native-argv.bin'))).replace('@@STATE@@',repr(str(output/'state.json'))))
    commands=output/'commands.gdb'
    commands.write_text('set pagination off\nset confirm off\nset exec-wrapper '+str(wrapper)+'\nrun > '+str(output/'stdout.bin')+' 2> '+str(output/'stderr.bin')+'\nbt\nsource '+str(collector)+'\nquit\n')
    started=time.perf_counter()
    result=subprocess.run(['gdb','--batch','-x',str(commands),'--args',str(binary)],cwd=output,env=environment,
                          stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=15)
    elapsed=time.perf_counter()-started
    (output/'gdb.txt').write_bytes(result.stdout)
    if result.returncode or b'Program received signal SIGTRAP' not in result.stdout:
        raise ValueError('native pass has no actual INT3 frontier; '+result.stdout.decode(errors='replace'))
    return result,elapsed
