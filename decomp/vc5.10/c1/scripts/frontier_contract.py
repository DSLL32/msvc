"""Exact reached frontier roles for the explicitly branched vector case."""
import re

def validate_original_frontier(events,candidate,probe):
 target=re.findall(r'^frontier rva=(\w+)$',events,re.M)
 dependency=re.findall(r'^dependency-frontier module=(\S+) rva=(\w+) seq=(\d+)$',events,re.M)
 expected=candidate['expected_frontier']
 if expected['kind']=='option-vectors-complete':
  assert candidate['vector_observers']==dict(entry_rva=0x11e3c,return_rvas=[0x12beb,0x12bfc,0x12c0d],count=3,capacity=40,arena_kind=1,header_bytes=12,item_bytes=160)
  assert expected==dict(kind='option-vectors-complete',rva=0x12f9c,native_symbol='next_argument',native_args=[],calling_convention='cdecl',caller=dict(native_symbol='parse_command_line',return_rva=0x12c45),normal_dependency=dict(module='MSPDB50.dll',rva=0xf22d,iat_rva=0x5d1d4,native_symbol='recon_mspdb_SigForPbCb',native_args=['bytes','byte_count','signature'],caller_return_rva=0x12c31),restore_frontier='actual next_argument; c9_il=9 skips signature import')
  if probe=='normal':assert not target and len(dependency)==1 and dependency[0][:2]==('MSPDB50.dll','0000f22d')
  else:assert probe=='restore'and target==['00012f9c']and not dependency
 else:assert target==['%08x'%expected['rva']]and not dependency
 return dict(target=target,dependency=dependency)

def validate_native_stop(native,candidate):
 transcript=(native/'gdb.txt').read_text()
 assert 'recon_unsupported'not in transcript
 if candidate['expected_frontier']['kind']=='option-vectors-complete' and __import__('json').loads((native/'receipt.json').read_text())['probe']=='normal':
  assert re.search(r'Breakpoint \d+, recon_mspdb_SigForPbCb \(',transcript)
  assert 'break *recon_mspdb_SigForPbCb\n'in(native/'commands.gdb').read_text()
 else:assert 'Program received signal SIGTRAP'in transcript
