"""Reject corrupted entry expectations, application mappings and native oracles."""
import hashlib
from case_paths import CAPTURE_ROOT
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import time

BINARY = Path(__file__).resolve().parents[1]


def main():
    directory = Path(tempfile.mkdtemp(prefix='native-rejections-', dir=CAPTURE_ROOT))
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
    complete=cache['cases']['cpp_basic']['terminal']['kind']=='exit'
    stop='return 0;' if complete else 'raise(SIGTRAP);'
    build_started = time.perf_counter()
    for name, body in [
        ('wrong-terminal', 'raise(SIGSEGV);'),
        ('wrong-stdout', 'write(1,"wrong",5);'+stop),
        ('wrong-artifact', 'int f=open("irex",O_WRONLY|O_CREAT,0600);write(f,"wrong",5);close(f);'+stop)]:
        source = directory/(name+'.c')
        source.write_text('#include <signal.h>\n#include <unistd.h>\n#include <fcntl.h>\nint main(void){'+body+'return 0;}\n')
        subprocess.run(['clang','-m32','-O0','-g',str(source),'-o',str(directory/name)],check=True)
    build_seconds = time.perf_counter()-build_started
    reject('reject-terminal', ['--exe',str(directory/'wrong-terminal')], 'expected completed frontend exit status' if complete else 'expected named frontier SIGTRAP')
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
    # Tiny cdecl varargs wrappers share instructions across unrelated APIs.
    # The custom formatter's call relocation disproves both CRT byte matches.
    from runtime_boundary_loader import load_boundary
    boundary = load_boundary()
    for name in ('_fscanf', '_fwscanf'):
        started = time.perf_counter()
        try:
            boundary.identities(names=(name,))
        except AssertionError as error:
            assert str(error) == 'original CRT call relocation differs', str(error)
            checks.append(dict(name='reject-formatter-as-'+name.lstrip('_'),
                status='pass', rejected_because=str(error),
                seconds=time.perf_counter()-started))
        else:
            raise AssertionError('custom formatter accepted as CRT '+name)
    proof_path = BINARY/'evidence/runtime-analysis/close/validation.json'
    assert proof_path.is_file(), "missing required independent CRT boundary proof"
    if True:
        spec = importlib.util.spec_from_file_location('c1xx_runtime_boundary',BINARY/'scripts/runtime-boundary.py')
        boundary = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(boundary)
        proof = boundary.check(proof_path)
        changes = [
            ('close-return', lambda p:p['native'].__setitem__('result', -1), 'close boundary differs'),
            ('close-failed-both', lambda p:(p['original'].__setitem__('result', -1), p['native'].__setitem__('result', -1)), 'close did not close'),
            ('close-descriptor', lambda p:(p['original'].__setitem__('descriptor', 8), p['native'].__setitem__('descriptor', 8)), ''),
            ('close-order', lambda p:p['runtime_order'].reverse(), 'unobserved runtime boundary'),
            ('close-library', lambda p:p['identity']['records'][0].__setitem__('object_sha256', '0'*64), 'CRT library/import identity changed'),
            ('close-observation', lambda p:p['evidence_files'][0].__setitem__('sha256','0'*64), 'runtime observation evidence changed')]
        for name, change, expected in changes:
            altered=json.loads(json.dumps(proof))
            change(altered)
            altered_path=directory/(name+'.json')
            altered_path.write_text(json.dumps(altered))
            started=time.perf_counter()
            try:
                boundary.check(altered_path)
            except AssertionError as error:
                assert expected in str(error),(name,str(error))
                checks.append(dict(name=name,status='pass',rejected_because=str(error) or 'observation bytes contradict claimed descriptor',seconds=time.perf_counter()-started))
            else:raise AssertionError(name+' accepted corrupted runtime evidence')
    stream_path=BINARY/'evidence/runtime-analysis/flush/validation.json'
    assert stream_path.is_file(), "missing required independent CRT boundary proof"
    if True:
        from runtime_boundary_loader import load_boundary
        boundary=load_boundary()
        proof=boundary.check(stream_path)
        changes=[
            ('fflush-write-bit',lambda p:p['native']['after'].__setitem__('flags',p['native']['before']['flags']),'fflush boundary differs'),
            ('fflush-stale-both',lambda p:(p['original']['after'].__setitem__('flags',p['original']['before']['flags']),p['native']['after'].__setitem__('flags',p['native']['before']['flags'])),'fflush stream transition differs'),
            ('fflush-failed-both',lambda p:(p['original'].__setitem__('result',-1),p['native'].__setitem__('result',-1)),''),
            ('fflush-library',lambda p:p['identity']['records'][0].__setitem__('object_sha256','0'*64),'fflush library identity changed')]
        for name,change,expected in changes:
            altered=json.loads(json.dumps(proof));change(altered)
            altered_path=directory/(name+'.json');altered_path.write_text(json.dumps(altered))
            started=time.perf_counter()
            try:boundary.check(altered_path)
            except AssertionError as error:
                assert expected in str(error),(name,str(error))
                checks.append(dict(name=name,status='pass',rejected_because=str(error) or 'fflush did not successfully drain buffer',seconds=time.perf_counter()-started))
            else:raise AssertionError(name+' accepted corrupted runtime evidence')
    exit_path=BINARY/'evidence/runtime-analysis/exit/validation.json'
    assert exit_path.is_file(), "missing required independent CRT boundary proof"
    if True:
        from runtime_boundary_loader import load_boundary
        boundary=load_boundary();proof=boundary.check(exit_path)
        changes=[
            ('exit-runtime-order',lambda p:p['runtime_order'].reverse(),'unobserved termination sequence'),
            ('exit-library',lambda p:p['identity']['records'][0].__setitem__('object_sha256','0'*64),'termination library identity changed'),
            ('exit-observation',lambda p:p['evidence_files'][0].__setitem__('sha256','0'*64),'termination evidence changed')]
        for name,change,expected in changes:
            altered=json.loads(json.dumps(proof));change(altered)
            altered_path=directory/(name+'.json');altered_path.write_text(json.dumps(altered))
            started=time.perf_counter()
            try:boundary.check(altered_path)
            except AssertionError as error:
                assert expected in str(error),(name,str(error))
                checks.append(dict(name=name,status='pass',rejected_because=str(error),seconds=time.perf_counter()-started))
            else:raise AssertionError(name+' accepted corrupted termination evidence')
    realloc_path=BINARY/'evidence/runtime-analysis/realloc/validation.json'
    assert realloc_path.is_file(), "missing required independent CRT boundary proof"
    if True:
        from runtime_boundary_loader import load_boundary
        boundary=load_boundary();proof=boundary.check(realloc_path)
        changes=[
            ('realloc-library',lambda p:p['identity']['records'][0].__setitem__('object_sha256','0'*64),'realloc library identity changed'),
            ('realloc-observation',lambda p:p['evidence_files'][0].__setitem__('sha256','0'*64),'realloc evidence changed'),
            ('realloc-size',lambda p:p['native'].__setitem__('size',32),''),
            ('realloc-lost-prefix',lambda p:p['original'].__setitem__('after','00'*16),'')]
        for name,change,expected in changes:
            altered=json.loads(json.dumps(proof));change(altered)
            altered_path=directory/(name+'.json');altered_path.write_text(json.dumps(altered))
            started=time.perf_counter()
            try:boundary.check(altered_path)
            except AssertionError as error:
                assert expected in str(error),(name,str(error))
                checks.append(dict(name=name,status='pass',rejected_because=str(error) or 'raw realloc observation contradicts claimed size or preserved prefix',seconds=time.perf_counter()-started))
            else:raise AssertionError(name+' accepted corrupted reallocation evidence')
    copy_path=BINARY/'evidence/runtime-analysis/memmove/validation.json'
    assert copy_path.is_file(), "missing required independent CRT boundary proof"
    if True:
        from runtime_boundary_loader import load_boundary
        boundary=load_boundary();proof=boundary.check(copy_path)
        changes=[
            ('copy-library',lambda p:p['identity']['records'][0].__setitem__('member_sha256','0'*64),'copy library identity changed'),
            ('copy-alias',lambda p:p['alias_identity']['members'][1].__setitem__('text_sha256','0'*64),'copy alias identity changed'),
            ('copy-overlap',lambda p:p['original'].__setitem__('delta',0),'copy arguments/return differ'),
            ('copy-return',lambda p:p['original'].__setitem__('returned_destination',0),'copy arguments/return differ'),
            ('copy-lost-prefix',lambda p:p['original'].__setitem__('after','00'*8),'copy raw original bytes differ')]
        for name,change,expected in changes:
            altered=json.loads(json.dumps(proof));change(altered)
            altered_path=directory/(name+'.json');altered_path.write_text(json.dumps(altered))
            started=time.perf_counter()
            try:boundary.check(altered_path)
            except AssertionError as error:
                assert expected in str(error),(name,str(error))
                checks.append(dict(name=name,status='pass',rejected_because=str(error),seconds=time.perf_counter()-started))
            else:raise AssertionError(name+' accepted corrupted overlapping-copy evidence')
    if 'virtual_diamond' in cache['cases']:
        wrong=json.loads(json.dumps(cache))
        wrong['cases']['virtual_diamond']['seen'].remove('main')
        wrong_path=directory/'missing-case-prefix.json';wrong_path.write_text(json.dumps(wrong))
        reject('missing-case-prefix',['--trace','--trace-cache',str(wrong_path)],'application entry 0')
        reject('partial-case-wrong-exit',['--case','virtual_diamond','--exe',str(directory/'wrong-stdout')],'expected named frontier SIGTRAP')
    receipt = dict(status='pass', scope='native regression-boundary fault injection; no original execution',
        negative_helper_build_seconds=build_seconds,checks=checks)
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(directory/'receipt.json')


if __name__ == '__main__':
    main()
