#!/usr/bin/env python3
"""Affected MRE native/state controls; restore each retained execution verbatim."""
import argparse,copy,json,time
from pathlib import Path
from mre_state import verify

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--original',type=Path,required=True);parser.add_argument('--native',type=Path,required=True)
parser.add_argument('--receipt',type=Path,required=True);args=parser.parse_args();directory=args.native
started=time.perf_counter();passed=[]
verify(args.original,directory)


def negative(name,filename,change):
    path=directory/filename;before=path.read_bytes()
    try:
        value=json.loads(before);change(value)
        path.write_text(json.dumps(value,indent=2)+'\n')
        try:verify(args.original,directory)
        except(AssertionError,ValueError,KeyError,IndexError,UnicodeError):passed.append(name)
        else:raise AssertionError('accepted '+name)
    finally:path.write_bytes(before)


def byte(record,address):
    # Change every overlapping observation consistently: tests the actual
    # semantic comparison, rather than merely contradictory duplicate ranges.
    changed=False
    for span in record['ranges']:
        raw=bytearray.fromhex(span['bytes']);offset=address-span['address']
        if 0<=offset<len(raw):raw[offset]^=1;span['bytes']=raw.hex();changed=True
    assert changed


def scalar(record,name):byte(record,record['symbols'][name])

for name in('errno','recon_doserrno','last_error','recon_mb_codepage','recon_lc_codepage','recon_crt_environment_updates','mre_call_count'):
    negative('actual post-Open '+name,'native-mre-1-after-gdb.json',lambda r,n=name:scalar(r,n))
for name in('ebx','esi','edi','ebp'):
    negative('normal-return '+name,'native-mre-2-after-gdb.json',lambda r,n=name:r['general'].__setitem__(n,r['general'][n]^1))
negative('stdcall cleanup','native-mre-1-after-gdb.json',lambda r:r['general'].__setitem__('esp',r['general']['esp']+4))
negative('wrong call return','native-mre-1-after-gdb.json',lambda r:r['general'].__setitem__('eip',r['general']['eip']+1))
for data,offset in(('recon_iob',12),('recon_osfile',1),('recon_osfile',3),('recon_osfhnd',12),('standard_handles',4),('option_buffer',8),('recon_mbctype',32)):
    negative('addressed '+data+'+'+str(offset),'native-mre-2-after-gdb.json',lambda r,d=data,o=offset:byte(r,r['data_addresses'][d]+o))
for name in('position','inode','flags','size'):
    negative('genuine fd3 '+name,'native-mre-3-after-gdb.json',lambda r,n=name:r['descriptor3'].__setitem__(n,r['descriptor3'][n]^1))
for name in('fctrl','fstat','ftag','fioff','fooff','fop','fiseg','foseg','st0','st7'):
    def fp(record,n=name):
        value=bytearray.fromhex(record['x87'][n]);value[0]^=1;record['x87'][n]=value.hex()
    negative('architectural FP '+name,'native-mre-1-before-gdb.json',fp)
negative('frontier pending words','native-mre-frontier-gdb.json',lambda r:byte(r,r['arguments']['pending']))
negative('frontier state formal slot','native-mre-frontier-gdb.json',lambda r:byte(r,r['arguments']['slot']))
negative('frontier observed caller','native-mre-frontier-gdb.json',lambda r:r['arguments'].__setitem__('caller','next_jobs'))
negative('missing MRE request','mre-native.json',lambda r:r['requests'].pop(3))
negative('extra MRE request','mre-native.json',lambda r:r['requests'].append(copy.deepcopy(r['requests'][-1])))
negative('cross-process MRE response','mre-native.json',lambda r:r['requests'][3].__setitem__('pid',r['pid']+1))
negative('wrong response identity','mre-native.json',lambda r:r.__setitem__('response_sha256','0'*64))
negative('main profile','native-entry-errors.json',lambda r:r.__setitem__('last_error',2))
negative('mktemp missing call','native-mktemp-gdb.json',lambda r:r['calls'].clear())
negative('mktemp result alias','native-mktemp-gdb.json',lambda r:r['calls'][0].__setitem__('result_alias',False))
for filename in('test.obj','case.idb','mre-binding.bin'):
    path=directory/filename;before=path.read_bytes()
    try:
        raw=bytearray(before);raw[-1]^=1;path.write_bytes(raw)
        try:verify(args.original,directory)
        except(AssertionError,ValueError,KeyError):passed.append('actual complete '+filename)
        else:raise AssertionError('accepted altered '+filename)
    finally:path.write_bytes(before)
# Exercise the live semantic callback validator independently of the GDB/raw
# comparison so altered real invocation inputs cannot rely on duplicate fields.
from mre_native import Provider,Pointers,decode
from mre_object_packet import decode as original_packet
from case_inputs import load
packet=original_packet(args.original);wire=json.loads((directory/'mre-native.json').read_bytes())
requests=[None]+[decode(bytes.fromhex(r['payload_hex']))for r in wire['requests'][1:]]
def semantic(kind,field,value):
    provider=Provider.__new__(Provider);provider.original=args.original.resolve();provider.directory=directory.resolve()
    provider.packet=packet;provider.pointer=Pointers();provider.inputs=load('mre-object')['inputs']
    provider.config_path=directory/'mre-binding.bin';provider.config=provider.config_path.read_bytes()
    provider.idb=(directory/'case.idb').read_bytes();provider.os_block=(args.original/'observer/mre-1-before-os-environment.bin').read_bytes()
    provider.native_before=provider.native_after=None
    for index in range(1,kind):provider.validate(index,copy.deepcopy(requests[index]),live=False)
    changed=copy.deepcopy(requests[kind]);changed[field]=value
    try:provider.validate(kind,changed,live=False)
    except(AssertionError,ValueError,KeyError):passed.append('actual callback semantic '+str(kind)+' '+field)
    else:raise AssertionError('accepted semantic input '+field)
semantic(1,'reproducible_arg',1);semantic(1,'write_arg',0)
semantic(1,'path',{'address':requests[1]['path']['address'],'text':'other.idb'})
semantic(1,'owner_out',0);semantic(1,'error_out',requests[1]['owner_out'])
semantic(1,'errno',2);semantic(1,'last_error',0)
semantic(3,'engine',requests[3]['engine']+4);semantic(3,'driver_out',0)
semantic(5,'mre_driver',requests[5]['mre_driver']-4)
verify(args.original,directory)
result={'passed':True,'count':len(passed),'controls':passed,'seconds':time.perf_counter()-started,
    'scope':'Affected initial MRE native request, real ABI/state/file/frontier fields; original raw untouched'}
args.receipt.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
