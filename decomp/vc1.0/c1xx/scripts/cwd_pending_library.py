"""The single positively identified GETCWD library addition, not an unmask list."""
import hashlib
import json
from pathlib import Path
import re
from runtime_boundary_loader import load_boundary
from case_paths import paths
BINARY = Path(__file__).resolve().parents[1]
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def verify(path, config, candidate, case, historical=False):
    packet = json.loads(path.read_text())
    assert candidate == packet['function'] == '_browser_working_directory'
    assert case == packet['case'] == 'member_registry_browser'
    count = packet['accepted_before']
    assert packet['accepted_prefix_sha256'] == hashlib.sha256(json.dumps(config['accepted'][:count],separators=(',',':')).encode()).hexdigest()
    if historical:
        assert config['accepted'][count:count+2] == [candidate,'__getcwd']
    else:
        assert count == len(config['accepted'])
        assert packet['configuration_sha256'] == sha(BINARY/'functions.json')
        assert packet['native_executable_sha256'] == sha(BINARY/'build/native/C1XX3232.EXE')
    for name, digest in packet['evidence_sha256'].items():
        assert sha(path.parent/name) == digest, 'pending GETCWD evidence changed: '+name
    origin = json.loads((path.parent/packet['origin_receipt']).read_text())
    events = (path.parent/packet['origin_events']).read_text()
    proof = json.loads((path.parent/packet['boundary_receipt']).read_text())
    assert proof['kind'] == 'getcwd' and proof['runtime_order'] == ['__getcwd']
    assert proof['identity'] == load_boundary().identities(canonical=True,names=['__getcwd'])
    assert proof['target_sha256'] == origin['original_sha256'] == config['target']['sha256']
    assert proof['configuration_sha256'] == origin['configuration_sha256'] == packet['configuration_sha256']
    assert origin['candidate'] == candidate and origin['terminal'] == '__getcwd'
    assert proof['accepted_before'] == origin['accepted_count'] == count
    assert proof['original_capture_sha256'] == sha(path.parent/packet['origin_receipt'])
    words = {int(i):int(value,16) for i,value in re.findall(r'^argument (\d+) ([0-9a-f]+)$',events,re.M)}
    left, right = proof['original'], proof['native']
    assert re.findall(r'^caller ([0-9a-f]+)$',events,re.M) == ['00059ae8']
    assert words[0] == left['buffer'] and words[1] == left['capacity'] == right['capacity'] == 256
    assert left['return_va'] == 0x69ae8 and right['caller'] == 'browser_working_directory'
    assert left['returned_buffer'] == right['returned_buffer'] == 1
    assert json.loads((path.parent/packet['native_observation']).read_text()) == right
    raw = (path.parent/packet['original_observation']).read_text()
    assert re.findall(r'^runtime (\S+)$',raw,re.M) == ['__getcwd']
    for name in ('return_va','buffer'):
        assert int(re.search(r'^'+name+r' ([0-9a-f]+)$',raw,re.M)[1],16) == left[name]
    for name in ('capacity','returned_buffer'):
        assert int(re.search(r'^'+name+r' (\d+)$',raw,re.M)[1]) == left[name]
    _, fixture, _, _ = paths(config,case)
    phase = json.loads((fixture/'manifest.json').read_text())['phases']['c1']
    assert proof['canonical_working_directory'] == phase['canonical_working_directory']
    expected = proof['canonical_working_directory'].encode('latin1')+b'\0'
    for side in (left,right):
        before, after = side['before'], side['after']
        assert before['next'] == after['next'] == side['buffer']+256
        assert before['remaining'] == after['remaining']
        for name in ('errno','doserrno','last_error','control','status','tag'):
            assert before[name] == after[name], 'successful GETCWD clobbered '+name
        a, b = bytes.fromhex(before['bytes']), bytes.fromhex(after['bytes'])
        assert len(a) == len(b) == 256 and b[:len(expected)] == expected
        assert a[len(expected):] == b[len(expected):], 'GETCWD changed defined tail'
    assert left['before'] == dict(right['before'],next=left['before']['next'])
    assert left['after'] == dict(right['after'],next=left['after']['next'])
    for stage in ('before','after'):
        row = left[stage]
        expected_line = '%s errno %d doserrno %d last_error %d control %04x status %04x tag %04x next %08x remaining %d bytes %s' % (stage,row['errno'],row['doserrno'],row['last_error'],row['control'],row['status'],row['tag'],row['next'],row['remaining'],row['bytes'])
        assert expected_line in raw.splitlines(), 'raw readable GETCWD state differs'
    assert packet['library_order'] == ['__getcwd']
    return packet
