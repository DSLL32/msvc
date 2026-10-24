"""Read-only owner/range proof at the actual first named call completion."""
import gdb,json,struct
from pathlib import Path
out=Path(gdb.convenience_variable('metadata_directory').string())
u=lambda e:int(gdb.parse_and_eval(e))&0xffffffff
read=lambda a,n:bytes(gdb.selected_inferior().read_memory(a,n))if n else b''
words=lambda a,n:list(struct.unpack('<'+'I'*n,read(a,4*n)))
def text(address):
    value=bytearray()
    for offset in range(260):
        byte=read(address+offset,1)[0]
        if not byte:return value.decode('ascii')
        value.append(byte)
    raise AssertionError('unterminated bounded metadata string')
def find_symbol(name):
    scope=gdb.parse_and_eval('global_scope').dereference();mask=int(scope['bucket_mask']);assert 0<=mask<=1023
    for bucket in range(mask+1):
        entry=scope['buckets'][bucket];steps=0
        while int(entry):
            steps+=1;assert steps<=1024;value=entry.dereference()
            if int(value['name'])and value['name'].string()==name:return int(entry)&0xffffffff
            entry=value['next']
    raise AssertionError('real parsed symbol absent: '+name)

gdb.execute('set pagination off');gdb.execute('set confirm off');gdb.execute('set disable-randomization off')
bp=gdb.Breakpoint('*finish_call_arguments',temporary=True,internal=True)
gdb.execute('run >'+str(out/'stdout')+' 2>&1')
assert u('$eip')==u('(unsigned)finish_call_arguments')
registers={r:u('$'+r)for r in ['eip','esp','eax','ecx','edx','ebx','esi','edi','ebp','fctrl']}
frame=words(registers['esp'],2);call=words(frame[1],5);context=u('(unsigned)call_context');ctx=words(context,8)
assert ctx[6]==3 and text(ctx[3])=='metadata_sink'and ctx[2]==find_symbol('metadata_sink')
assert call[0]&65535==0x0272 and call[4]
page_count=u('permanent_page_count');assert 1<=page_count<=511;pages=[]
for i in range(page_count):
    start,marks=words(u('(unsigned)permanent_pages')+8*i,2);assert start and not start&4095 and start<=0xffffffff-0xeffc and marks and words(marks,1)[0]==0xeffc
    assert not any(start<p['start']+0xeffc and start+0xeffc>p['start']for p in pages)
    pages.append(dict(index=i,start=start,bytes=0xeffc,marks=marks,bits=0xeffc))
def permanent(a,n):return any(p['start']<=a and a-p['start']<=0xeffc and n<=0xeffc-(a-p['start'])for p in pages)
block_bytes=u('compiler_arena.block_bytes');assert block_bytes==0x3ff4;pool=words(u('(unsigned)&compiler_arena.pools[2]'),5);assert pool[0]and pool[1]and pool[3]<=block_bytes
blocks=[];at=pool[0]
while at:
    assert len(blocks)<32 and at<=0xffffffff-block_bytes-12 and all(b['address']!=at for b in blocks)
    next,recorded=words(at,2);assert recorded<=block_bytes;used=pool[2]-at-8 if at==pool[1]else recorded;assert 0<=used<=block_bytes
    if at==pool[1]:assert next==0 and used+pool[3]==block_bytes
    blocks.append(dict(address=at,allocation=block_bytes+12,data=at+8,used=used,next=next,last=int(at==pool[1])));at=next
assert blocks[-1]['address']==pool[1]
def temporary(a,n):
    assert a and a<=0xffffffff-n
    assert any(a>=b['data']and a-b['data']<=b['used']and n<=b['used']-(a-b['data'])for b in blocks)
    assert all(not(a<p['start']+p['bytes']and a+n>p['start'])for p in pages)
    return True
assert temporary(frame[1],20)and temporary(context,36)
arguments=[];at=call[4]
for i in range(3):
    assert temporary(at,20);w=words(at,5);assert w[0]&65535==0x025f and permanent(w[1],8)and w[3]
    arguments.append(dict(index=i,address=at,type=w[1],next=w[2],expression=w[3],previous=w[4],owned_bytes=20,outside_all_pages=True));at=w[4]
assert at==0
for i,a in enumerate(arguments):assert a['next']==arguments[(i+2)%3]['address']
vectors={}
for name in ['pch_saved_defines','pch_saved_include_directories']:
    address=u('(unsigned)'+name);assert permanent(address,12);w=words(address,3);count,capacity=struct.unpack('<hh',read(address,4));assert 3<=count<=16 and count<=capacity<=256 and w[1]==0 and permanent(w[2],4*count)
    values=words(w[2],count);assert all(permanent(a,1)for a in values);vectors[name]=dict(address=address,count=count,capacity=capacity,arena=w[1],items=w[2],values=values,names=[text(a)for a in values])
browser=u('(unsigned)browser_output');assert browser;stream=words(browser,4);assert stream[2]and stream[0]>=stream[2]and stream[0]-stream[2]<=4096 and stream[3]
template=words(u('(unsigned)&browser_line_template'),6);buffers={}
for name in ['expression_file','symbol_file','line_file','global_file','debug_file','pch_file','browser_output']:
    stream_address=u('(unsigned)'+name)
    if not stream_address:buffers[name]=None;continue
    p,c,b=words(stream_address,3);assert p>=b and p-b<=65536;buffers[name]=read(b,p-b).hex()
result=dict(status='natural-context-proved',registers=registers,frame=frame,call=frame[1],context=context,symbol=ctx[2],callee=text(ctx[3]),argument_count=3,pages=pages,arena_blocks=blocks,arguments=arguments,vectors=vectors,browser=dict(address=browser,pending=stream[0]-stream[2],flags=stream[3],template=template),buffers=buffers,inferior_pid=gdb.selected_inferior().pid)
(out/'state.json').write_text(json.dumps(result,indent=2)+'\n');gdb.execute('kill')
