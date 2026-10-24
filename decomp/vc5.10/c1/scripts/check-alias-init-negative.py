#!/usr/bin/env python3
"""Affected alias-root allocation, exact state delta and real ABI controls."""
import argparse,copy,importlib.util,json,re,tempfile,time
from pathlib import Path
import command_line_state as command

def main():
 p=argparse.ArgumentParser();p.add_argument('original',type=Path);p.add_argument('native',type=Path);a=p.parse_args();started=time.perf_counter()
 spec=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'));trial=importlib.util.module_from_spec(spec);spec.loader.exec_module(trial);trial.check(a.original,a.native)
 state=json.loads((a.native/'state.json').read_text());candidate=json.loads((a.original/'candidate.json').read_text());fields=json.loads((Path(__file__).resolve().parents[1]/candidate['field_map']).read_text())
 events=(a.original/'events.txt').read_text();module=int(re.findall(r'^module name=c1n32_l.dll actual=([0-9a-f]+)',events,re.M)[0],16);sequence=int(re.findall(r'^trap seq=(\d+)',events,re.M)[-1])
 labels=['cursor-not-advanced','cursor-wrong-width','committed-limit-changed','published-new-cursor','root-head-write','root-size','neighbor-arena','neighbor-pool','saved-state-write','snapshot-option-write','formal-omission','invented-argument','caller-name','caller-argument','stack-cleanup','callee-save','error-state','fpu-status','fpu-tag','missing-entry','entry-formal','entry-receiver','entry-return','original-root-write','native-root-write','original-committed-write','native-committed-write','original-bss-write','original-snapshot-write','original-caller-return','root-extent-map']
 with tempfile.TemporaryDirectory(prefix='alias-init-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1')as temporary:
  root=Path(temporary);o=root/'original';n=root/'native'
  for source,destination in ((a.original,o),(a.native,n)):
   destination.mkdir()
   for file in source.iterdir():
    if file.is_file():(destination/file.name).symlink_to(file.resolve())
  tracked=[n/'alias-initializer-entries.json',o/'events.txt',o/('compiler-bss-%03d.bin'%sequence),o/('compiler-snapshot-%03d.bin'%sequence)]
  tracked += [side/name for side in (o,n)for name in ('include-alias-root-000.bin','include-alias-committed-000.bin')]
  saved={file:file.read_bytes()for file in tracked}
  for label in labels:
   for file,raw in saved.items():
    if file.is_symlink():file.unlink()
    file.write_bytes(raw)
   value=copy.deepcopy(state);mapping=copy.deepcopy(fields);bss=value['compiler_state']['bss']
   if label=='cursor-not-advanced':bss['arenas'][0]['cursor']-=4
   elif label=='cursor-wrong-width':bss['arenas'][0]['cursor']+=4
   elif label=='committed-limit-changed':bss['arenas'][0]['committed_limit']+=4
   elif label=='published-new-cursor':value['compiler_state']['option_state']['initialized']['include_aliases']+=4
   elif label=='root-head-write':value['alias_root']['head']=1
   elif label=='root-size':value['alias_root']['size']=8
   elif label=='neighbor-arena':bss['arenas'][1]['reset_value']=1
   elif label=='neighbor-pool':bss['pools'][0]['remaining']+=1
   elif label=='saved-state-write':value['compiler_state']['snapshot']['compiler_flags']^=1
   elif label=='snapshot-option-write':value['compiler_state']['option_state']['snapshot']['packing_alignment']^=1
   elif label=='formal-omission':value['formal_inventory']=[dict(name='invented',type='int')]
   elif label=='invented-argument':value['args']['invented']=1
   elif label=='caller-name':value['caller']['symbol']='c1_main'
   elif label=='caller-argument':value['caller']['args']['argc']+=1
   elif label=='stack-cleanup':value['alias_esp']+=4
   elif label=='callee-save':value['alias_preserved']['esi']^=4
   elif label=='error-state':value['win32_last_error']+=1
   elif label=='fpu-status':value['command_line_fp']['sw']^=0x20
   elif label=='fpu-tag':value['command_line_fp']['tag']^=3
   elif label in ('missing-entry','entry-formal','entry-receiver','entry-return'):
    file=n/'alias-initializer-entries.json';rows=json.loads(file.read_text())
    if label=='missing-entry':rows=[]
    elif label=='entry-formal':rows[0]['formal_inventory']=[dict(name='invented',type='int')]
    elif label=='entry-receiver':rows[0]['compiler_state']['bss']['arenas'][0]['cursor']+=4
    else:rows[0]['return']+=1
    file.write_text(json.dumps(rows))
   elif label=='root-extent-map':mapping['include_aliases']['root_bytes']=8
   elif label=='original-caller-return':
    file=o/'events.txt';text=file.read_text();pattern=r'(^trap seq=%d rva=00012e1c .*stack=)([0-9a-f]+)'%sequence
    changed,count=re.subn(pattern,lambda m:m[1]+'%08x'%(int(m[2],16)+1),text,flags=re.M);assert count==1;file.write_text(changed)
   else:
    side=o if label.startswith('original-')else n
    name='include-alias-root-000.bin'if 'root-'in label else 'include-alias-committed-000.bin'if 'committed-'in label else 'compiler-bss-%03d.bin'%sequence if 'bss-'in label else 'compiler-snapshot-%03d.bin'%sequence
    file=side/name;raw=bytearray(file.read_bytes());raw[0]^=1;file.write_bytes(raw)
   try:
    entry,at=command.boundary(o,n,value,sequence,module)
    command.validate_alias_initializer(o,n,mapping,entry,value,at,sequence,module,candidate)
   except(AssertionError,KeyError,ValueError,IndexError):continue
   raise AssertionError('accepted '+label)
 print(json.dumps(dict(status='PASS',fresh_target_controls=len(labels),rejected=labels,seconds=time.perf_counter()-started,scope='Fresh full positive plus affected root identity/defined extent/exact state delta/ABI/x87 guards; unchanged core transport controls reused.'),indent=2))
if __name__=='__main__':main()
