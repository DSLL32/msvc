#!/usr/bin/env python3
"""Offline/native provenance controls; synthetic receipts are not original evidence."""
import copy
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
TOOL = Path(os.environ.get('RECON_TOOL_TEST_EXE', ROOT / 'target/i686-unknown-linux-gnu/debug/recon-tool'))
FIXTURE = ROOT / 'tests/fixture'
subprocess.run(['make', '-C', str(FIXTURE), '-j4', 'cases'], check=True, stdout=subprocess.DEVNULL)
sha = lambda raw: hashlib.sha256(raw).hexdigest()

with tempfile.TemporaryDirectory(prefix='comparison-derivation-', dir=Path.home() / '.ai-tmp') as directory:
    work = Path(directory)
    case = FIXTURE / 'cases/0004-event'
    info = json.loads(subprocess.check_output([TOOL, 'inspect', case / '.build/case.so']))

    def pack(name, value):
        source, capture = work / (name + '.json'), work / (name + '.bin')
        source.write_text(json.dumps(value))
        subprocess.run([TOOL, 'capture', 'pack', source, capture], check=True)
        return capture

    parent_value = {'args': ['fixture', 'trap'], 'stdin': 'text:', 'stdout': 'text:FINAL',
                    'assets': [{'path': 'fixed.bin', 'data': 'text:existing harness input'}],
                    'output_files': [{'path': 'final.obj', 'sha256': 'ab' * 32}],
                    'partial': {'stdout': 'text:', 'stderr': 'text:', 'reject_unexpected_files': True}}
    parent = pack('parent', parent_value)
    report = work / 'original.json'
    observed = {'case': info['case'], 'oracle_written': True, 'terminal': 'frontier:1200',
                'first_entry_order': [0x1000, 0x1200],
                'identity': {key: info[key] for key in ('case', 'binary', 'original_path', 'original_sha256',
                                                       'execution', 'first_entry_seen')},
                'capture_sha256': sha(parent.read_bytes()),
                'purpose': 'synthetic provenance control, no original execution',
                'actual_snapshot': {'eax': 7}}
    report.write_text(json.dumps(observed))
    parent_bytes, report_bytes = parent.read_bytes(), report.read_bytes()
    next_value = copy.deepcopy(parent_value)
    next_value['output_files'][0].update(sha256='cd' * 32, zero_ranges=[[4, 8]])
    next_value['assets'].append({'path': 'oracle.bin', 'data': 'text:actual parent output'})
    comparison = pack('comparison', next_value)

    def derive(name, capture, passed=True, original_report=report):
        target = work / (name + '-reference.json')
        command = [TOOL, 'capture', 'derive-reference', '--original-report', original_report,
                   '--original-capture', parent, '--capture', capture, '--output', target,
                   '--reason', 'explicit control: reviewed output normalization and harness oracle only']
        result = subprocess.run(command, capture_output=True, text=True)
        assert (result.returncode == 0) == passed, (name, result.stdout, result.stderr)
        if not passed:
            assert not target.exists()
        return target

    reference = derive('valid', comparison)
    derived = json.loads(reference.read_text())
    assert derived['actual_snapshot'] == observed['actual_snapshot']
    assert derived['comparison_derivation']['changed_output_rules'][0]['path'] == 'final.obj'
    assert derived['comparison_derivation']['added_harness_assets'][0]['path'] == 'oracle.bin'
    assert parent.read_bytes() == parent_bytes and report.read_bytes() == report_bytes

    def replay(name, selected, capture=comparison, passed=False):
        native = work / (name + '-native.json')
        result = subprocess.run([TOOL, 'run', case, '--capture', capture,
                                 '--original-report', selected, '--report', native], capture_output=True, text=True)
        assert (result.returncode == 0) == passed, (name, result.stdout, result.stderr)
        if passed:
            actual = json.loads(native.read_text())[0]
            assert actual['first_entry_order'] == observed['first_entry_order'] and actual['trap_rva'] == 0x1200
            proof = actual['original_reference']['comparison_derivation']
            assert proof['parent_report']['sha256'] == sha(report_bytes)
            assert proof['parent_capture']['sha256'] == sha(parent_bytes)
        return result

    replay('derived-match', reference, passed=True)
    replay('ordinary-report-still-strict', report)
    for key, value in [('args', ['fixture', 'other']), ('stdin', 'text:different'),
                       ('stdin_kind', 'null'), ('env', {'NEW': 'value'}),
                       ('input_files', [{'path': 'target.in', 'data': 'text:changed'}]),
                       ('events', [{'after_ms': 0, 'action': {'kind': 'signal', 'signal': 15}}]),
                       ('logical_cwd', 'T:\\different'), ('timeout_ms', 9),
                       ('stdout', 'text:different'), ('exit_code', 7), ('reject_unexpected_files', True),
                       ('readonly_dependencies', [{'path': '/tmp/x', 'sha256': 'ab' * 32}])]:
        bad = copy.deepcopy(next_value)
        bad[key] = value
        if key == 'stdin_kind':
            bad.pop('stdin')
        derive('reject-' + key, pack('changed-' + key, bad), passed=False)
    for field, value in [('path', 'another.obj'), ('absent', True)]:
        bad = copy.deepcopy(next_value)
        bad['output_files'][0][field] = value
        if field == 'absent':
            bad['output_files'][0].pop('sha256')
            bad['output_files'][0].pop('zero_ranges')
        derive('reject-output-' + field, pack('changed-output-' + field, bad), passed=False)
    for name in ['remove', 'modify']:
        bad = copy.deepcopy(next_value)
        if name == 'remove':
            bad['assets'].pop(0)
        else:
            bad['assets'][0]['data'] = 'text:modified existing harness input'
        derive('reject-existing-asset-' + name, pack('changed-asset-' + name, bad), passed=False)
    for field, value in [('terminal', 'frontier:1400'), ('first_entry_order', [0x1000]),
                         ('actual_snapshot', {'eax': 9})]:
        bad = copy.deepcopy(derived)
        bad[field] = value
        path = work / ('tamper-' + field + '.json')
        path.write_text(json.dumps(bad))
        replay('reject-observation-' + field, path)
    bad = copy.deepcopy(derived)
    bad['comparison_derivation']['added_harness_assets'][0]['sha256'] = '00' * 32
    path = work / 'tampered-proof.json'
    path.write_text(json.dumps(bad))
    replay('reject-proof', path)
    report.write_bytes(report_bytes + b'\n')
    replay('reject-parent-mutation', reference)
    report.write_bytes(report_bytes)
    legacy = work / 'legacy.json'
    legacy.write_text(json.dumps({k: observed[k] for k in ('case', 'terminal', 'first_entry_order', 'oracle_written')}))
    derive('reject-unbound-parent', comparison, passed=False, original_report=legacy)
    derive('reject-derived-parent', comparison, passed=False, original_report=reference)
print('comparison derivation: strict parent/input/observation bindings and native match PASS; no original execution')
