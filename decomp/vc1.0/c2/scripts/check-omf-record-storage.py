"""Actual typed OMF record owner, member identity, zeros and pointer modulo."""
from pathlib import Path
import copy,hashlib,json,struct,subprocess,time
from pe_indexes import indexes,publics
started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();owner=names['_omf_record_storage'];pool=names['_omf_records'];empty=names['_omf_empty_name']

def contract(d):
 assert d['bytes']==4020 and d['empty']==d['owner']and d['pool']==d['owner']+4 and d['owner']%8==0
 assert d['slots']==[d['pool']+i*1004 for i in range(4)]and [a%8 for a in d['slots']]==[4,0,4,0]
 assert d['slots'][-1]+1004==d['owner']+4020 and d['zero_bytes']==4020
source=Path('src/omf-output.c').read_text();header=Path('src/omf-output.h').read_text();assert '#define omf_empty_name omf_record_storage.empty_name'in header and '#define omf_records omf_record_storage.records'in header
assert 'offsetof(OmfRecordStorage, records) == 4'in header and 'sizeof(OmfRecordStorage) == 4020'in header
assert '#pragma data_seg("OMFREC")\nOmfRecordStorage omf_record_storage = {0};\n#pragma data_seg()'in source
assert 'omf_record_storage __attribute__((aligned(8)))'in source
# Original loader owns all real adjacent words, not a file-offset-zero read.
s=next(s for s in original.sections if original.base+s['rva']<=0x7a8d8 and 0x7b88c<=original.base+s['rva']+s['virtual_size']);assert s['flags']&0x80
s=next(s for s in canonical.sections if canonical.base+s['rva']<=owner<canonical.base+s['rva']+s['virtual_size']);assert s['name']=='OMFREC'and s['virtual_size']==4020 and canonical.base+s['rva']==owner and canonical.read(owner,4020)==bytes(4020)
# No independent alias globals in the map. Both legacy names resolve to the
# one actual compiler object via pe_indexes.publics(), at exact member offsets.
raw=Path('build/c2.map').read_text();assert '_omf_record_storage'in raw and '_omf_records 'not in raw and '_omf_empty_name 'not in raw
rows=[]
for label,base in [('original',0x7a8d8),('canonical',owner)]:
 d=dict(side=label,owner=base,empty=base,pool=base+4,slots=[base+4+i*1004 for i in range(4)],bytes=4020,zero_bytes=4020);contract(d);rows.append(d)
for path in ['build/native/C23232.EXE','build/native-trace/C23232.EXE']:
 symbols=subprocess.check_output(['nm','-S','--defined-only',path],text=True).splitlines();hits=[line.split()for line in symbols if line.split()[-1]=='omf_record_storage'];assert len(hits)==1;v,size,kind,_=hits[0];assert kind=='B'and int(size,16)==4020
 assert not any(line.split()[-1]in ['omf_records','omf_empty_name']for line in symbols)
 base=int(v,16);d=dict(side=path,owner=base,empty=base,pool=base+4,slots=[base+4+i*1004 for i in range(4)],bytes=int(size,16),zero_bytes=4020);contract(d);rows.append(d)
# Actual canonical object relocations reference that owner with real member
# addends. VC1 emits the section base, then explicit member-offset arithmetic;
# the cache-reset whole-body check independently verifies every actual slot write.
# This binds the single compiled owner, rather than comparing separate zeros.
data=Path('build/omf-output.obj').read_bytes();_,count,_,sym,nsyms,opt,_=struct.unpack_from('<HHIIIHH',data);strings=sym+18*nsyms;symbols={};i=0
while i<nsyms:
 off=sym+18*i;name=data[off:off+8];zero,tail=struct.unpack('<II',name)
 if zero==0:name=data[strings+tail:].split(b'\0',1)[0]
 symbols[i]=dict(name=name.rstrip(b'\0').decode(),value=struct.unpack_from('<I',data,off+8)[0],section=struct.unpack_from('<h',data,off+12)[0]);i+=1+data[off+17]
owner_symbol=next(v for v in symbols.values()if v['name']=='_omf_record_storage');section_symbol=next(v for v in symbols.values()if v['name']=='OMFREC');assert owner_symbol['section']==section_symbol['section']==2 and owner_symbol['value']==section_symbol['value']==0
uses=[]
for i in range(count):
 sec=20+opt+40*i;name,_,_,size,raw,rel,_,nr,_,_=struct.unpack_from('<8sIIIIIIHHI',data,sec)
 for n in range(nr):
  offset,index,kind=struct.unpack_from('<IIH',data,rel+10*n)
  if symbols[index]['name']in ['_omf_record_storage','OMFREC']:uses.append(dict(section=name.rstrip(b'\0').decode(),offset=offset,addend=struct.unpack_from('<I',data,raw+offset)[0],relocation=kind))
assert uses and all(u['relocation']==6 for u in uses)and {u['addend']for u in uses}=={0},uses
rejected=[]
for label,edit in [('split empty-name identity',lambda d:d.update(empty=d['owner']+8)),('lost leading member',lambda d:d.update(pool=d['owner'])),('misaligned owner',lambda d:d.update(owner=d['owner']+4)),('wrong slot stride',lambda d:d['slots'].__setitem__(1,d['slots'][1]+4)),('wrong owner extent',lambda d:d.update(bytes=4024)),('nonzero initial field',lambda d:d.update(zero_bytes=4016))]:
 d=copy.deepcopy(rows[0]);edit(d)
 try:contract(d)
 except AssertionError:rejected.append(label)
 else:raise AssertionError('accepted '+label)
report=dict(actual_storage=rows,rejected=rejected,canonical_owner_relocations=uses,canonical_section=s['name'],source_sha256=hashlib.sha256(Path('src/omf-output.c').read_bytes()).hexdigest(),seconds=time.perf_counter()-started,scope='One genuine4020B typed owner: original loader-zero empty-nameDWORD and four1004B records. Canonical dedicated initialized-zero PE section changes file representation only; native BSS explicitly aligned8. Actual member/slot identities and every modulo8, map/nm single definitions and canonical relocation def/use bound. No extra code/function credit.')
Path('cases/omf-record-storage-validation.json').write_text(json.dumps(report,indent=2)+'\n');print('Typed4020B OMF owner/zeros/aliases/4slot stride+modulo pass in original/PE/bothELFs;6 rejects')
