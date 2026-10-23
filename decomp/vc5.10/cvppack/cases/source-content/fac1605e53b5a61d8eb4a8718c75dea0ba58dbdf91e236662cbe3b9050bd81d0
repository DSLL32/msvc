"""Verify encountered source evidence, then use the shared native encoder."""
import importlib.util
import json
from pathlib import Path
import re
from state import BINARY, ROOT, sha


def prepare(output):
    evidence=json.loads((BINARY/'cases/help/resources.json').read_text())
    target=ROOT/'toolchains/5.10/bin/CVPACK.EXE'
    assert sha(target)==evidence['target_sha256']
    raw=target.read_bytes()
    source=BINARY/'src/cvpack.rc'
    source_proof=json.loads((BINARY/'cases/help/resource-source.json').read_text())
    assert source_proof['rc_sha256']==sha(source)
    assert source_proof['evidence_sha256']==sha(BINARY/'cases/help/resources.json')
    entries=[]
    source_entries={int(identifier):text for identifier,text in re.findall(r'^\s*(\d+)\s+"([^"\n]*)"\s*$',source.read_text(),re.M)}
    for item in evidence['resources']:
        offset=item['record_file_offset'];length=int.from_bytes(raw[offset:offset+2],'little')
        encoded=bytes.fromhex(item['utf16le'])
        assert item['type']==6 and item['language']==1033 and length==item['length_utf16']
        assert raw[offset+2:offset+2+length*2]==encoded
        text=encoded.decode('utf-16le')
        assert text==item['text']==source_entries[item['id']]
        assert text.encode('ascii').hex()==item['ansi_bytes']
        entries.append(dict(kind='string',id=item['id'],language=item['language'],text=text))
    assert set(source_entries)=={entry['id'] for entry in entries}=={8008,8009,1029}
    spec=importlib.util.spec_from_file_location('shared_resources',ROOT/'lib/recon/resources.py')
    encoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(encoder)
    encoder.write_resources(output,evidence['target_sha256'],entries)
    return dict(provider_sha256=sha(output),source_rc_sha256=sha(source),entries=len(entries),
                resource_evidence_sha256=sha(BINARY/'cases/help/resources.json'))
