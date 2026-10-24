"""Construct an ordinary C6-debug OMF input; these are test data, not target code.

Framing/constants: Microsoft's published cvdump/type6.cpp, typeinfo.h,
dumpsym6.cpp, symrec.h, debsym.h (analysis/primary-sources.json).
OMF framing/SEGDEF/LEDATA/FIXUPP: TIS OMF1.1, as in prior owned fixtures.
No original converter executes here. No compiler-output provenance is claimed.
"""
from pathlib import Path
import hashlib,json,struct
HERE=Path(__file__).resolve().parent;CASE=HERE.parent
w=lambda n:struct.pack('<H',n&65535)
d=lambda n:struct.pack('<I',n&0xffffffff)
def name(s):
    b=s.encode('ascii');assert len(b)<128;return bytes([len(b)])+b
def omf_index(n):
    assert 0<=n<0x8000
    return bytes([n]) if n<128 else bytes([0x80|(n>>8),n&255])
def rec(kind,body):
    raw=bytes([kind])+w(len(body)+1)+body
    return raw+bytes([-sum(raw)&255])
def idx(n):return b'\x83'+w(n)
def text(s):return b'\x82'+name(s)
def numeric(n):
    assert n>=0
    return bytes([n]) if n<128 else b'\x85'+w(n) if n<65536 else b'\x86'+d(n)
def typ(body):return b'\0'+w(len(body))+body
# C6 nonprimitive indices begin at0x200; no C7 signature DWORD is present.
types=[];definitions=[]
def add(label,body):
    n=0x200+len(types);types.append(typ(body));definitions.append(dict(index=hex(n),name=label,body=body.hex()));return n
assert add('Value field types',b'\x7f'+idx(0x82)+idx(0x89))==0x200
add('Value field names',b'\x7f'+text('integer')+numeric(0)+text('real')+numeric(0))
add('Value union',b'\x79'+numeric(64)+numeric(2)+idx(0x200)+idx(0x201)+text('Value')+b'\x69')
add('Node field types',b'\x7f'+idx(0x202)+idx(0x209)+idx(0x206))
add('Node field names',b'\x7f'+text('value')+numeric(0)+text('flags')+numeric(64)+text('next')+numeric(96))
add('Node structure',b'\x79'+numeric(128)+numeric(3)+idx(0x203)+idx(0x204)+text('Node')+b'\x69')
add('Node near32 pointer',b'\x7a\x5a'+idx(0x205))
add('Four Nodes',b'\x78'+numeric(512)+idx(0x205)+idx(0x82)+text('Nodes4'))
add('Four longs',b'\x78'+numeric(128)+idx(0x82)+idx(0x82)+text('Longs4'))
add('Three-bit unsigned field',b'\x5c'+numeric(3)+b'\x7c\x01')
add('Node pointer procedure',b'\x75\0'+idx(0x206)+b'\x5b'+numeric(1)+idx(0x20b))
add('Identity arguments',b'\x7f'+idx(0x206))
add('Long procedure',b'\x75\0'+idx(0x82)+b'\x5b'+numeric(0)+idx(0x20d))
add('No arguments',b'\x7f')
add('Long procedure pointer',b'\x7a\x5a'+idx(0x20c))
add('Two callback pointers',b'\x78'+numeric(64)+idx(0x20e)+idx(0x82)+text('Callbacks2'))
add('Two Node pointers',b'\x78'+numeric(64)+idx(0x206)+idx(0x82)+text('NodePtrs2'))
assert len(types)==17
# Legal framed i386 routines described by legacy.asm; never executed by this case.
identity=bytes.fromhex('55 8b ec 8b 45 08 90 c9 c3')
value=bytes.fromhex('55 8b ec 83 ec 04 c7 45 fc 2a 00 00 00 8b 45 fc c9 c3')
code=identity+b'\x90'*7+value;assert len(code)==34
# Node{Value8,unsigned storage4,next4}; flags:3 starts at bit1.
data=b''.join(d(n)+d(0)+d((i+1)<<1)+d((i+1)*16 if i<3 else 0) for i,n in enumerate([19,23,31,37]))
data+=d(0)+d(16)+d(16)+b''.join(d(n) for n in [2,4,6,8])+d(0)+d(32)
assert len(data)==100
symbols=bytearray();symbol_rows=[];debug_fixups=[]
def symbol(kind,body,label,code_offset=False,data_offset=False):
    start=len(symbols);payload=bytes([kind])+body;assert len(payload)<=128
    symbols.extend(bytes([len(payload)])+payload);symbol_rows.append(dict(offset=start,type=hex(kind),label=label,bytes=len(payload)+1))
    if code_offset:debug_fixups.append((start+2,9,1)) # offset32, frame/target segment1
    if data_offset:
        debug_fixups.extend([(start+2,9,2),(start+6,2,2)]) # offset32 + segment16
# Old32-bit symbols use bit0x80, not C7 length/leaf WORD framing.
symbol(0x81,d(0)+w(0x20a)+w(9)+w(6)+w(7)+w(0)+b'\0'+name('legacy_identity'),'identity procedure',code_offset=True)
symbol(0x84,d(8)+w(0x206)+name('head'),'stack formal head')
symbol(0x8d,w(0x206)+b'\x10'+name('head'),'register formal head in EAX')
symbol(0x82,b'','end identity')
symbol(0x81,d(16)+w(0x20c)+w(18)+w(6)+w(16)+w(0)+b'\0'+name('legacy_value'),'value procedure',code_offset=True)
symbol(0x80,d(22)+w(12)+name(''),'value lexical block',code_offset=True)
symbol(0x84,d(-4)+w(0x82)+name('result'),'stack local result')
symbol(0x8b,d(32)+b'\0'+name('epilogue'),'value epilogue label',code_offset=True)
symbol(0x8d,w(0x82)+b'\x10'+name('result'),'value result in EAX')
symbol(0x82,b'','end block')
symbol(0x82,b'','end value')
publics=[('_legacy_identity',1,0,0x20a),('_legacy_value',1,16,0x20c),('_legacy_nodes',2,0,0x207),('_legacy_head',2,64,0x206),('_legacy_calls',2,68,0x20f),('_legacy_numbers',2,76,0x208),('_legacy_refs',2,92,0x210)]
for spelling,section,offset,type_index in publics[2:]:symbol(0x85,d(offset)+w(0)+w(type_index)+name(spelling[1:]),spelling,data_offset=True)
# No symbol terminator is appended to the valid record stream. The original
# collector calloc(0x20000,1) supplies its own defined zero guard after the data.
def fix(offset,location,target):
    assert 0<=offset<1024 and location in [2,9]
    return bytes([0xc0|(location<<2)|(offset>>8),offset&255,0x54,target])
names=['','_TEXT','CODE','_DATA','DATA','$$SYMBOLS','DEBSYM','$$TYPES','DEBTYP','FLAT']
records=[(0x80,name('legacy.asm')),(0x96,b''.join(map(name,names)))]
for size,ni,ci,attr in [(len(code),2,3,0xa9),(len(data),4,5,0xa9),(len(symbols),6,7,0x21),(sum(map(len,types)),8,9,0x21)]:records.append((0x99,bytes([attr])+d(size)+bytes([ni,ci,1])))
records.append((0x9a,b'\x0a\xff\x01\xff\x02'))
for spelling,section,offset,type_index in publics:records.append((0x91,bytes([1,section])+name(spelling)+d(offset)+omf_index(type_index)))
records.extend([(0xa1,b'\x01'+d(0)+code),(0xa1,b'\x02'+d(0)+data)])
data_fixups=[(12,2),(28,2),(44,2),(64,2),(68,1),(72,1),(92,2),(96,2)]
records.append((0x9d,b''.join(fix(off,9,target) for off,target in data_fixups)))
records.append((0xa1,b'\x03'+d(0)+symbols));records.append((0x9d,b''.join(fix(*item) for item in debug_fixups)))
records.append((0xa1,b'\x04'+d(0)+b''.join(types)))
records.append((0x95,b'\x01\x01'+b''.join(w(line)+d(off) for line,off in [(5,0),(7,1),(8,3),(9,6),(14,16),(17,19),(18,22),(19,29),(21,32)])))
records.append((0x8b,b'\0'))
obj=b''.join(rec(k,v) for k,v in records);(CASE/'inputs/legacy.obj').write_bytes(obj)
# Independent framing/checksum sweep; this validates input structure, not LINK acceptance.
at=0;count=0
while at<len(obj):
    n=struct.unpack_from('<H',obj,at+1)[0];raw=obj[at:at+3+n];assert len(raw)==3+n and sum(raw)%256==0;at+=3+n;count+=1
assert at==len(obj) and count==len(records)
layout=dict(status='Generated input only; untouched-original preflight still required',bytes=len(obj),sha256=hashlib.sha256(obj).hexdigest(),OMF_records=count,checksums_valid=True,types=definitions,type_bytes=sum(map(len,types)),symbols=symbol_rows,symbol_bytes=len(symbols),debug_fixups=[dict(offset=o,location_kind=k,target_segment=t) for o,k,t in debug_fixups],publics=publics,code_bytes=len(code),data_bytes=len(data),data_fixups=data_fixups,format='C6 linkageBYTE/lengthWORD/leafBYTE types, lengthBYTE/typeBYTE386 symbols; no C7 signature',provenance='Fixture generator and human-readable assembly; never claimed compiler-emitted C6 debug. Original C2 candidate retained as incompatible separate evidence.')
(CASE/'inputs/layout.json').write_text(json.dumps(layout,indent=2)+'\n');print(CASE/'inputs/legacy.obj',len(obj),'bytes',count,'records')
