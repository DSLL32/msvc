from pathlib import Path
import struct,json,hashlib
root=Path('/root/msvc2/decomp/vc1.0/cvpack');out=Path('/root/.ai-tmp/vc1.0-cvpack/multitu-cache-build');b=(out/'cache.dll').read_bytes();U16=lambda p:struct.unpack_from('<H',b,p)[0];U32=lambda p:struct.unpack_from('<I',b,p)[0]
free=list(range(16));used={};table={};lru=[];cursor=None;position=0;first=None;events=[];label='open'
def flush(p):
 table[used[p]['page']]=1

def alloc(page):
 global first
 if not free:
  first=dict(consumer=label,file_offset=position,target_page=page,simultaneously_owned_pages=len(used),owners=[dict(identity=p,**v)for p,v in sorted(used.items())],lru_oldest_to_newest=list(lru),free_pages=0)
  raise StopIteration
 p=free.pop();used[p]=dict(page=page,pinned=True,birth=label);table[page]=p+2;return p

def seek(pos):
 global position,cursor
 position=pos;page=pos//4096
 if cursor is not None:used[cursor]['pinned']=False
 value=table.get(page)
 if value is None or value==1:cursor=alloc(page)
 else:cursor=value-2;used[cursor]['pinned']=True
 if cursor in lru:lru.remove(cursor)
 lru.append(cursor);events.append(dict(op='seek',consumer=label,offset=pos,owner=cursor,live=len(used)))

def read(size):
 global position,cursor
 while size:
  copied=min(size,4096-position%4096);position+=copied;size-=copied
  if position%4096==0:
   page=position//4096;value=table.get(page)
   if value is not None and value!=1:
    old=cursor;used[old]['pinned']=False;flush(old);del used[old];free.append(old);cursor=value-2;used[cursor]['pinned']=True
   else:
    flush(cursor);used[cursor]['page']=page;table[page]=cursor+2

def sr(pos,size,why):
 global label
 label=why;seek(pos);read(size)
try:
 seek(0);read(64);pe=U32(60);sr(pe,4+20+224,'PE signature/file/optional headers');n=U16(pe+6);optsize=U16(pe+20);sect=pe+24+optsize
 for i in range(n):
  sr(sect+i*40,40,'find .debug section')
  if b[sect+i*40:sect+i*40+6]==b'.debug':break
 sr(len(b),0,'physical EOF')
 debugrva=U32(pe+24+144);debugsize=U32(pe+24+148)
 for i in range(n):
  q=sect+i*40;sr(q,40,'debug RVA section walk');va,rs,rp=struct.unpack_from('<III',b,q+12)
  if va<=debugrva<va+rs:break
 for j in range(debugsize//28):
  q=debugrva-va+rp+j*28;sr(q,28,'debug directory')
  if U32(q+12)==2:base=U32(q+24);break
 sr(base,4,'CodeView signature');assert b[base:base+4]==b'NB05';read(4);di=base+U32(base+4);sr(di,16,'CodeView directory header');hs,es,count=struct.unpack_from('<HHI',b,di);assert(hs,es)==(16,12);read(count*12)
 ss=[struct.unpack_from('<HHII',b,di+hs+i*es)for i in range(count)];ss.sort(key=lambda s:(s[1],s[0]));assert all(s[0]in(0x120,0x121,0x123,0x124,0x128,0x12d)for s in ss)
 for tag,mod,off,size in ss:
  if tag in (0x128,0x12d):sr(base+off,size,'initialize_modules global subsection%x'%tag)
 cv_offset=0
 for tag,mod,off,size in ss:
  if mod in(0,65535):continue
  assert tag in(0x120,0x121,0x123,0x124)
  label='module%d subsection%x'%(mod,tag)
  if cv_offset!=off:seek(base+off)
  read(size);cv_offset=off+size
except StopIteration:pass
assert first and first['simultaneously_owned_pages']==16 and any(not x['pinned']for x in first['owners']),'producer does not prove eviction'
r=dict(input_sha256=hashlib.sha256(b).hexdigest(),input_bytes=len(b),codeview_base=base,codeview_signature='NB05',directory_entries=count,global_type_subsections=[dict(module=s[1],offset=base+s[2],bytes=s[3],end_page=(base+s[2]+s[3])//4096)for s in ss if s[0]==0x121],first_predicted_eviction=first,events=events,model_scope='Exact single-existing-file prefix until first eviction: seek pin/unpin/allocate/touch, read at4096 boundary advances, cached-real next releases current; sentinel/NULL reuses same Page with old table flushed to sentinel. No counting one owner per physical page. read_new_global_types sequential physical reads coalesced; no type/PCH external reads in these24 modules. No original execution or acceptance.',source_sha256={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest()for p in [root/'src/files.c',root/'src/main.c',root/'src/image.c',root/'src/modules.c']})
(out/'cache-gate.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps({k:v for k,v in r.items()if k not in('events','global_type_subsections','source_sha256')},indent=2))
