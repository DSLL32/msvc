#!/usr/bin/env python3
"""Focused fresh repeated-call/ABI/state controls; no process replays."""
import argparse,copy,json,tempfile,time
from pathlib import Path
from option_action_state import validate_sequence
from provenance import sha

class Overlay:
 def __init__(self,base,files):self.base,self.files=base,files
 def __truediv__(self,name):return self.files.get(name,self.base/name)

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('original',type=Path);p.add_argument('native',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 c=json.loads((a.original/'candidate.json').read_text());started=time.perf_counter();validate_sequence(a.original,a.native,c);controls=[]
 def native_json(label,name,change):
  value=json.loads((a.native/name).read_text());change(value)
  with tempfile.TemporaryDirectory(prefix='action-control-',dir=a.native.parent)as tmp:
   path=Path(tmp)/name;path.write_text(json.dumps(value));n=Overlay(a.native,{name:path})
   try:validate_sequence(a.original,n,c)
   except AssertionError:pass
   else:raise AssertionError('accepted '+label)
  controls.append(dict(scope='native',control=label))
 def raw(label,name,offset):
  value=bytearray((a.original/name).read_bytes());value[offset]^=1
  with tempfile.TemporaryDirectory(prefix='action-control-',dir=a.original.parent)as tmp:
   path=Path(tmp)/name;path.write_bytes(value);o=Overlay(a.original,{name:path})
   try:validate_sequence(o,a.native,c)
   except AssertionError:pass
   else:raise AssertionError('accepted '+label)
  controls.append(dict(scope='original',control=label))
 for label,name,change in [
  ('missing-third-action','option-action-entries.json',lambda x:x.pop()),
  ('second-action-reused-index','option-action-returns.json',lambda x:x[1].__setitem__('index',0)),
  ('action-formal-omission','option-action-entries.json',lambda x:x[1]['formal_inventory'].pop()),
  ('action-copy-mode','option-action-entries.json',lambda x:x[0]['args'].__setitem__('copy',1)),
  ('action-wrong-rule','option-action-entries.json',lambda x:x[1]['args'].__setitem__('rule',x[0]['args']['rule'])),
  ('action-borrowed-input','option-action-entries.json',lambda x:x[1]['args'].__setitem__('value',x[0]['args']['value'])),
  ('action-value-NUL','option-action-entries.json',lambda x:x[1]['value_bytes'].__setitem__(-1,1)),
  ('action-RET-cleanup','option-action-returns.json',lambda x:x[0].__setitem__('esp',0)),
  ('action-callee-save','option-action-returns.json',lambda x:x[1]['preserved'].__setitem__('ebx',0)),
  ('action-IL-store','option-action-returns.json',lambda x:x[0]['compiler_state']['option_state']['initialized'].__setitem__('il_prefix',0)),
  ('action-source-borrowed-store','option-action-returns.json',lambda x:x[1]['compiler_state']['option_state']['bss'].__setitem__('source_file',0)),
  ('action-neighbor','option-action-returns.json',lambda x:x[1]['compiler_state']['bss'].__setitem__('debug_break',1)),
  ('outer-call-omission','argument-cursor-entries.json',lambda x:x.pop()),
  ('outer-cursor-step','argument-cursor-returns.json',lambda x:x[2]['compiler_state']['bss'].__setitem__('command_argc',0)),
  ('inner-callback-formal','option-next-entries.json',lambda x:x[1]['formal_inventory'].append(dict(name='fake',type='int'))),
  ('inner-callback-identity','option-next-entries.json',lambda x:x[1]['caller']['args'].__setitem__('next',0)),
  ('inner-return-borrow','option-next-returns.json',lambda x:x[2].__setitem__('eax',0)),
  ('matcher-return-result','option-match-returns.json',lambda x:x[1].__setitem__('eax',0)),
  ('matcher-next-current-argument','option-next-entries.json',lambda x:x[2]['compiler_state']['bss'].__setitem__('current_argument',0)),
  ('signature-count-omission','signature-call-entries.json',lambda x:x.pop()),
  ('signature-reused-index','signature-call-entries.json',lambda x:x[-1].__setitem__('index',0)),
  ('signature-formal-omission','signature-call-entries.json',lambda x:x[-1]['formal_inventory'].pop()),
  ('signature-borrowed-input','signature-call-entries.json',lambda x:x[-1]['args'].__setitem__('bytes',0)),
  ('signature-length','signature-call-entries.json',lambda x:x[-1]['args'].__setitem__('byte_count',1)),
  ('signature-seed-chain','signature-call-entries.json',lambda x:x[-1]['args'].__setitem__('signature',0)),
  ('signature-return-value','signature-call-returns.json',lambda x:x[-1].__setitem__('eax',0)),
  ('signature-third-private-slot','signature-call-returns.json',lambda x:x[-1]['argument_slots_after'].__setitem__(2,0)),
  ('signature-FP','signature-call-returns.json',lambda x:x[-1]['fp'].__setitem__('sw',0)),
  ('signature-error','signature-call-returns.json',lambda x:x[-1]['errors'].__setitem__(0,1)),
  ('final-module-token','state.json',lambda x:x.__setitem__('module_token',x['module_token']^1)),
  ('final-module-hash','state.json',lambda x:x['module_identity'].__setitem__('sha256','0'*64)),
  ('final-Invoke-BOOL','state.json',lambda x:x['invocation']['args'].__setitem__('mode',0)),
  ('final-attach-count','state.json',lambda x:x.__setitem__('crt_attached',0)),
  ('final-raw-hook','state.json',lambda x:x.__setitem__('raw_dllmain',1)),
  ('final-LastError','state.json',lambda x:x.__setitem__('win32_last_error',1)),
  ('final-x87-CW','state.json',lambda x:x.__setitem__('x87_control',0x37f)),
  ('final-pass-mode','state.json',lambda x:x.__setitem__('pass_mode',0)),
  ('numeric-frontier-PC','state.json',lambda x:x.__setitem__('pc',x['pc']-1)),
  ('numeric-formal-omission','state.json',lambda x:x['formal_inventory'].clear()),
  ('numeric-input-identity','state.json',lambda x:x['args'].__setitem__('value',0)),
  ('numeric-real-caller','state.json',lambda x:x['caller'].__setitem__('symbol','match_option')),
  ('numeric-caller-formal','state.json',lambda x:x['caller']['args'].__setitem__('copy',1)),
  ('premature-warning-write','state.json',lambda x:x['compiler_state']['option_state']['initialized'].__setitem__('warning_level',0)),
 ]:native_json(label,name,change)
 for label,name,offset in [
  ('original-frontier-omission','events.txt',(a.original/'events.txt').read_bytes().rindex(b'frontier rva=')),
  ('original-frontier-wrong','events.txt',(a.original/'events.txt').read_bytes().rindex(b'frontier rva=')+len(b'frontier rva=000132f')),
  ('original-completion-marker','events.txt',(a.original/'events.txt').read_bytes().index(b'option-actions-complete ')),
  ('original-final-CRT-state','state-%03d.bin'%int(__import__('re').findall(r'^trap seq=(\d+) rva=000132f9 ',(a.original/'events.txt').read_text(),__import__('re').M)[0]),0x90),
  ('original-action-rule','actions-action-entry-1-registers.bin',44),
  ('original-action-cleanup','actions-action-return-0-registers.bin',8),
  ('original-source-store','actions-action-return-1-target-BSS.bin',0x4170),
  ('original-IL-store','actions-action-return-0-target-data.bin',0x1394),
  ('original-snapshot-neighbor','actions-action-return-0-target-snapshot.bin',0),
  ('original-current-argument','actions-inner-entry-2-target-BSS.bin',0x38b0),
  ('original-numeric-value-NUL','actions-inner-2-value.bin',1),
  ('original-argument-return','actions-outer-return-2-registers.bin',12),
  ('original-handler-callee-save','actions-action-return-1-registers.bin',24),
  ('original-FP','actions-action-return-1-fp.bin',4),
  ('original-signature-body','actions-signature-1-provider-body-000.bin',0),
  ('original-signature-table','actions-signature-1-return-table-000.bin',16),
  ('original-signature-third-slot','actions-signature-1-return-registers.bin',56),
  ('original-signature-error','locale-actions-signature-1-entry-errors.bin',0),
  ('original-signature-vector-tail','actions-signature-1-return-vector-000.bin',32767),
 ]:raw(label,name,offset)
 result=dict(status='all affected controls rejected',count=len(controls),seconds=time.perf_counter()-started,controls=controls,execution_receipts=dict(original=sha(a.original/'receipt.json'),native=sha(a.native/'receipt.json')),validators={str(p):sha(p)for p in [Path(__file__),Path(__file__).with_name('option_action_state.py')]})
 a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(count=result['count'],seconds=result['seconds'])))

if __name__=='__main__':main()
