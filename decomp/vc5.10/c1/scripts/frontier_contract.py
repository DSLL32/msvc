"""Exact reached frontier roles for the explicitly branched vector case."""
import re

def validate_original_frontier(events,candidate,probe):
 target=re.findall(r'^frontier rva=(\w+)$',events,re.M)
 dependency=re.findall(r'^dependency-frontier module=(\S+) rva=(\w+) seq=(\d+)$',events,re.M)
 expected=candidate['expected_frontier']
 if expected['kind']=='packing-initializer-allocator':
  assert probe=='normal'and candidate['allowed_probes']==['normal']
  assert expected==dict(kind='packing-initializer-allocator',rva=0x134c9,native_symbol='allocate_packing_memory',native_args=['bytes'],calling_convention='cdecl',caller=dict(native_symbol='initialize_packing',return_rva=0x13494))
  assert target==['000134c9']and not dependency
 elif expected['kind']=='path-normalizer-complete':
  assert probe=='normal'and candidate['allowed_probes']==['normal']
  assert expected==dict(kind='path-normalizer-complete',rva=0x1348d,native_symbol='initialize_packing',native_args=[],calling_convention='cdecl',caller=dict(native_symbol='parse_command_line',return_rva=0x12d55))
  assert target==['0001348d']and not dependency
 elif expected['kind']=='option-path-normalizer':
  assert probe=='normal'and candidate['allowed_probes']==['normal']
  assert expected==dict(kind='option-path-normalizer',rva=0x1344d,native_symbol='normalize_path',native_args=['path'],calling_convention='cdecl',caller=dict(native_symbol='resolve_option_path',return_rva=0x133c9))
  assert target==['0001344d']and not dependency
 elif expected['kind']=='option-integers-complete':
  assert expected==dict(kind='option-integers-complete',rva=0x1335b,native_symbol='resolve_option_path',native_args=['output','input','size'],calling_convention='cdecl',caller=dict(native_symbol='parse_command_line',return_rva=0x12c8b))
  assert target==['0001335b']and not dependency
 elif expected['kind']=='option-action-integer':
  assert expected==dict(kind='option-action-integer',rva=0x132f9,native_symbol='parse_option_integer',native_args=['value'],calling_convention='cdecl',caller=dict(native_symbol='apply_option_action',return_rva=0x1320c))
  assert target==['000132f9']and not dependency
 elif expected['kind']=='option-match-action':
  assert expected==dict(kind='option-match-action',rva=0x131aa,native_symbol='apply_option_action',native_args=['rule','value','copy'],calling_convention='cdecl',caller=dict(native_symbol='match_option',return_rva=0x130bc))
  assert target==['000131aa']and not dependency
 elif expected['kind']in ('option-vectors-complete','argument-cursor-complete'):
  assert candidate['vector_observers']==dict(entry_rva=0x11e3c,return_rvas=[0x12beb,0x12bfc,0x12c0d],count=3,capacity=40,arena_kind=1,header_bytes=12,item_bytes=160)
  cursor=expected['kind']=='argument-cursor-complete'
  assert expected==dict(kind='argument-cursor-complete'if cursor else'option-vectors-complete',rva=0x12fbe if cursor else 0x12f9c,native_symbol='match_option'if cursor else'next_argument',native_args=['rules','argument','next','copy']if cursor else[],calling_convention='cdecl',caller=dict(native_symbol='parse_command_line',return_rva=0x12c50 if cursor else 0x12c45),normal_dependency=dict(module='MSPDB50.dll',rva=0xf22d,iat_rva=0x5d1d4,native_symbol='recon_mspdb_SigForPbCb',native_args=['bytes','byte_count','signature'],caller_return_rva=0x12c31),restore_frontier='actual match_option after next_argument; c9_il9 skips signature import'if cursor else'actual next_argument; c9_il=9 skips signature import')
  if probe=='normal':assert not target and len(dependency)==1 and dependency[0][:2]==('MSPDB50.dll','0000f22d')
  else:assert probe=='restore'and target==['%08x'%expected['rva']]and not dependency
 else:assert target==['%08x'%expected['rva']]and not dependency
 return dict(target=target,dependency=dependency)

def validate_native_stop(native,candidate):
 transcript=(native/'gdb.txt').read_text()
 assert 'recon_unsupported'not in transcript
 if candidate['expected_frontier']['kind']in ('option-vectors-complete','argument-cursor-complete') and __import__('json').loads((native/'receipt.json').read_text())['probe']=='normal':
  assert re.search(r'Breakpoint \d+, recon_mspdb_SigForPbCb \(',transcript)
  assert 'break *recon_mspdb_SigForPbCb\n'in(native/'commands.gdb').read_text()
 else:assert 'Program received signal SIGTRAP'in transcript
