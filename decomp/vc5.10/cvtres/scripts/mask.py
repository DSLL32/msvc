"""Fail-closed target masking with proven loader data and explicit restorations."""
import hashlib
import json
from pathlib import Path
from binary import PE
from layout import loader_data
from runtime import identify

BINARY=Path(__file__).resolve().parents[1]
ROOT=BINARY.parents[2]
TARGET=ROOT/'toolchains/5.10/bin/CVTRES.EXE'

def sha(data):return hashlib.sha256(data).hexdigest()
def load():
    config=json.loads((BINARY/'functions.json').read_text())
    pe=PE(TARGET)
    assert sha(pe.data)==config['target']['sha256']
    return config,pe

def build(destination,initial=False,candidates=(),unmasked=False,configuration=None):
    config,pe=load() if configuration is None else (configuration,PE(TARGET))
    assert sha(pe.data)==config['target']['sha256']
    data=bytearray(pe.data)
    code=[s for s in pe.sections if s['flags']&0x20]
    protected=loader_data(pe)
    runtime=identify()
    app=[f for f in config['functions'] if f['status']=='accepted' or f['name'] in candidates]
    assert set(candidates)<={f['name'] for f in config['functions']}
    if initial:assert not candidates
    elif not unmasked:assert {f['name'] for f in config['functions'] if f['status']=='candidate'}==set(candidates),'wrong trial candidate set'
    restored=[]
    if not unmasked:
        for section in code:
            data[section['off']:section['off']+section['size']]=b'\xcc'*section['size']
        def restore(start,end,kind):
            assert start<end
            assert any(pe.base+s['rva']<=start and end<=pe.base+s['rva']+s['size'] for s in code)
            off=pe.offset(start);data[off:off+end-start]=pe.read(start,end-start)
            restored.append(dict(start=start,end=end,kind=kind))
        for item in protected:restore(item['start'],item['end'],item['kind'])
        if not initial:
            for item in runtime['data']:restore(item['start'],item['end'],item['kind'])
            for item in runtime['functions']:restore(item['start'],item['end'],'verified-runtime')
            for item in app:
                restore(item['start'],item['end'],'application:'+item['name'])
                for a,b in item.get('text_data',[]):restore(a,b,'application-data:'+item['name'])
        # First entries are trapped even for restored functions; controller logs
        # then restores only that exact audited range. Unknown code stays INT3.
        entries=[] if initial else [*runtime['functions'],*app]
        for item in entries:data[pe.offset(item['start'])]=0xcc
        if not initial:
            for item in config.get('observations',[]):
                assert any(f['start']<=item['address']<f['end'] for f in app),'observation outside restored app code'
                data[pe.offset(item['address'])]=0xcc
    else:entries=[]
    destination.write_bytes(data)
    return dict(schema=1,target_sha256=sha(pe.data),masked_sha256=sha(data),
                initial=initial,unmasked=unmasked,candidates=list(candidates),
                loader_data=protected,restored=restored,
                entry_ranges=[dict(start=r['start'],end=r['end'],offset=pe.offset(r['start'])) for r in entries],
                runtime=runtime,configuration_sha256=sha((BINARY/'functions.json').read_bytes()) if configuration is None else sha(json.dumps(config,sort_keys=True).encode()))
