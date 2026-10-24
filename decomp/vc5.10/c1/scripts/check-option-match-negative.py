#!/usr/bin/env python3
"""Affected matcher/ABI/data/public-call controls, without replaying old prefix."""
import argparse,copy,json,struct,tempfile,time
from pathlib import Path
from option_match_state import validate_boundary
from signature_call_state import validate as validate_signature
from provenance import sha
import counterpart_correction

def shadow(source,destination,name,offset):
 for f in source.iterdir():
  if f.is_file():(destination/f.name).symlink_to(f)
 raw=bytearray((destination/name).read_bytes());raw[offset]^=1;(destination/name).unlink();(destination/name).write_bytes(raw)

def main():
 p=argparse.ArgumentParser();p.add_argument('original',type=Path);p.add_argument('native',type=Path);p.add_argument('--positive',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();project=Path(__file__).resolve().parents[1]
 positive=json.loads(a.positive.read_text());assert positive['result']['application_candidate']=='_match_option'
 assert positive['result']['execution_receipts']==dict(original=sha(a.original/'receipt.json'),native=sha(a.native/'receipt.json'))
 for name,value in positive['validator_hashes'].items():assert sha(project/'scripts'/name)==value
 c=json.loads((a.original/'candidate.json').read_text());e=json.loads((a.native/'option-match-entries.json').read_text())[0];n=json.loads((a.native/'option-next-entries.json').read_text())[0];r=json.loads((a.native/'option-next-returns.json').read_text())[0];s=json.loads((a.native/'state.json').read_text());validate_boundary(a.original,a.native,c,e,n,r,s)
 started=time.perf_counter();controls=[]
 changes=[('matcher-formals',lambda e,n,r,s:e['formal_inventory'].pop()),('matcher-argument',lambda e,n,r,s:e['args'].__setitem__('argument',0)),('next-formals',lambda e,n,r,s:n['formal_inventory'].append(dict(name='fake',type='int'))),('next-real-PC',lambda e,n,r,s:n.__setitem__('pc',0)),('callback-argument-identity',lambda e,n,r,s:n['caller']['args'].__setitem__('next',0)),('current-argument-store',lambda e,n,r,s:n['compiler_state']['bss'].__setitem__('current_argument',0)),('next-return-cleanup',lambda e,n,r,s:r.__setitem__('esp',0)),('next-callee-save',lambda e,n,r,s:r['preserved'].__setitem__('ebx',0)),('borrowed-value-identity',lambda e,n,r,s:r.__setitem__('eax',0)),('borrowed-NUL',lambda e,n,r,s:r['result_bytes'].__setitem__(9,1)),('second-argc-step',lambda e,n,r,s:r['compiler_state']['bss'].__setitem__('command_argc',10)),('FP-state',lambda e,n,r,s:r['fp'].__setitem__('sw',0)),('error-state',lambda e,n,r,s:r['errors'].__setitem__(0,1)),('action-real-trap',lambda e,n,r,s:s.__setitem__('pc',s['pc']-1)),('action-rule',lambda e,n,r,s:s['args'].__setitem__('rule',0)),('action-value',lambda e,n,r,s:s['args'].__setitem__('value',0)),('action-copy',lambda e,n,r,s:s['args'].__setitem__('copy',1)),('action-formals',lambda e,n,r,s:s['formal_inventory'].pop()),('neighbor-state',lambda e,n,r,s:s['compiler_state']['bss'].__setitem__('debug_break',1))]
 for label,change in changes:
  x,y,z,w=copy.deepcopy((e,n,r,s));change(x,y,z,w)
  try:validate_boundary(a.original,a.native,c,x,y,z,w)
  except AssertionError:pass
  else:raise AssertionError('accepted '+label)
  controls.append(dict(scope='matcher-native',control=label))
 for label,name,offset in [('original-return-PC','option-match-next-return-registers.bin',4),('original-return-value','option-match-next-return-registers.bin',12),('original-preserved','option-match-next-return-registers.bin',24),('current-store-neighbor','option-match-next-entry-target-BSS.bin',0),('second-cursor-write','option-match-next-return-target-BSS.bin',0x5c38),('original-data-neighbor','option-match-action-target-data.bin',0),('original-snapshot-neighbor','option-match-action-target-snapshot.bin',0),('original-value-NUL','option-match-value-000.bin',9)]:
  with tempfile.TemporaryDirectory(prefix='matcher-negative-',dir=a.original.parent)as tmp:
   d=Path(tmp);shadow(a.original,d,name,offset)
   try:validate_boundary(d,a.native,c,e,n,r,s)
   except AssertionError:pass
   else:raise AssertionError('accepted '+label)
  controls.append(dict(scope='matcher-original',control=label))
 se=json.loads((a.native/'signature-call-entries.json').read_text())[0];sr=json.loads((a.native/'signature-call-returns.json').read_text())[0];sn=json.loads((a.native/'argument-cursor-entries.json').read_text())[0];validate_signature(a.original,a.native,se,sr,sn)
 sigchanges=[('public-formals',lambda e,r,n:e['formal_inventory'].pop()),('public-length',lambda e,r,n:e['args'].__setitem__('byte_count',2)),('public-seed',lambda e,r,n:e['args'].__setitem__('signature',0)),('public-input',lambda e,r,n:e['input'].__setitem__(0,68)),('public-result',lambda e,r,n:r.__setitem__('eax',0)),('public-return-cleanup',lambda e,r,n:r.__setitem__('esp',0)),('public-third-slot',lambda e,r,n:r['argument_slots_after'].__setitem__(2,0)),('public-FP',lambda e,r,n:r['fp'].__setitem__('sw',0)),('public-errors',lambda e,r,n:r['errors'].__setitem__(2,1)),('public-neighbor',lambda e,r,n:r['compiler_state']['bss'].__setitem__('debug_break',1)),('caller-result-store',lambda e,r,n:n['compiler_state']['bss'].__setitem__('pdb_signature',0))]
 for label,change in sigchanges:
  x,y,z=copy.deepcopy((se,sr,sn));change(x,y,z)
  try:validate_signature(a.original,a.native,x,y,z)
  except AssertionError:pass
  else:raise AssertionError('accepted '+label)
  controls.append(dict(scope='public-native',control=label))
 for label,name,offset in [('provider-body','signature-provider-body-000.bin',0),('provider-table','signature-provider-table-after-000.bin',33),('original-third-slot','signature-call-return-registers.bin',56),('original-FP','locale-signature-return-fp.bin',4),('owned-vector-neighbor','signature-call-return-vector-000.bin',32767)]:
  with tempfile.TemporaryDirectory(prefix='signature-call-negative-',dir=a.original.parent)as tmp:
   d=Path(tmp);shadow(a.original,d,name,offset)
   try:validate_signature(d,a.native,se,sr,sn)
   except AssertionError:pass
   else:raise AssertionError('accepted '+label)
  controls.append(dict(scope='public-original',control=label))
 packet=counterpart_correction.PACKET;original_packet=json.loads(packet.read_text());counterpart_correction.validate(a.original)
 packet_changes=[('extra-audit-delta',lambda x:x['allowed_audit_deltas'].__setitem__('wrong',dict(old='0',new='0'))),('wrong-original-mask',lambda x:x['originals'][sha(a.original/'receipt.json')].__setitem__('ranges_sha256','0')),('wrong-helper',lambda x:x['originals'][sha(a.original/'receipt.json')].__setitem__('helper_sha256','0')),('wrong-environment',lambda x:x['originals'][sha(a.original/'receipt.json')].__setitem__('environment_sha256','0')),('wrong-native-ELF',lambda x:x['current_native_build'].__setitem__('elf_sha256','0')),('wrong-new-source',lambda x:x['current_source'].__setitem__('sha256','0')),('wrong-old-source',lambda x:x['old_source'].__setitem__('sha256','0')),('wrong-current-audit',lambda x:x['current_audit'].__setitem__('sha256','0')),('wrong-observer',lambda x:x['observer'].__setitem__('sha256','0')),('wrong-current-rebuilt',lambda x:x['allowed_audit_deltas']['decomp/vc5.10/c1/build/c1.dll'].__setitem__('new','0'))]
 for label,change in packet_changes:
  with tempfile.TemporaryDirectory(prefix='counterpart-negative-',dir=a.original.parent)as tmp:
   path=Path(tmp)/'packet.json';x=copy.deepcopy(original_packet);change(x);path.write_text(json.dumps(x));counterpart_correction.PACKET=path
   try:counterpart_correction.validate(a.original)
   except AssertionError:pass
   else:raise AssertionError('accepted '+label)
   finally:counterpart_correction.PACKET=packet
  controls.append(dict(scope='unchanged-original-derivation',control=label))
 result=dict(status='PASS',controls=controls,seconds=time.perf_counter()-started,positive_sha256=sha(a.positive),controller_sha256=sha(Path(__file__)),checker_sha256=sha(project/'scripts/option_match_state.py'),signature_checker_sha256=sha(project/'scripts/signature_call_state.py'),correction_checker_sha256=sha(project/'scripts/counterpart_correction.py'),scope='fresh affected matcher/newfield/publicAPI/explicit original reuse controls; unchanged core prefix controls retained')
 assert not a.output.exists();a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(status='PASS',controls=len(controls),seconds=result['seconds'])))
if __name__=='__main__':main()
