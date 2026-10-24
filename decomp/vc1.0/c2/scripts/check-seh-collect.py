"""Scope discovery: typed inputs, list relocation and fresh child outputs."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
from seh_collect_inputs import projection
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@collect_exception_scopes@4';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(d):
 assert d['matched']and d['function']==name and d['no_hidden_inputs_proof']and d['hidden_inputs']==row['abi']['hidden_inputs']==[]and d['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0 and d['return_contract']=='void; caller ignores EAX'
 assert [{k:f[k]for k in ['name','location','type']}for f in d['formals']]==row['abi']['parameters']==[dict(name='function',location='ECX',type='Function pointer')]
 for side in ['original','native']:
  v=d[side];assert v['function']==v['current']==v['parent']==d['formals'][0][side]and v['function']and v['first']==v['expressions'][0]['pointer']and v['depth']==0 and len(v['expressions'])==55 and len({x['pointer']for x in v['expressions']})==55
 assert d['original']['return_address']==0x65717 and d['native']['caller']=='prepare_inline_function'
 assert projection(d['original'])==projection(d['native'])==d['projection']
validate(e);rejected=[]
for label,edit in [('missing formal',lambda d:d['formals'].clear()),('wrong register',lambda d:d['formals'][0].update(location='EDX')),('lost current alias',lambda d:d['original'].update(current=0)),('lost caller alias',lambda d:d['native'].update(parent=0)),('wrong expression root',lambda d:d['original'].update(first=0)),('broken next closure',lambda d:d['native']['expressions'][0].update(next=0)),('changed label marker',lambda d:d['native']['expressions'][12].update(marker=0x47)),('changed Node alias',lambda d:d['native']['expressions'][1].update(tree=d['native']['expressions'][0]['tree'])),('changed finally depth',lambda d:d['native'].update(depth=1)),('unknown hidden input',lambda d:d.update(hidden_inputs=['?'])),('wrong cleanup',lambda d:d.update(callee_stack_cleanup=4)),('invented return value',lambda d:d.update(return_contract='EAX zero'))]:
 d=copy.deepcopy(e);edit(d)
 try:validate(d)
 except(AssertionError,IndexError,KeyError):rejected.append(label)
 else:raise AssertionError('accepted '+label)
assert row['start']==0x65b00 and row['end']==0x65d26 and original.read(0x65d25,1)==b'\xc3'
assert original.string(struct.unpack('<I',original.read(0x84e80,4))[0])=='@(#)except.c:1.25'
assert original.read(0x65b1d,3)==bytes.fromhex('8b7010')and original.read(0x65b8f,7)==bytes.fromhex('c74028ffffffff')
source=Path('src/exception-scope.c').read_text();assert '((LabelSymbol *)expression->tree->right.symbol)->field28'in source and '((ExceptionScope *)node->right.pointer)->kind'in source
for n,a in [('_current_exception_scope',0x660d0),('_allocate_exception_scope',0x661c0),('_pop_exception_scope',0x65ec0)]:
 r=next(r for r in cfg['driver']if r['name']==n);assert r['start']==a
 if n not in cfg['accepted']:assert r['provisional']and r['end']==a+1
if args.abi_only:print('SEH discovery Function/55-expression/label ABI,550B extent,conditional layouts and12 rejects pass');raise SystemExit

def node(op,label=None,marker=0):return dict(op=op,label=label,marker=marker)
def run(image,side,spec,depth,mutation,expected_order=None,error=None):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x20000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 memory=bytearray((i*13+7)&255 for i in range(0x20000));function=0x400000;expressions=0x401000;nodes=0x402000;symbols=0x404000;scopes=0x406000;alternate_scope=0x406f00;newnode=0x408000;alternate_return=0x408040;newlabel=0x408200;alternate_marker=0x408400;alternate_head=0x408500;alternate_node=0x409000;alternate_label=0x409100
 def put(a,v,fmt='<I'):struct.pack_into(fmt,memory,a-0x400000,v)
 put(function+16,expressions);labels={};return_indices=[]
 for i,s in enumerate(spec):
  ep=expressions+16*i;np=nodes+32*i;put(ep,ep+16 if i+1<len(spec)else 0);put(ep+4,np);put(np,s['op'])
  if s['op']==0x47:return_indices.append(i)
  if s['label']is not None:
   lp=labels.setdefault(s['label'],symbols+64*len(labels));put(np+20,lp);put(lp+12,3,'<B');put(lp+13,s['marker'],'<B')
 assert return_indices or error
 for i in range(16):put(scopes+64*i+4,0);put(scopes+64*i+12,0x80000000+i)
 put(alternate_scope+4,1);put(alternate_scope+12,0xf00);put(newnode,0x29);put(alternate_return,0x29);put(alternate_marker,0x47);put(alternate_head,0);put(alternate_head+4,alternate_marker);put(alternate_node,0x29);put(alternate_node+20,alternate_label);put(alternate_label+12,3,'<B');put(alternate_label+13,0,'<B')
 u.mem_write(0x400000,bytes(memory));depth_cell=names['_exception_finally_depth']if side else 0x7b9a0;return_cell=names['_exception_return_label']if side else 0x7b99c;u.mem_write(depth_cell,struct.pack('<I',depth));u.mem_write(return_cell,struct.pack('<I',0xdeadbeef));u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,function);u.reg_write(UC_X86_REG_EDX,0xdeadbeef)
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 child_rows=[('current','_current_exception_scope',0x660d0),('allocate','_allocate_exception_scope',0x661c0),('pop','_pop_exception_scope',0x65ec0),('newnode','@new_node@4',0x4f740),('newlabel','_new_internal_label',0x4e3d0),('insert','@insert_tree_expression@12',0x4fd10),('release-node','@release_node@4',0x4f490),('release-expression','@release_expression@4',0x4fcf0),('error','@compiler_internal_error@8',0x6fb70)];children={names[n]if side else a:k for k,n,a in child_rows};events=[];terminal=[];stack=[];allocated=[];query_count=0;released=[];errors=[];inserted=[]
 def word(a):return struct.unpack('<I',u.mem_read(a,4))[0]
 def write(a,v):u.mem_write(a,struct.pack('<I',v))
 def text(a):
  b=bytearray()
  while len(b)<64:
   c=u.mem_read(a+len(b),1)[0]
   if not c:return b.decode('latin1')
   b.append(c)
  raise AssertionError('unterminated source')
 def hook(m,a,size,_):
  nonlocal query_count
  if a==0x500000:terminal.append('return');m.emu_stop();return
  if a not in children:return
  child=children[a];esp=m.reg_read(UC_X86_REG_ESP);ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);value=0xdeadbeef;cleanup=0;detail=[]
  if child=='current':
   value=stack[-1]if stack else 0;detail=[value];query_count+=1
   if mutation=='label-tree'and query_count==1:write(expressions+4,alternate_node)
   if mutation=='query-next'and query_count==1:write(expressions,expressions+32)
  elif child=='allocate':
   value=scopes+64*len(allocated);assert len(allocated)<16;allocated.append(value);stack.append(value);detail=[value]
   if mutation=='allocated-node'and len(allocated)==1:write(expressions+4,alternate_node)
  elif child=='pop':
   assert stack;detail=[stack.pop()]
   if mutation=='pop-right':write(nodes+32*2+20,alternate_scope)
  elif child=='newnode':assert ecx==0x29;value=newnode;detail=[0x29]
  elif child=='newlabel':
   value=newlabel
   if mutation=='return-global':write(return_cell,alternate_return)
   elif mutation=='return-head':write(function+16,alternate_head)
   elif mutation=='return-tree':write(expressions+16*return_indices[-1]+4,alternate_marker)
  elif child=='insert':
   expected_before=word(expressions+16*return_indices[-1]+4);assert ecx==word(function+16)and edx==expected_before and word(esp+4)==word(return_cell);detail=[ecx,edx,word(esp+4)];inserted.append(detail);cleanup=4
  elif child=='release-node':assert nodes<=ecx<nodes+32*len(spec)and(ecx-nodes)%32==0;detail=[(ecx-nodes)//32];released.append(['node',detail[0]])
  elif child=='release-expression':assert expressions<=ecx<expressions+16*len(spec)and(ecx-expressions)%16==0;detail=[(ecx-expressions)//16];released.append(['expression',detail[0]])
  else:
   assert text(ecx)=='@(#)except.c:1.25'and edx==error;errors.append(edx);events.append([child,edx]);terminal.append('diagnostic-stop');m.emu_stop();return
  events.append([child,*detail]);m.reg_write(UC_X86_REG_EAX,value);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+4+cleanup)
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if side else row['start'],0,count=100000)
 if error:assert terminal==['diagnostic-stop']and errors==[error]and not inserted
 else:
  assert terminal==['return']and len(inserted)==1 and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)
  assert word(word(return_cell)+20)==newlabel
 order=[];cursor=word(function+16)
 while cursor:
  assert len(order)<100 and expressions<=cursor<expressions+16*len(spec)or cursor==alternate_head
  if cursor==alternate_head:order.append('alternate-head');break
  index=(cursor-expressions)//16;assert cursor==expressions+16*index and index not in order;order.append(index);cursor=word(cursor)
 if expected_order is not None:assert order==expected_order,(mutation,order,expected_order)
 if mutation=='label-tree':assert word(alternate_label+40)==0xffffffff and word(labels[0]+40)==struct.unpack_from('<I',memory,labels[0]+40-0x400000)[0]
 if mutation=='allocated-node':assert word(nodes+20)==scopes and word(alternate_node+20)==alternate_label
 if mutation=='pop-right':assert word(depth_cell)==(depth+1)&0xffffffff
 return dict(terminal=terminal[0],events=events,released=released,depth=word(depth_cell),return_label=word(return_cell),expression_order=order,memory_sha256=hashlib.sha256(u.mem_read(0x400000,len(memory))).hexdigest())
rows=[]
def case(label,spec,depth=0,mutation=None,order=None,error=None):
 a=run(original,False,spec,depth,mutation,order,error);b=run(canonical,True,spec,depth,mutation,order,error);assert a==b,(label,a,b);rows.append(dict(case=label,input=spec,initial_finally_depth=depth,mutation=mutation,result=a))
for depth in [0,7,0xffffffff]:
 case('label outside scope',[node(0x29,0),node(0x47)],depth)
 for kind in [0x8d,0x8e]:case('one scope',[node(0x8f),node(0x29,0),node(kind),node(0x29,1),node(0x90),node(0x29,2),node(0x47)],depth)
 case('nested except in finally',[node(0x8f),node(0x8e),node(0x29,0),node(0x8f),node(0x8d),node(0x29,1),node(0x90),node(0x29,2),node(0x90),node(0x29,3),node(0x47)],depth)
 case('last return marker',[node(0x47),node(0x41),node(0x47)],depth)
for body_count in [0,1,3]:
 spec=[node(0x3a,0,0x47),node(0x3a,1),node(0x41),node(0x29,0,0x47)]+[node(0x32)for _ in range(body_count)]+[node(0x3a,1),node(0x47)];case('relocate special handler',spec,order=[0,3,*range(4,4+body_count),1,2,5+body_count])
case('ordinary jump skips relocation',[node(0x3a,0,0x46),node(0x47)])
case('special target absent',[node(0x3a,0,0x47),node(0x47)])
case('other marked label first',[node(0x3a,0,0x47),node(0x3a,1),node(0x41),node(0x29,2,0x47),node(0x32),node(0x3a,1),node(0x47)])
case('two moves reuse cached predecessor',[node(0x3a,0,0x47),node(0x3a,1),node(0x41),node(0x3a,2,0x47),node(0x3a,3),node(0x41),node(0x29,0,0x47),node(0x32),node(0x3a,1),node(0x29,2,0x47),node(0x32),node(0x3a,3),node(0x47)],order=[0,6,7,1,2,3,9,10,4,5,12])
case('bad saved branch opcode',[node(0x3a,0,0x47),node(0x32,1),node(0x41),node(0x29,0,0x47),node(0x32),node(0x3a,1),node(0x47)],error=0x13c)
case('different branch pointer identity',[node(0x3a,0,0x47),node(0x3a,1),node(0x41),node(0x29,0,0x47),node(0x32),node(0x3a,2),node(0x47)],error=0x13d)
case('label reloads Expression tree after query',[node(0x29,0),node(0x47)],mutation='label-tree')
case('current next read after query',[node(0x29,0),node(0x8e),node(0x47)],mutation='query-next',order=[0,2])
case('allocation retains original cached node',[node(0x8f),node(0x47)],mutation='allocated-node')
case('post-pop reloads node scope pointer',[node(0x8f),node(0x8e),node(0x90),node(0x47)],mutation='pop-right')
for mutation in ['return-global','return-head','return-tree']:case('fresh final constructor outputs',[node(0x47)],mutation=mutation)
Path('cases/seh-collect-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=rejected,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole550B original/canonical with accepted and named-unreconstructed child bodies mocked: scope nesting/label IDs/typed kind transitions and wrappingfinallydepth, returnmarker selection, marked handler movement0/1/3 bodies and2 moves using cached predecessor, ordinary/missing targets, precise fatal13c/13d stops with source identity. Full synthetic object bytes/events/release identities/linked order/RET0/nonvolatile agree; voidEAX unclaimed. Targeted child mutations distinguish fresh Expression.tree on labelquery, cachedNode onallocation, next links, postpop Node.right and final globalNode/Function head/marker tree reloads. Malformed-list assumptions remain original, no syntheticcursor initialization or repair. Mocked mutations are structural controls, not captured publicIR or real childfailure equivalence. Actual55-expression input inventory separately bound; no real splice/diagnostic execution claim or childcredit.'),indent=2)+'\n');print(len(rows),'whole scope-discovery controls and12 actual ABI negatives pass')
