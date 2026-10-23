set pagination off
set confirm off
set disable-randomization off
break recon_stat
run >/root/.ai-tmp/vc1.0-c1/automatic-pch-stat-proof/stdout 2>&1
python
import gdb,json,os,struct
name=gdb.parse_and_eval('name').string();out=int(gdb.parse_and_eval('out'))
state={'argument':name,'buffer_size':36,'expected_mtime':1792736790}
assert name.lower()=='case.h',name
end
finish
python
result=int(gdb.parse_and_eval('$eax'));raw=bytes(gdb.selected_inferior().read_memory(out,36))
state.update(result=result,status_hex=raw.hex(),mtime=struct.unpack_from('<i',raw,28)[0])
assert result==0 and state['mtime']==state['expected_mtime'],state
with open('/root/.ai-tmp/vc1.0-c1/automatic-pch-stat-proof/state.json','w')as f:json.dump(state,f,indent=2)
end
continue
