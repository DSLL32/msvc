"""Pure validation of the one native host image-memory correspondence."""
import hashlib,json,struct
from pathlib import Path
from binary import PE
BINARY=Path(__file__).resolve().parents[1]
def validate(proof):
    config=json.loads((BINARY/'functions.json').read_text())
    image=PE(BINARY/'../../../toolchains/1.0-nt/bin/LINK.EXE')
    assert hashlib.sha256(image.data).hexdigest()==proof['original_sha256']==config['target']['sha256'], 'Original identity'
    assert proof['image_base']==image.base, 'Original image base'
    assert proof['tls_directory']==list(struct.unpack_from('<II',image.data,image.optional+96+9*8))==[0,0], 'Original has no TLS directory'
    assert proof['original_rva']+image.base==proof['original_va'], 'VA/RVA coordinates'
    definitions=json.loads((BINARY/'data-symbols.json').read_text())
    definition=next(r for r in definitions if r['name']=='editor_workspace')
    assert definition==proof['global_definition'], 'Contextual global owner/extent'
    assert definition['address']==proof['original_va'] and definition['size']==proof['bytes']==36
    sections=[s for s in image.sections if image.base+s['rva']<=proof['original_va'] and
              proof['original_va']+proof['bytes']<=image.base+s['rva']+s['virtual_size']]
    assert len(sections)==1 and sections[0]==proof['section'] and sections[0]['flags']&0x80000000, 'Owned writable PE section'
    source=proof['host_source']
    assert source['path']=='scripts/native-module.c' and hashlib.sha256((BINARY/source['path']).read_bytes()).hexdigest()==source['sha256'], 'Host proof source'
    rows=[json.loads(s) for s in (BINARY/proof['evidence']).read_text().splitlines()]
    rows=[r for r in rows if r['event']=='editor-api' and r['name']=='FlushViewOfFile']
    assert len(rows)==2 and rows[0]['before']==1 and rows[1]['before']==0
    assert rows[0]['args']==rows[1]['args']==[proof['original_va'],0,0,0]
    assert rows[1]['result']==1 and rows[1]['cleanup']==8
    assert all(r['errno']==r['last_error']==0 for r in rows)
    return dict(status='pass',original_va=proof['original_va'],original_rva=proof['original_rva'],bytes=proof['bytes'],section=sections[0]['name'])
