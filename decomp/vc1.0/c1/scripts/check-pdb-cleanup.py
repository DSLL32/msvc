#!/usr/bin/env python3
"""Check frozen public PDB cleanup ownership and reject damaged observations."""
import importlib.util
import json
from pathlib import Path
import re

BINARY = Path(__file__).resolve().parents[1]
CASE = BINARY/'cases/legacy-pdb-object'
spec = importlib.util.spec_from_file_location('pdb_contract', Path(__file__).with_name('pdb-contract.py'))
contract = importlib.util.module_from_spec(spec)
spec.loader.exec_module(contract)


def changed_memory(text, name, offset, value):
    pattern = r'^(pdb-memory '+re.escape(name)+r' [0-9a-f]+ \d+ )([0-9a-f]+)$'
    def replace(match):
        raw = bytearray.fromhex(match[2])
        raw[offset:offset+len(value)] = value
        return match[1]+raw.hex()
    result, count = re.subn(pattern, replace, text, flags=re.M)
    assert count == 1
    return result


def main():
    header = (CASE/'data/bad-magic/msvc.pdb').read_bytes()
    evidence = CASE/'evidence/cleanup'
    expected = json.loads((evidence/'contracts.json').read_text())
    entry = (evidence/'pdb-cleanup-owned-entry/events').read_text()
    assert contract.cleanup_entry(entry, 'entry', header) == expected['fresh_entry_contract']
    texts = [(evidence/name/'events').read_text() for name in
             ['pdb-cleanup-body-first', 'pdb-cleanup-body-second', 'pdb-cleanup-owned-native-body']]
    for text in texts:
        assert contract.cleanup_return(text, header) == expected['twice_original_return_contract']
    text = texts[0]
    mutations = {
        'missing free': re.sub(r'^pdb-free .*\n', '', text, flags=re.M),
        'duplicate free': re.sub(r'^(pdb-free .*\n)', r'\1\1', text, flags=re.M),
        'foreign free': re.sub(r'^pdb-free .*$', 'pdb-free 00000001', text, flags=re.M),
        'wrong header': changed_memory(text, 'object-before', 0, b'X'),
        'wrong constructor mode': changed_memory(text, 'object-before', 108, bytes(4)),
        'rewritten list': changed_memory(text, 'list-after', 8, b'\x01'),
        'rewritten owner': changed_memory(text, 'object-after', 112, b'\xff'),
        'wrong stack cleanup': re.sub(r'^(pdb-registers return )\S+', r'\g<1>00000000', text, flags=re.M),
        'changed nonvolatile': re.sub(r'^(pdb-registers return .* )\S+$', r'\g<1>ffffffff', text, flags=re.M),
    }
    rejected = []
    for label, damaged in mutations.items():
        try:
            contract.cleanup_return(damaged, header)
        except (AssertionError, KeyError):
            rejected.append(label)
        else:
            raise AssertionError('accepted '+label)
    # Unknown constructor fields are compared only for preservation in this
    # destructor. They are never declared initialized or compared across hosts.
    unspecified = changed_memory(changed_memory(text, 'object-before', 112, b'\x53\x46\x79\x82'),
                                 'object-after', 112, b'\x53\x46\x79\x82')
    assert contract.cleanup_return(unspecified, header) == expected['twice_original_return_contract']
    print(json.dumps(dict(status='pass', frozen_positive_observations=4,
                          rejected=rejected, own_unwritten_bytes_positive=True), indent=2))


if __name__ == '__main__':
    main()
