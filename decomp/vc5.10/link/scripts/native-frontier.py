#!/usr/bin/env python3
"""Native i386 GDB capture. This module has no Wine/PE dependency runner."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time

BINARY = Path(__file__).resolve().parents[1]


def sha(data): return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--config', type=Path, default=BINARY / 'functions.json')
    args = parser.parse_args()
    subprocess.run(['make','-q','native'],cwd=BINARY,check=True)
    scratch = Path.home() / '.ai-tmp/msvc2/vc5.10/link'
    capture = Path(tempfile.mkdtemp(prefix='native-', dir=scratch))
    config = args.config.read_bytes()
    (capture / 'functions.json').write_bytes(config)
    (capture / 'runtime.json').write_bytes((BINARY / 'runtime.json').read_bytes())
    source_content={}
    content_paths=[]
    for category,directory in [('project',BINARY/'src'),('recon',BINARY.parents[2]/'lib/recon')]:
        for source in sorted(directory.rglob('*.[ch]')):
            if category=='recon' and source.relative_to(directory).parts[0] not in ('src','include','native'):continue
            name=category+'/'+str(source.relative_to(directory))
            data=source.read_bytes();destination=capture/'source'/name
            destination.parent.mkdir(parents=True,exist_ok=True);destination.write_bytes(data)
            source_content[name]=sha(data);content_paths.append((source,name))
    configuration = json.loads(config)
    targets = []
    for record in configuration['driver']:
        if record['name'] in configuration['accepted']:
            targets.append((record['name'], record.get('native_symbol','link_main' if record['name']=='_main' else record['name'].lstrip('_'))))
    script = '''
import gdb,json
state={'entries':[],'initerm_calls':[],'operator_new_calls':[],'atexit_calls':[],'console_calls':[]}
class NewReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,size):
        super().__init__(frame,internal=True);self.size=size
    def stop(self):
        state['operator_new_calls'].append({'bytes':self.size,'result':int(self.return_value)})
        return False
class NewEntry(gdb.Breakpoint):
    def stop(self):
        size=int(gdb.parse_and_eval('((unsigned*)$esp)[1]'))
        NewReturned(gdb.newest_frame(),size)
        return False
NewEntry('*recon_cpp_new',internal=True)
class AtExitReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,callback):
        super().__init__(frame,internal=True);self.callback=callback
    def stop(self):
        state['atexit_calls'].append({'callback':self.callback,'result':int(gdb.parse_and_eval('(int)$eax'))})
        return False
class AtExitEntry(gdb.Breakpoint):
    def stop(self):
        callback=int(gdb.parse_and_eval('((unsigned*)$esp)[1]'))
        name=gdb.execute('info symbol '+str(callback),to_string=True).strip().split(' ')[0]
        if name in ('free_buffer12','free_buffer8','free_buffer4','close_timing'):
            AtExitReturned(gdb.newest_frame(),name)
        return False
AtExitEntry('*atexit',internal=True)
class ConsoleReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,callback,add):
        super().__init__(frame,internal=True);self.callback=callback;self.add=add
    def stop(self):
        state['console_calls'].append(dict(callback=self.callback,add=self.add,result=int(self.return_value)))
        return False
class ConsoleEntry(gdb.Breakpoint):
    def stop(self):
        callback=int(gdb.parse_and_eval('((unsigned*)$esp)[1]'))
        add=int(gdb.parse_and_eval('((unsigned*)$esp)[2]'))
        name=gdb.execute('info symbol '+str(callback),to_string=True).strip().split(' ')[0]
        ConsoleReturned(gdb.newest_frame(),name,add)
        return False
ConsoleEntry('*recon_SetConsoleCtrlHandler',internal=True)
def callback_names(begin,count):
    result=[]
    for index in range(count):
        pointer=int((begin+index).dereference())
        result.append(gdb.execute('info symbol '+str(pointer),to_string=True).strip().split(' ')[0] if pointer else None)
    return result
class InitermEntry(gdb.Breakpoint):
    def stop(self):
        address=int(gdb.parse_and_eval('((unsigned*)$esp)[1]'))
        end=int(gdb.parse_and_eval('((unsigned*)$esp)[2]'))
        begin=gdb.Value(address).cast(gdb.lookup_type('ReconInitializer').pointer())
        state['initerm_calls'].append(callback_names(begin,(end-address)//4))
        return False
InitermEntry('*recon_initterm',internal=True)
class FirstEntry(gdb.Breakpoint):
    def __init__(self,logical,symbol):
        super().__init__(symbol,internal=True)
        self.logical=logical
    def stop(self):
        state['entries'].append(self.logical)
        self.enabled=False
        return False
for logical,symbol in %r:
    FirstEntry(logical,symbol)
def stopped(event):
    state['signal']=getattr(event,'stop_signal',None)
    state['signal_number']=int(gdb.parse_and_eval('$_siginfo.si_signo'))
    frame=gdb.newest_frame()
    state['function']=frame.name()
    state['pc']=int(gdb.parse_and_eval('$pc'))
    state['previous_byte']=int.from_bytes(gdb.selected_inferior().read_memory(state['pc']-1,1).tobytes(),'little')
    state['x87_control_word']=int(gdb.parse_and_eval('$fctrl'))
    state['crt_fmode']=int(gdb.parse_and_eval('recon_fmode'))
    state['crt_commode']=int(gdb.parse_and_eval('recon_commode'))
    state['crt_adjust_fdiv']=int(gdb.parse_and_eval('recon_adjust_fdiv'))
    stdout_address=int(gdb.parse_and_eval('&recon_iob[1]'))
    state['crt_stdout_address']=stdout_address
    state['crt_stdout_bytes']=gdb.selected_inferior().read_memory(stdout_address,32).tobytes().hex()
    state['initializers']=[gdb.execute('info symbol '+str(int(gdb.parse_and_eval('application_initializers[%%d]'%%i))),to_string=True).strip().split(' ')[0] if i else None for i in range(5)]
    state['diagnostic_output']=int(gdb.parse_and_eval('diagnostic_output'))
    if state['function']=='initialize_debug':
        state['frontier_arguments']=dict(options=int(frame.read_var('options')),environment=frame.read_var('environment').string())
    if state['function']=='initialize_messages':
        state['frontier_arguments']={name:int(frame.read_var(name)) for name in ('a','b','c','d')}
        state['program_name_address']=int(gdb.parse_and_eval('program_name'))
        state['program_name']=gdb.parse_and_eval('program_name').string()
        owner=frame
        while owner and owner.name()!='link_main':owner=owner.older()
        assert owner
        state['basename_address']=int(owner.read_var('basename').address)
    state['debug_flags']=gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('&debug_flags')),256).tobytes().hex()
    state['app_globals']={}
    for symbol,size in [('buffer12',12),('buffer8',12),('buffer4',12),('timing',20)]:
        address=int(gdb.parse_and_eval('&'+symbol))
        state['app_globals'][symbol]=gdb.selected_inferior().read_memory(address,size).tobytes().hex()
    frame=gdb.newest_frame()
    while frame and frame.name()!='recon_initterm':frame=frame.older()
    if frame:
        frame.select()
        current=int(gdb.parse_and_eval('begin'))
        end=int(gdb.parse_and_eval('end'))
        start=int(gdb.parse_and_eval('&application_initializers[0]'))
        state['initerm_current_index']=(current-start)//4
        state['initerm_end_index']=(end-start)//4
        gdb.newest_frame().select()
    frame=gdb.newest_frame()
    while frame and frame.name()!='target_startup':frame=frame.older()
    if frame:
        frame.select()
        count=int(gdb.parse_and_eval('argc'))
        state['crt_argv']=[gdb.parse_and_eval('argv[%%d]'%%i).string() for i in range(count)]
        gdb.newest_frame().select()
    state['stack']=[int(gdb.parse_and_eval('((unsigned*)$esp)[%%d]'%%i)) for i in range(32)]
def exited(event):
    state['exit_status']=getattr(event,'exit_code',None)
gdb.events.stop.connect(stopped)
gdb.events.exited.connect(exited)
gdb.execute('set pagination off')
gdb.execute('set confirm off')
gdb.execute('run /? > %s/stdout.bin 2> %s/stderr.bin')
open(%r,'w').write(json.dumps(state,indent=2)+'\\n')
''' % (targets, capture, capture, str(capture / 'state.json'))
    (capture / 'capture.gdb.py').write_text(script)
    executable = BINARY / 'build/native/LINK.EXE'
    command = ['gdb', '-q', '-nx', '-batch', '-ex', 'source ' + str(capture / 'capture.gdb.py'),
               '--args', str(executable), '/?']
    start = time.perf_counter()
    environment = os.environ.copy()
    for name in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB','LINK_DB']:environment.pop(name,None)
    environment['RECON_PROGRAM_PATH'] = r'R:\toolchains\5.10\bin\LINK.EXE'
    result = subprocess.run(command, cwd=capture, env=environment, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=20)
    elapsed = time.perf_counter() - start
    assert all(sha(path.read_bytes())==source_content[name] for path,name in content_paths), 'compiled sources changed during native capture'
    subprocess.run(['make','-q','native'],cwd=BINARY,check=True)
    (capture / 'gdb.stdout').write_bytes(result.stdout); (capture / 'gdb.stderr').write_bytes(result.stderr)
    receipt = dict(kind='native-i386-gdb-frontier', configuration_sha256=sha(config),
                   native_sha256=sha(executable.read_bytes()),
                   rebuilt_sha256=sha((BINARY / 'build/link.exe').read_bytes()),
                   source_files={str(p.relative_to(BINARY)):sha(p.read_bytes())
                                 for p in sorted((BINARY / 'src').glob('*.[ch]'))},
                   source_content=source_content,
                   command=command, status=result.returncode, elapsed_seconds=elapsed,
                   files={p.name:dict(bytes=p.stat().st_size,sha256=sha(p.read_bytes()))
                          for p in capture.iterdir() if p.is_file()})
    (capture / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(capture)
    if result.returncode or not (capture / 'state.json').exists():
        raise SystemExit('native debugger capture failed')


if __name__ == '__main__': main()
