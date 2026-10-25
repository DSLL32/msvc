"""Authored NB02/C6 based-pointer owners and genuine embedded symbol families.
No historical producer-output claim; no BARRAY or nonadvancing callback.
"""
from pathlib import Path
import struct,hashlib,json,sys
HERE=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path.cwd()
HERE.mkdir(parents=True,exist_ok=True)
H=lambda v:struct.pack('<H',v)
I=lambda v:struct.pack('<I',v)
def counted(s):
 v=s.encode('ascii');assert len(v)<128;return bytes([len(v)])+v
def typ(leaf,payload):return b'\x01'+H(1+len(payload))+bytes([leaf])+payload
def sym(kind,payload=b''):return bytes([len(payload)+1,kind])+payload
payload=bytearray(b'NB02'+I(0));directory=[];provenance=[]
for number,seg,wide in [(1,1,False),(2,3,True)]:
 flag=0x80 if wide else 0;off=I if wide else H
 module=H(seg)+H(0)+H(0x400)+H(0)+H(0)+b'\x02\0'+counted('bases%d.obj'%number)+H(seg+1)+H(0)+H(0x400)
 rows=[sym(0x11,H(seg)),sym(1|flag,off(0)+H(0x200)+H(0x100)+H(0)+H(0x100)+H(0)+b'\0'+counted('based_worker'))]
 owners={}
 for name,record in [('bprel',sym(4|flag,off(4)+H(0x85)+counted('frame_base'))),('data',sym(5|flag,off(0x20)+H(seg)+H(0x85)+counted('data_base'))),('register',sym(0x0d,H(0x85)+b'\x11'+counted('register_base')))]:
  owners[name]=sum(map(len,rows));rows.append(record)
 types=[('procedure',typ(0x75,b'\0\x83'+H(0x85)+b'\x63\0\x83'+H(0xffff)))]
 types += [('self',typ(0x91,b'\x83'+H(0x85)+b'\x81')),('type',typ(0x91,b'\x83'+H(0x85)+b'\x83')),('segment',typ(0x91,b'\x83'+H(0x85)+b'\x92'+H(seg)))]
 refs=[]
 for base_kind in [0x93,0x94,0x97,0x98]:
  for owner,offset in owners.items():
   name='base%02x_%s'%(base_kind,owner);types.append((name,typ(0x91,b'\x83'+H(0x85)+bytes([base_kind])+H(offset))));refs.append({'name':name,'type':hex(0x200+len(types)-1),'kind':hex(base_kind),'symbol_offset':offset,'owner':owner})
 for i,(name,record) in enumerate(types[1:],1):rows.append(sym(0x0d,H(0x200+i)+b'\x11'+counted(name)))
 rows.append(sym(2));symbols=b''.join(rows);type_data=b''.join(v for _,v in types)
 # Independently traverse full local-symbol stream and prove each pointer owner.
 cursor=0;boundaries={};depth=0
 while cursor<len(symbols):
  length,kind=symbols[cursor:cursor+2];assert cursor+length+1<=len(symbols);boundaries[cursor]=(kind,symbols[cursor:cursor+length+1]);cursor+=length+1
  if kind&0x7f in (0,1):depth+=1
  if kind&0x7f==2:depth-=1
  assert depth>=0
 assert cursor==len(symbols) and depth==0
 for ref in refs:
  kind,record=boundaries[ref['symbol_offset']];assert kind&0x7f in [4,5,0x0d]
  assert (kind&0x80)!=0 if (wide and ref['owner']!='register') else (kind&0x80)==0
  type_at={4:2+(4 if wide else 2),5:4+(4 if wide else 2),0x0d:2}[kind&0x7f];assert struct.unpack_from('<H',record,type_at)[0]==0x85
 cursor=0
 while cursor<len(type_data):
  length=struct.unpack_from('<H',type_data,cursor+1)[0];assert length>=1 and cursor+length+3<=len(type_data);assert type_data[cursor+3] in (0x75,0x91);cursor+=length+3
 assert cursor==len(type_data)
 lines=b''
 for filename,segment,pairs in [('bases%d.c'%number,seg,[(10,0),(20,0x10),(30,0x20)]),('bases%d.c'%number,seg+1,[(40,4),(50,8)]),('shared.h',seg,[(60,0x40),(70,0x44)])]:lines+=counted(filename)+H(segment)+H(len(pairs))+b''.join(H(line)+H(offset) for line,offset in pairs)
 for kind,data in [(0x101,module),(0x103,type_data),(0x104,symbols),(0x109,lines)]:directory.append((kind,number,len(payload),len(data)));payload+=data
 provenance.append({'module':number,'wide_symbols':wide,'contributions':[seg,seg+1],'type_count':len(types),'local_symbol_size':len(symbols),'owners':owners,'references':refs,'types':[{'name':n,'index':hex(0x200+i),'record':v.hex()} for i,(n,v) in enumerate(types)]})
o=len(payload);struct.pack_into('<I',payload,4,o);payload+=H(len(directory))+b''.join(struct.pack('<HHII',*row) for row in directory);payload+=b'NB02'+I(len(payload)+8)
assert o+2+12*len(directory)+8==len(payload)
for kind,module,start,size in directory:assert 8<=start and start+size<=o
assert payload[:4]==payload[-8:-4]==b'NB02' and struct.unpack_from('<I',payload,len(payload)-4)[0]==len(payload)
(HERE/'based-symbols.dbg').write_bytes(payload)
recipe={'authored_input':True,'historical_producer_output':False,'source':'generate_input.py, decoded original0x91 pointer reader and23730 typed symbol dispatch, original loaded0x104 owner','size':len(payload),'sha256':hashlib.sha256(payload).hexdigest(),'directory':directory,'modules':provenance,'excluded':'BARRAY unwritten control; nonadvancing0x96 callback; no original state forcing','expected_new':'get_old_symbol,convert_old_based_symbol,register serializer,16/32 bprel/data serializers; all related actual based-pointer forms','validation':'Independent structural checks; original CLI validity and frontier still required'}
(HERE/'input-recipe.json').write_text(json.dumps(recipe,indent=2)+'\n');print(len(payload),recipe['sha256'])
