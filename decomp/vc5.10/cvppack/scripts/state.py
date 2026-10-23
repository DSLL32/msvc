"""Target metadata and explicit masking; contains no execution or CFG engine."""
import hashlib
import json
from pathlib import Path
from binary import PE

BINARY = Path(__file__).resolve().parents[1]
ROOT = BINARY.parents[2]


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def load():
    config = json.loads((BINARY / 'functions.json').read_text())
    target = ROOT / config['target']['path']
    assert sha(target) == config['target']['sha256'], 'original target changed'
    return config, PE(target)


def mask(config, pe, candidates=()):
    image = bytearray(pe.data)
    for section in pe.sections:
        if section['flags'] & 0x20:
            image[section['off']:section['off']+section['size']] = b'\xcc' * section['size']
    for record in config['functions']:
        if record['status'] != 'accepted' and record['name'] not in candidates:
            continue
        assert record['extent'], 'candidate has no complete extent'
        for start, end in [record['extent']] + record.get('text_data', []):
            a, b = pe.offset(start), pe.offset(end-1)+1
            image[a:b] = pe.data[a:b]
    for record in config.get('support_unmasked', []) + config.get('loader_data', []):
        a, b = pe.offset(record['start']), pe.offset(record['end']-1)+1
        image[a:b] = pe.data[a:b]
    return image
