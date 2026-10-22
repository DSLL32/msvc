"""Reject corrupted entry expectations, application mappings and native oracles."""
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import time

BINARY = Path(__file__).resolve().parents[1]


def main():
    directory = Path(tempfile.mkdtemp(prefix='native-rejections-', dir=Path.home()/'.ai-tmp/vc1.0-c1xx'))
    checks = []
    cache = json.loads((BINARY/'cases/native-traces.json').read_text())

    def reject(name, arguments, expected):
        started = time.perf_counter()
        result = subprocess.run([sys.executable, str(BINARY/'scripts/check-native.py'),
            '--directory', str(directory/name), *arguments], cwd=BINARY, capture_output=True, text=True)
        receipt = directory/name/'receipt.json'
        message = result.stdout + result.stderr
        if receipt.exists():
            message += receipt.read_text()
        assert result.returncode and expected in message, (name, result.returncode, message)
        checks.append(dict(name=name, status='pass', rejected_because=expected,
                           seconds=time.perf_counter()-started))

    wrong = json.loads(json.dumps(cache))
    wrong['cases']['cpp_basic']['expected'][0] = 'deliberately_wrong_entry'
    wrong_path = directory/'wrong-entry.json'
    wrong_path.write_text(json.dumps(wrong))
    reject('wrong-entry', ['--trace','--trace-cache',str(wrong_path)], 'application entry 0')
    missing = json.loads(json.dumps(cache))
    del missing['application_definitions']['main']
    missing_path = directory/'missing-definition.json'
    missing_path.write_text(json.dumps(missing))
    reject('missing-definition', ['--trace','--trace-cache',str(missing_path)], 'unresolved native application definition')
    build_started = time.perf_counter()
    for name, body in [
        ('wrong-terminal', 'raise(SIGSEGV);'),
        ('wrong-stdout', 'write(1,"wrong",5);raise(SIGTRAP);'),
        ('wrong-artifact', 'int f=open("irex",O_WRONLY|O_CREAT,0600);write(f,"wrong",5);close(f);raise(SIGTRAP);')]:
        source = directory/(name+'.c')
        source.write_text('#include <signal.h>\n#include <unistd.h>\n#include <fcntl.h>\nint main(void){'+body+'return 0;}\n')
        subprocess.run(['clang','-m32','-O0','-g',str(source),'-o',str(directory/name)],check=True)
    build_seconds = time.perf_counter()-build_started
    reject('reject-terminal', ['--exe',str(directory/'wrong-terminal')], 'expected named frontier SIGTRAP')
    reject('reject-stdout', ['--exe',str(directory/'wrong-stdout')], 'stdout differs')
    reject('reject-artifact', ['--exe',str(directory/'wrong-artifact')], 'irex: visible bytes differ')
    spec = importlib.util.spec_from_file_location('c1xx_native',BINARY/'scripts/check-native.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    try:
        module.reject_dependency(dict(file='unknown.exe'))
    except AssertionError as error:
        assert 'unexpected C1XX external dependency' in str(error)
        checks.append(dict(name='unknown-dependency',status='pass',rejected_because=str(error),seconds='not separately measured'))
    else:
        raise AssertionError('unknown dependency accepted')
    receipt = dict(status='pass', scope='native regression-boundary fault injection; no original execution',
        negative_helper_build_seconds=build_seconds,checks=checks)
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(directory/'receipt.json')


if __name__ == '__main__':
    main()
