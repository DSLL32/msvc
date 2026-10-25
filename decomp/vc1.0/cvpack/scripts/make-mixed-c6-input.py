"""Format-authored NB02: target-supported 12-byte directory and C6 records.
No producer output or relabeled NB09 bytes are used. All integers are LE.
The recipe is derived from the original readers/converter dispatch and callers.
"""
from pathlib import Path
import hashlib,json,struct,sys
HERE=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path.cwd()
HERE.mkdir(parents=True,exist_ok=True)
WITHOUT_BARRAY = '--without-barray' in sys.argv[2:]
FILENAME = 'mixed-c6-no-barray.dbg' if WITHOUT_BARRAY else 'mixed-c6.dbg'
H=lambda n:struct.pack('<H',n)
I=lambda n:struct.pack('<I',n)
def counted(s):
 b=s.encode('ascii');assert len(b)<128;return bytes([len(b)])+b
def typ(leaf,payload=b'',marker=1):
 return bytes([marker])+H(1+len(payload))+bytes([leaf])+payload
def sym(kind,payload=b''):
 assert len(payload)+1<256
 return bytes([len(payload)+1,kind])+payload
# Every type is named by an in-scope register, so the real compactor reaches it.
TYPES=[
 ('procedure',typ(0x75,b'\0\x83'+H(0x85)+b'\x63\0\x83'+H(0xffff))),
 ('nil',typ(0x80)),
 ('not-translated',typ(0x8e)),
 ('label',typ(0x72,b'\0\x73')),
 ('bitfield',typ(0x5c,b'\x08\x7d\0')),
 ('scalar-array',typ(0x8d,b'\0\x40')),
 ('based-array',typ(0x8c,b'\x83'+H(0x85))),
 ('based-pointer',typ(0x91,b'\x83'+H(0x85)+b'\x81')),
 ('modifier-a6',typ(0xa6,H(0x85)+b'\0',0)),
 ('opaque-a7',typ(0xa7,b'\0',0)),
 ('pointer',typ(0x7a,b'\x74\x83'+H(0x207))),
 ('array',typ(0x78,b'\x40\x83'+H(0x85)+b'\x83'+H(0x85)+b'\x82'+counted('bytes'))),
]
if WITHOUT_BARRAY:
 TYPES=[(name,value) for name,value in TYPES if name!='based-array']
 indices={name:0x200+i for i,(name,_) in enumerate(TYPES)}
 TYPES=[(name,typ(0x7a,b'\x74\x83'+H(indices['based-pointer']))) if name=='pointer' else (name,value) for name,value in TYPES]
indices={name:0x200+i for i,(name,_) in enumerate(TYPES)}
type_bytes=b''.join(v for _,v in TYPES)

def symbols(wide,seg):
 flag=0x80 if wide else 0
 off=I if wide else H
 rows=[]
 rows.append(sym(0x11,H(seg)))
 rows.append(sym(1|flag,off(0)+H(0x200)+H(0x100)+H(0)+H(0x100)+H(0)+b'\0'+counted('worker')))
 for i,(name,_) in enumerate(TYPES[1:],1):
  rows.append(sym(0x0d,H(0x200+i)+b'\x11'+counted(name)))
 rows.append(sym(4|flag,off(4)+H(indices['based-pointer'])+counted('base')))
 rows.append(sym(5|flag,off(0x20)+H(seg)+H(indices['based-pointer'] if WITHOUT_BARRAY else indices['based-array'])+counted('window')))
 rows.append(sym(0x0b|flag,off(0x30)+b'\0'+counted('again')))
 rows.append(sym(0|flag,off(0x40)+H(0x20)+counted('scope')))
 rows.append(sym(4|flag,off(8)+H(0x204)+counted('bits')))
 rows.append(sym(2))
 # Original narrow reg-relative converter consumes the fixed 9-byte form.
 # The wide form is deliberately excluded: its RET handler never advances input.
 if not wide: rows.append(sym(0x16,H(0x10)+b'\x11'+H(0x205)+counted('r')))
 for name,value in [('small',b'\x01\x07'),('large',b'\x85'+H(65534)),('wide',b'\x86'+I(0x12345678))]:
  rows.append(sym(0x0e,H(0x86 if name=='wide' else 0x85)+value+counted(name)))
 rows.append(sym(0x12,H(indices['based-pointer'])+counted('Based')))
 rows.append(sym(0x10,b'\0\0'))
 rows.append(sym(2))
 return b''.join(rows)

def module(number,seg):
 name=counted('mixed%d.obj'%number)
 # 16-bit NB02 module container; per-symbol width is independently encoded.
 return H(seg)+H(0)+H(0x400)+H(0)+H(0)+b'\x02\0'+name+H(seg+1)+H(0)+H(0x400)

def lines(number,seg):
 result=[]
 for name,segment,pairs in [('mixed%d.c'%number,seg,[(10,0),(20,0x10),(30,0x20)]),('mixed%d.c'%number,seg+1,[(40,4),(50,8)]),('shared.h',seg,[(60,0x40),(70,0x44)])]:
  result.append(counted(name)+H(segment)+H(len(pairs))+b''.join(H(line)+H(offset) for line,offset in pairs))
 return b''.join(result)

payload=bytearray(b'NB02'+I(0));directory=[];inventory=[]
for number,seg,wide in [(1,1,False),(2,3,True)]:
 for kind,data in [(0x101,module(number,seg)),(0x103,type_bytes),(0x104,symbols(wide,seg)),(0x109,lines(number,seg))]:
  directory.append((kind,number,len(payload),len(data)));payload+=data
  inventory.append({'kind':hex(kind),'module':number,'offset':directory[-1][2],'size':len(data)})
directory_offset=len(payload);struct.pack_into('<I',payload,4,directory_offset)
payload+=H(len(directory))+b''.join(struct.pack('<HHII',*v) for v in directory)
payload+=b'NB02'+I(len(payload)+8)
# Structural checks independently traverse every extent, counted record and ref.
assert payload[:4]==payload[-8:-4]==b'NB02'
assert struct.unpack_from('<I',payload,len(payload)-4)[0]==len(payload)
assert directory_offset+2+12*len(directory)+8==len(payload)
for kind,number,offset,size in directory:
 assert 8<=offset and offset+size<=directory_offset
 data=payload[offset:offset+size]
 if kind==0x103:
  cursor=0
  while cursor<len(data):
   length=struct.unpack_from('<H',data,cursor+1)[0]
   assert cursor+length+3<=len(data)
   assert data[cursor+3] in (0x75,0x80,0x8e,0x72,0x5c,0x8d,0x8c,0x91,0xa6,0xa7,0x7a,0x78)
   cursor+=length+3
  assert cursor==len(data)
 elif kind==0x104:
  cursor=0;depth=0
  while cursor<len(data):
   length,kind=data[cursor:cursor+2];assert cursor+length+1<=len(data)
   assert kind!=0x96
   base=kind&0x7f;wide=bool(kind&0x80);width=4 if wide else 2
   type_offset={1:2+width,4:2+width,5:4+width,0x0d:2,0x0e:2,0x12:2,0x16:5}.get(base)
   if type_offset is not None:
    referenced=struct.unpack_from('<H',data,cursor+type_offset)[0]
    assert referenced<0x200 or 0x200<=referenced<0x200+len(TYPES)
   if base==0x0e:
    value=cursor+4;tag=data[value]
    value+=1+({0x85:2,0x86:4}[tag] if tag>=0x80 else tag)
    assert value+1+data[value]==cursor+length+1
   if kind&0x7f in (0,1):depth+=1
   if kind&0x7f==2:depth-=1
   assert depth>=0;cursor+=length+1
  assert cursor==len(data) and depth==0
 elif kind==0x109:
  cursor=0
  while cursor<len(data):
   length=data[cursor];cursor+=length+1
   segment,count=struct.unpack_from('<HH',data,cursor);cursor+=4
   assert segment in ((1,2) if number==1 else (3,4))
   for i in range(count):
    line,offset=struct.unpack_from('<HH',data,cursor);assert 0<=offset<0x400 and line>0;cursor+=4
  assert cursor==len(data)
(HERE/FILENAME).write_bytes(payload)
recipe={'authored_input':True,'historical_producer_output':False,'variant':'without-barray-v2' if WITHOUT_BARRAY else 'original-v1','excluded':'BARRAY unwritten control local; retained as pending0013 frontier' if WITHOUT_BARRAY else None,'source':'generate_input.py; original NB02/C6 reader and typed converter layouts, not a producer binary','sha256':hashlib.sha256(payload).hexdigest(),'size':len(payload),'directory_offset':directory_offset,'directory_entries':inventory,'type_records':[{'index':hex(0x200+i),'name':n,'bytes':v.hex()} for i,(n,v) in enumerate(TYPES)],'symbols':'Two modules: complete procedure/block/end scopes, register references for every old type, 16/32-bit bprel/data/labels, narrow one-character reg-relative, constants7/65534/0x12345678, UDT alias, skip; wide reg-relative excluded','lines':'Two filename groups and two module contributions each; three segment-line blocks per module with all offsets in actual contributions; drives real range-sort comparator','environment':{'_MSC_IDE_FLAGS':None},'environment_reason':'Normal CLI output with feedback mode absent; no PATH or host/Wine sanitization settings copied','expected_new_families':['old leaf80/8e/72/5c/8d/8c/91/a6/a7','32bit symbol sizing/conversion for proc/block/bprel/data/label','16bit register-relative,constant,UDT12,skip','line-range comparator'],'validation':'Original CLI accepted this exact input without warnings on2026-10-25; source recipe remains authored, not historical producer output.'}
(HERE/'input-recipe.json').write_text(json.dumps(recipe,indent=2)+'\n')
print(recipe['size'],recipe['sha256'],len(directory),'entries',len(TYPES),'types/module')
