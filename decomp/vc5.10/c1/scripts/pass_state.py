"""The already reconstructed Invoke wrapper stores its known BOOL before main."""
import copy,struct

def plain_globals_after_invoke_store(original,raw,candidate,module,boundary_rva=None):
 if boundary_rva!=0x127a7 and candidate['expected_frontier']['kind']!='compiler-main':return raw
 snapshot=(original/'relocated-text.bin').read_bytes()
 assert snapshot[0x12258-0x1000:0x12260-0x1000]==b'\x8b\x45\x10\xa3'+struct.pack('<I',module+0x5c08c)
 assert struct.unpack_from('<i',raw,0x8c)[0] in (0,1)
 result=bytearray(raw);struct.pack_into('<i',result,0x8c,candidate['case_contract']['BOOL'])
 return bytes(result)

def native_after_invoke_store(raw,candidate,boundary_rva=None):
 result=copy.deepcopy(raw)
 if boundary_rva==0x127a7 or candidate['expected_frontier']['kind']=='compiler-main':
  assert result['pass_mode'] in (0,1)
  result['pass_mode']=candidate['case_contract']['BOOL']
 return result
