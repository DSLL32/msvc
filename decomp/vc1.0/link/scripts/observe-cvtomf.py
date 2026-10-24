#!/usr/bin/env python3
"""Native GDB observations for the direct C6 converter, outside fixture runs."""
import argparse,hashlib,json,os,subprocess,tempfile,time
from pathlib import Path
from case_paths import SCRATCH,PROGRAM,native_work
BINARY=Path(__file__).resolve().parents[1];CASE=BINARY/'cases/direct-cvtomf-codeview'
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--function',default='omf_write_debug_symbol_data');ap.add_argument('--next-function');ap.add_argument('--trial',action='store_true');ap.add_argument('--variant',choices=['with-lines','no-lines'],default='with-lines');args=ap.parse_args()
    config=json.loads((BINARY/'functions.json').read_text());allowed='_'+args.function in config['accepted'] or args.trial
    directory=Path(tempfile.mkdtemp(prefix='cvtomf-gdb-',dir=SCRATCH/'vc1.0/link'));work,drives=native_work(directory,r'S:\vc1.0\link\direct-cvtomf-codeview');input_case=CASE if args.variant=='with-lines' else CASE/'variants/no-lines';(work/'legacy.obj').write_bytes((input_case/'inputs/legacy.obj').read_bytes())
    source=r'''
import gdb,json,struct,hashlib
from pathlib import Path
events=[];before=None;terminal=[];directory=Path(OUTPUT)
def val(s):return int(gdb.parse_and_eval(s))&0xffffffff
def mem(p,n):return bytes(gdb.selected_inferior().read_memory(p,n)) if n else b''
def words(p,n):return list(struct.unpack('<'+'I'*n,mem(p,n*4)))
def persist():
 (directory/'state.json').write_text(json.dumps(dict(events=events,terminal=terminal),indent=2)+'\n')
def snapshot(event,logical=None):
 logical=logical or LOGICAL
 esp=val('$esp');stack=words(esp,8);stream=words(val('&recon_iob[1]'),16);pending=[]
 for i in range(2):
  used=stream[i*8]-stream[i*8+2];assert 0<=used<=4096
  pending.append(mem(stream[i*8+2],used).hex() if used else '')
 result=dict(event=event,name=logical,eip=val('$eip'),esp=esp,eax=val('$eax'),ebx=val('$ebx'),esi=val('$esi'),edi=val('$edi'),ebp=val('$ebp'),stack=stack,errors=[val('*cvtomf_test_errno'),val('recon_doserrno'),val("'win32.c'::last_error")],files=[stream[:8],stream[8:]],pending=pending,ctype=mem(val('recon_pctype')-2,514).hex(),mb_cur_max=val('recon_mb_cur_max'),parser_globals=[val(x) for x in ['omf_program_name','omf_argument_index','omf_option_argument','omf_option_character','omf_debug_mode','omf_option_offset']])
 if event in ('frontier','entry'):
  if logical in ('omf_write_debug_symbol_data','omf_write_debug_type_data'):
   result['formals']=dict(offset=stack[1],data=stack[2],length=stack[3]);assert stack[3]<=4096
   result['payload']=mem(stack[2],stack[3]).hex()
  elif logical=='omf_convert_debug_type':result['formals']=dict(index=stack[1],enclosing=stack[2])
  elif logical in ('omf_debug_number','omf_debug_index'):
   result['formals']=dict(record_pointer=stack[1]);result['cursor']=words(stack[1],1)[0]
  elif logical=='omf_find_debug_fixup':result['formals']=dict(offset=stack[1])
  elif logical=='omf_debug_register':result['formals']=dict(number=stack[1])
  elif logical=='omf_debug_type_auxiliary':
   result['formals']=dict(type=stack[1]&65535,coff_type=stack[2],auxiliary=stack[3])
  elif logical=='omf_compare_debug_fixups':
   result['formals']=dict(left=stack[1],right=stack[2]);result['rows']=[dict(offset=words(v,1)[0],external=struct.unpack('<H',mem(v+4,2))[0],section=struct.unpack('<h',mem(v+6,2))[0]) for v in [stack[1],stack[2]]]
  elif logical in ('omf_debug_primitive_type','omf_debug_primitive_size'):result['formals']=dict(type=stack[1])
  elif logical=='omf_debug_is_union':result['formals']=dict(types=stack[1],names=stack[2])
  elif logical=='omf_convert_debug_structure':
   result['formals']=dict(kind=stack[1],record=stack[2],type=stack[3],index=stack[4]);result['type_before']=mem(stack[3],12).hex()
  else:
   assert logical in ('omf_save_debug_fixups','omf_convert_debug_types','omf_resolve_debug_types','omf_convert_debug_symbols','omf_finish_debug_types');result['formals']=[]
 try:
  pointer=val('omf_debug_symbol_data');capacity=val('omf_debug_symbol_capacity');assert capacity<=0x100000
  result['symbol_buffer']=dict(pointer=pointer,capacity=capacity)
  if pointer:
   raw=mem(pointer,capacity);name=event+'-symbols.bin';(directory/name).write_bytes(raw);result['symbol_buffer'].update(file=name,sha256=hashlib.sha256(raw).hexdigest())
 except gdb.error:
  assert not ALLOWED
 pointer=val('omf_debug_type_data');size=val('omf_debug_type_bytes');assert size<=0x100000
 result['type_buffer']=dict(pointer=pointer,size=size)
 if pointer:
  raw=mem(pointer,size);name=event+'-types.bin';(directory/name).write_bytes(raw);result['type_buffer'].update(file=name,sha256=hashlib.sha256(raw).hexdigest())
 result.update(record_remaining=val('omf_record_remaining')&65535,record_checksum=val('omf_record_checksum'),data_offset=val('omf_data_offset'),data_segment=val('omf_data_segment'))
 count=val('omf_debug_fixup_count');pointer=val('omf_debug_fixups');assert count<=512
 fixups_live=logical!='omf_finish_debug_types' and not(logical=='omf_convert_debug_symbols' and event=='return')
 result['debug_fixups']=dict(pointer=pointer,count=count,records=mem(pointer,count*8).hex() if count and fixups_live else '',records_live=fixups_live)
 pointer=val('omf_input')
 if pointer:
  f=words(pointer,8);assert f[1]<=4096;result.update(input_file=f,input_unread=mem(f[0],f[1]).hex() if f[1] else '')
 else:result.update(input_file=None,input_unread='')
 count=val('omf_defined_segment_count');groups=val('omf_group_count');assert count<256 and groups<256
 result.update(defined_segments=count,groups=groups,segments=mem(val('&omf_segments'),(count+1)*16).hex(),segment_indices=mem(val('&omf_segment_symbol_indices'),(count+1)*4).hex(),group_indices=mem(val('&omf_group_symbol_indices'),(groups+1)*4).hex(),thread_symbols=mem(val('&omf_thread_symbols'),16).hex())
 count=val('omf_debug_type_count');pointer=val('omf_debug_type_records');assert count<=256
 live=val('omf_debug_type_bytes')!=0 and pointer!=0
 result['type_directory']=dict(records_pointer=pointer,map_pointer=val('omf_debug_type_map'),count=count,records=words(pointer,count) if count and live else [],records_live=live)
 if logical=='omf_convert_debug_symbols' and event in ('entry','frontier'):
  result['formals']=[];result['line_count']=val('omf_line_count');result['lines']=[]
  for i in range(result['line_count']):
   p=val('omf_line_data')+i*8;result['lines'].append([words(p,1)[0],struct.unpack('<H',mem(p+4,2))[0]])
  result['line_first_allocated_row']=mem(val('omf_line_data'),8).hex()
  result['text_section']=val('omf_text_section')&65535;result['debug_record']=mem(val('&omf_debug_record'),128).hex();result['symbol_nodes']=[]
  result['symbol_indices']=mem(val('&omf_symbol_indices'),16384).hex()
  for i in range(211):
   p=val('omf_symbol_buckets['+str(i)+']')
   while p:
    assert len(result['symbol_nodes'])<128
    name,value,kind,section,ordinal,typ,weak=struct.unpack('<IIHhHHH',mem(p,18));text=bytearray()
    while True:
     byte=mem(name+len(text),1);text.extend(byte)
     if byte==b'\0':break
     assert len(text)<260
    result['symbol_nodes'].append(dict(bucket=i,name=text.hex(),value=value,kind=kind,section=section,ordinal=ordinal,type=typ,weak_default=weak));p=words(p+20,1)[0]
 if event=='return' and logical in ('omf_debug_number','omf_debug_index'):
  result['cursor']=words(before['formals']['record_pointer'],1)[0]
 if event=='return' and logical=='omf_find_debug_fixup':
  result['returned_offset']=result['eax']-val('omf_debug_fixups') if result['eax'] else None;result['returned_row']=mem(result['eax'],8).hex() if result['eax'] else None
 if event=='return' and logical=='omf_debug_type_auxiliary':
  result['coff_type']=struct.unpack('<H',mem(before['formals']['coff_type'],2))[0];result['auxiliary']=mem(before['formals']['auxiliary'],18).hex()
 if event=='return' and logical in ('omf_convert_debug_type','omf_convert_debug_structure'):
  index=before['formals']['index'];result.update(map_slot_index=index,map_slot=mem(val('omf_debug_type_map')+index*12,12).hex())
 if logical in ('omf_resolve_debug_types','omf_debug_type_auxiliary') or (logical=='omf_finish_debug_types' and event in ('entry','frontier')):
  result['deferred_head']=val('omf_deferred_debug_types');result['type_map']=[]
  for i in range(count):
   slot=mem(val('omf_debug_type_map')+i*12,12);kind,reserved,size,value=struct.unpack('<HHII',slot);entry=dict(type=kind,reserved=reserved,size=size,value=value)
   if kind&0x30==0x30:entry['array']=mem(value,18).hex()
   result['type_map'].append(entry)
 if logical=='omf_finish_debug_types':result['anonymous_counter']=val('omf_anonymous_type_counter')
 count=val('omf_coff_symbol_count');assert count<=200
 result.update(coff_symbol_count=count,coff_symbols=mem(val('omf_coff_symbols'),count*18).hex() if count else '')
 return result
class Return(gdb.Breakpoint):
 def stop(self):
  event=snapshot('return');events.append(event);persist()
  assert event['esp']==before['esp']+4 and all(event[r]==before[r] for r in ['ebx','esi','edi','ebp'])
  self.enabled=False;return True
class Entry(gdb.Breakpoint):
 def stop(self):
  global before
  before=snapshot('entry' if ALLOWED else 'frontier');caller=gdb.newest_frame().older();before['caller']=caller.name()
  if LOGICAL=='omf_debug_type_auxiliary':
   current=gdb.newest_frame();caller.select();before['actual_output_owners']=dict(coff_type=val('&type'),auxiliary=val('&auxiliary'));current.select();assert all(before['formals'][k]==v for k,v in before['actual_output_owners'].items())
  if LOGICAL in ('omf_debug_number','omf_debug_index'):
   current=gdb.newest_frame();caller.select();owner=val('&record');current.select();before['actual_cursor_owner']=owner;assert owner==before['formals']['record_pointer']
  events.append(before);persist()
  if not ALLOWED:return True
  Return('*'+str(before['stack'][0]),internal=True);return False
class Next(gdb.Breakpoint):
 def stop(self):
  event=snapshot('frontier',NEXT);event['caller']=gdb.newest_frame().older().name();events.append(event);persist();return True
def exited(event):terminal.append(dict(exit=getattr(event,'exit_code',None)));persist()
gdb.events.exited.connect(exited)
gdb.execute('set pagination off');gdb.execute('set confirm off');Entry('*'+LOGICAL,internal=True)
if NEXT:Next('*'+NEXT,internal=True)
gdb.execute('run -g -o converted.obj legacy.obj >'+STDOUT+' 2>'+STDERR)
assert events and (events[-1]['event']=='frontier' or len(events)==2)
persist()
'''
    for key,value in dict(OUTPUT=str(directory),LOGICAL=args.function,ALLOWED=allowed,NEXT=args.next_function,STDOUT=str(directory/'stdout.bin'),STDERR=str(directory/'stderr.bin')).items():source=source.replace(key,repr(value))
    (directory/'observe.py').write_text(source);env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')};env.update(drives,RECON_PROGRAM_PATH=PROGRAM,LC_ALL='C',TZ='PST8PDT')
    for key in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB','LINK_REPRO']:env.pop(key,None)
    command=['gdb','-q','-nx','-batch','-ex','file '+str(BINARY/'build/native/cvtomf'),'-ex','source '+str(directory/'observe.py')];start=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,capture_output=True,timeout=15);elapsed=time.perf_counter()-start
    (directory/'gdb.stdout').write_bytes(r.stdout);(directory/'gdb.stderr').write_bytes(r.stderr)
    (directory/'receipt.json').write_text(json.dumps(dict(function=args.function,trial=args.trial,variant=args.variant,input_sha256=hashlib.sha256((input_case/'inputs/legacy.obj').read_bytes()).hexdigest(),seconds=elapsed,exit=r.returncode,command=command,elf_sha256=hashlib.sha256((BINARY/'build/native/cvtomf').read_bytes()).hexdigest(),source_sha256={str(p.relative_to(BINARY)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [BINARY/'scripts/native-cvtomf.c',BINARY/'src/omf.c',BINARY/'src/omf-debug.c',BINARY/'src/omf.h',BINARY/'src/omf-stubs.c',BINARY/'scripts/observe-cvtomf.py']}),indent=2)+'\n');print(directory);assert r.returncode==0,(r.stdout,r.stderr)
if __name__=='__main__':main()
