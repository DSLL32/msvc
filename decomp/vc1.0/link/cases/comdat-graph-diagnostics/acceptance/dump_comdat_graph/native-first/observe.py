
import gdb,json,struct
from pathlib import Path
out=Path('/root/.ai-tmp/msvc2/vc1.0/link/comdat-gdb-2bay6cp_/state.json');events=[];terminal=[];frames=[];returns={}
def val(x):return int(gdb.parse_and_eval(x))&0xffffffff
def mem(p,n):return bytes(gdb.selected_inferior().read_memory(p,n))
def words(p,n):return list(struct.unpack('<'+'I'*n,mem(p,n*4)))
def string(p):
    result=b''
    for i in range(256):
        c=mem(p+i,1)
        if c==b'\0':return result
        result+=c
    raise AssertionError('unterminated string')
def persist():out.write_text(json.dumps(dict(events=events,terminal=terminal),indent=2)+'\n')
def snap(event,name):
    esp=val('$esp');count=1 if event=='entry' else 0;fmt=value=b''
    if name=='debug_printf' and event=='entry':
        fmt=string(words(esp,2)[1])
        if b'%' in fmt:
            assert fmt in [b'%s',b'|sec=%.8s',b'|grp=%.8s',b'|con=0x%p',b'|mod=%s',b'|%s()'];count=2
            if fmt!=b'|con=0x%p':value=string(words(esp,3)[2])
    stream=words(val('&recon_iob[1]'),8);used=stream[0]-stream[2];assert 0<=used<=4096
    return dict(event=event,name=name,eip=val('$eip'),esp=esp,eax=val('$eax'),ebx=val('$ebx'),esi=val('$esi'),edi=val('$edi'),ebp=val('$ebp'),arguments=words(esp,count+1),format=fmt.hex(),string_value=value.hex(),input_base=val('comdat_test_region'),region=mem(val('comdat_test_region'),4096).hex(),stdout_file=stream,pending=mem(stream[2],used).hex() if used else '',errors=[val('*comdat_test_errno'),val('recon_doserrno'),val("'win32.c'::last_error")],debug_flags=mem(val('&debug_flags[0]'),100).hex(),debug_log=val('debug_log') if gdb.lookup_global_symbol('debug_log') else 0,mb_cur_max=val('recon_mb_cur_max'),ctype=mem(val('recon_pctype')-2,514).hex(),return_text=string(val('$eax')).hex() if name=='comdat_usage_label' and event=='return' else '')
class Return(gdb.Breakpoint):
    def stop(self):
        assert frames and frames[-1]['arguments'][0]==val('$eip'),'return without actual current entry'
        before=frames.pop();after=snap('return',before['name']);events.append(after);persist()
        assert after['esp']==before['esp']+4
        assert all(after[r]==before[r] for r in ['ebx','esi','edi','ebp'])
        return False
class Entry(gdb.Breakpoint):
    def __init__(self,name):self.name=name;super().__init__('*'+name,internal=True)
    def stop(self):
        entry=snap('entry',self.name);events.append(entry);persist()
        if self.name not in []:events.append(dict(event='frontier',name=self.name,eip=val('$eip')));persist();return True
        frames.append(entry);address=entry['arguments'][0]
        if address not in returns:returns[address]=Return('*'+str(address),internal=True)
        return False
def exited(event):terminal.append(dict(event='exit',code=getattr(event,'exit_code',None)));persist()
gdb.events.exited.connect(exited)
gdb.execute('set pagination off');gdb.execute('set confirm off')
for name in ['dump_comdat_graph', 'debug_printf', 'print_comdat_node', 'comdat_usage_label', 'print_comdat_edge']:Entry(name)
gdb.execute('run '+'/root/msvc2/decomp/vc1.0/link/cases/comdat-graph-diagnostics/inputs/graph.bin'+' >'+'/root/.ai-tmp/msvc2/vc1.0/link/comdat-gdb-2bay6cp_/stdout.bin'+' 2>'+'/root/.ai-tmp/msvc2/vc1.0/link/comdat-gdb-2bay6cp_/stderr.bin')
assert events and (terminal or events[-1]['event']=='frontier')
if terminal:assert terminal==[dict(event='exit',code=0)] and not frames
persist()
