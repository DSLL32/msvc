#!/usr/bin/env python3
"""Compare bounded actual CVTRES WCHAR16 printf observations, native-only."""
import argparse,copy,json,re,struct
from pathlib import Path
from mask import sha

def compare(original,native,state=None):
 text=(original/'events.txt').read_text();provided_state=state is not None;state=state or json.loads((native/'state.json').read_text())
 native_receipt=json.loads((native/'receipt.json').read_text())
 if not provided_state:assert sha((native/'state.json').read_bytes())==native_receipt['raw_sha256']['state.json'],'native state differs'
 assert sha((native/'native.elf').read_bytes())==native_receipt['native_sha256'],'native ELF differs'
 receipt=json.loads((original/'receipt.json').read_text());assert receipt['provider_verified'] and receipt['provider_actual_hfile_sha256']=='e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210'
 assert sha((original/'events.txt').read_bytes())==receipt['raw_sha256']['events.txt'],'original events differ'
 records=[]
 for address,block in re.findall(r'^observation ([0-9a-f]+)\n(.*?)(?=^observation |^exit |\Z)',text,re.M|re.S):
  wide=re.search(r'^observation-printf-wide pointer ([0-9a-f]+) raw ([0-9a-f]+) terminated 1$',block,re.M)
  if not wide:continue
  fmt=re.search(r'^observation-argument-string 0 pointer ([0-9a-f]+) raw ([0-9a-f]+) terminated 1$',block,re.M);stack=[int(v,16) for v in re.search(r'^observation-stack ([0-9a-f ]+)$',block,re.M)[1].split()]
  assert stack[:2]==[int(fmt[1],16),int(wide[1],16)] and stack[0]==0x100457c,'original printf argument identity differs'
  memory=dict(re.findall(r'^observation-crt-memory ([0-9a-f]+) ([0-9a-f]*)$',block,re.M));pending=dict(re.findall(r'^observation-crt-buffer ([0-9a-f]+) ([0-9a-f]*)$',block,re.M))
  records.append(dict(address=int(address,16),format_hex=fmt[2],wide_hex=wide[2],return_value=int(re.search(r'^observation-trap .*? eax ([0-9a-f]+)',block,re.M)[1],16),following_argument=stack[2],file=memory['0003f590'],pending=pending['0003f590']))
 assert records and len(records)%2==0,'missing original wide printf calls/returns'
 caller='insert_resource' if records[0]['address']==0x1001f1e else 'convert_resources'
 indices=[n for n,call in enumerate(state['printf_calls']) if call['caller']==caller]
 assert len(indices)*2==len(records),'missing native wide printf calls'
 returned_by_call={item['call']:item for item in state['printf_returns']};assert len(returned_by_call)==len(state['printf_calls']),'missing native printf returns'
 native_buffer=state['entries'][0]['globals']['message_buffer']['address']
 def file_state(old,new):
  left=list(struct.unpack('<8I',bytes.fromhex(old['file'])));right=list(struct.unpack('<8I',bytes.fromhex(new['raw_hex'])))
  assert left[0]-left[2]==right[0]-right[2]==new['used'],'printf FILE cursor differs'
  for index in (1,3,4,5,6,7):assert left[index]==right[index],('printf FILE field differs',index)
  assert old['pending']==new['buffer_hex'],'printf pending stdout differs'
 for index,native_index in enumerate(indices):
  before,after=records[2*index:2*index+2];call=state['printf_calls'][native_index];returned=returned_by_call[native_index]
  assert (before['address'],after['address']) in [(0x1002fff,0x1003005),(0x100302d,0x1003033),(0x1001f1e,0x1001f20),(0x1001f50,0x1001f52)]
  assert before['format_hex']==after['format_hex']==call['format_hex'],'wide printf format differs'
  assert call['format_pointer']==native_buffer,'wide printf format pointer is not message buffer'
  assert before['wide_hex']==after['wide_hex']==call['wide_hex'],'wide printf argument differs'
  wide=bytes.fromhex(call['wide_hex']);assert len(wide)%2==0 and wide[-2:]==b'\0\0' and all(wide[n:n+2]!=b'\0\0' for n in range(0,len(wide)-2,2))
  assert call['second_argument']==before['following_argument'],'wide printf following argument differs'
  assert returned['call']==native_index and returned['return_value']==after['return_value'],'wide printf return differs'
  file_state(before,call['crt']['recon_iob[1]']);file_state(after,returned['crt']['recon_iob[1]'])
 return dict(calls=len(indices),returns=[record['return_value'] for record in records[1::2]],formats=[bytes.fromhex(state['printf_calls'][n]['format_hex']).decode() for n in indices],wchar16='Full terminated Windows16-bit arguments match',stdout='Complete FILE32 nonaddress fields, cursor offsets and pending bytes match')

if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--original',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--negatives',action='store_true');a=p.parse_args();result=compare(a.original,a.native)
 controls=[]
 if a.negatives:
  baseline=json.loads((a.native/'state.json').read_text());caller='insert_resource' if result['calls']==2 else 'convert_resources';index=next(n for n,item in enumerate(baseline['printf_calls']) if item['caller']==caller);return_index=next(n for n,item in enumerate(baseline['printf_returns']) if item['call']==index)
  for name,change in [('wide-argument',lambda s:s['printf_calls'][index].update(wide_hex='0000')),('following-argument',lambda s:s['printf_calls'][index].update(second_argument=0)),('return',lambda s:s['printf_returns'][return_index].update(return_value=-1)),('pending-bytes',lambda s:s['printf_returns'][return_index]['crt']['recon_iob[1]'].update(buffer_hex='00'))]:
   state=copy.deepcopy(baseline);change(state)
   try:compare(a.original,a.native,state)
   except AssertionError as error:controls.append(dict(name=name,rejected=True,reason=str(error)))
   else:raise AssertionError(name+' was accepted')
 report=dict(schema=1,comparison=result,controls=controls,original=str(a.original),native=str(a.native),pinned={str(path):sha(path.read_bytes()) for path in [a.original/'events.txt',a.original/'receipt.json',a.native/'state.json',a.native/'receipt.json',Path(__file__)]})
 a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(result,indent=2))
