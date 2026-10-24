#!/usr/bin/env python3
"""Affected typed MRE prefix/raw mutations with refreshed transport hashes."""
import argparse,hashlib,json,shutil,struct,tempfile,time
from pathlib import Path
from mre_packet import decode
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('original',type=Path);args=parser.parse_args();started=time.perf_counter();passed=[];expected=decode(args.original)
with tempfile.TemporaryDirectory(prefix='mre-packet-check-',dir='/root/.ai-tmp/msvc2/vc5.10/cl')as temporary:
 directory=Path(temporary)/'raw';shutil.copytree(args.original,directory);original=(directory/'capture.json').read_bytes()
 def change(label,name=None,transform=None,metadata=None):
  saved=None;capture=json.loads(original)
  try:
   if name:
    path=directory/name;saved=path.read_bytes();raw=transform(saved);path.write_bytes(raw)
    capture['files'][name]={'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest()}
   if metadata:metadata(capture)
   (directory/'capture.json').write_text(json.dumps(capture,indent=2)+'\n')
   try:decode(directory)
   except(AssertionError,KeyError,ValueError,IndexError,struct.error,StopIteration):passed.append(label)
   else:raise AssertionError('accepted changed interpretation '+label)
  finally:
   if name:(directory/name).write_bytes(saved)
   (directory/'capture.json').write_bytes(original)
 def xor(offset):return lambda raw:raw[:offset]+bytes([raw[offset]^1])+raw[offset+1:]
 for label,field,value in [('baseline required','baseline',False),('analysis flag required','mre_analysis',False),('wrong case','case','mre-dry-run'),('mixed family','clock_response',True)]:change(label,metadata=lambda v,field=field,value=value:v.__setitem__(field,value))
 change('wrong marker','mre-observer.txt',lambda b:b+b'X')
 for i in range(1,4):
  for suffix,offset in [('arguments',0),('before-context',196),('after-context',196),('before-identity',4)]:
   name='observer/mre-%d-'%i+('before-arguments.bin'if suffix=='arguments'else suffix+'.bin')
   change('call%d %s'%(i,suffix),name,xor(offset))
 for label,name,offset in[
  ('Open write BOOL','observer/mre-1-before-arguments.bin',20),
  ('shared scratch pointer','observer/mre-1-before-arguments.bin',12),
  ('engine result','observer/mre-1-after-engine-out.bin',0),
  ('driver role alias','observer/mre-2-after-driver-out.bin',0),
  ('error out','observer/mre-1-after-error-out.bin',0),
  ('PDB spelling','observer/mre-1-before-pdb-path.bin',0),
  ('scratch retention','observer/mre-1-after-message.bin',0),
  ('Open errno effect','mktemp-mre-1-after-ptd.bin',8),
  ('Init LastError','mktemp-mre-3-after-teb.bin',0x34),
  ('full FP preservation','observer/mre-3-after-context.bin',56),
  ('descriptor noinherit','observer/mre-1-after-lowio-0.bin',112),
  ('Pending known zeros','observer/mre-signature-entry-pending.bin',0),
  ('signature formal','observer/mre-signature-entry-stack.bin',4),
  ('source name','observer/mre-1-after-job-0-source.bin',0),
  ('option ordering/flags','observer/mre-1-after-option-0.bin',4),
  ('option text','observer/mre-1-after-option-0-field-3.bin',0)]:change(label,name,xor(offset))
 change('Input allocation bound','observer/mre-1-after-job-0-input.bin',lambda b:b[:-1])
 def position(raw):
  v=json.loads(raw);v[3]['query']['position_low']+=1;return(json.dumps(v)+'\n').encode()
 change('actual fd3 offset','observer/mre-3-after-open-descriptors.json',position)
 change('must stop before frontend','trace.txt',lambda b:b+b'execute_pass\n')
 change('wrong private stop','events.txt',lambda b:b.replace(b'status=28676',b'status=0'))
assert decode(args.original)==expected
print(json.dumps({'passed':True,'negative_controls':len(passed),'seconds':time.perf_counter()-started,'controls':passed,'scope':'Partial analysis decoding only; final IDB data is not an intermediate API output oracle; no native success/credit.'}))
