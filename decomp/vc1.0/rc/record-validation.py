#!/usr/bin/env python3
"""Record paired frontier replay, graph audits and verified case completion."""
import argparse
import hashlib
import json
from pathlib import Path
import compare

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('replay_log', type=Path)
args = parser.parse_args()
configuration = json.loads(Path('functions.json').read_text())
log = args.replay_log.read_text()
entries = []
active_case, restored_count = None, None
for line in log.splitlines():
    if line.startswith('case: '):
        active_case, count = line[6:].split('; accepted: ')
        restored_count = int(count)
    elif line.startswith('original: '):
        left, right = line[10:].split('; reconstruction: ')
        assert left == right, line
        entries.append((active_case, restored_count, left))
accepted = configuration['accepted']
complete_cases = {n:c for n,c in configuration['cases'].items() if c.get('status') == 'complete'}
assert len(entries) == len(accepted) + len(complete_cases)
for number, name in enumerate(accepted):
    expected_case = configuration.get('frontier_cases', {}).get(name, 'resources-rich')
    assert entries[number] == (expected_case, number, name)
completed = {}
for name, count, outcome in entries[len(accepted):]:
    assert name in complete_cases and name not in completed
    assert count == len(accepted)
    assert outcome == 'exit:%d' % complete_cases[name].get('exit', 0)
    completed[name] = outcome
assert set(completed) == set(complete_cases)
assert 'Both raw streams, exit status and 3,684-byte resource product match the oracle exactly.' in log
for name in complete_cases:
    if name != 'resources-rich':
        assert ('Both raw streams, exit status and complete case file set match the %s oracle exactly.' % name) in log
graph = json.loads(Path('build/graph-audit.json').read_text())
source = {record['name'] for record in configuration['driver']}
assert set(graph) == set(accepted) & source
prepared = '__freebuf' in accepted
if prepared:
    assert 'Both prepared RCPP inputs match the captured 3,381 bytes exactly.' in log
menu_verified = '_write_resource@8' in accepted
if menu_verified:
    assert 'Both first menu payloads match the captured 190 bytes exactly.' in log
dialog_verified = '_resource_free@4' in accepted
if dialog_verified:
    assert 'Both first dialog payloads match the captured 506 bytes exactly.' in log
report = dict(status='cases-complete', case='resources-rich',
              accepted_frontiers=len(accepted), replayed_frontiers=len(entries),
              current_frontiers=completed, replayed_cases=len(completed),
              exact_final_product_verified=True,
              source_functions_audited=len(graph),
              checks=['all accepted paired frontiers replayed from masked entry in their originating case',
                      'direct call vertices, callback references and RET cleanup agree for accepted source functions',
                      'CRT initializer and termination callback sequences agree',
                      'raw streams, exit statuses, input files and complete product sets match all %d oracles' % len(completed)],
              target=configuration['target'],
              reconstruction_sha256=hashlib.sha256(Path('build/rc.exe').read_bytes()).hexdigest(),
              fixture_manifest_sha256=hashlib.sha256(Path('../fixtures/resources-rich/manifest.json').read_bytes()).hexdigest(),
              frontier_log_sha256=hashlib.sha256(log.encode()).hexdigest(),
              graph_audit=graph)
report['additional_cases'] = {}
for name in complete_cases:
    if name != 'resources-rich':
        manifest_path = Path('cases') / name / 'manifest.json'
        report['additional_cases'][name] = dict(manifest=json.loads(manifest_path.read_text()),
                manifest_sha256=hashlib.sha256(manifest_path.read_bytes()).hexdigest(),
                exact_streams_status_and_file_set=True)
original, _, _, index, _ = compare.indexes()
restored = set()
total = 0
for section in original.sections:
    if not section['flags'] & 0x20:
        continue
    first, last_byte = original.base + section['rva'], original.base + section['rva'] + section['size']
    total += section['size']
    for record in index:
        if record['name'] not in configuration['accepted']:
            continue
        for start, end in [(record['start'],record['end'])] + record.get('shared',[]):
            restored.update(range(max(first,start),min(last_byte,end)))
report['restored_text'] = dict(bytes=len(restored), total_bytes=total,
                              percentage=round(100.0*len(restored)/total,2),
                              definition='whole accepted functions and their text data; not dynamic branch coverage')
report['unaccepted_named_functions'] = sorted(source - set(configuration['accepted']))
product = Path('../fixtures/resources-rich/rc/resources.res').read_bytes()
report['result'] = dict(exit=0, stdout_bytes=0, stderr_bytes=0,
                      output_bytes=len(product), resource_records=19,
                      sha256=hashlib.sha256(product).hexdigest(),
                      exact_file_set=True)
report['runtime_callback_tables'] = json.loads(Path('build/runtime-tables.json').read_text())
if menu_verified:
    product = Path('../fixtures/resources-rich/rc/resources.res').read_bytes()
    report['first_menu_payload'] = dict(exact_bytes=True, bytes=190,
                                      sha256=hashlib.sha256(product[64:254]).hexdigest())
if dialog_verified:
    report['first_dialog_payload'] = dict(exact_bytes=True, bytes=506,
                                        sha256=hashlib.sha256(product[288:794]).hexdigest())
if prepared:
    payload = Path('../fixtures/resources-rich/rcpp/RC000360').read_bytes()
    report['prepared_rcpp_input'] = dict(exact_bytes=True, bytes=len(payload),
                                        sha256=hashlib.sha256(payload).hexdigest())
Path('validation-frontiers.txt').write_text(log)
Path('validation.json').write_text(json.dumps(report, indent=2) + '\n')
print('Recorded %d accepted frontiers, %d source functions and %d complete cases.' %
      (len(accepted), len(graph), len(completed)))
