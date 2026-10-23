"""Read-only GDB projection and explicitly labeled restore-input auxiliary."""
import struct
import gdb
from command_line_fields import DATA,BSS,width,seed,seed_bss,targets,INITIALIZED_SIZE,BSS_SIZE

def address(expression):
 return int(gdb.parse_and_eval('(unsigned int)('+expression+')'))
def invocation():
 frame=gdb.newest_frame()
 while frame and frame.name()!='InvokeCompilerPass':frame=frame.older()
 assert frame
 args={name:int(frame.read_var(name).cast(gdb.lookup_type('unsigned int')))for name in ('argc','argv','mode')}
 vector=frame.read_var('argv')
 return dict(symbol=frame.name(),args=args,argv=[vector[i].string()for i in range(args['argc'])],argv_pointers=[int(vector[i].cast(gdb.lookup_type('unsigned int')))for i in range(args['argc'])],argv_terminator=int(vector[args['argc']].cast(gdb.lookup_type('unsigned int'))))
def read_fields(expression,rows):
 # This compact native object consists entirely of encountered semantic fields.
 # The original noncontiguous storage is decoded separately from retained raw.
 base=address('&'+expression);size=sum(width(row[2])for row in rows)
 data=gdb.selected_inferior().read_memory(base,size).tobytes();out={};at=0
 for name,rva,kind,default in rows:
  length=width(kind)
  out[name]=list(data[at:at+length])if '['in kind else struct.unpack_from('<i'if kind=='int'else'<I',data,at)[0]
  at+=length
 return out
_layout=None
_tables=None
def tables():
 global _tables
 if _tables is None:
  rules=address('option_rules');z=address('z_rules')
  raw=gdb.selected_inferior().read_memory(rules,85*12).tobytes()
  rows=[]
  for index in range(85):
   pattern,target,result,action=struct.unpack_from('<IIbB',raw,index*12)
   text=gdb.Value(pattern).cast(gdb.lookup_type('char').pointer()).string()if pattern else None
   assert text is None or len(text)<80
   rows.append(dict(pattern=text,target=target,result=result,action=action))
  zraw=gdb.selected_inferior().read_memory(z,12*12).tobytes()
  _tables=dict(rules=rows,z_rules=[list(struct.unpack_from('<3I',zraw,i*12))for i in range(12)],
   rule_size=address('sizeof(option_rules[0])'),z_rule_size=address('sizeof(z_rules[0])'),z_address=z,
   targets={str(rva):address('&'+field)for rva,field in targets().items()})
 return _tables
def capture():
 global _layout
 if _layout is None:
  _layout={}
  for name,root,rows in [('initialized','c1_data',DATA),('bss','c1_bss',BSS)]:
   _layout[name]={field:address('&'+root+'.options.'+field)-address('&'+root)for field,rva,kind,default in rows}
  _layout['initialized_size']=address('sizeof(c1_data)');_layout['bss_size']=address('sizeof(c1_bss)')
 saved=address('c1_bss.saved_data')
 return dict(initialized=read_fields('c1_data.options',DATA),bss=read_fields('c1_bss.options',BSS),
  snapshot=read_fields('c1_bss.saved_data->options',DATA)if saved else None,
  current_pdb=address('c1_current_pdb_context'),
  default_strings={default['symbol']:address(default['symbol'])for name,rva,kind,default in DATA if isinstance(default,dict)},layout=_layout,tables=tables())

def write_fields(expression,rows,values):
 for name,rva,kind,default in rows:
  target=address('&'+expression+'.'+name)
  value=values[name]
  raw=bytes(value)if '['in kind else struct.pack('<I',value&0xffffffff)
  assert len(raw)==width(kind)
  gdb.selected_inferior().write_memory(target,raw)
def seed_restore():
 saved=address('c1_bss.saved_data');assert saved
 write_fields('c1_bss.saved_data->options',DATA,seed('snapshot',saved))
 write_fields('c1_data.options',DATA,seed('live',saved))
 write_fields('c1_bss.options',BSS,seed_bss(saved))
 # Persistent selection is an existing typed object, never a bogus host handle.
 gdb.execute('set var c1_current_pdb_context = &c1_pdb_context')
