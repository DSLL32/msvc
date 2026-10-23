#!/usr/bin/env python3
"""Pure Mac loader/resource/code-copy evidence and actual native input flow."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import struct
import time
import native_clock

BASE=Path('cases/mac-loader-map')
def digest(data):return hashlib.sha256(data).hexdigest()
def image(data):
    assert len(data)==2097152 and data[:2]==b'MZ'
    pe=struct.unpack_from('<I',data,60)[0];assert data[pe:pe+4]==b'PE\0\0'
    assert struct.unpack_from('<H',data,pe+4)[0]==0x268
    assert struct.unpack_from('<H',data,pe+6)[0]==5
    optional=pe+24
    assert struct.unpack_from('<I',data,optional+16)[0]==4
    assert struct.unpack_from('<I',data,optional+28)[0]==0x10000
    sections={}
    for i in range(5):
        pos=optional+224+i*40;name=data[pos:pos+8].rstrip(b'\0').decode('ascii')
        virtual,rva,size,offset=struct.unpack_from('<4I',data,pos+8)
        assert virtual==size and rva==offset and offset+size<=len(data)
        sections[name]=data[offset:offset+size]
    assert set(sections)=={'.text','.text2','.jtable','.dfix','.resmap'}
    assert sections['.text'][:4]==bytes.fromhex('00000001')
    assert sections['.text'][4:24]==bytes.fromhex('4ead00324ead003a6100000a6100000e4e754e75')
    # Each distinct duplicated source appears in both CODE resources. The same
    # real linker jump slots are used by those copies and by the startup calls.
    one=bytes.fromhex('4eb9000000324e75');two=bytes.fromhex('4eb90000003a70034e75')
    assert sections['.text'][24:42]==one+two
    assert sections['.text2'][40:48]==bytes.fromhex('70014e7570024e75')
    assert sections['.text2'][48:66]==one+two
    assert len(sections['.jtable'])==48
    assert sections['.jtable'][-16:]==struct.pack('>HHIHHI',2,0xa9f0,40,2,0xa9f0,44)
    assert sections['.dfix']==bytes(4)
    return dict(section_sizes={n:len(p) for n,p in sections.items()},CODE_resources=2,distinct_copied_contributions=2,far_jump_offsets=[40,44])


def flow(receipt):
    creation=next(c for c in receipt['cases'] if c['case']=='mac-loader-map')
    dump=next(c for c in receipt['cases'] if c['case']=='mac-loader-map-dump')
    assert creation['status']==dump['status']=='pass'
    observed=dump['native_input_flow'];assert len(observed)==1
    observed=observed[0];assert observed['source_case']=='mac-loader-map' and observed['output_name']=='loader-map.exe'
    artifact=next(o for o in creation['outputs'] if o['name']=='loader-map.exe')
    assert observed['sha256']==artifact['sha256'] and observed['bytes']==artifact['bytes']==2097152
    assert observed['source']=='actual preceding native output; no frozen input replacement'


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--native-receipt',type=Path);ap.add_argument('--output',type=Path);args=ap.parse_args()
    start=time.perf_counter();data=(BASE/'clock/loader-map.exe').read_bytes();result=image(data)
    mapping=(BASE/'clock/loader-map.map').read_bytes()
    assert b'CODE0001' in mapping and b'CODE0002' in mapping
    assert mapping.index(b'_helper_function_one')<mapping.index(b'_helper_function_two')
    assert b'0000:fffffcf8       _copy_function_one' in mapping and b'0000:fffffcf8       _copy_function_two' in mapping
    meta=json.loads((BASE/'manifest.json').read_text());native_clock.validate(meta['native_clock_input'],'mac-loader-map')
    rejected=[]
    # These fields are independently structural, not covered solely by a hash.
    for name,offset in [('entry',128+24+16),('copy-one',656+24+4),('second-target',700+44),('jump-resource',776+32)]:
        bad=bytearray(data);bad[offset]^=1
        try:image(bytes(bad))
        except AssertionError:rejected.append(name)
        else:raise AssertionError('Changed Mac semantics accepted: '+name)
    if args.native_receipt:
        receipt=json.loads(args.native_receipt.read_text());flow(receipt)
        work=next((Path(receipt['directory'])/'mac-loader-map').rglob('loader-map.exe')).parent
        assert (work/'loader-map.exe').read_bytes()==data
        assert (work/'loader-map.map').read_bytes()==mapping
        image((work/'loader-map.exe').read_bytes())
        for label in ['missing-flow','wrong-flow','changed-input']:
            bad=copy.deepcopy(receipt);row=next(c for c in bad['cases'] if c['case']=='mac-loader-map-dump')
            if label=='missing-flow':row['native_input_flow']=[]
            elif label=='wrong-flow':row['native_input_flow'][0]['source_case']='mac-loader-map-dump'
            else:row['native_input_flow'][0]['sha256']='0'*64
            try:flow(bad)
            except AssertionError:rejected.append(label)
            else:raise AssertionError('Invalid native input flow accepted: '+label)
    result.update(status='pass',rejected=rejected,seconds=time.perf_counter()-start,clock='Full raw image/MAP exact under captured external time input',limits='No Macintosh execution; original MAP zero-section copied-source rows retained')
    if args.output:args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
if __name__=='__main__':main()
