import gdb,json
from pathlib import Path
f=gdb.newest_frame();assert f.name()=='recon_mbsstr'
v=lambda e:int(gdb.parse_and_eval(e))
def text(p):
 out=bytearray()
 for i in range(4096):
  b=bytes(gdb.selected_inferior().read_memory(p+i,1))
  if b==b'\0':return out.decode('ascii')
  out+=b
 raise AssertionError('unterminated')
r={'kind':'native-only read-only unsupported-API input observation; not acceptance','text':text(int(f.read_var('text'))),'needle':text(int(f.read_var('needle'))),'caller':f.older().name(),'state':{n:v(e)for n,e in [('errno','(int)errno'),('doserrno','recon_doserrno'),('last_error','last_error'),('mb_codepage','recon_mb_codepage'),('mb_cur_max','recon_mb_cur_max'),('lc_codepage','recon_lc_codepage'),('lc_ctype','recon_lc_handle[2]'),('x87_control_word','$fctrl')]}}
Path('mbsstr-input.json').write_text(json.dumps(r,indent=2)+'\n')
