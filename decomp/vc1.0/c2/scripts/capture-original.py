"""Explicit masked-original capture; never called by native fixtures.

Run from the binary root after a canonical build. Saves terminal evidence and,
for a successful complete case, clock-normalized output verified against its
independently captured fixture. Wine and the private prefix are confined here.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import time
import pe_indexes as index


def sha(data):
    return hashlib.sha256(data).hexdigest()


def capture(profile, receipt_path):
    started = time.perf_counter()
    original, rebuilt, config, records, unused = index.indexes()
    case = config['cases'][profile]
    manifest = Path(case['fixture_manifest'])
    raw = manifest.read_bytes()
    assert sha(raw) == case['fixture_manifest_sha256']
    phase = json.loads(raw)['phases']['c2']
    assert phase['arguments'] == [] and set(phase['environment']) == {'MSC_CMD_FLAGS'}
    directory = Path(tempfile.mkdtemp(prefix='original-'+profile+'-',
                                    dir=Path.home()/'.ai-tmp/vc1.0-c2'))
    work = directory/'work'
    work.mkdir()
    for source in phase['inputs']:
        data = (manifest.parent/source['path']).read_bytes()
        assert sha(data) == source['sha256']
        (work/Path(source['path']).name).write_bytes(data)
    image = directory/'C23232.EXE'
    index.mask(original, records, config['accepted'], image)
    environment = index.wt.environment()
    environment['MSC_CMD_FLAGS'] = phase['environment']['MSC_CMD_FLAGS'].format(
        bin=index.wt.windows(index.wt.TC/'bin'))
    command = ['wine', 'build/debug-run.exe', index.wt.windows(image), '',
               *[index.wt.windows(directory/name) for name in ('stdout','stderr','events')],
               index.wt.windows(work)]
    prepared = time.perf_counter()
    subprocess.run(command, env=environment, check=True, timeout=90,
                   stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    executed = time.perf_counter()
    events = (directory/'events').read_text()
    assert not (directory/'stdout').read_bytes() and not (directory/'stderr').read_bytes()
    receipt = dict(label=receipt_path.stem, profile=profile, accepted=len(config['accepted']),
                   events=events, target_sha256=sha(original.data), masked_sha256=sha(image.read_bytes()),
                   config_sha256=sha(Path('functions.json').read_bytes()), manifest_sha256=sha(raw),
                   inputs={s['path']:s['sha256'] for s in phase['inputs']},
                   environment=phase['environment'], directory=str(directory))
    if events == 'exit 0\n':
        receipt.update(logical='exit:0', original_va=None)
        artifacts = []
        artifact_directory = receipt_path.with_suffix('')
        artifact_directory.mkdir(parents=True, exist_ok=False)
        for output in phase['outputs']:
            fields = output['ignored_fields']
            assert len(fields) == 1 and (fields[0]['offset'], fields[0]['bytes']) == (4,4)
            expected = bytearray((manifest.parent/output['path']).read_bytes())
            assert sha(expected) == output['sha256']
            name = output.get('output_name', Path(output['path']).name)
            data = (work/name).read_bytes()
            normalized = bytearray(data)
            expected[4:8] = normalized[4:8] = b'\0'*4
            assert normalized == expected, 'original output differs beyond timestamp'
            assert sha(normalized) == output['normalized_sha256']
            target = artifact_directory/name
            shutil.copyfile(work/name, target)
            artifacts.append(dict(path=str(target), name=name, bytes=len(data), sha256=sha(data),
                                  normalized_sha256=sha(normalized), ignored_timestamp=[4,4]))
        receipt['outputs'] = artifacts
    else:
        match = re.fullmatch(r'exception 80000003 rva ([0-9a-f]+) base ([0-9a-f]+) first 1\nexit 99\n',events)
        assert match, events
        address = int(match[1],16)+int(match[2],16)
        found = [r for r in records if r['start'] == address]
        assert len(found) == 1 and found[0]['name'] not in config['accepted']
        receipt.update(logical=found[0]['name'], original_va=hex(address))
    receipt.update(setup_seconds=prepared-started, execution_seconds=executed-prepared,
                   check_seconds=time.perf_counter()-executed, total_seconds=time.perf_counter()-started)
    receipt_path.write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(receipt,indent=2))
    return receipt


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', required=True)
    parser.add_argument('--receipt', required=True, type=Path)
    args = parser.parse_args()
    assert not args.receipt.exists(), 'refusing to replace reference evidence'
    capture(args.profile, args.receipt)
