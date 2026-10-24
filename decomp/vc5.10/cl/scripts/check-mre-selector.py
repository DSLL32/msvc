#!/usr/bin/env python3
"""Refreshed-raw semantic controls for the selector/lowercase response only."""
import argparse,copy,hashlib,json,shutil,struct,tempfile,time
from pathlib import Path
from mre_selector_packet import decode
P=Path(__file__).resolve().parents[1];sha=lambda b:hashlib.sha256(b).hexdigest()
a=argparse.ArgumentParser(description=__doc__);a.add_argument('original',type=Path);a.add_argument('--receipt',type=Path,required=True);args=a.parse_args();start=time.perf_counter();expected=decode(args.original);passed=[]
with tempfile.TemporaryDirectory(dir='/root/.ai-tmp/msvc2/vc5.10/cl',prefix='selector-controls-')as tmp:
 d=Path(tmp)/'original';shutil.copytree(args.original,d);capture=json.loads((d/'capture.json').read_bytes())
 def reject(label,name,change):
  f=d/name;old=f.read_bytes();j=copy.deepcopy(capture)
  try:
   f.write_bytes(change(old));j['files'][name]={'bytes':f.stat().st_size,'sha256':sha(f.read_bytes())};(d/'capture.json').write_text(json.dumps(j,indent=2)+'\n')
   try:decode(d)
   except(AssertionError,ValueError,KeyError,UnicodeDecodeError,struct.error):passed.append(label)
   else:raise AssertionError('accepted semantic mutation '+label)
  finally:f.write_bytes(old);(d/'capture.json').write_text(json.dumps(capture,indent=2)+'\n')
 def word(offset,value=None):
  def mutation(b):
   x=bytearray(b);current=struct.unpack_from('<I',x,offset)[0];struct.pack_into('<I',x,offset,current^1 if value is None else value);return bytes(x)
  return mutation
 for label,name,change in[
 ('wrong response family','mre-observer.txt',lambda b:b.replace(b'SELECTOR',b'SIGNATURE')),
 ('unrestored original formal forbidden','observer/mre-frontier-signature-frame.bin',word(8,expected['signature_frontier']['state'])),
 ('changed saved parent','observer/mre-frontier-signature-frame.bin',word(0)),
 ('changed saved return','observer/mre-frontier-signature-frame.bin',word(4)),
 ('wrong actual qsort return','observer/mre-frontier-stack.bin',word(0)),
 ('wrong first formal','observer/mre-frontier-formal-arguments.bin',word(0)),
 ('array count outside owned span','observer/mre-frontier-signature-count.bin',word(0,65)),
 ('wrong slot array','observer/mre-frontier-signature-array.bin',word(0)),
 ('changed compare input','observer/mre-frontier-compare-left.bin',lambda b:b'!'+b[1:]),
 ('changed actual item','observer/mre-frontier-signature-item-1.bin',lambda b:b'!'+b[1:]),
 ('changed lower input','observer/lower-before-buffer.bin',lambda b:b'!'+b[1:]),
 ('changed lower result alias','observer/lower-after-context.bin',word(176)),
 ('changed lower provider','observer/lower-before-identity.bin',word(0)),
 ('changed lower initialized length','observer/lower-before-identity.bin',word(16)),
 ('changed lower ESP','observer/lower-after-context.bin',word(196)),
 ('changed lower nonvolatile','observer/lower-after-context.bin',word(164)),
 ('changed lower FP','observer/lower-after-context.bin',word(32)),
 ('changed locale','observer/lower-after-mb-state.bin',word(0)),
 ('changed API errno','mktemp-lower-after-ptd.bin',word(8)),
 ('changed API doserrno','mktemp-lower-after-ptd.bin',word(12)),
 ('changed API LastError','mktemp-lower-after-teb.bin',word(0x34)),
 ('changed intermediate file','observer/mre-files-idb-mre-2-after-contents.bin',lambda b:bytes([b[0]^1])+b[1:])]:reject(label,name,change)
 assert decode(d)['signature_frontier']==expected['signature_frontier']
receipt={'passed':True,'count':len(passed),'controls':passed,'seconds':time.perf_counter()-start,'scope':'Private-copy refreshed rawhash semantic mutations; actual original unchanged; no native or app acceptance','original_capture_sha256':sha((args.original/'capture.json').read_bytes()),'decoder_sha256':sha((P/'scripts/mre_selector_packet.py').read_bytes()),'API_decoder_sha256':sha((P/'scripts/mbslwr_packet.py').read_bytes())};args.receipt.write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt,indent=2))
