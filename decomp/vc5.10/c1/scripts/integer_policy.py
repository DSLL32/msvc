"""Advance one stale guard label without changing captured execution metadata."""
import copy
import json
from pathlib import Path
from provenance import PROJECT, sha

PACKET = PROJECT / 'cases/minimal/integer-policy-refinement.json'
PATHS = [['value_copy', 'hard_frontier_rva']]


def derive(old, current, packet):
    assert packet['schema'] == 1
    assert packet['kind'] == 'validator-only integer frontier guard refinement; execution metadata remains frozen'
    assert packet['allowed_paths'] == PATHS
    assert packet['old_rva'] == 0x132f9 and packet['new_rva'] == 0x1335b
    expected = copy.deepcopy(old)
    assert expected['value_copy']['hard_frontier_rva'] == 0x132f9
    expected['value_copy']['hard_frontier_rva'] = 0x1335b
    assert current == expected, 'any other field/layout/default/alias/consumer or mask delta is forbidden'
    assert current['command_line']['unknown_semantic_consumer_frontier'] == 0x1335b
    assert current['multibyte_scanner']['unknown_initialized_bytes'] == 4200
    assert packet['semantic_memory'] == dict(
        integer_parser='only known owned input bytes, three public CRT imports and private stack; no target global reads or writes',
        executed_caller_stores=['il_prefix', 'source_file', 'warning_level', 'processor', 'optimize_size', 'inline_level', 'suppress_banner', 'current_argument', 'command_argc', 'command_argv', 'pdb_signature'],
        unknown_initialized_bytes=4200, literal_tail_unmodeled_bytes=16,
        unknown_consumer_reached=False, actual_masked_frontier=0x1335b)
    return current


def validate(directory, candidate, inputs):
    key = 'decomp/vc5.10/c1/' + candidate['field_map']
    current = PROJECT / candidate['field_map']
    if inputs[key] == sha(current):
        return inputs
    packet = json.loads(PACKET.read_text())
    old = Path(packet['old_field_map']['path'])
    assert inputs[key] == sha(old) == packet['old_field_map']['sha256']
    assert sha(current) == packet['new_field_map']['sha256']
    derive(json.loads(old.read_text()), json.loads(current.read_text()), packet)
    matches = [row for row in packet['executions'] if row['path'] == str(directory.resolve())]
    assert len(matches) == 1
    row = matches[0]
    assert sha(directory / 'receipt.json') == row['receipt_sha256']
    receipt = json.loads((directory / 'receipt.json').read_text())
    assert receipt['kind'] == row['kind'] and receipt['probe'] == row['probe']
    assert receipt['kind'] in ('explicit-original-capture', 'native-gdb-frontier')
    if receipt['kind'] == 'explicit-original-capture':
        assert receipt['mode'] == 'masked' and receipt['exit'] == 125
        assert receipt['helper_sha256'] == row['helper_sha256'] == packet['helper_sha256'] == sha(PROJECT / 'build/dll-capture.exe')
        assert sha(directory / 'ranges.txt') == packet['ranges_sha256']
        assert sha(directory / 'loaded-target.bin') == receipt['target_sha256']
        assert sha(directory / 'loaded-msvcrt.bin') == receipt['canonical_crt_sha256']
    for name, packet_key, filename in [('candidate_sha256', 'candidate_sha256', 'candidate.json'), ('functions_sha256', 'config_sha256', 'functions.json')]:
        assert receipt[name] == row[name] == packet[packet_key] == sha(directory / filename)
    assert sha(PROJECT / candidate['audit_config']) == packet['config_sha256']
    assert sha(PROJECT / 'cases/minimal/candidate.json') == packet['candidate_sha256']
    assert receipt['rebuilt_sha256'] == row['rebuilt_sha256'] == packet['rebuilt_sha256'] == sha(PROJECT / 'build/c1.dll')
    assert receipt['native_sha256'] == row['native_sha256'] == packet['native_sha256']
    assert receipt['source_hashes'] == row['source_hashes']
    assert all(sha(PROJECT / path) == value for path, value in row['source_hashes'].items())
    assert candidate['expected_frontier']['rva'] == 0x1335b
    function = [f for f in candidate['functions'] if f['name'] == '_parse_option_integer']
    assert len(function) == 1 and function[0]['chunks'] == [[0x132f9, 0x1335b], [0x30a17, 0x30a3c]]
    # The surrounding audit protocol still verifies every unchanged input. Only
    # this validator-policy hash changes; frozen receipts/maps stay untouched.
    result = dict(inputs)
    result[key] = sha(current)
    return result
