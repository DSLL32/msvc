#!/usr/bin/env python3
"""Cached original RC application first-entry projection; no original execution."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

BINARY=Path(__file__).resolve().parents[1]
CACHE=BINARY/'cases/native-traces.json'
CRT_FILES={'startup','heap-crt','arguments','paths','open','lowio','format','stdio','process','atol',
           'crt-state','termination','flush-all','temporary-buffer','scan-input','spawn-search'}
MIXED_CRT={'rc_toupper':'_rc_toupper','rc_tolower':'_rc_tolower','recon_wcscpy':'_wcscpy'}

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def native_name(name):
    assert name[0] in '_@',name
    value=re.sub(r'@\d+$','',name[1:])
    assert re.fullmatch(r'[A-Za-z_]\w*',value),name
    return value

def definitions(directory):
    files=sorted(directory.glob('*.o'));assert files,'native objects missing'
    raw=subprocess.check_output(['nm','-A','--defined-only','--format=posix',*map(str,files)],text=True)
    result={}
    for line in raw.splitlines():
        bits=line.split()
        if len(bits)<4 or bits[2] not in ('T','t','W','w'):continue
        name=bits[1];assert name not in result,('duplicate native definition',name)
        result[name]='src/'+Path(bits[0][:-1]).stem+'.c'
    return result

def project(directory):
    config=json.loads((BINARY/'functions.json').read_text())
    native=definitions(directory)
    drivers={native_name(r['name']):r for r in config['driver']}
    # The only retained CRT definitions in mixed application files are the two
    # locale case converters and the recovered wchar copy routine. They remain
    # compiled but are outside this application projection on both sides.
    app={n:s for n,s in native.items() if n not in MIXED_CRT}
    assert set(app)<=set(drivers),('unmapped native application definitions',sorted(set(app)-set(drivers)))
    for n in MIXED_CRT:
        assert n in native,('missing mixed-file CRT definition',n)
    logical={r['name']:n for n,r in drivers.items() if n in app}
    accepted=config['accepted'];hist=config.get('frontier_cases',{})
    assert len(accepted)==len(set(accepted))
    runtime={r['name'] for r in json.loads((BINARY/'runtime.json').read_text())}
    excluded={'CRT':[],'import':[]}
    for name in accepted:
        if name in logical:continue
        if name.startswith('import:'):excluded['import'].append(name)
        else:
            assert name in runtime or name in {r['name'] for r in config['driver']},name
            excluded['CRT'].append(name)
    # Every canonical application function must also be represented natively;
    # CRT source units are deliberately supplied by the shared host boundary.
    map_sources={}
    for line in (BINARY/'build/rc.map').read_text().splitlines():
        match=re.match(r'\s+\w+:\w+\s+(\S+)\s+\w+\s+f\s+(\S+)\.obj$',line)
        if match:map_sources.setdefault(match[1],Path(match[2]).name)
    for name,r in ((r['name'],r) for r in config['driver']):
        source=map_sources.get(name)
        assert source is not None,('missing canonical source provenance',name)
        if source not in CRT_FILES and name not in MIXED_CRT.values():
            assert name in logical,('canonical application function missing natively',name,source)
    cases={}
    for name,settings in config['cases'].items():
        indexes=[i for i,n in enumerate(accepted) if hist.get(n,'resources-rich')==name]
        first,last=(indexes[0],indexes[-1]+1) if indexes else (len(accepted),len(accepted))
        assert not indexes or indexes==list(range(first,last)),name
        project_names=lambda names:[logical[n] for n in names if n in logical]
        cases[name]=dict(first=first,last=last,seen=project_names(accepted[:first]),
                        expected=project_names(accepted[first:last]),exit=settings.get('exit',0),
                        kind=settings.get('kind','CLI'))
    return dict(schema=1,description='Frozen original acceptance first-entry sequence projected onto RC application functions, excluding CRT/import/shims on both sides',
        metadata_sha256={name:sha(BINARY/name) for name in ('functions.json','runtime.json','validation-frontiers.txt')},
        application_definitions={n:dict(logical=drivers[n]['name'],source=s) for n,s in sorted(app.items())},
        excluded_accepted=excluded,mixed_file_CRT=MIXED_CRT,cases=cases)

def load(directory,path=CACHE):
    cache=json.loads(path.read_text())
    for name,digest in cache['metadata_sha256'].items():assert sha(BINARY/name)==digest,'stale native cache: '+name
    actual=definitions(directory)
    expected={n:r['source'] for n,r in cache['application_definitions'].items()}
    assert {n:s for n,s in actual.items() if n not in MIXED_CRT}==expected,'native application definitions changed'
    assert set(MIXED_CRT)<=set(actual)
    return cache

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--write',action='store_true')
    ap.add_argument('--objects',type=Path,default=BINARY/'build/native-trace');options=ap.parse_args()
    cache=project(options.objects) if options.write else load(options.objects)
    if options.write:CACHE.write_text(json.dumps(cache,indent=2)+'\n')
    print(len(cache['application_definitions']),'application definitions;',len(cache['cases']),'cached original segments')
if __name__=='__main__':main()
