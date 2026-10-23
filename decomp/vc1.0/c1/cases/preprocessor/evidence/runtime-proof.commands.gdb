set pagination off
set confirm off
set disable-randomization off
break recon_dup
break recon_freopen
run >/root/.ai-tmp/vc1.0-c1/pp-P-runtime-proof/stdout 2>/root/.ai-tmp/vc1.0-c1/pp-P-runtime-proof/stderr
python
import gdb,json,os
source=int(gdb.parse_and_eval('source'));flags=int(gdb.parse_and_eval('recon_osfile[1]'))
state={'dup':{'source':source,'source_flags':flags}}
assert source==1
end
finish
python
result=int(gdb.parse_and_eval('$eax'))
state['dup'].update(result=result,result_flags=int(gdb.parse_and_eval('recon_osfile[%d]'%result)))
assert result==3 and state['dup']['result_flags']==flags,state
end
continue
python
name=gdb.parse_and_eval('name').string();mode=gdb.parse_and_eval('mode').string();stream=int(gdb.parse_and_eval('stream'))
state['freopen']={'filename':name,'mode':mode,'stream':stream,'before_exists':os.path.exists(name)}
assert name=='test.i' and mode=='wt+' and not state['freopen']['before_exists']
end
finish
python
returned=int(gdb.parse_and_eval('$eax'))
state['freopen'].update(result=returned,after_exists=os.path.isfile(name),descriptor=int(gdb.parse_and_eval('((struct ShimFILE *)%d)->_file'%stream)))
assert returned==stream and state['freopen']['after_exists'] and state['freopen']['descriptor']==1,state
with open('/root/.ai-tmp/vc1.0-c1/pp-P-runtime-proof/state.json','w')as f:json.dump(state,f,indent=2)
end
continue
