#!/usr/bin/env python3
"""Refreshed-hash negatives against actual original clock/LINK raw records."""
import argparse,copy,hashlib,json,shutil,struct,tempfile,time
from pathlib import Path
from clock_packet import decode
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('original',type=Path);args=parser.parse_args();started=time.perf_counter();passed=[]
expected=decode(args.original)
with tempfile.TemporaryDirectory(prefix='clock-packet-controls-',dir='/root/.ai-tmp/msvc2/vc5.10/cl')as temporary:
    directory=Path(temporary)/'original';shutil.copytree(args.original,directory);original_capture=(directory/'capture.json').read_bytes()
    def mutate(label,changes=None,metadata=None):
        saved={};capture=json.loads(original_capture)
        try:
            for name,change in (changes or {}).items():
                path=directory/name;saved[name]=path.read_bytes();value=change(saved[name]);path.write_bytes(value)
                capture['files'][name]={'bytes':len(value),'sha256':hashlib.sha256(value).hexdigest()}
            if metadata:metadata(capture)
            (directory/'capture.json').write_text(json.dumps(capture,indent=2)+'\n')
            try:decode(directory)
            except (AssertionError,ValueError,KeyError,IndexError,StopIteration):passed.append(label)
            else:raise AssertionError('accepted invalid clock evidence: '+label)
        finally:
            for name,value in saved.items():(directory/name).write_bytes(value)
            (directory/'capture.json').write_bytes(original_capture)
    def xor(offset):return lambda raw:raw[:offset]+bytes([raw[offset]^1])+raw[offset+1:]
    def replace(old,new):
        def change(raw):assert old in raw;return raw.replace(old,new,1)
        return change
    for name in ('baseline','clock_analysis'):
        mutate('required '+name,metadata=lambda value,name=name:value.__setitem__(name,False))
    mutate('wrong case',metadata=lambda value:value.update(case='link-object'))
    mutate('ambiguous masked response',metadata=lambda value:value.update(link_response=True))
    mutate('wrong clock marker',{'clock-observer.txt':lambda raw:raw+b'X'})
    for label,old,new in [('callsite',b'call=00405402',b'call=00405403'),('returnsite',b'return=00405408',b'return=00405409'),('functionidentity',b'function=78002a41',b'function=78002a42'),('globalbuffer',b'argument=00409798',b'argument=00409688'),('ESP',b'esp=0031f234',b'esp=0031f238')]:
        mutate('clock '+label,{'observer/clock-events.txt':replace(old,new)})
    mutate('missing clock call',{'observer/clock-events.txt':lambda raw:b'\n'.join(line for line in raw.split(b'\n')if not line.startswith(b'ftime sequence=2'))})
    mutate('extra clock call',{'observer/clock-events.txt':lambda raw:raw+b'ftime sequence=1 phase=before call=00405402 return=00405408 function=78002a41 argument=00409798 esp=0031f234 event=100\n'})
    mutate('raw stack buffer pointer',{'observer/clock-1-before-stack.bin':xor(0)})
    mutate('raw EIP',{'observer/clock-1-before-context.bin':xor(184)})
    mutate('x87 register state',{'observer/clock-1-after-context.bin':xor(56)})
    mutate('x87 control word',{'observer/clock-2-after-context.bin':xor(28)})
    mutate('nonzero unwritten input',{'observer/clock-1-before-timeb.bin':xor(0)})
    for label,offset in [('seconds',0),('milliseconds',4),('timezone',6),('DST',8),('padding',10)]:
        mutate('output '+label,{'observer/clock-2-after-timeb.bin':xor(offset),'observer/clock-2-after-stopped.bin':xor(offset)})
    mutate('millisecond bound',{'observer/clock-2-after-timeb.bin':lambda b:b[:4]+struct.pack('<H',1000)+b[6:], 'observer/clock-2-after-stopped.bin':lambda b:b[:4]+struct.pack('<H',1000)+b[6:]})
    mutate('first result lifetime',{'observer/clock-2-before-started.bin':xor(0)})
    for name,offset,suffix in [('errno',8,'ptd'),('doserrno',12,'ptd'),('LastError',0x34,'teb')]:
        mutate('actual '+name,{'mktemp-clock-1-after-'+suffix+'.bin':xor(offset)})
    for label,offset in [('timezone scalar',4),('daylight scalar',12),('name static pointer',20),('environment vector pointer',32)]:
        mutate(label,{'observer/clock-1-after-tz-words.bin':xor(offset)})
    mutate('original zone label',{'observer/clock-2-after-tz-name-0.bin':xor(0)})
    mutate('CRT environment bytes',{'observer/clock-1-after-environment.bin':xor(16)})
    mutate('formatted clock output',{'stdout.bin':xor(0)})
    mutate('child namespace',{'observer/spawn-after-os-environment.bin':xor(0)})
    mutate('child executable bytes',{'observer/spawn-executable.bin':xor(64)})
    mutate('child object bytes',{'observer/link-input/test.obj':xor(0)})
    mutate('child response bytes',{'observer/link-input/'+expected['LINK']['child']['argv'][2][3:]:xor(0)})
    mutate('child PE result',{'observer/link-output/test.exe':xor(64)})
    mutate('terminal standard handle',{'observer/exit-before-parameters.bin':xor(0x1c)})
    mutate('terminal error',{'mktemp-exit-before-teb.bin':xor(0x34)})
assert decode(args.original)==expected
print(json.dumps({'passed':True,'negative_controls':len(passed),'seconds':time.perf_counter()-started,'controls':passed,'scope':'Baseline raw interpretation only; no function acceptance/native success'}))
