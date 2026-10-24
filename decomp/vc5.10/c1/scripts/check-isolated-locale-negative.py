#!/usr/bin/env python3
"""Focused semantic denials after recomputing local capture artifact hashes."""
import argparse,copy,importlib.util,json,re,struct,tempfile,time
from pathlib import Path
from provenance import sha

spec=importlib.util.spec_from_file_location('locale_check',Path(__file__).with_name('check-isolated-locale.py'));module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)

def main():
 parser=argparse.ArgumentParser();parser.add_argument('original',type=Path);args=parser.parse_args();started=time.perf_counter();module.check(args.original)
 controls=['stale-artifact','wrong-provider-export','wrong-target-call','wrong-codepage-argument','mb-RET-cleanup','mb-callee-save','mb-return-result','locale-conflated-MBcp','missing-mb-transition','wrong-MBflag','MB-table-tail','ctype-EOF','old-table-lifetime','result-lifetime','FP-status','actual-errno','target-state-neighbor','scanner-saved-byte','unmapped-result','omitted-vector-byte','missing-API-inventory','wrong-next-frontier']
 with tempfile.TemporaryDirectory(prefix='isolated-locale-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1')as temporary:
  root=Path(temporary)
  for label in controls:
   directory=root/label;directory.mkdir()
   for file in args.original.iterdir():
    if file.is_file():(directory/file.name).symlink_to(file.resolve())
   receipt=copy.deepcopy(json.loads((directory/'receipt.json').read_text()))
   def write(name,raw,rehash=True):
    path=directory/name;path.unlink();path.write_bytes(raw)
    if rehash:receipt['artifacts'][name]=dict(size=len(raw),sha256=sha(path))
   def word(name,index,value):
    raw=bytearray((directory/name).read_bytes());struct.pack_into('<I',raw,index*4,value);write(name,raw)
   if label=='stale-artifact':write('locale-after-result.bin',b'bad\0',False)
   elif label in ('wrong-provider-export','wrong-target-call'):
    name='locale-schema.json';data=json.loads((directory/name).read_text())
    if label=='wrong-provider-export':data['exports']['_setmbcp']+=1
    else:data['call']['call_instruction_rva']+=1
    write(name,json.dumps(data).encode())
   elif label in ('wrong-codepage-argument','mb-RET-cleanup','mb-callee-save','mb-return-result','missing-API-inventory','wrong-next-frontier'):
    name='events.txt';text=(directory/name).read_text()
    if label=='wrong-codepage-argument':text=text.replace('codepage=-4 ','codepage=-3 ')
    elif label=='mb-RET-cleanup':text=re.sub(r'(^mb-return .* esp=)(\w+)',lambda m:m[1]+'%08x'%(int(m[2],16)+4),text,flags=re.M)
    elif label=='mb-callee-save':text=re.sub(r'(^mb-return .* ebx=)(\w+)',lambda m:m[1]+'%08x'%(int(m[2],16)^1),text,flags=re.M)
    elif label=='mb-return-result':text=text.replace('result=00000000 ebx=','result=00000001 ebx=')
    elif label=='missing-API-inventory':text=re.sub(r'^mb-entry .*\n','',text,flags=re.M)
    else:text=text.replace('frontier rva=00012f3e','frontier rva=00012f3f')
    write(name,text.encode())
   elif label in ('locale-conflated-MBcp','missing-mb-transition','wrong-MBflag'):
    phase='after'if label=='locale-conflated-MBcp'else'mb-after';index=9 if label=='wrong-MBflag'else 10;value=0 if label=='missing-mb-transition'else 1252 if index==10 else 1
    word('locale-'+phase+'-words.bin',index,value)
   elif label=='unmapped-result':word('locale-after-result-region.bin',4,0x2000)
   else:
    name={'MB-table-tail':'locale-mb-after-mbctype.bin','ctype-EOF':'locale-after-ctype.bin','old-table-lifetime':'locale-after-old-ctype.bin','result-lifetime':'locale-next-result.bin','FP-status':'locale-mb-after-fp.bin','actual-errno':'locale-after-errors.bin','target-state-neighbor':'locale-next-target-BSS.bin','scanner-saved-byte':'locale-before-target-snapshot.bin','omitted-vector-byte':'locale-next-CRT-environment.bin'}[label]
    raw=bytearray((directory/name).read_bytes());offset=8 if label=='FP-status'else 8 if label=='scanner-saved-byte'else len(raw)-1 if label=='MB-table-tail'else 0
    raw[offset]^=1;write(name,raw)
   path=directory/'receipt.json';path.unlink();path.write_text(json.dumps(receipt))
   try:module.check(directory)
   except(ValueError,AssertionError,IndexError,KeyError,struct.error):continue
   raise AssertionError('accepted semantic denial '+label)
 print(json.dumps(dict(status='PASS',fresh_controls=len(controls),rejected=controls,seconds=time.perf_counter()-started,scope='Original-only API contract/provenance controls; no native/app acceptance'),indent=2))
if __name__=='__main__':main()
