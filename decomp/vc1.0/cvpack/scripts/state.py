"""Pure metadata and byte masking; no original process execution."""
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

def mask(config, pe, output, candidates=()):
    image = bytearray(pe.data)
    for section in pe.sections:
        if section['flags'] & 0x20:
            image[section['off']:section['off']+section['size']] = b'\xcc' * section['size']
    for record in config['functions']:
        if record['status'] != 'accepted' and record['name'] not in candidates:
            continue
        for start, end in [record['extent']] + record.get('text_data', []):
            a, b = pe.offset(start), pe.offset(end-1)+1
            image[a:b] = pe.data[a:b]
    for record in config.get('support_unmasked',[]):
        a,b=pe.offset(record['start']),pe.offset(record['end']-1)+1
        image[a:b]=pe.data[a:b]
    Path(output).write_bytes(image)

def case(name):
    result = json.loads((BINARY / 'cases' / name / 'case.json').read_text())
    assert result['name'] == name
    return result

def prepare(record, work):
    work.mkdir()
    for item in record.get('inputs', []):
        source = ROOT / item['path']
        assert sha(source) == item['sha256'], 'changed input: ' + str(source)
        assert Path(item['name']).name == item['name']
        (work / item['name']).write_bytes(source.read_bytes())

    from inputs import prepare as prepare_extra
    prepare_extra(record,work)
