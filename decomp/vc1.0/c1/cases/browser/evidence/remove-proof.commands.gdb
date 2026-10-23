set pagination off
set confirm off
set disable-randomization off
break recon_remove
run >/root/.ai-tmp/vc1.0-c1/browser-remove-proof/stdout 2>/root/.ai-tmp/vc1.0-c1/browser-remove-proof/stderr
python
import gdb,json,os
name=gdb.parse_and_eval('name').string()
state={'filename':name,'before_exists':os.path.isfile(name),'before_bytes':os.path.getsize(name)}
end
finish
python
state['return']=int(gdb.parse_and_eval('$eax'))
state['after_exists']=os.path.exists(name)
assert name=='test.sbt' and state['before_exists'] and state['return']==0 and not state['after_exists'],state
with open('/root/.ai-tmp/vc1.0-c1/browser-remove-proof/state.json','w')as f:json.dump(state,f,indent=2)
end
continue
