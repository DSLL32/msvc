#!/usr/bin/env python3
"""Native-only comparator controls; synthetic receipts are not original evidence."""
import copy
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
TOOL = Path(os.environ.get('RECON_TOOL_TEST_EXE', ROOT / 'target/i686-unknown-linux-gnu/debug/recon-tool'))
FIXTURE = Path(os.environ.get('RECON_TOOL_TEST_REPO', ROOT.parents[1])) / 'tooling/recon-tool/tests/fixture'
subprocess.run(['make', '-C', str(FIXTURE), '-j4', 'cases'], check=True, stdout=subprocess.DEVNULL)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


with tempfile.TemporaryDirectory(prefix='reference-control-', dir=Path.home() / '.ai-tmp') as temporary:
    work = Path(temporary)
    case = FIXTURE / 'cases/0004-event'
    info = json.loads(subprocess.check_output([TOOL, 'inspect', case / '.build/case.so']))
    source = work / 'input.json'
    capture = work / 'capture.bin'
    source.write_text(json.dumps({'args': ['fixture', 'trap'], 'stdout': 'text:WRONG FINAL',
                                 'partial': {'stdout': 'text:', 'stderr': 'text:',
                                             'reject_unexpected_files': True}}))
    subprocess.run([TOOL, 'capture', 'pack', source, capture], check=True)
    identity = {key: info[key] for key in ('case', 'binary', 'original_path', 'original_sha256',
                                          'execution', 'first_entry_seen')}
    receipt = {'case': info['case'], 'oracle_written': True, 'terminal': 'frontier:1200',
               'first_entry_order': [0x1000, 0x1200], 'identity': identity,
               'capture_sha256': sha(capture.read_bytes()),
               'purpose': 'synthetic native comparator control; no original execution'}

    def run(name, value, passed, expected_error=None):
        reference = work / (name + '.json')
        reference.write_text(json.dumps(value))
        report = work / (name + '-native.json')
        command = [TOOL, 'run', case, '--capture', capture, '--original-report', reference, '--report', report]
        result = subprocess.run(command, capture_output=True, text=True)
        assert (result.returncode == 0) == passed, (name, result.stdout, result.stderr)
        if expected_error:
            assert expected_error in result.stdout + result.stderr, (name, result.stdout, result.stderr)
        if report.exists():
            actual = json.loads(report.read_text())[0]
            assert not Path(actual['workspace']).exists()
            assert actual['original_reference']['report_sha256'] == sha(reference.read_bytes())
            assert actual['original_reference']['capture_sha256'] == sha(capture.read_bytes())
            if passed:
                assert actual['trap_rva'] == 0x1200 and actual['first_entry_order'] == [0x1000, 0x1200]
                assert actual['original_reference']['source_binding_available'] == ('identity' in value)
        return result

    # Descriptor expects final exit and a different trace. The selected receipt
    # controls both terminal and partial-I/O selection; its exact native match passes.
    run('new-bound-match', receipt, True)
    legacy = {key: receipt[key] for key in ('case', 'oracle_written', 'terminal', 'first_entry_order')}
    run('explicit-legacy-match', legacy, True)
    wrong = copy.deepcopy(receipt)
    wrong.update(terminal='frontier:1000', first_entry_order=[0x1000])
    run('wrong-frontier-order', wrong, False, 'expected frontier 1000')
    for name, key, value, error in (
        ('wrong-capture', 'capture_sha256', '00' * 32, 'capture SHA256 differs'),
        ('wrong-case', 'case', 'other-case', 'case differs'),
    ):
        wrong = copy.deepcopy(receipt)
        wrong[key] = value
        run(name, wrong, False, error)
    for key, value in (('binary', 'other'), ('original_sha256', '00' * 32), ('execution', 1)):
        wrong = copy.deepcopy(receipt)
        wrong['identity'][key] = value
        run('wrong-' + key, wrong, False, 'identity differs')
    for command in ([TOOL, 'run', case, '--original-report', work / 'new-bound-match.json'],
                    [TOOL, 'run', case, case, '--capture', capture,
                     '--original-report', work / 'new-bound-match.json']):
        result = subprocess.run(command, capture_output=True, text=True)
        assert result.returncode != 0
print('native original-reference: actual frontier/order/partial selection, binding rejection, legacy and CLI controls PASS')
