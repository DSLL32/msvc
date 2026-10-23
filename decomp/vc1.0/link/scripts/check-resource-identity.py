#!/usr/bin/env python3
"""Pure named-resource tree, payload and complete ABI-observer negative checks."""
import argparse
import copy
import json
from pathlib import Path
import struct
import time
from resource_abi import compare

BASE=Path('cases/resource-identity')


def image(data):
    u16=lambda p:struct.unpack_from('<H',data,p)[0]
    u32=lambda p:struct.unpack_from('<I',data,p)[0]
    assert len(data)==2097152 and data[:2]==b'MZ'
    pe=u32(60);assert data[pe:pe+4]==b'PE\0\0' and u16(pe+4)==0x14c
    optional=pe+24;assert u16(optional)==0x10b
    table=optional+u16(pe+20)
    def raw(rva):
        for i in range(u16(pe+6)):
            p=table+40*i
            if u32(p+12)<=rva<u32(p+12)+u32(p+16):return u32(p+20)+rva-u32(p+12)
        raise AssertionError('RVA outside raw resource sections')
    rva,size=struct.unpack_from('<II',data,optional+112);root=raw(rva)
    assert size>=16
    def entries(relative):
        assert 0<=relative<size and relative+16<=size
        p=root+relative;count=u16(p+12)+u16(p+14);assert relative+16+8*count<=size
        return [struct.unpack_from('<II',data,p+16+8*i) for i in range(count)]
    def name(value):
        assert value&0x80000000
        p=root+(value&0x7fffffff);n=u16(p)
        return data[p+2:p+2+n*2].decode('utf-16le')
    types=entries(0);assert len(types)==1 and name(types[0][0])=='SAMPLE'
    assert types[0][1]&0x80000000
    names=entries(types[0][1]&0x7fffffff);assert len(names)==1 and name(names[0][0])=='Caf\xe9'
    assert names[0][1]&0x80000000
    leaves=entries(names[0][1]&0x7fffffff);assert [n for n,v in leaves]==[0x409,0x411]
    payloads=[]
    for language,relative in leaves:
        assert not relative&0x80000000
        payload,length,codepage,reserved=struct.unpack_from('<4I',data,root+relative)
        assert codepage==reserved==0
        p=raw(payload);payloads.append(data[p:p+length])
    assert payloads==[b'English identity payload\0',b'Japanese identity payload\0']
    return dict(type='SAMPLE',name='Caf\xe9',languages=[0x409,0x411],payload_bytes=[len(p) for p in payloads])


def abi_controls():
    rejected=[]
    for variant in ['ansi-typed','byte-typed','fatal-typed','unlink-typed']:
        receipt=BASE/'acceptance'/variant/'receipt.json'
        if not receipt.exists():continue
        row=json.loads(receipt.read_text())['resource_abi_state'];original=row['original'];native=row['native']
        compare([original],native)
        for label in ['missing-original','missing-formal','changed-formal','wrong-caller']:
            event=copy.deepcopy(original);actual=copy.deepcopy(native);events=[event]
            if label=='missing-original':events=[]
            elif label=='missing-formal':actual['formals'].pop(next(iter(actual['formals'])))
            elif label=='changed-formal':
                field=next(iter(actual['formals']));value=actual['formals'][field]
                actual['formals'][field]=value+1 if isinstance(value,int) else value+'changed'
            else:actual['caller_name']='unrelated_caller'
            try:compare(events,actual)
            except (AssertionError,KeyError):rejected.append(variant+':'+label)
            else:raise AssertionError('Invalid ABI input observation accepted: '+variant+':'+label)
    return rejected


def jump_controls():
    receipt=json.loads((BASE/'longjmp/native-complete/receipt.json').read_text())
    def validate(value):
        original=value['original'];native=value['native']
        assert original['registration_is_current']==1 and original['saved_scope']==original['registration_scope']==0
        assert original['value']==native['entry']['value']==native['resume']['value']==0xffffffff
        assert native['entry']['buffer_is_resource'] and native['entry']['buffer_bytes']==32 and native['entry']['registered_contexts']==[0]
        assert native['entry']['caller']=='insert_resource' and native['resume']['function']=='convert_resource_input'
        assert native['resume']['signed_value']==-1 and native['entry']['errno']==native['resume']['errno']==2
        assert native['exit']==26 and not native.get('late_error_readers',[])
        assert value['ambient_last_error']['live_result_errno_doserrno_exact']
    validate(receipt);rejected=[]
    for label in ['coerced-plus-one','unknown-context','wrong-buffer','wrong-continuation','late-error-reader','scope-needs-unwind']:
        bad=copy.deepcopy(receipt)
        if label=='coerced-plus-one':bad['native']['resume']['value']=1;bad['native']['resume']['signed_value']=1
        elif label=='unknown-context':bad['native']['entry']['registered_contexts']=[]
        elif label=='wrong-buffer':bad['native']['entry']['buffer_is_resource']=False
        elif label=='wrong-continuation':bad['native']['resume']['function']='unrelated_function'
        elif label=='late-error-reader':bad['native']['late_error_readers']=['exit_callback']
        else:bad['original']['registration_scope']=1
        try:validate(bad)
        except AssertionError:rejected.append('resource-jump:'+label)
        else:raise AssertionError('Invalid jump/def-use observation accepted: '+label)
    return rejected


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--image',type=Path);ap.add_argument('--output',type=Path);args=ap.parse_args()
    start=time.perf_counter();rejected=abi_controls()+jump_controls();result={}
    if args.image:
        data=args.image.read_bytes();result=image(data)
        for text in [b'S\0A\0M\0P\0L\0E\0',b'C\0a\0f\0\xe9\0',b'Japanese identity payload']:
            offset=data.index(text);changed=bytearray(data);changed[offset]^=1
            try:image(bytes(changed))
            except AssertionError:rejected.append('resource-content:'+text.decode('latin1'))
            else:raise AssertionError('Mutated resource content accepted')
    result.update(status='pass',rejected=rejected,seconds=time.perf_counter()-start)
    if args.output:args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
if __name__=='__main__':main()
