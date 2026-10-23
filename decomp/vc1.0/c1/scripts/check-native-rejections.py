#!/usr/bin/env python3
"""Check native C1 oracle failures using private copies of existing case data."""
import copy
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

    receipt = dict(status='pass', elapsed_seconds=time.perf_counter()-start,
                   tests=results, evidence=str(directory), new_fixtures=0,
                   policy='Private mutations only; fixture captures unchanged; native execution only.')
    (directory/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(receipt, indent=2))


if __name__ == '__main__':
    main()
