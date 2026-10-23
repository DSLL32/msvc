set pagination off
set confirm off
run
bt
python
import gdb,json
frame=gdb.newest_frame()
args={name:int(gdb.parse_and_eval('(unsigned int)'+name)) for name in ('module','reason','reserved')}
value={'symbol':frame.name(),'pc':int(gdb.parse_and_eval('$pc')),'args':args}
identity=gdb.parse_and_eval('*c1_host_identity')
value['module_identity']={key:identity[key].string() for key in ('name','path','sha256')}
value['module_identity']['flags']=int(identity['flags'])
value['module_token']=int(gdb.parse_and_eval('(unsigned int)c1_host_module'))
value['crt_attached']=int(gdb.parse_and_eval('c1_crt_attached'))
value['raw_dllmain']=int(gdb.parse_and_eval('(unsigned int)c1_raw_dllmain'))
value['win32_last_error']=int(gdb.parse_and_eval('last_error'))
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-trial-entry-003/state.json','w').write(json.dumps(value,indent=2)+'\n')
end
quit
