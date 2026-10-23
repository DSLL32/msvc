#!/usr/bin/env python3
"""Reject unproved C: filesystem transport while using retained original data."""
import argparse,hashlib,json,re,shutil,tempfile
from pathlib import Path
from case_inputs import prepare_c_drive
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('original',type=Path);args=parser.parse_args();passed=[]
with tempfile.TemporaryDirectory(prefix='drive-parent-checks-',dir='/root/.ai-tmp/msvc2/vc5.10/cl')as temporary:
    work=Path(temporary);raw=work/'raw';raw.mkdir();capture=json.loads((args.original/'capture.json').read_bytes())
    for name in ['capture.json','loaded-msvcrt.bin','access-events.txt',*[n for n in capture['files']if n.startswith(('access-','mktemp-access-'))]]:
        shutil.copyfile(args.original/name,raw/name)
    events=(raw/'access-events.txt').read_bytes();matches=re.findall(rb'^call sequence=(\d+) .* mode=4$',events,re.M)
    number=next(n.decode()for n in matches if(raw/('access-'+n.decode()+'-path.bin')).read_bytes().startswith(b'C:\\'))
    target=work/'positive';target.mkdir();assert prepare_c_drive(raw,target)==target/'drive-c'
    receipt=json.loads((target/'native-filesystem.json').read_text());assert receipt['dos_parents'] and all(p.startswith('C:\\')for p in receipt['dos_parents'])
    def denied(label,name,change):
        path=raw/name;old=path.read_bytes();original=(raw/'capture.json').read_bytes()
        try:
            value=change(old);path.write_bytes(value);updated=json.loads(original);updated['files'][name]={'bytes':len(value),'sha256':hashlib.sha256(value).hexdigest()};(raw/'capture.json').write_text(json.dumps(updated)+'\n')
            output=work/('negative-'+str(len(passed)));output.mkdir()
            try:prepare_c_drive(raw,output)
            except (AssertionError,KeyError):passed.append(label)
            else:raise AssertionError('accepted unproved C: transport: '+label)
        finally:path.write_bytes(old);(raw/'capture.json').write_bytes(original)
    denied('C: success is not a missing parent proof','access-events.txt',lambda b:b.replace(('return sequence='+number+' result=ffffffff').encode(),('return sequence='+number+' result=00000000').encode()))
    for field,offset,suffix in [('errno',8,'ptd'),('doserrno',12,'ptd'),('LastError',0x34,'teb')]:
        denied('wrong '+field,'mktemp-access-'+number+'-after-'+suffix+'.bin',lambda b,offset=offset:b[:offset]+bytes([b[offset]^1])+b[offset+1:])
    denied('directory traversal','access-'+number+'-path.bin',lambda b:b'C:\\..\\file\0')
    denied('empty path component','access-'+number+'-path.bin',lambda b:b'C:\\windows\\\\x\\file\0')
print(json.dumps({'passed':True,'negative_controls':len(passed),'controls':passed}))
