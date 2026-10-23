"""Native-only synthetic model injection for both unwritten-result paths.

Use the actual qualifier and (for the data branch) its actual class value member.
Finish the real model callee, including its compiled RET4, then replace EAX.
No original execution, reference modification or invalid-input equivalence claim.
"""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time
from case_paths import BINARY, ROOT, CAPTURE_ROOT, native_work, windows


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    started = time.perf_counter()
    directory = Path(tempfile.mkdtemp(prefix='member-pointer-model-policy-', dir=CAPTURE_ROOT))
    fixture = BINARY.parent/'fixtures/c1xx-state-machine-debug'
    phase = json.loads((fixture/'manifest.json').read_text())['phases']['c1']
    executable = BINARY/'build/native-trace/C1XX3232.EXE'
    configuration = json.loads((BINARY/'functions.json').read_text())
    helper_stub = '_member_pointer_inheritance_model@4' not in configuration['accepted']
    frozen = {str(p): sha(p) for p in [executable, fixture/'manifest.json', BINARY/'functions.json', BINARY/'cases/native-traces.json']}
    observations = []
    for branch in ('method', 'data'):
        for model in (0, 1):
            run = directory/('%s-model%d' % (branch, model))
            run.mkdir()
            work, drives = native_work(run, phase['canonical_working_directory'])
            for item in phase['inputs']:
                path = fixture/item['path']
                raw = path.read_bytes()
                assert len(raw) == item['bytes'] and hashlib.sha256(raw).hexdigest() == item['sha256']
                frozen[str(path)] = sha(path)
                (work/Path(item['path']).name).write_bytes(raw)
            commands = ['set pagination off', 'break member_pointer_expression',
                'run > '+str(run/'stdout.bin')+' 2> '+str(run/'stderr.bin'),
                'python', 'import gdb,json',
                'assert gdb.newest_frame().name()=="member_pointer_expression"',
                'qualifier=gdb.parse_and_eval("qualifier")',
                'binding=qualifier["resolution"]["binding"]',
                'member=binding["target"].cast(gdb.lookup_type("CppSymbol").pointer())',
                'assert (int(member["kind"])&31)==7 and member["name"].string()=="add"',
                'assert not (int(member["storage"][1])&2)']
            if branch == 'data':
                commands += [
                    'owner=binding["next"]["target"].cast(gdb.lookup_type("CppSymbol").pointer())',
                    'assert (int(owner["kind"])&31)==4 and owner["name"].string()=="Counter"',
                    'data=owner["auxiliary"]["scope_members"]',
                    'for index in range(512):',
                    '    assert int(data)!=0, "actual value member absent"',
                    '    if (int(data["kind"])&31)==3 and data["name"].string()=="value": break',
                    '    data=data["next"]',
                    'else: raise AssertionError("bounded member owner search exhausted")',
                    'assert not (int(data["storage"][1])&2)',
                    'binding["target"].assign(data.cast(gdb.lookup_type("ClassReference").pointer()))']
            commands += ['end', 'disable 1', 'break member_pointer_inheritance_model', 'continue',
                'python', 'assert gdb.newest_frame().name()=="member_pointer_inheritance_model"', 'end',
                'disable 2', 'finish']
            if helper_stub:
                commands += ['python',
                    'assert gdb.newest_frame().name()=="member_pointer_inheritance_model"',
                    'assert int(gdb.parse_and_eval("$_siginfo.si_signo"))==5',
                    'pc=gdb.newest_frame().pc()',
                    'assert bytes(gdb.selected_inferior().read_memory(pc-1,1))==b"\\xcc"',
                    'end', 'finish']
            commands += ['python', 'assert gdb.newest_frame().name()=="member_pointer_expression"', 'end',
                'set $eax = '+str(model)]
            if model == 1:
                downstream = 'make_compact_method_pointer' if branch == 'method' else 'new_integer_constant'
                commands += ['break '+downstream, 'continue', 'python',
                    'assert gdb.newest_frame().name()=='+repr(downstream)]
            else:
                commands += ['continue', 'python',
                    'frame=gdb.newest_frame()',
                    'assert frame.name()=="member_pointer_expression", "invalid model escaped exact guard"',
                    'assert int(gdb.parse_and_eval("model"))==0',
                    'assert int(gdb.parse_and_eval("$_siginfo.si_signo"))==5',
                    'assert bytes(gdb.selected_inferior().read_memory(frame.pc()-1,1))==b"\\xcc"']
            commands += [
                'result={"branch":'+repr(branch)+',"injected_model":'+str(model)+',"helper_was_unaccepted_stub":'+repr(helper_stub)+',"stopped_function":gdb.newest_frame().name(),"status":"pass"}',
                'open('+repr(str(run/'observation.json'))+',"w").write(json.dumps(result,indent=2)+"\\n")', 'end']
            (run/'commands.gdb').write_text('\n'.join(commands)+'\n')
            environment = {k: v for k, v in os.environ.items() if not k.startswith('RECON_')}
            for key in ('CL', '_CL_', 'LINK', '_LINK_'):
                environment.pop(key, None)
            environment.update(drives, TZ='PST8PDT',
                MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(ROOT/'toolchains/1.0-nt/bin'), include=windows(ROOT/'toolchains/1.0-nt/include')),
                RECON_PROGRAM_PATH=phase['command_line_template'].format(bin=windows(ROOT/'toolchains/1.0-nt/bin')))
            prepared = time.perf_counter()
            result = subprocess.run(['gdb', '-q', '-batch', '-x', str(run/'commands.gdb'), '--args', str(executable)], cwd=work, env=environment, capture_output=True, text=True, timeout=60)
            ran = time.perf_counter()
            (run/'gdb.log').write_text(result.stdout+result.stderr)
            assert result.returncode == 0 and (run/'observation.json').exists(), result.stdout+result.stderr
            observation = json.loads((run/'observation.json').read_text())
            observation['debugger_seconds'] = ran-prepared
            observations.append(observation)
    assert all(sha(p) == value for p, value in frozen.items()), 'frozen native policy inputs changed'
    report = dict(status='pass', scope='native-only synthetic callee-return injection, no original invalid-model equivalence or app coverage credit', frozen=frozen, observations=observations, debugger_seconds=sum(r['debugger_seconds'] for r in observations), total_seconds=time.perf_counter()-started)
    report['setup_and_checks_seconds'] = report['total_seconds']-report['debugger_seconds']
    (directory/'receipt.json').write_text(json.dumps(report, indent=2)+'\n')
    print(directory/'receipt.json')


if __name__ == '__main__':
    main()
