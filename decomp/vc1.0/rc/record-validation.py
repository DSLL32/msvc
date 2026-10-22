#!/usr/bin/env python3
"""Record a completed, successful frontier replay without claiming case completion."""
import argparse
import hashlib
import json
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('replay_log', type=Path)
args = parser.parse_args()
configuration = json.loads(Path('functions.json').read_text())
log = args.replay_log.read_text()
frontiers = [line for line in log.splitlines() if line.startswith('original: ')]
assert len(frontiers) == len(configuration['accepted']) + 1
for name, line in zip(configuration['accepted'], frontiers):
    assert line == 'original: %s; reconstruction: %s' % (name, name)
last = frontiers[-1].split(';')[0][10:]
assert frontiers[-1] == 'original: %s; reconstruction: %s' % (last, last)
assert last not in configuration['accepted'] and not last.startswith('unknown:')
graph = json.loads(Path('build/graph-audit.json').read_text())
source = {record['name'] for record in configuration['driver']}
assert set(graph) == set(configuration['accepted']) & source
prepared = '__freebuf' in configuration['accepted']
if prepared:
    assert 'Both prepared RCPP inputs match the captured 3,381 bytes exactly.' in log
menu_verified = '_write_resource@8' in configuration['accepted']
if menu_verified:
    assert 'Both first menu payloads match the captured 190 bytes exactly.' in log
dialog_verified = '_resource_free@4' in configuration['accepted']
if dialog_verified:
    assert 'Both first dialog payloads match the captured 506 bytes exactly.' in log
report = dict(status='incomplete', case='resources-rich',
              accepted_frontiers=len(configuration['accepted']),
              replayed_frontiers=len(frontiers), current_frontier=last,
              exact_final_product_verified=False,
              source_functions_audited=len(graph),
              checks=['all accepted paired frontiers replayed from masked entry',
                      'direct call vertices, callback references and RET cleanup agree for accepted source functions'],
              target=configuration['target'],
              reconstruction_sha256=hashlib.sha256(Path('build/rc.exe').read_bytes()).hexdigest(),
              fixture_manifest_sha256=hashlib.sha256(Path('../fixtures/resources-rich/manifest.json').read_bytes()).hexdigest(),
              frontier_log_sha256=hashlib.sha256(log.encode()).hexdigest(),
              graph_audit=graph)
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
print('Recorded %d accepted frontiers, %d source functions; next: %s' %
      (len(configuration['accepted']), len(graph), last))
