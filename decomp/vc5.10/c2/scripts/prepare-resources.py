#!/usr/bin/env python3
"""Verify encountered target resource evidence and prepare the shared provider."""
import importlib.util
import json
from pathlib import Path
import re
from state import BINARY, ROOT, TARGET, sha

def main():
    evidence=json.loads((BINARY/'cases/analysis/encountered-strings.json').read_text())
    target=TARGET.read_bytes()
    assert sha(target)==evidence['target_sha256']
    source=(BINARY/'src/messages.rc').read_text()
    rows={int(n):text for n,text in re.findall(r'^\s+(\d+) "([^"\n]*)"$',source,re.M)}
    assert len(rows)==len(evidence['entries'])==7
    for entry in evidence['entries']:
        off=entry['utf16_file_offset'];data=target[off:off+entry['utf16_bytes']]
        assert sha(data)==entry['utf16_sha256']
        assert data.decode('utf-16le')==entry['text']==rows[entry['id']]
        assert entry['language']==1033
    spec=importlib.util.spec_from_file_location('shared_recon_resources',ROOT/'lib/recon/resources.py')
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    output=BINARY/'build/messages.rmr'
    module.write_resources(output,evidence['target_sha256'],evidence['entries'])
    (BINARY/'build/resources.json').write_text(json.dumps({'target_sha256':sha(target),'source_sha256':sha(source.encode()),
        'evidence_sha256':sha((BINARY/'cases/analysis/encountered-strings.json').read_bytes()),'provider_sha256':sha(output.read_bytes())},indent=2)+'\n')

if __name__=='__main__':main()
