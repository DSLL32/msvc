"""Inspect a native frontier in GDB; no original process or reference refresh.

This reconstruction diagnostic is separate from ordinary cached fixtures.
"""
import argparse
import json
import os
import shutil
from pathlib import Path
import subprocess
import tempfile
import time
from case_paths import paths, native_work, windows, ROOT, CAPTURE_ROOT

BINARY = Path(__file__).resolve().parents[1]


def capture(directory, executable=None, breakpoint=None, expressions=None, case_name='cpp_basic', caller_locals=None):
    started = time.perf_counter()
    config = json.loads((BINARY/'functions.json').read_text())
    _,fixture,_,_=paths(config,case_name)
    phase = json.loads((fixture/'manifest.json').read_text())['phases']['c1']
    directory.mkdir(parents=True, exist_ok=True)
    work, drive_environment = native_work(directory, phase['canonical_working_directory'])
    for item in phase['inputs']:
        (work/Path(item['path']).name).write_bytes((fixture/item['path']).read_bytes())
    commands = (['break '+breakpoint] if breakpoint else []) + ['python',
                'import gdb, json', 'exit_codes = []',
                'gdb.events.exited.connect(lambda event: exit_codes.append(getattr(event, "exit_code", None)))',
                'end', 'run > '+str(directory/'stdout.bin')+' 2> '+str(directory/'stderr.bin'), 'python',
                'if exit_codes:',
                '    assert len(exit_codes) == 1 and exit_codes[0] is not None, "missing normal exit status"',
                '    result = {"buffers": [], "states": [], "function": "exit:" + str(exit_codes[0]), "signal": 0, "exit_code": exit_codes[0]}',
                'else:', '    gdb.execute("bt")',
                '    result = {"buffers": [], "states": [], "function": gdb.newest_frame().name(), "signal": int(gdb.parse_and_eval("$_siginfo.si_signo"))}']
    if expressions:
        commands += ['    block = gdb.newest_frame().block()',
                     '    arguments = []',
                     '    while block is not None:',
                     '        arguments += [symbol.name for symbol in block if symbol.is_argument]',
                     '        if block.function is not None: break',
                     '        block = block.superblock',
                     '    result["declared_arguments"] = sorted(arguments)']
    for key, expression in (expressions or {}).items():
        commands += ['    result['+repr(key)+'] = int(gdb.parse_and_eval('+repr(expression)+'))']
    if caller_locals:
        commands += ['    result["caller_locals"] = {}']
        for key, owner in caller_locals.items():
            assert owner['frame'] == 1, 'only the immediate actual owner frame is supported'
            fields=owner.get('fields', [])
            assert len(fields)<=1, 'only one proved actual caller pointer field is supported'
            commands += ['    owner = gdb.newest_frame().older()',
                         '    assert owner is not None and owner.name() == '+repr(owner['function']),
                         '    owner_value = owner.read_var('+repr(owner['local'])+')']
            for field in fields:
                commands += ['    owner_value = owner_value['+repr(field)+']']
            commands += ['    result["caller_locals"]['+repr(key)+'] = {"function": owner.name(), "local": '+repr(owner['local'])+', "fields": '+repr(fields)+', "value": int(owner_value)}']
            if owner.get('address_of'):
                assert not fields, 'address observation is bounded to an actual immediate caller local'
                commands += ['    result["caller_locals"]['+repr(key)+']["address"] = int(owner_value.address)']
    for field in config['cases'][case_name].get('buffered_streams', config['buffered_streams']):
        name = field['name'][1:]
        commands += ['    stream = gdb.parse_and_eval("'+name+'")',
                     '    length = int(stream["_ptr"]) - int(stream["_base"]) if int(stream) else 0',
                     '    result["buffers"].append(bytes(gdb.selected_inferior().read_memory(int(stream["_base"]), length)).hex() if length else "")']
    for field in config['checkpoint_state']:
        name = field['name'][1:]
        commands += ['    value = int(gdb.parse_and_eval("*(unsigned int *)&'+name+'"))',
                     '    result["states"].append(int(value != 0) if '+str(bool(field.get('pointer')))+' else value)']
    commands += ['open('+repr(str(directory/'checkpoint.json'))+',"w").write(json.dumps(result, indent=2)+"\\n")', 'end']
    command_path = directory/'commands.gdb'
    command_path.write_text('\n'.join(commands)+'\n')
    env = {k:v for k,v in os.environ.items() if not k.startswith('RECON_')}
    for name in ('CL','_CL_','LINK','_LINK_'):
        env.pop(name, None)
    windows_bin = windows(ROOT/'toolchains/1.0-nt/bin')
    env['MSC_CMD_FLAGS'] = phase['environment']['MSC_CMD_FLAGS'].format(bin=windows_bin,include=windows(ROOT/'toolchains/1.0-nt/include'))
    env['RECON_PROGRAM_PATH'] = phase['command_line_template'].format(bin=windows_bin)
    env['TZ'] = 'PST8PDT'
    env.update(drive_environment)
    prepared = time.perf_counter()
    result = subprocess.run(['gdb','-q','-batch','-x',str(command_path),'--args',
        str(executable or BINARY/'build/native/C1XX3232.EXE')],cwd=work,env=env,
        stdin=subprocess.DEVNULL,capture_output=True,text=True,timeout=60)
    shutil.copytree(work,directory/'case')
    (directory/'gdb.log').write_text(result.stdout+result.stderr)
    timing = dict(setup_seconds=prepared-started,debugger_seconds=time.perf_counter()-prepared,
                  total_seconds=time.perf_counter()-started,returncode=result.returncode)
    (directory/'timing.json').write_text(json.dumps(timing,indent=2)+'\n')
    assert result.returncode == 0 and (directory/'checkpoint.json').exists(), result.stdout+result.stderr
    return json.loads((directory/'checkpoint.json').read_text())


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case',default='cpp_basic')
    parser.add_argument('--directory',type=Path)
    parser.add_argument('--breakpoint',help='explicit runtime entry breakpoint instead of first INT3')
    parser.add_argument('--observe',action='append',default=[],metavar='NAME=EXPRESSION')
    args=parser.parse_args()
    scratch=CAPTURE_ROOT/'native'
    scratch.mkdir(parents=True,exist_ok=True)
    directory=args.directory or Path(tempfile.mkdtemp(prefix='native-frontier-',dir=scratch))
    checkpoint=capture(directory,breakpoint=args.breakpoint,expressions=dict(item.split('=',1) for item in args.observe),case_name=args.case)
    print(checkpoint['function'])
    print(directory)


if __name__ == '__main__':
    main()
