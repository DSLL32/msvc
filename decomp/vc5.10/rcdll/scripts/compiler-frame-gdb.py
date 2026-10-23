"""Observe Linux compiler-owned arrays; never manufacture a Win32 helper call."""
import gdb
import hashlib
import json
import os
from pathlib import Path
output=Path(os.environ['RCDLL_NATIVE_SNAPSHOT'])
try:
    frame=gdb.newest_frame();caller=frame.older()
    assert frame.name()=='recon_IsDBCSLeadByteEx'and caller.name()=='rc_prepare_input'
    gdb.execute('printf "RCDLL_COMPILER_FRAME_PC=%08x\\n", $pc');gdb.execute('bt')
    sp=int(frame.read_register('esp'))&0xffffffff;bp=int(caller.read_register('ebp'))&0xffffffff
    assert 0<bp-sp<65536
    objects=[]
    for name,size in [('escaped',520),('directive',8196),('line',8354)]:
        value=caller.read_var(name);address=int(value.address)&0xffffffff
        assert value.type.sizeof==size and sp<=address<address+size<=bp
        objects.append(dict(name=name,address=address,bytes=size))
    ordered=sorted(objects,key=lambda row:row['address'])
    assert all(a['address']+a['bytes']<=b['address']for a,b in zip(ordered,ordered[1:]))
    maps=Path('/proc/%d/maps'%gdb.selected_inferior().pid).read_text().splitlines()
    stack=[line for line in maps if line.endswith('[stack]')];assert len(stack)==1
    low,high=[int(value,16)for value in stack[0].split()[0].split('-')]
    assert low<=sp<bp<high
    data=dict(kind='native-compiler-frame',callee=frame.name(),caller=caller.name(),native_esp=sp,parent_ebp=bp,objects=objects,stack_mapping=stack[0],raw_pc=int(frame.pc())&0xffffffff,caller_pc=int(caller.pc())&0xffffffff,codepage=int(frame.read_var('page')),byte=int(frame.read_var('byte')),path=caller.read_var('path').string(),config_sha256=hashlib.sha256(Path(os.environ['RCDLL_NATIVE_CONFIG']).read_bytes()).hexdigest(),scope='Actual Linux compiler-owned frame and typed array bounds only; no Win32 helper ABI or case-completion claim')
except Exception as error:data=dict(kind='error',error=str(error))
output.write_text(json.dumps(data,indent=2)+'\n')
