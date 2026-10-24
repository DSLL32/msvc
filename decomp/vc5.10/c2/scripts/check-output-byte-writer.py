#!/usr/bin/env python3
"""Finite writer payload/frontier and recovered error-callback boundary controls."""
import argparse
import copy
import importlib.util
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile
import time
from compare import compare
from state import BINARY, ROOT, sha
import writer_evidence
from data_evidence import reader, cstring


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('original','native','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();start=time.monotonic();positive=compare(a.original,a.native);passed=[]
    def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n')
    def native(root,field,change):
        path=root/'n/state.json';v=json.loads(path.read_text());old=json.dumps(v[field],sort_keys=True);change(v[field]);assert old!=json.dumps(v[field],sort_keys=True);write(path,v)
        prefix={'coff_write_byte_input':'C2-COFF-WRITE-BYTE-INPUT','globals_complete':'C2-GLOBALS-COMPLETE'}[field];path=root/'n/gdb.stdout';raw=path.read_text();assert len(re.findall('^'+prefix+' ',raw,re.M))==1;path.write_text(re.sub('^'+prefix+r' .*$',lambda m:prefix+' '+json.dumps(v[field],sort_keys=True),raw,flags=re.M))
        path=root/'n/receipt.json';r=json.loads(path.read_text());r['state_sha256']=sha((root/'n/state.json').read_bytes());r['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write(path,r)
    def original(root,file,index):
        path=root/'o'/file;data=bytearray(path.read_bytes());data[index]^=1;path.write_bytes(data);path=root/'o/receipt.json';r=json.loads(path.read_text());r['globals_complete_hashes'][file]=sha(data);write(path,r)
    def event(root):
        path=root/'o/events.txt';raw=path.read_text();changed=re.sub(r'^(observe 00453f53 eax=[0-9a-f]+ ecx=)[0-9a-f]+',r'\g<1>00000000',raw,flags=re.M);assert changed!=raw;path.write_text(changed);path=root/'o/receipt.json';r=json.loads(path.read_text());r['events_sha256']=sha((root/'o/events.txt').read_bytes());write(path,r)
    mutations=[('uchar formal omitted',lambda r:native(r,'coff_write_byte_input',lambda v:v.__setitem__('formals',[]))),
        ('uchar formal widened',lambda r:native(r,'coff_write_byte_input',lambda v:v['formals'][0].__setitem__('type','unsigned int'))),
        ('actual byte value differs',lambda r:native(r,'coff_write_byte_input',lambda v:v.__setitem__('value',0))),
        ('section byte counter differs',lambda r:native(r,'coff_write_byte_input',lambda v:v.__setitem__('section_size',15))),
        ('selected list alias lost',lambda r:native(r,'coff_write_byte_input',lambda v:v.__setitem__('selected_list_alias',False))),
        ('last completed prefix payload differs',lambda r:native(r,'coff_write_byte_input',lambda v:v.__setitem__('payload','2b'+v['payload'][2:]))),
        ('list used counter differs',lambda r:native(r,'coff_write_byte_input',lambda v:v.__setitem__('used_bytes',15))),
        ('list write count differs',lambda r:native(r,'coff_write_byte_input',lambda v:v.__setitem__('write_count',1))),
        ('original actual CL byte differs',event),
        ('original buffered prefix byte differs',lambda r:original(r,'writer-pool.bin',1996)),
        ('original buffered library byte differs',lambda r:original(r,'writer-pool.bin',2008)),
        ('original used bytes differs',lambda r:original(r,'writer-pool.bin',1980)),
        ('original write count differs',lambda r:original(r,'writer-pool.bin',1984)),
        ('original unwritten payload differs',lambda r:original(r,'writer-pool.bin',2012)),
        ('original uninterpreted chunk tail differs from allocator writer',lambda r:original(r,'writer-pool.bin',2499)),
        ('original section count differs',lambda r:original(r,'pool-block.bin',1904))]
    with tempfile.TemporaryDirectory(prefix='byte-writer-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        folder=Path(folder)
        for i,(name,change)in enumerate(mutations):
            root=folder/str(i);shutil.copytree(a.original,root/'o');shutil.copytree(a.native,root/'n');change(root)
            try:compare(root/'o',root/'n')
            except(AssertionError,KeyError,ValueError)as e:passed.append({'control':name,'reason':str(e)})
            else:raise AssertionError('writer input mutation accepted: '+name)
        config=json.loads((a.original/'configuration.json').read_text());graph=json.loads((a.original/'graph-audit.json').read_text())
        clone=folder/'model';shutil.copytree(BINARY/'src',clone/'src');shutil.copytree(BINARY/'cases/analysis',clone/'cases/analysis');(clone/'build').mkdir()
        for name in ('c2.exe','c2.map'):shutil.copyfile(BINARY/'build'/name,clone/'build'/name)
        writer_evidence.BINARY=clone
        assert writer_evidence.verify(config,graph)
        for name,snippet in [('clock member invocation','void bad(C2ObjectOutput *p) { p->output_clock.target(); }'),
            ('unreviewed typed error invocation','void bad(void) { output_error_handler(14, "x", 1); }'),
            ('error member escape','void bad(C2ObjectOutput *p) { void *q = &p->output_error; }'),
            ('pending clock target invocation','void bad(void) { writer_callback_45477a(); }')]:
            path=clone/'src/main.c';before=path.read_bytes();path.write_bytes(before+b'\n'+snippet.encode()+b'\n')
            try:writer_evidence.verify(config,graph)
            except AssertionError as e:passed.append({'control':name,'reason':str(e)})
            else:raise AssertionError('writer callback escape accepted: '+name)
            path.write_bytes(before)
        for name,change in [('error callback body given source credit',lambda c:c['accepted'].append('_writer_callback_48511c')),
            ('error callback loses pending extent',lambda c:next(r for r in c['stubs']if r['name']=='_writer_callback_48511c').__setitem__('pending_extent',False)),
            ('error callback fake cdecl MAP',lambda c:next(r for r in c['stubs']if r['name']=='_writer_callback_48511c').__setitem__('symbol','_writer_callback_48511c'))]:
            v=copy.deepcopy(config);change(v)
            try:writer_evidence.verify(v,graph)
            except(AssertionError,KeyError)as e:passed.append({'control':name,'reason':str(e)})
            else:raise AssertionError('callback pending-role mutation accepted: '+name)
        writer_evidence.BINARY=BINARY
        flags=['clang','-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-Wno-ignored-attributes','-DRECON_NATIVE_TEST','-I'+str(ROOT/'lib/recon/include'),'-I'+str(ROOT/'lib/recon/native'),'-I'+str(BINARY/'src'),'-fsyntax-only']
        path=folder/'abi.c';path.write_text('#include "coff-output.h"\nvoid good(C2ObjectOutput *p) { p->output_error(14, "coffemit.c", 3254); }\n');r=subprocess.run(flags+[str(path)],capture_output=True);assert r.returncode==0,r.stderr.decode()
        for name,snippet in [('error call missing stack line','p->output_error(14, "x");'),('error erased target member','p->output_error.target(14, "x", 1);'),('clock remains noncallable','p->output_clock();')]:
            path.write_text('#include "coff-output.h"\nvoid bad(C2ObjectOutput *p) { '+snippet+' }\n');r=subprocess.run(flags+[str(path)],capture_output=True);assert r.returncode!=0,r.stderr.decode();passed.append({'control':name,'diagnostic_sha256':sha(r.stderr)})
    write(a.output,{'positive':positive,'count':len(passed),'rejected':passed,'seconds':time.monotonic()-start,'checker_sha256':sha(Path(__file__).read_bytes()),'original_receipt_sha256':sha((a.original/'receipt.json').read_bytes()),'native_receipt_sha256':sha((a.native/'receipt.json').read_bytes()),'scope':'16fresh raw/formal/payload/defined-byte controls,4 finite source escapes,3 pending-role controls,3 compile negatives. Full500B payloadprefix nowcontextual;508B allocationtail last4 remainszero-writer only. Callbackbody remainsmaskedINT3; error typedfastcall3formals,clocknoncallable.'})

if __name__=='__main__':main()
