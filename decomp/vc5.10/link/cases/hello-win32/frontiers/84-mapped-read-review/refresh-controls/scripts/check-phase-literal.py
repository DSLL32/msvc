#!/usr/bin/env python3
"""Check the exact six-byte Pass1 ownership correction and masked padding."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import struct
from binary import PE
from mask import construct,ROOT,BINARY
from observers import plan


def instrumented(config):
    raw,_=construct(config);data=bytearray(raw)
    rva=struct.unpack_from('<I',raw)[0]
    records=config['driver']+json.loads((BINARY/'runtime.json').read_text())
    for record in records:
        if record['name']in config['accepted']:
            data[8+record['start']-0x400000-rva]=0xcc
    for site,name in plan(config['accepted'],'hello-win32'):
        data[8+site-rva]=0xcc
    return bytes(data)


def verify(config,data,before):
    literal=[r for r in config['text_data']if r['start']<=0x407a50<r['end']]
    assert len(literal)==1 and(literal[0]['start'],literal[0]['end'])==(0x407a50,0x407a56)
    expected=instrumented(config)
    assert data==expected
    rva,size=struct.unpack_from('<2I',data)
    assert len(data)==size+8 and rva==0x1000
    offset=8+0x7a50-rva
    assert data[offset:offset+6]==b'Pass1\0'
    assert before[offset:offset+8]==b'\xcc'*8
    assert data[offset+6:offset+8]==b'\xcc'*2
    assert[i for i,(a,b)in enumerate(zip(before,data))if a!=b]==list(range(offset,offset+6))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--before',type=Path,required=True)
    parser.add_argument('--after',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    previous=json.loads((args.before/'functions.json').read_text())
    config=json.loads((args.after/'functions.json').read_text())
    assert {k:v for k,v in previous.items()if k!='text_data'}=={k:v for k,v in config.items()if k!='text_data'}
    assert previous['text_data']==config['text_data'][:-1]
    before=(args.before/'mask.bin').read_bytes();data=(args.after/'mask.bin').read_bytes()
    assert len(before)==len(data)
    verify(config,data,before)
    rejected=[]
    def negative(name,index,blob):
        try:verify(index,blob,before)
        except AssertionError:rejected.append(name)
        else:raise AssertionError('ownership control accepted: '+name)
    wrong=copy.deepcopy(config);wrong['text_data'].pop()
    negative('missing literal ownership',wrong,instrumented(wrong))
    wrong=copy.deepcopy(config);wrong['text_data'][-1]['end']+=2
    pe=PE(ROOT/config['target']['path'])
    wrong['text_data'][-1]['sha256']=hashlib.sha256(pe.read(0x407a50,8)).hexdigest()
    negative('validly hashed but overbroad padding ownership',wrong,instrumented(wrong))
    altered=bytearray(data);altered[8+0x7a50-0x1000]=0xcc
    negative('literal remains partially trapped',config,bytes(altered))
    altered=bytearray(data);altered[8+0x7a56-0x1000]=0
    negative('unclaimed padding byte unmasked',config,bytes(altered))
    args.report.write_text(json.dumps(dict(positive=True,negative_count=len(rejected),rejected=rejected,
        original_before=str(args.before),original_after=str(args.after),
        before_mask_sha256=hashlib.sha256(before).hexdigest(),after_mask_sha256=hashlib.sha256(data).hexdigest(),
        owned_bytes=6,padding_bytes_still_masked=2,whole_mask_changed_bytes=6),indent=2)+'\n')
    print('Exact Pass1 ownership passes; four literal/padding controls rejected.')


if __name__=='__main__':main()
