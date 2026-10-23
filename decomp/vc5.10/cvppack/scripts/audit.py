#!/usr/bin/env python3
"""Target/source/data ownership checks plus the shared Rust graph engine."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import time
from state import BINARY, ROOT, load, mask, sha
from binary import PE


def ownership(config, pe):
    intervals = []
    for record in config['functions']:
        assert record['status'] in ('stub', 'candidate', 'accepted')
        if record['status'] == 'stub':
            assert record['extent'] is None and not record.get('text_data')
            continue
        assert record['extent'][0] == record['original']
        for start, end in [record['extent']] + record.get('text_data', []):
            assert start < end
            assert any(s['flags'] & 0x20 and pe.base+s['rva'] <= start < end <=
                       pe.base+s['rva']+min(s['size'],s['virtual_size']) for s in pe.sections)
            intervals.append((start, end, record['name']))
    for record in config['support_unmasked'] + config['loader_data']:
        assert record.get('evidence'), 'restored support/data requires evidence'
        assert (BINARY / record['evidence']).is_file()
        start, end = record['start'], record['end']
        assert start < end and pe.offset(end-1)-pe.offset(start) == end-start-1
        intervals.append((start, end, record['name']))
    for i, left in enumerate(intervals):
        for right in intervals[i+1:]:
            assert left[1] <= right[0] or right[1] <= left[0], ('overlapping ownership', left, right)
    for start, end, owner in intervals:
        for record in config['functions']:
            assert record['name'] == owner or not start <= record['original'] < end, ('another entry owned', owner, record['name'])
    return intervals


def run(config_path, candidates=(), output=None):
    begin = time.perf_counter()
    config = json.loads(config_path.read_text())
    assert sha(ROOT / config['target']['path']) == config['target']['sha256']
    pe = PE(ROOT / config['target']['path'])
    intervals = ownership(config, pe)
    from runtime import identify
    exact_runtime = identify(ROOT/config['target']['path'], {'_main':config['target']['application_main']})
    assert exact_runtime == json.loads((BINARY/'runtime.json').read_text()), 'runtime identity index is stale'
    exact_ranges = {(r['name'],r['start'],r['end']) for r in exact_runtime['functions']+exact_runtime['imports']}
    for record in config['support_unmasked']:
        assert (record['name'],record['start'],record['end']) in exact_ranges, 'unproved runtime range'
        proof = json.loads((BINARY/record['evidence']).read_text())
        assert proof['library_sha256'] == exact_runtime['library_sha256']
        assert proof['original_sha256'] == exact_runtime['image_sha256']
        assert record in proof['selected'], 'range absent from reviewed runtime boundary'
    functions = config['functions']
    assert len({r['name'] for r in functions}) == len(functions)
    assert len({r['symbol'] for r in functions}) == len(functions)
    source = '\n'.join(path.read_text() for path in (BINARY / 'src').glob('*.c'))
    for record in functions:
        native_name = record['native_symbol']
        declaration = 'main' if record['symbol'] == '_main' else record['symbol'].lstrip('_').split('@')[0]
        assert re.search(r'\b' + re.escape(declaration) + r'\s*\([^;]+?\)\s*\{', source), ('missing source body', record['name'])
        assert native_name
        if record['status']=='stub':
            stub = re.search(r'\b'+re.escape(declaration)+r'\s*\([^;{}]*\)\s*\{\s*__asm\s*\{\s*int\s+3\s*\}\s*(?:return\s+(?:0|NULL);\s*)?\}',source)
            assert stub, ('stub is not a named INT3 boundary',record['name'])
    checked = [r for r in functions if r['status'] == 'accepted' or r['name'] in candidates]
    assert all(r['status'] != 'stub' and r['extent'] for r in checked), 'candidate not reconstructed'
    graph_config = {
        'target': config['target'], 'accepted': [r['symbol'] for r in checked],
        'driver': [dict(name=r['symbol'], start=r['original'],
                        end=r['extent'][1] if r['extent'] else r['original']+1) for r in functions],
    }
    destination = output.parent if output else BINARY / 'build'
    destination.mkdir(parents=True,exist_ok=True)
    generated = destination / 'graph-config.json'
    generated.write_text(json.dumps(graph_config, indent=2) + '\n')
    empty = destination / 'graph-runtime.json'
    empty.write_text('[]\n')
    import importlib.util
    spec=importlib.util.spec_from_file_location('switch_data',BINARY/'scripts/switch-data.py')
    switches=importlib.util.module_from_spec(spec);spec.loader.exec_module(switches)
    metadata,metadata_proof=switches.prepare(checked,destination)
    tool = Path(os.environ.get('GRAPH_AUDIT_BIN',str(ROOT/'tooling/graph-audit/target/release/graph-audit'))).resolve()
    command = [str(tool), '--profile', 'cl', '--project', str(BINARY), '--repo', str(ROOT),
               '--original', str(ROOT / config['target']['path']), '--rebuilt', str(BINARY / 'build/CVPACK.EXE'),
               '--map', str(BINARY / 'build/cvpack.map'), '--config', str(generated),
               '--runtime', str(empty), '--rebuilt-runtime', str(empty), '--metadata',str(metadata),'--discovery', 'cfg',
               '--check-ret', '--check-references', '--check-ownership', '--check-extents']
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=15)
    assert result.returncode == 0, result.stderr.decode()
    graph = json.loads(result.stdout)
    rebuilt=PE(BINARY/'build/CVPACK.EXE')
    publics={}
    for line in (BINARY/'build/cvpack.map').read_text().splitlines():
        found=re.match(r'\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',line)
        if found:publics[found[1]]=int(found[2],16)
    symbols=subprocess.check_output(['nm','-S','--defined-only',str(BINARY/'build/native/CVPACK')],text=True)
    sizes={line.split()[-1]:int(line.split()[1],16) for line in symbols.splitlines() if len(line.split())==4}
    data=json.loads((BINARY/'data.json').read_text())
    for item in data:
        assert sizes[item['name']]==item['size'], ('native global type/extent differs',item['name'])
        assert pe.read(item['original'],item['size'])==rebuilt.read(publics['_'+item['name']],item['size']), ('initial Win32 data differs',item['name'])
    import importlib.util
    spec=importlib.util.spec_from_file_location('resource_evidence',BINARY/'scripts/resource-evidence.py')
    resources=importlib.util.module_from_spec(spec);spec.loader.exec_module(resources)
    expected=json.loads((BINARY/'cases/help/resources.json').read_text())['resources']
    actual=resources.resources_from(rebuilt)
    assert [(r['id'],r['language'],r['utf16le']) for r in actual]==[(r['id'],r['language'],r['utf16le']) for r in expected], 'encountered Win32 resources differ'
    receipt = dict(schema=1, config_sha256=sha(config_path), graph_config_sha256=sha(generated),
        original_sha256=sha(ROOT / config['target']['path']), rebuilt_sha256=sha(BINARY / 'build/CVPACK.EXE'),
        native_sha256=sha(BINARY / 'build/native/CVPACK'), map_sha256=sha(BINARY / 'build/cvpack.map'),
        graph_tool_sha256=sha(tool),graph_metadata_sha256=sha(metadata),graph_metadata_proof_sha256=sha(metadata_proof),
        switch_data_validator_sha256=sha(BINARY/'scripts/switch-data.py'), data_sha256=sha(BINARY/'data.json'),
        runtime_sha256=sha(BINARY/'runtime.json'), graph_runtime_sha256=sha(empty),
        audit_wrapper_sha256=sha(BINARY/'scripts/audit.py'),
        data_objects_checked=len(data), resource_strings_checked=len(actual),
        resource_evidence_sha256=sha(BINARY/'cases/help/resources.json'),
        candidates=list(candidates), checked_functions=[r['name'] for r in checked],
        owned_ranges=len(intervals), graph_command=command, graph=graph,
        seconds=time.perf_counter()-begin)
    if output:
        output.write_text(json.dumps(receipt, indent=2) + '\n')
    return receipt


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--config', type=Path, default=BINARY / 'functions.json')
    parser.add_argument('--candidate', action='append', default=[])
    parser.add_argument('--output', type=Path, default=BINARY / 'build/audit.json')
    args = parser.parse_args()
    print(json.dumps(run(args.config, args.candidate, args.output), indent=2))
