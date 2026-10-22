#!/usr/bin/env python3
"""Project the recorded original history onto audited RCPP application entries."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

BINARY = Path(__file__).resolve().parents[1]
CACHE = BINARY/'cases/native-traces.json'
CRT_SOURCES = {'startup', 'arguments', 'runtime', 'stubs', 'file-runtime'}

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def native_name(logical):
    assert logical.startswith('_'), logical
    return re.sub(r'@\d+$', '', logical[1:])

def definitions(directory):
    objects = [p for p in sorted(directory.glob('*.o')) if p.stem not in CRT_SOURCES]
    assert objects, 'missing native application objects'
    output = subprocess.check_output(['nm','-A','--defined-only','--format=posix',
                                     *map(str,objects)],text=True)
    result = {}
    for line in output.splitlines():
        parts = line.split()
        if len(parts)<4 or parts[2] not in ('T','t','W','w'): continue
        name, source = parts[1], 'src/'+Path(parts[0][:-1]).stem+'.c'
        assert name not in result, 'duplicate native application definition: '+name
        result[name] = source
    return result

def project(directory):
    cfg = json.loads((BINARY/'functions.json').read_text())
    original = {f['name']:f for f in cfg['functions']}
    # Source-backed application vertices exclude individually identified CRT
    # startup/argument/allocator boundaries, despite their descriptive rc_ names.
    excluded_source = {'_rcpp_start','_rc_heap_initialize','_rc_setargv','_rc_parse_command'}
    app = {name:f for name,f in original.items()
           if f['status']=='source' and name.startswith('_rc_') and name not in excluded_source}
    compiled = definitions(directory)
    expected = {native_name(n):n for n in app}
    assert compiled.keys()==expected.keys(), ('unmapped application definitions',
        sorted(compiled.keys()-expected.keys()),sorted(expected.keys()-compiled.keys()))
    accepted = cfg['accepted']
    classified = {n:'application' if n in app else 'import' if n.startswith('import:') else 'CRT'
                  for n in accepted}
    # Reject unknown nonapplication identities instead of silently filtering them.
    for name,kind in classified.items():
        if kind=='CRT':
            assert name in original and (name in excluded_source or
                original[name]['status'] in ('library','library-boundary','source-runtime') or name=='_malloc'),name
    projected = lambda names:[native_name(n) for n in names if classified[n]=='application']
    cases = {}
    metadata={'functions.json':sha(BINARY/'functions.json')}
    for case in list(cfg['cases'])+list(cfg['direct_cases']):
        indexes = [i for i,n in enumerate(accepted)
                   if cfg.get('acceptance_cases',{}).get(n,'resources-rich')==case]
        first,last = (indexes[0],indexes[-1]+1) if indexes else (len(accepted),len(accepted))
        assert not indexes or indexes==list(range(first,last)), case
        code=0
        if case!='option-helper':
            fixture=(BINARY/cfg['cases'][case]['fixture']).resolve()
            manifest=fixture/'manifest.json'
            metadata[str(manifest.relative_to(BINARY.parent)).replace('fixtures/','../fixtures/',1)]=sha(manifest)
            data=json.loads(manifest.read_text())
            if case!='resources-rich':code=next(c['expected_exit']for c in data['cases']if c['id']==case)
        cases[case] = dict(first=first,last=last,seen_count=len(projected(accepted[:first])),
                          expected=projected(accepted[first:last]),terminal=dict(kind='exit',code=code))
    return dict(schema=1,description='Projection of cached original-backed acceptance history; no original execution or recapture',
                metadata_sha256=metadata,
                application_definitions={n:dict(source=compiled[n],logical=expected[n])for n in sorted(compiled)},
                accepted_application=projected(accepted),
                excluded_accepted={kind:[n for n in accepted if classified[n]==kind]for kind in ('CRT','import')},cases=cases)

def load(directory):
    cache = json.loads(CACHE.read_text())
    for name,digest in cache['metadata_sha256'].items():
        assert sha(BINARY/name)==digest, 'stale cached original projection: '+name
    assert definitions(directory)=={n:r['source']for n,r in cache['application_definitions'].items()}, 'new/missing/unmapped application symbol'
    return cache

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--write',action='store_true')
    options=parser.parse_args()
    directory=BINARY/'build/native-trace'
    if options.write: CACHE.write_text(json.dumps(project(directory),indent=2)+'\n')
    else: load(directory)
