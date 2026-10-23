import gdb,json
from pathlib import Path
frame=gdb.newest_frame()
info={'function':frame.name(),'pc':int(gdb.parse_and_eval('$pc')),'signal':gdb.execute('info program',to_string=True),'backtrace':gdb.execute('bt',to_string=True),'trace':trace,'globals':{}}
pid=gdb.selected_inferior().pid
actual_args=Path('/proc/%d/cmdline'%pid).read_bytes().rstrip(b'\0').split(b'\0')
actual_env=Path('/proc/%d/environ'%pid).read_bytes().split(b'\0')
Path('/root/.ai-tmp/msvc2/vc5.10/c2/firstcase/precision-native/native-maps.txt').write_text(Path('/proc/%d/maps'%pid).read_text())
gdb.execute('info registers eip')
gdb.execute('info symbol $pc')
gdb.execute('x/1ib $pc-1')
gdb.execute('info program')
info['arguments']=[v.decode()for v in actual_args[1:]]
flags=[v[14:].decode()for v in actual_env if v.startswith(b'MSC_CMD_FLAGS=')]
info['MSC_CMD_FLAGS']=flags[0]if flags else None
records=[r for r in [{'name': '_c2_math_error', 'native': 'c2_math_error', 'start': 4484380, 'end': 4484383, 'abi': 'cdecl, one pointer parameter', 'kind': 'crt-source'}, {'name': '_c2_main', 'native': 'c2_main', 'start': 4532400, 'end': 4532715, 'code_chunks': [[4670673, 4670813]], 'abi': 'cdecl, int argc and two pointer vectors', 'kind': 'application', 'state_policy': 'main-arguments-and-initial-state', 'state': [{'original': '0x499708', 'native': '(unsigned long)c2_onexit_end'}, {'original': '0x4996d4', 'native': '(unsigned long)c2_onexit_begin'}, {'original': '0x49975c', 'native': 'c2_fmode'}, {'original': '0x4980d4', 'native': 'c2_commode'}, {'original': '0x499718', 'native': 'c2_adjust_fdiv'}, {'original': '0x499694', 'native': 'c2_default_math_error'}, {'original': '0x4980d0', 'native': 'c2_new_mode'}, {'original': '0x4980cc', 'native': 'c2_wildcards'}, {'original': '0x7803fa88', 'native': 'recon_fmode'}, {'original': '0x7803fa94', 'native': 'recon_commode'}, {'original': '0x7803f928', 'native': 'recon_adjust_fdiv'}, {'original': '0x7803fa90', 'native': 'recon_crt_app_type'}, {'original': '0x7803f198', 'native': 'recon_crt_newmode'}]}] if r['native']==frame.name()]
if len(records)==1:
    for view in records[0].get('state',[]):
        info['globals'][view['original']]=int(gdb.parse_and_eval(view['native'])) & 0xffffffff
    if records[0].get('state_policy')=='main-arguments-and-initial-state':
        def vector(expression):
            pointer=gdb.parse_and_eval(expression);result=[];i=0
            while int(pointer[i]):
                result.append(pointer[i].string());i+=1
                assert i<4096
            return result
        info['main']={'argc':int(gdb.parse_and_eval('argc')),'argv':vector('argv'),'environment':vector('environment'),
                      'initenv_alias':int(gdb.parse_and_eval('recon_crt_initenv == environment')),
                      'x87_control':int(gdb.parse_and_eval('$fctrl'))}
open('/root/.ai-tmp/msvc2/vc5.10/c2/firstcase/precision-native/state.json','w').write(json.dumps(info,indent=2)+'\n')
