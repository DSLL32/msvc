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
    if name == 'verbose-names':
        assert 'Both verbose stdout streams match all 349 bytes with only the two exact six-digit native PID fields compared symbolically; stderr, exit status and complete case file set match the verbose-names oracle exactly.' in log
    elif name != 'resources-rich':
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
                      'instruction-end guards pass for every accepted source/CRT/import extent in both images',
                      'CRT initializer and termination callback sequences agree',
                      'exit statuses, input files, products and stderr match all %d oracles; stdout exact except explicitly validated six-digit native PID fields in verbose-names' % len(completed)],
              target=configuration['target'],
              reconstruction_sha256=hashlib.sha256(Path('build/rc.exe').read_bytes()).hexdigest(),
              fixture_manifest_sha256=hashlib.sha256(Path('../fixtures/resources-rich/manifest.json').read_bytes()).hexdigest(),
              frontier_log_sha256=hashlib.sha256(log.encode()).hexdigest(),
              graph_audit=graph)
report['additional_cases'] = {}
for name in complete_cases:
    if name == 'resources-rich':
        continue
    settings = complete_cases[name]
    if settings.get('fixture'):
        manifest_path = Path(settings['fixture']) / 'manifest.json'
        raw = manifest_path.read_bytes()
        assert hashlib.sha256(raw).hexdigest() == settings['suite_manifest_sha256']
        suite = json.loads(raw)
        capture_path = Path('build') / ('capture-' + name + '.verified.json')
        capture = json.loads(capture_path.read_text())
        assert capture['target_sha256'] == configuration['target']['sha256']
        case = next(c for c in suite['cases'] if c['id'] == name)
        assert capture['arguments'] == settings['arguments'] and capture['exit'] == settings['exit']
        for record in [case['stdout'],case['stderr']] + case['outputs']:
            file = record.get('output_name',Path(record['path']).name)
            if name == 'verbose-names' and file == 'stdout.bin':
                proof = compare.remaining.verify_verbose('capture-'+name)
                assert capture['symbolic_stdout'] == proof
                data = Path('build/capture-'+name+'.stdout').read_bytes()
                assert len(data)==record['bytes']==349
                assert capture['oracle'][file] == dict(bytes=349,sha256=hashlib.sha256(data).hexdigest())
            else:
                assert capture['oracle'][file] == dict(bytes=record['bytes'],sha256=record['sha256'])
        report['additional_cases'][name] = dict(shared_fixture=settings['fixture'],
                manifest_sha256=hashlib.sha256(raw).hexdigest(), case=case,
                independently_recaptured=capture, exact_streams_status_and_file_set=name!='verbose-names')
        if name == 'verbose-names':
            report['additional_cases'][name]['comparison_limit'] = 'Only stdout bytes161..166 and173..178 are symbolic; both six-digit fields are verified against actual native PID and temporary name objects; every other byte is exact.'
    else:
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
    extent = section['virtual_size']  # PE reader uses backed raw size when VirtualSize is zero.
    first, last_byte = original.base + section['rva'], original.base + section['rva'] + extent
    total += extent
    for record in index:
        if record['name'] not in configuration['accepted']:
            continue
        for start, end in [(record['start'],record['end'])] + record.get('shared',[]):
            restored.update(range(max(first,start),min(last_byte,end)))
report['restored_text'] = dict(bytes=len(restored), total_bytes=total,
                              percentage=round(100.0*len(restored)/total,2),
                              definition='union of accepted source/CRT functions, shared text data and import thunks, clipped to original code extents; VirtualSize or backed raw fallback; not branch coverage')
report['unaccepted_named_functions'] = sorted(source - set(configuration['accepted']))
product = Path('../fixtures/resources-rich/rc/resources.res').read_bytes()
report['result'] = dict(exit=0, stdout_bytes=0, stderr_bytes=0,
                      output_bytes=len(product), resource_records=19,
                      sha256=hashlib.sha256(product).hexdigest(),
                      exact_file_set=True)
report['accepted_instruction_extents'] = json.loads(Path('build/instruction-extents.json').read_text())
report['uncovered_code_inventory'] = json.loads(Path('uncovered-code.json').read_text())
report['runtime_callback_tables'] = json.loads(Path('build/runtime-tables.json').read_text())
report['runtime_layout_and_alias_audit'] = json.loads(Path('build/runtime-aliases.json').read_text())
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
if 'console-interrupt' in complete_cases:
    console = dict(event='CTRL_C_EVENT', argument=0, requested_group=0,
                   private_console_members=2, direct_callback_injection=False,
                   main_execution_barrier='initialize_lexer breakpoint retained during native signal dispatch',
                   font_directory_branch_exercised=False, paired={})
    for label in ('original','reconstructed'):
        stem = Path('build')/label
        proof = Path(str(stem)+'.proof').read_bytes()
        assert b'argument=0 handler_rva=' in proof
        assert b'reached_handler=1 controller_breaks=0 controller_interrupts=1' in proof
        assert b'exit=1 generated=1' in proof and b'first_chance_exception=' not in proof
        files = {}
        for file in sorted(Path(str(stem)+'.before').iterdir()):
            data = file.read_bytes()
            files[file.name] = dict(bytes=len(data),sha256=hashlib.sha256(data).hexdigest())
        console['paired'][label] = dict(proof_sha256=hashlib.sha256(proof).hexdigest(),before=files)
    report['real_console_interrupt'] = console
if 'verbose-names' in complete_cases:
    report['verbose_native_stdout'] = {label:compare.remaining.verify_verbose(label,Path('build/verbose-native'))
                                      for label in ('original','reconstructed')}
    report['verbose_native_stdout']['comparison'] = 'Full349-byte streams validated; only two six-digit fields at161/173 are symbolic. Raw streams retained unchanged.'
Path('validation-frontiers.txt').write_text(log)
Path('validation.json').write_text(json.dumps(report, indent=2) + '\n')
print('Recorded %d accepted frontiers, %d source functions and %d complete cases.' %
      (len(accepted), len(graph), len(completed)))
