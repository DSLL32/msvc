#!/usr/bin/env python3
"""Check native C1 oracle failures using private copies of existing case data."""
import copy
import struct
import importlib.util
import json
from pathlib import Path
import tempfile
import time

HERE = Path(__file__).resolve().parent


def module(name):
    spec = importlib.util.spec_from_file_location(name.replace('-', '_'), HERE/(name+'.py'))
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


def main():
    checker = module('check-native')
    projection = module('native-traces')
    start = time.perf_counter()
    directory = Path(tempfile.mkdtemp(prefix='native-rejections-', dir=Path.home()/'.ai-tmp/msvc2/vc1.0/c1/validation'))
    executable = checker.BINARY/'build/native-trace/C13232.EXE'
    cache = projection.load(checker.BINARY/'build/native-trace')
    filter_path = directory/'filter'
    filter_path.write_text(''.join(n+'\n' for n in cache['application_definitions']))
    results = []

    def rejects(name, expected, action):
        began = time.perf_counter()
        try:
            action()
        except AssertionError as error:
            assert expected in str(error), (name, str(error))
            results.append(dict(name=name, status='rejected', diagnostic=str(error),
                                seconds=time.perf_counter()-began))
        else:
            raise AssertionError('bad input accepted: '+name)

    _, work = checker.run_case('minimal', executable, directory, (cache, filter_path))
    fixture = checker.FIXTURES/'minimal-c'
    phase = json.loads((fixture/'manifest.json').read_text())['phases']['c1']
    original = (work/'irex').read_bytes()
    altered = bytearray(original)
    altered[0] ^= 1
    (work/'irex').write_bytes(altered)
    rejects('intermediate-byte', 'output bytes differ: irex',
            lambda: checker.validate_artifacts(work, fixture, phase))
    (work/'irex').write_bytes(original)
    (work/'unexpected').write_bytes(b'')
    rejects('complete-file-set', 'complete file set differs',
            lambda: checker.validate_artifacts(work, fixture, phase))
    (work/'unexpected').unlink()

    wrong = copy.deepcopy(cache)
    wrong['cases']['minimal']['expected'][:2] = list(reversed(wrong['cases']['minimal']['expected'][:2]))
    wrong_root = directory/'wrong-order'
    wrong_root.mkdir()
    rejects('wrong-entry-order', 'native trace minimal index 0: expected',
            lambda: checker.run_case('minimal', executable, wrong_root, (wrong, filter_path)))

    wrong = copy.deepcopy(cache)
    wrong['application_definitions'].pop(next(iter(wrong['application_definitions'])))
    wrong_path = directory/'missing-definition.json'
    wrong_path.write_text(json.dumps(wrong))
    rejects('unmapped-application-definition', 'new/missing/unmapped native application definition',
            lambda: projection.load(checker.BINARY/'build/native-trace', wrong_path))

    _, numeric = checker.run_case('numeric-varargs', executable, directory)
    numeric_fixture = checker.BINARY/'cases/numeric-varargs'
    numeric_phase = json.loads((numeric_fixture/'manifest.json').read_text())['phases']['c1']
    pdb = bytearray((numeric/'msvc.pdb').read_bytes())
    pdb[48] ^= 1
    (numeric/'msvc.pdb').write_bytes(pdb)
    rejects('raw-timestamp-correlation', 'raw cross-output timestamps differ',
            lambda: checker.validate_artifacts(numeric, numeric_fixture, numeric_phase))

    _, producer = checker.run_case('pch-create', executable, directory)
    pch = bytearray((producer/'state.pch').read_bytes())
    pch[-1] ^= 1
    corrupt = directory/'corrupt.pch'
    corrupt.write_bytes(pch)
    rejects('generated-pch-dependency', 'generated PCH differs from captured dependency',
            lambda: checker.run_case('pch-use', executable, directory, dependency=corrupt))
    assert not (directory/'pch-use/execution.json').exists(), 'consumer ran with bad dependency'
    _, debug_producer = checker.run_case('debug-pch-create', executable, directory)
    debug_pch = bytearray((debug_producer/'debug.pch').read_bytes())
    debug_pch[-1] ^= 1
    corrupt_debug = directory/'corrupt-debug.pch'
    corrupt_debug.write_bytes(debug_pch)
    rejects('debug-pch-generated-dependency', 'generated PCH differs from captured dependency',
            lambda: checker.run_case('debug-pch-use', executable, directory, dependency=corrupt_debug))
    assert not (directory/'debug-pch-use/execution.json').exists(), 'debug consumer ran with bad PCH'
    debug_valid = directory/'valid-debug-use'
    debug_valid.mkdir()
    _, debug_used = checker.run_case('debug-pch-use', executable, debug_valid,
                                    dependency=debug_producer/'debug.pch')
    debug_fixture = checker.BINARY/'cases/debug-pch'
    debug_phase = json.loads((debug_fixture/'manifest.json').read_text())['phases']['c1-use']
    (debug_used/'debug.pch').write_bytes(debug_pch)
    rejects('debug-pch-preserved-input-byte', 'output bytes differ: debug.pch',
            lambda: checker.validate_artifacts(debug_used, debug_fixture, debug_phase))

    _, browser = checker.run_case('browser', executable, directory)
    browser_fixture = checker.BINARY/'cases/browser'
    browser_phase = json.loads((browser_fixture/'manifest.json').read_text())['phases']['c1']
    sbr = bytearray((browser/'test.sbr').read_bytes())
    sbr[4] ^= 1
    (browser/'test.sbr').write_bytes(sbr)
    rejects('browser-record-byte', 'output bytes differ: test.sbr',
            lambda: checker.validate_artifacts(browser, browser_fixture, browser_phase))

    _, preprocessed = checker.run_case('preprocessor-P', executable, directory)
    pp_fixture = checker.BINARY/'cases/preprocessor'
    pp_phase = json.loads((pp_fixture/'manifest.json').read_text())['phases']['c1-P']
    text = bytearray((preprocessed/'test.i').read_bytes())
    text[text.index(b'\r\n')] = ord(' ')
    (preprocessed/'test.i').write_bytes(text)
    rejects('preprocessor-line-ending-byte', 'output bytes differ: test.i',
            lambda: checker.validate_artifacts(preprocessed, pp_fixture, pp_phase))

    _, assembly = checker.run_case('inline-asm-rich', executable, directory, (cache, filter_path))
    assembly_fixture = checker.BINARY/'cases/inline-asm-rich'
    assembly_phase = json.loads((assembly_fixture/'manifest.json').read_text())['phases']['c1']
    asm_ir = bytearray((assembly/'irex').read_bytes())
    asm_ir[-1] ^= 1
    (assembly/'irex').write_bytes(asm_ir)
    rejects('rich-assembler-expression-byte', 'output bytes differ: irex',
            lambda: checker.validate_artifacts(assembly, assembly_fixture, assembly_phase))

    _, recovery = checker.run_case('diagnostic-recovery', executable, directory, (cache,filter_path))
    recovery_fixture = checker.BINARY/'cases/diagnostic-recovery'
    recovery_phase = json.loads((recovery_fixture/'manifest.json').read_text())['phases']['c1']
    recovery_stdout = (recovery.parent/'stdout').read_bytes()
    assert recovery_phase['expected_exit'] == 6
    for code in [0,7]:
        rejects('diagnostic-wrong-status-'+str(code), 'native status',
                lambda code=code:checker.validate_result('diagnostic-recovery',code,recovery_stdout,b'',recovery_phase))
    lines = recovery_stdout.splitlines(keepends=True)
    lines[0],lines[1] = lines[1],lines[0]
    rejects('diagnostic-order', 'stdout differs',
            lambda:checker.validate_result('diagnostic-recovery',6,b''.join(lines),b'',recovery_phase))
    rejects('missing-recovery-tail-marker', 'stdout differs',
            lambda:checker.validate_result('diagnostic-recovery',6,recovery_stdout.replace(b'RECOVERY_TAIL_REACHED\r\n',b''),b'',recovery_phase))
    (recovery/'recovery.obj').write_bytes(b'')
    rejects('diagnostic-forbidden-object', 'forbidden output exists: recovery.obj',
            lambda:checker.validate_artifacts(recovery,recovery_fixture,recovery_phase))
    (recovery/'recovery.obj').unlink()
    partial = bytearray((recovery/'irex').read_bytes())
    partial[-1] ^= 1
    (recovery/'irex').write_bytes(partial)
    rejects('diagnostic-partial-file-byte', 'output bytes differ: irex',
            lambda:checker.validate_artifacts(recovery,recovery_fixture,recovery_phase))
    rejects('success-case-still-requires-zero', 'native status',
            lambda:checker.validate_result('minimal',6,b'',b'',phase))

    _, arena = checker.run_case('based-record-arena', executable, directory, (cache,filter_path))
    arena_fixture = checker.BINARY/'cases/based-record-arena'
    arena_phase = json.loads((arena_fixture/'manifest.json').read_text())['phases']['c1']
    database = bytearray((arena/'irdb').read_bytes())
    name_offset = database.index(b'arena_base')
    database[name_offset] = ord('A')
    (arena/'irdb').write_bytes(database)
    rejects('based-debug-base-symbol-name', 'output bytes differ: irdb',
            lambda:checker.validate_artifacts(arena,arena_fixture,arena_phase))
    (arena/'irdb').write_bytes((arena_fixture/'expected/c1/irdb').read_bytes())
    expression = bytearray((arena/'irex').read_bytes())
    expression[-1] ^= 1
    (arena/'irex').write_bytes(expression)
    rejects('based-expression-stream-byte', 'output bytes differ: irex',
            lambda:checker.validate_artifacts(arena,arena_fixture,arena_phase))

    _, binding_producer = checker.run_case('pdb-pch-binding-create', executable, directory)
    binding_originals = {name:(binding_producer/name).read_bytes() for name in ['binding.pch','msvc.pdb','irex']}
    for name in ['binding.pch','msvc.pdb','irex']:
        altered = bytearray(binding_originals[name])
        altered[-1] ^= 1
        (binding_producer/name).write_bytes(altered)
        private_root = directory/('bad-binding-'+name)
        private_root.mkdir()
        rejects('pdb-binding-producer-'+name, 'output bytes differ: '+name,
                lambda:checker.run_case('pdb-pch-binding-use', executable, private_root,
                                        dependency=binding_producer/'binding.pch'))
        assert not (private_root/'pdb-pch-binding-use/execution.json').exists(), 'consumer ran with bad producer contract'
        (binding_producer/name).write_bytes(binding_originals[name])
    missing_root = directory/'missing-binding-producer'
    missing_root.mkdir()
    rejects('pdb-binding-missing-producer', 'consumer requires actual producer state',
            lambda:checker.run_case('pdb-pch-binding-use', executable, missing_root))
    _, binding_consumer = checker.run_case('pdb-pch-binding-use', executable, directory,
                                          dependency=binding_producer/'binding.pch')
    signature = struct.unpack_from('<I',binding_originals['msvc.pdb'],48)[0]
    for name,offset in [('msvc.pdb',48),('irdb',14)]:
        altered = bytearray((binding_consumer/name).read_bytes())
        struct.pack_into('<I',altered,offset,signature ^ 1)
        (binding_consumer/name).write_bytes(altered)
    rejects('pdb-binding-clock-propagation', 'consumer database signature changed',
            lambda:checker.validate_generated_signature(binding_consumer, signature))

    _, legacy = checker.run_case('legacy-callback-adapter', executable, directory, (cache,filter_path))
    legacy_fixture = checker.BINARY/'cases/legacy-callback-adapter'
    legacy_phase = json.loads((legacy_fixture/'manifest.json').read_text())['phases']['c1']
    warnings = legacy_phase['expected_stdout'].encode('latin1').splitlines(keepends=True)
    swapped = warnings[:]
    swapped[0],swapped[1] = swapped[1],swapped[0]
    rejects('legacy-callback-warning-order', 'stdout differs',
            lambda:checker.validate_result('legacy-callback-adapter',0,b''.join(swapped),b'',legacy_phase))
    missing = b''.join(line for line in warnings if b'(57)' not in line)
    rejects('legacy-callback-pointer-warning', 'stdout differs',
            lambda:checker.validate_result('legacy-callback-adapter',0,missing,b'',legacy_phase))
    debug_types = bytearray((legacy/'irdb').read_bytes())
    debug_types[-1] ^= 1
    (legacy/'irdb').write_bytes(debug_types)
    rejects('legacy-callback-debug-type-byte', 'output bytes differ: irdb',
            lambda:checker.validate_artifacts(legacy,legacy_fixture,legacy_phase))

    receipt = dict(status='pass', elapsed_seconds=time.perf_counter()-start,
                   tests=results, evidence=str(directory), new_fixtures=0,
                   policy='Private mutations only; fixture captures unchanged; native execution only.')
    (directory/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(receipt, indent=2))


if __name__ == '__main__':
    main()
