"""Bounded fresh object reads with immutable DWARF layout, never mutable caches.

Used only by the native observer. Every reader owns one stopped-checkpoint
snapshot. Raw padding is retained for debugging, not decoded as semantic state.
The pending equivalence mode compares this decoder with the legacy GDB scalar
reader at the same stop, before any target execution can occur.
"""
import hashlib,json,re,struct,time
from pathlib import Path
import gdb

ROOTS=('c1_data','c1_bss','c1_pdb_context','c1_output_pdb_context',
       'c1_arena_descriptors','c1_initialization_callbacks','c1_reuse_state','c1_pass_mode')
_schema=None
_records=[]
_directory=None
_field_map=None


def scalar_schema(typ):
 typ=typ.strip_typedefs()
 kind=typ.code;size=int(typ.sizeof)
 result=dict(type=str(typ),size=size)
 if kind==gdb.TYPE_CODE_ARRAY:
  first,last=typ.range();require(first==0 and 0<=last<100000,'array bounds')
  child=scalar_schema(typ.target());require((last+1)*child['size']==size,'array extent')
  result.update(kind='array',count=last+1,element=child)
 elif kind in (gdb.TYPE_CODE_STRUCT,gdb.TYPE_CODE_UNION):
  fields={}
  for field in typ.fields():
   require(not field.is_base_class and not field.bitsize,'unmodeled inheritance/bitfield')
   offset=int(field.bitpos)//8;require(int(field.bitpos)%8==0,'unaligned member')
   child=scalar_schema(field.type);require(0<=offset and offset+child['size']<=size,'member bounds')
   if field.name is None:
    require(child['kind']=='record','anonymous non-record')
    for name,row in child['fields'].items():
     require(name not in fields,'anonymous duplicate field');fields[name]=dict(offset=offset+row['offset'],schema=row['schema'])
   else:
    require(field.name not in fields,'duplicate field');fields[field.name]=dict(offset=offset,schema=child)
  result.update(kind='record',fields=fields)
 elif kind==gdb.TYPE_CODE_PTR:
  require(size==4,'i386 pointer width');result.update(kind='scalar',signed=False)
 else:
  require(kind in (gdb.TYPE_CODE_INT,gdb.TYPE_CODE_CHAR,gdb.TYPE_CODE_BOOL,gdb.TYPE_CODE_ENUM)and size in(1,2,4,8),'unsupported scalar '+str(typ))
  result.update(kind='scalar',signed=int(gdb.Value(-1).cast(typ))<0)
 return result


def require(ok,message):
 if not ok:raise ValueError('grouped modeled reader: '+message)


def locate(expression,schema):
 match=re.fullmatch(r'([A-Za-z_]\w*)((?:\.[A-Za-z_]\w*|\[\d+\])*)',expression)
 require(bool(match),'unsupported field expression '+expression)
 root,path=match.groups();require(root in schema,'unknown owned root '+root);current=schema[root];offset=0
 for name,index in re.findall(r'\.([A-Za-z_]\w*)|\[(\d+)\]',path):
  if name:
   require(current['kind']=='record'and name in current['fields'],'unknown typed member '+expression)
   row=current['fields'][name];offset+=row['offset'];current=row['schema']
  else:
   index=int(index);require(current['kind']=='array'and index<current['count'],'array index '+expression)
   current=current['element'];offset+=index*current['size']
 require(offset+current['size']<=schema[root]['size'],'object field extent')
 return root,offset,current


def configure(directory,field_map):
 global _directory,_field_map
 _directory=Path(directory);_field_map=json.loads(Path(field_map).read_text())
 require(_schema is None and not _records,'collector configured after use')


def schema():
 global _schema
 if _schema is None:
  result={}
  for root in ROOTS:
   symbol=gdb.lookup_global_symbol(root);require(symbol is not None,'missing global '+root)
   result[root]=scalar_schema(symbol.type)
  require(result['c1_data']['size']==2348 and result['c1_bss']['size']==1860,'current source object sizeof')
  require(result['c1_pdb_context']['size']==result['c1_output_pdb_context']['size']==24,'persistent object sizes')
  require(result['c1_initialization_callbacks']['size']==136 and result['c1_arena_descriptors']['size']==120,'source table extents')
  require(_field_map is not None,'field map absent')
  for region in ('modeled_initialized','modeled_bss'):
   for row in _field_map[region]:
    root,offset,node=locate(row['native_field'],result)
    require(root==('c1_data'if region=='modeled_initialized'else'c1_bss')and offset==row['native_offset']and node['size']==row['bytes'],'field map/DWARF disagreement '+row['name'])
  result['saved']=result['c1_data']
  text=json.dumps(result,sort_keys=True,indent=2)+'\n';(_directory/'grouped-layout.json').write_text(text)
  _schema=result
 return _schema


class Reader:
 def __init__(self):
  self.started=time.perf_counter();self.schema=schema();self.addresses={};self.raw={};self.calls=0
  inferior=gdb.selected_inferior()
  for root in ROOTS:
   # Resolve actual symbol address at EVERY checkpoint. Only layout is cached.
   address=int(gdb.parse_and_eval('(unsigned int)&'+root));size=self.schema[root]['size']
   require(address>0 and address+size<=0x100000000,'fresh object address bounds')
   self.addresses[root]=address;self.raw[root]=inferior.read_memory(address,size).tobytes()
  saved=struct.unpack_from('<I',self.raw['c1_bss'],0)[0];self.addresses['saved']=saved
  if saved:
   # Proven target-owned6548-byte allocation; read only the2348 modeled prefix.
   require(saved+2348<=0x100000000,'owned snapshot address overflow')
   require(all(not address<=saved<address+len(self.raw[root])for root,address in self.addresses.items()if root!='saved'),'snapshot aliases live source object')
   self.raw['saved']=inferior.read_memory(saved,2348).tobytes()
  self.read_seconds=time.perf_counter()-self.started

 def evaluate(self,expression):
  self.calls+=1;expression=expression.strip()
  if expression.startswith('(char*)&')and'-(char*)&'in expression:
   first,last=expression[len('(char*)&'):].split('-(char*)&',1)
   return self.address(first)-self.address(last)
  if expression.startswith('sizeof('):
   match=re.fullmatch(r'sizeof\(([^()]*)\)(?:/sizeof\(([^()]*)\))?',expression)
   require(bool(match),'sizeof grammar '+expression)
   first=self.node(match[1])[2]['size']
   if match[2]:
    last=self.node(match[2])[2]['size'];require(first%last==0,'sizeof ratio');return first//last
   return first
  cast=None
  for prefix in ('(unsigned int)','(int)'):
   if expression.startswith(prefix):cast=prefix;expression=expression[len(prefix):];break
  if expression.startswith('&'):return self.address(expression[1:])
  root,offset,node=self.node(expression)
  require(node['kind']=='scalar'and root in self.raw,'scalar read from absent/record object '+expression)
  raw=self.raw[root][offset:offset+node['size']];require(len(raw)==node['size'],'short scalar read')
  value=int.from_bytes(raw,'little',signed=node['signed'])
  if cast=='(unsigned int)':value&=0xffffffff
  if cast=='(int)':value=(value&0xffffffff);value=value-0x100000000 if value&0x80000000 else value
  return value

 def node(self,expression):return locate(expression.replace('c1_bss.saved_data->','saved.'),self.schema)
 def address(self,expression):
  root,offset,node=self.node(expression);require(self.addresses[root],'address of absent saved object');return self.addresses[root]+offset

 def record(self,new_seconds,legacy_seconds):
  index=len(_records);rows={}
  for root,raw in self.raw.items():
   name='grouped-%03d-%s.bin'%(index,root);(_directory/name).write_bytes(raw)
   rows[root]=dict(address=self.addresses[root],bytes=len(raw),path=name,sha256=hashlib.sha256(raw).hexdigest())
  _records.append(dict(checkpoint=index,fresh_objects=rows,field_evaluations=self.calls,read_seconds=self.read_seconds,
    new_seconds=new_seconds,legacy_seconds=legacy_seconds,equal=True,padding_semantics_claimed=False))
  with (_directory/'grouped-equivalence.jsonl').open('a')as output:output.write(json.dumps(_records[-1])+'\n')
