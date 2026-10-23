#!/usr/bin/env python3
"""Focused writer memory, actual-formal, callback and source-escape controls."""
import copy
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile
import time
from compare import compare
from state import BINARY,ROOT,sha
import writer_evidence

def main():
    import argparse
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--original',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();begin=time.monotonic();positive=compare(a.original,a.native);rejects=[]
    def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n')
    def native(root,section,change):
        path=root/'n/state.json';s=json.loads(path.read_text());change(s[section]);write(path,s)
        path=root/'n/gdb.stdout';prefix='C2-GLOBALS-COMPLETE'if section=='globals_complete'else'C2-OBJECT-WRITER'
        path.write_text(re.sub('^'+prefix+r' .*$',lambda m:prefix+' '+json.dumps(s[section],sort_keys=True),path.read_text(),flags=re.M))
        path=root/'n/receipt.json';r=json.loads(path.read_text());r['state_sha256']=sha((root/'n/state.json').read_bytes());r['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write(path,r)
    def byte(root,name,index):
        path=root/'o'/name;data=bytearray(path.read_bytes());data[index]^=1;path.write_bytes(data)
        path=root/'o/receipt.json';r=json.loads(path.read_text());r['globals_complete_hashes'][name]=sha(data);write(path,r)
    def writer_byte(root,index):
        def change(s):
            value=s['writer'];raw=bytearray.fromhex(value['pool_bytes']);raw[index]^=1;value['pool_bytes']=raw.hex()
        native(root,'globals_complete',change)
    def original_formal(root):
        path=root/'o/events.txt';path.write_text(re.sub(r'^(object-writer-input writer=)[0-9a-f]+',lambda m:m[1]+'00000000',path.read_text(),flags=re.M))
        path=root/'o/receipt.json';r=json.loads(path.read_text());r['events_sha256']=sha((root/'o/events.txt').read_bytes());write(path,r)
    def artifact(root):
        path=root/'n/receipt.json';r=json.loads(path.read_text());item=r['artifacts'][0];data=b'x';(root/'n'/item['path']).write_bytes(data);item.update(bytes=1,sha256=sha(data));write(path,r)
    controls=[('actual writer formal omitted',lambda r:native(r,'object_writer',lambda v:v.__setitem__('formals',[]))),
        ('actual writer formal type changed',lambda r:native(r,'object_writer',lambda v:v.__setitem__('formals',[{'name':'writer','type':'void *'}]))),
        ('caller writer pointer alias lost',lambda r:native(r,'object_writer',lambda v:v.__setitem__('caller_writer_alias',False))),
        ('writer points to wrong allocated offset',lambda r:native(r,'object_writer',lambda v:v.__setitem__('pool_offset',12))),
        ('writer output FILE identity changed',lambda r:native(r,'object_writer',lambda v:v.__setitem__('file_index',5))),
        ('writer actual original ECX changed',original_formal),
        ('writer initialized format differs',lambda r:writer_byte(r,8)),
        ('writer pending04 stored value differs',lambda r:writer_byte(r,12)),
        ('writer callback identity differs',lambda r:writer_byte(r,24)),
        ('writer untouched padding differs',lambda r:writer_byte(r,39)),
        ('writer initialized full tail differs',lambda r:writer_byte(r,4095)),
        ('original writer source alias differs',lambda r:byte(r,'writer-pool.bin',16)),
        ('original writer callback target differs',lambda r:byte(r,'writer-pool.bin',28)),
        ('original writer source string differs',lambda r:byte(r,'writer-pool.bin',40)),
        ('original output FILE mode differs',lambda r:byte(r,'output-file.bin',12)),
        ('partial object gains unexpected byte',artifact)]
    flags=['clang','-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-Wno-ignored-attributes','-DRECON_NATIVE_TEST','-I'+str(ROOT/'lib/recon/include'),'-I'+str(ROOT/'lib/recon/native'),'-I'+str(BINARY/'src'),'-fsyntax-only']
    with tempfile.TemporaryDirectory(prefix='writer-boundary-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        folder=Path(folder)
        for i,(name,change)in enumerate(controls):
            root=folder/str(i);shutil.copytree(a.original,root/'o');shutil.copytree(a.native,root/'n');change(root)
            try:compare(root/'o',root/'n')
            except (AssertionError,KeyError,ValueError)as e:rejects.append({'control':name,'reason':str(e)})
            else:raise AssertionError('writer mutation accepted: '+name)
        clone=folder/'model';shutil.copytree(BINARY/'src',clone/'src');(clone/'build').mkdir();(clone/'cases/analysis').mkdir(parents=True)
        for name in ('c2.exe','c2.map'):shutil.copyfile(BINARY/'build'/name,clone/'build'/name)
        shutil.copyfile(BINARY/'cases/analysis/object-output-initialization.json',clone/'cases/analysis/object-output-initialization.json')
        config=json.loads((a.original/'configuration.json').read_text());graph=json.loads((a.original/'graph-audit.json').read_text());writer_evidence.BINARY=clone
        assert writer_evidence.verify(config,graph)
        for name,snippet in [('pending member direct invocation','void bad(C2ObjectOutput *p) { p->output_allocator(1); }'),
            ('pending member target invocation','void bad(C2ObjectOutput *p) { p->output_error.target(1); }'),
            ('pending member escape','void bad(C2ObjectOutput *p) { void *q = &p->output_clock; }'),
            ('pending target direct invocation','void bad(void) { writer_callback_45477a(); }')]:
            path=clone/'src/main.c';before=path.read_bytes();path.write_bytes(before+b'\n'+snippet.encode()+b'\n')
            try:writer_evidence.verify(config,graph)
            except AssertionError as e:rejects.append({'control':name,'reason':str(e)})
            else:raise AssertionError('writer source escape accepted')
            path.write_bytes(before)
        modified=copy.deepcopy(config);next(r for r in modified['stubs']if r['native']=='writer_callback_41eb8a')['pending_signature']=False
        try:writer_evidence.verify(modified,graph)
        except AssertionError:rejects.append({'control':'erased pending callback signature guard','reason':'pending signature hold'})
        else:raise AssertionError('pending signature removal accepted')
        writer_evidence.BINARY=BINARY
        path=folder/'layout.c';path.write_text('#include "coff-output.h"\n#include <recon.h>\n');r=subprocess.run(flags+[str(path)],capture_output=True);assert r.returncode==0,r.stderr.decode()
        for field in ('output_allocator','output_error','output_clock'):
            path.write_text('#include "coff-output.h"\n#include <recon.h>\nvoid bad(C2ObjectOutput *p) { p->'+field+'(); }\n');r=subprocess.run(flags+[str(path)],capture_output=True);assert r.returncode!=0 and b'not a function'in r.stderr
            rejects.append({'control':'compile rejects '+field+' directcall','diagnostic_sha256':sha(r.stderr)})
    a.output.write_text(json.dumps({'positive':positive,'count':len(rejects),'rejected':rejects,'seconds':time.monotonic()-begin,'original_receipt_sha256':sha((a.original/'receipt.json').read_bytes()),'native_receipt_sha256':sha((a.native/'receipt.json').read_bytes()),'checker_sha256':sha(Path(__file__).read_bytes())},indent=2)+'\n')

if __name__=='__main__':main()
