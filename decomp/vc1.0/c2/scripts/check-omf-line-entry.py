"""Three-formal LINNUM32 ABI and complete six-byte wire/write ordering."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_write_line@12';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
params=[dict(name=k,location=l,type=t)for k,l,t in [('line','CX','unsigned16'),('offset','EDX','unsigned32'),('segment','entryESP+4','Segment pointer')]]
def validate(e):
 assert e['matched']and e['function']==name and e['original_va']=='0x6b780'and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==4 and e['no_hidden_inputs_proof']
 assert [{k:f[k]for k in['name','location','type']}for f in e['formals']]==row['abi']['parameters']==params
 assert e['shape']==dict(line=34,offset=0,identifier=5,bucket=5,index=1)and e['original']['return_address']==0x6b77d and e['native']['caller']=='omf_output_debug_record'
 for side in ['original','native']:
  d=e[side];assert d['line']==34 and d['offset']==0 and d['segment']==d['member']and d['segment']and d['identifier']==d['current_segment_id']==5 and d['bucket']==5 and d['index']==1 and[f[side]for f in e['formals']]==[d['line'],d['offset'],d['segment']]
 assert e['native']['caller_segment']==e['native']['segment']
validate(e);reject=[]
for label,change in [('missing segment',lambda e:e['formals'].pop()),('wrong line width',lambda e:e['formals'][0].update(type='unsigned32')),('wrong segment location',lambda e:e['formals'][2].update(location='EAX')),('wrong cleanup',lambda e:e.update(callee_stack_cleanup=0)),('wrong hash member',lambda e:e['original'].update(member=0)),('wrong caller segment',lambda e:e['native'].update(caller_segment=0)),('wrong segment ID',lambda e:e['original'].update(identifier=6)),('invented record input',lambda e:e.update(hidden_inputs=['record']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6b780 and row['end']==0x6b7ee and original.read(0x6b7eb,3)==bytes.fromhex('c20400')and original.read(0x6b78f,5)==bytes.fromhex('b995000000')
assert 'void __fastcall omf_write_line(unsigned short line, unsigned offset, Segment *segment);'in Path('src/omf-output.h').read_text()
if args.abi_only:print('LINNUM32 actual word/offset/stack context/8 rejects,110-byte extent/RET4 and typed signature pass');raise SystemExit

def run(image,native,line,offset,count,position,context):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in[(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x2000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 record=0x400400;segment=0x400900 if context else 0;guard=bytes((i*13+5)&255 for i in range(96));u.mem_write(0x400900,guard);buf=bytearray((i*29+7)&255 for i in range(1004));struct.pack_into('<2I',buf,12,count,record+position);u.mem_write(record,bytes(buf));u.mem_write(0x300800,struct.pack('<3I',0x500000,segment,0xfeedbeef));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,line);u.reg_write(UC_X86_REG_EDX,offset);saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 begin=names['@omf_begin_record@8']if native else 0x6b480;events=[];terminal=False
 def access(m,kind,address,size,value,_):assert address+size<=0x400900 or address>=0x400960,'Segment context is passed but not dereferenced'
 def hook(m,a,size,_):
  nonlocal terminal
  if a==0x500000:terminal=True;m.emu_stop();return
  if a!=begin:return
  assert not events and m.reg_read(UC_X86_REG_ECX)==0x95 and m.reg_read(UC_X86_REG_EDX)==segment and bytes(m.mem_read(record,1004))==buf;events.append(['begin',0x95,'segment'if segment else'null']);esp=m.reg_read(UC_X86_REG_ESP);m.reg_write(UC_X86_REG_EAX,record);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0])
 u.hook_add(UC_HOOK_CODE,hook);u.hook_add(UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,access);u.emu_start(names[name]if native else row['start'],0,count=500)
 data=struct.pack('<HI',line&65535,offset);buf[position:position+6]=data;struct.pack_into('<2I',buf,12,(count+6)&0xffffffff,record+position+6)
 assert terminal and len(events)==1 and u.reg_read(UC_X86_REG_ESP)==0x300808 and all(u.reg_read(r)==v for r,v in saved)and bytes(u.mem_read(0x300808,4))==struct.pack('<I',0xfeedbeef)and bytes(u.mem_read(record,1004))==buf and bytes(u.mem_read(0x400900,96))==guard
 return dict(events=events,data_hex=data.hex(),record_sha256=hashlib.sha256(buf).hexdigest(),stack_cleanup=4)
rows=[]
for line,offset,count,position,context in itertools.product([0,34,65535,0x5a5a1234],[0,0x12345678,0xffffffff],[0,17,0xffffffff],[20,68,998],[False,True]):
 a=run(original,False,line,offset,count,position,context);b=run(canonical,True,line,offset,count,position,context);assert a==b;rows.append(dict(line_register=line,offset=offset,record_count=count,record_cursor_offset=position,context=context,result=a))
Path('cases/omf-line-entry-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole110-byte original/canonical LINNUM32 versus independent six-byte <HI wire model.216 states cover word/high-register line shapes,32-bit offset patterns,countwrap/cursor positions including final6bytes,context/null forwarding to begin95, child clobber/saved line-offset, exact RET4/owned record and segment guard. Memory hook rejects any segment pointee access; no extra helper edges. Child begin mocked with bounded record result, not claimed real-null-segment support or cold child execution. Actual three formals line34/offset0/closed segmentID5/index1 and caller-result alias separately observed. No incoming record, raw segment tail or output seed.'),indent=2)+'\n');print(len(rows),'whole LINNUM32 controls/8 actual ABI rejects pass')
