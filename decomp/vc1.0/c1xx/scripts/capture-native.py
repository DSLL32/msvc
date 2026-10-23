"""Inspect a native frontier in GDB; no original process or reference refresh.

This reconstruction diagnostic is separate from ordinary cached fixtures.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time

BINARY = Path(__file__).resolve().parents[1]


def capture(directory, executable=None, breakpoint=None, expressions=None):
    started = time.perf_counter()
    fixture = BINARY.parent/'fixtures/cpp-basic'
    phase = json.loads((fixture/'manifest.json').read_text())['phases']['c1']
    config = json.loads((BINARY/'functions.json').read_text())
    directory.mkdir(parents=True, exist_ok=True)
    work = directory/'case'
    work.mkdir()
    for item in phase['inputs']:
        (work/Path(item['path']).name).write_bytes((fixture/item['path']).read_bytes())
    commands = (['break '+breakpoint] if breakpoint else []) + ['run > '+str(directory/'stdout.bin')+' 2> '+str(directory/'stderr.bin'), 'bt', 'python',
                'import gdb, json', 'result = {"buffers": [], "states": [], "function": gdb.newest_frame().name(), "signal": int(gdb.parse_and_eval("$_siginfo.si_signo"))}']
    for key, expression in (expressions or {}).items():
        commands += ['result['+repr(key)+'] = int(gdb.parse_and_eval('+repr(expression)+'))']
    for field in config['buffered_streams']:
        name = field['name'][1:]
        commands += ['stream = gdb.parse_and_eval("'+name+'")',
                     'length = int(stream["_ptr"]) - int(stream["_base"]) if int(stream) else 0',
                     'result["buffers"].append(bytes(gdb.selected_inferior().read_memory(int(stream["_base"]), length)).hex() if length else "")']
    for field in config['checkpoint_state']:
        name = field['name'][1:]
        commands += ['value = int(gdb.parse_and_eval("*(unsigned int *)&'+name+'"))',
                     'result["states"].append(int(value != 0) if '+str(bool(field.get('pointer')))+' else value)']
    commands += ['open('+repr(str(directory/'checkpoint.json'))+',"w").write(json.dumps(result, indent=2)+"\\n")', 'end']
    command_path = directory/'commands.gdb'
    command_path.write_text('\n'.join(commands)+'\n')
    env = {k:v for k,v in os.environ.items() if not k.startswith('RECON_')}
    for name in ('CL','_CL_','LINK','_LINK_'):
        env.pop(name, None)
    windows_bin = r'Z:\root\msvc2\toolchains\1.0-nt\bin'
    env['MSC_CMD_FLAGS'] = phase['environment']['MSC_CMD_FLAGS'].format(bin=windows_bin,include=r'Z:\root\msvc2\toolchains\1.0-nt\include')
    env['RECON_PROGRAM_PATH'] = phase['command_line_template'].format(bin=windows_bin)
    env['TZ'] = 'PST8PDT'
    prepared = time.perf_counter()
    result = subprocess.run(['gdb','-q','-batch','-x',str(command_path),'--args',
        str(executable or BINARY/'build/native/C1XX3232.EXE')],cwd=work,env=env,
        stdin=subprocess.DEVNULL,capture_output=True,text=True,timeout=60)
    (directory/'gdb.log').write_text(result.stdout+result.stderr)
    timing = dict(setup_seconds=prepared-started,debugger_seconds=time.perf_counter()-prepared,
                  total_seconds=time.perf_counter()-started,returncode=result.returncode)
    (directory/'timing.json').write_text(json.dumps(timing,indent=2)+'\n')
    assert result.returncode == 0 and (directory/'checkpoint.json').exists(), result.stdout+result.stderr
    return json.loads((directory/'checkpoint.json').read_text())


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory',type=Path)
    parser.add_argument('--breakpoint',help='explicit runtime entry breakpoint instead of first INT3')
    parser.add_argument('--observe',action='append',default=[],metavar='NAME=EXPRESSION')
    args=parser.parse_args()
    scratch=Path.home()/'.ai-tmp/vc1.0-c1xx'
    directory=args.directory or Path(tempfile.mkdtemp(prefix='native-frontier-',dir=scratch))
    checkpoint=capture(directory,breakpoint=args.breakpoint,expressions=dict(item.split('=',1) for item in args.observe))
    print(checkpoint['function'])
    print(directory)


if __name__ == '__main__':
    main()
