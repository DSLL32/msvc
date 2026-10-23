#!/usr/bin/env python3
"""Pure evidence checks for section edits, exact changed ranges and checksums."""
import importlib.util
import json
from pathlib import Path
import struct
import time

start=time.perf_counter()
spec=importlib.util.spec_from_file_location('section_artifacts',Path(__file__).with_name('artifacts-section-edit.py'))
parser=importlib.util.module_from_spec(spec);spec.loader.exec_module(parser)
base=Path('cases/edit-section-attributes')
old=(base/'inputs/original/edit.dll').read_bytes()
pe=struct.unpack_from('<I',old,60)[0]
optional=pe+24
section_start=optional+struct.unpack_from('<H',old,pe+20)[0]
expected=[
 ('edit-section-rename',[('.code',0x60000020),('.rdata',0x40300040),('.store',0xc0000040),('.reloc',0x40000040)]),
 ('edit-section-attributes',[('.code',0x68000020),('.rdata',0x40400040),('.store',0x50000040),('.reloc',0x42000040)]),
 ('edit-section-release',[('.text',0x60000020),('.rdata',0x40400040),('.data',0xc0000040),('.reloc',0x42000040)])]
changed=[]
for case,sections in expected:
    path=base/'reference'/case
    left,right=[(path/str(i)/'edit.dll').read_bytes() for i in (1,2)]
    assert left==right and len(left)==len(old)==4194304
    actual=[(left[section_start+i*40:section_start+i*40+8].rstrip(b'\0').decode(),struct.unpack_from('<I',left,section_start+i*40+36)[0]) for i in range(4)]
    assert actual==sections,(case,actual)
    allowed=set()
    for i in range(4):
        allowed.update(range(section_start+i*40,section_start+i*40+8))
        allowed.update(range(section_start+i*40+36,section_start+i*40+40))
    if case=='edit-section-release':
        allowed.update(range(optional+64,optional+68))
        assert parser.timestamp_fields(left,'section-release')==[]
    differences=[i for i,(a,b) in enumerate(zip(old,left)) if a!=b]
    assert differences and set(differences)<=allowed,(case,differences)
    assert old[pe+8:pe+12]==left[pe+8:pe+12], 'No COFF stamp change is ignored'
    changed.append(dict(case=case,changed_bytes=differences,sections=sections))
    old=left
assert b'LNK4039: section ".miss" does not exist' in (base/'reference/edit-section-attributes/1/stdout.bin').read_bytes()
rejected=[]
for label,offset in [('checksum',optional+64),('data',2048),('padding',len(old)-1)]:
    bad=bytearray(old);bad[offset]^=1
    try:parser.timestamp_fields(bytes(bad),'section-release')
    except AssertionError:rejected.append(label)
    else:raise AssertionError('Incorrect raw checksum accepted: '+label)
assert parser.normalized(old,[])==old
print(json.dumps(dict(status='pass',changed_ranges=changed,rejected=rejected,normalization='None',seconds=time.perf_counter()-start),indent=2))

# The final fixture receipt proves that later phases consumed actual native
# bytes, independently of the exact reference-byte oracle above.
import argparse
import copy
ap=argparse.ArgumentParser(description=__doc__)
ap.add_argument('--native-receipt',type=Path)
options=ap.parse_args()
if options.native_receipt:
    receipt=json.loads(options.native_receipt.read_text())
    inventory={'edit-section-attributes':'edit-section-rename',
               'edit-section-release':'edit-section-attributes',
               'dump-section-renamed':'edit-section-rename',
               'dump-section-attributes':'edit-section-attributes',
               'dump-section-released':'edit-section-release'}
    def validate_flow(value):
        records={r['case']:r for r in value['cases']}
        assert inventory and all(c in records for c in inventory)
        for case,source in inventory.items():
            current=records[case]
            assert current['status']==records[source]['status']=='pass'
            observations=current.get('native_input_flow',[])
            assert len(observations)==1
            observed=observations[0]
            assert observed['source_case']==source and observed['output_name']=='edit.dll'
            produced=next(r for r in records[source]['outputs'] if r['name']=='edit.dll')
            assert observed['sha256']==observed['normalized_sha256']==produced['sha256']
            assert observed['bytes']==produced['bytes']==4194304
            assert observed['source']=='actual preceding native output; no frozen input replacement'
    validate_flow(receipt)
    controls=[]
    for label in ['missing-observation','wrong-source','changed-input-bytes']:
        bad=copy.deepcopy(receipt)
        row=next(r for r in bad['cases'] if r['case']=='edit-section-attributes')
        if label=='missing-observation':row.pop('native_input_flow')
        elif label=='wrong-source':row['native_input_flow'][0]['source_case']='edit-section-release'
        else:row['native_input_flow'][0]['sha256']='0'*64
        try:validate_flow(bad)
        except AssertionError:controls.append(label)
        else:raise AssertionError('Invalid native input-flow proof accepted: '+label)
    print(json.dumps(dict(status='pass',actual_native_flows=len(inventory),rejected=controls),indent=2))
