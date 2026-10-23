set pagination off
set confirm off
set disable-randomization off
break *browser_index_overflow
run >/root/.ai-tmp/msvc2/vc1.0/c1/validation/browser-limit-return/stdout 2>&1
python
import gdb,json,struct,hashlib
from pathlib import Path
def u(x):return int(gdb.parse_and_eval(x)) & 0xffffffff
inf=gdb.selected_inferior();sp=u('$esp');ret=struct.unpack('<I',bytes(inf.read_memory(sp,4)))[0]
def snapshot():
    row={n:u(n)for n in ['next_symbol_index','browser_symbol_limit','options.option_FR','browser_filename','browser_output']}
    ptr,count,base=struct.unpack('<III',bytes(inf.read_memory(row['browser_output'],12)))
    row['pending']=bytes(inf.read_memory(base,ptr-base)).hex()
    row['registers']={r:u('$'+r)for r in ['ebx','esi','edi','ebp']}
    row['esp']=u('$esp');row['scanner']=u('scanner_flags');row['parser']=u('parser_flags')
    return row
before=snapshot()
Path('/root/.ai-tmp/msvc2/vc1.0/c1/validation/browser-limit-return/staged-browser-prefix.sbt').write_bytes(Path('registry.sbt').read_bytes())
gdb.Breakpoint('*'+str(ret),temporary=True)
end
delete 1
continue
python
row=dict(before=before,after=snapshot(),caller=gdb.newest_frame().name())
a=row['before'];z=row['after']
assert z['esp']-a['esp']==4 and z['registers']==a['registers'] and z['scanner']==a['scanner'] and z['parser']==a['parser']
assert a['options.option_FR']==a['browser_filename']!=0 and z['options.option_FR']==z['browser_filename']==0
assert a['browser_output']==z['browser_output']!=0 and a['next_symbol_index']==z['next_symbol_index']==a['browser_symbol_limit']==z['browser_symbol_limit']==65526
with open('/root/.ai-tmp/msvc2/vc1.0/c1/validation/browser-limit-return/registers.json','w')as out:json.dump(row,out,indent=2)
end
