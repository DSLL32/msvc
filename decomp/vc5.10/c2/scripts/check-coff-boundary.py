#!/usr/bin/env python3
"""Affected COFF copied-global/formal and still-pending callback controls."""
import copy
import json
from pathlib import Path
import re
import shutil
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
    def write(path,value):path.write_text(json.dumps(value,indent=2)+'\n')
    def native(root,section,change):
        path=root/'n/state.json';state=json.loads(path.read_text());change(state[section]);write(path,state)
        path=root/'n/gdb.stdout';prefix='C2-GLOBALS-COMPLETE'if section=='globals_complete'else'C2-OBJECT-WRITER'
        path.write_text(re.sub('^'+prefix+r' .*$',lambda m:prefix+' '+json.dumps(state[section],sort_keys=True),path.read_text(),flags=re.M))
        path=root/'n/receipt.json';receipt=json.loads(path.read_text());receipt['state_sha256']=sha((root/'n/state.json').read_bytes());receipt['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write(path,receipt)
    def original_capacity(root):
        path=root/'o/events.txt';path.write_text(re.sub(r'^(object-writer-input writer=)[0-9a-f]+',lambda m:m[1]+'0000012d',path.read_text(),flags=re.M))
        path=root/'o/receipt.json';receipt=json.loads(path.read_text());receipt['events_sha256']=sha((root/'o/events.txt').read_bytes());write(path,receipt)
    controls=[('table actual formal omitted',lambda r:native(r,'object_writer',lambda v:v.__setitem__('formals',[]))),
        ('table actual formal type changed',lambda r:native(r,'object_writer',lambda v:v.__setitem__('formals',[{'name':'capacity','type':'unsigned int'}]))),
        ('table actual capacity changed',lambda r:native(r,'object_writer',lambda v:v.__setitem__('capacity',301))),
        ('caller writer alias lost',lambda r:native(r,'object_writer',lambda v:v.__setitem__('caller_writer_alias',False))),
        ('original actual capacity changed',original_capacity)]
    state=json.loads((a.native/'state.json').read_text())
    for key in state['globals_complete']['coff']:
        controls.append(('copied/default global '+key,lambda r,key=key:native(r,'globals_complete',lambda v:v['coff'].__setitem__(key,v['coff'][key]^1))))
    flags=['clang','-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-Wno-ignored-attributes','-DRECON_NATIVE_TEST','-I'+str(ROOT/'lib/recon/include'),'-I'+str(ROOT/'lib/recon/native'),'-I'+str(BINARY/'src'),'-fsyntax-only']
    with tempfile.TemporaryDirectory(prefix='coff-boundary-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        folder=Path(folder)
        for index,(name,change)in enumerate(controls):
            root=folder/str(index);shutil.copytree(a.original,root/'o');shutil.copytree(a.native,root/'n');change(root)
            try:compare(root/'o',root/'n')
            except (AssertionError,KeyError,ValueError)as e:rejects.append({'control':name,'reason':str(e)})
            else:raise AssertionError('COFF mutation accepted: '+name)
        clone=folder/'model';shutil.copytree(BINARY/'src',clone/'src');(clone/'build').mkdir();(clone/'cases/analysis').mkdir(parents=True)
        for name in ('c2.exe','c2.map'):shutil.copyfile(BINARY/'build'/name,clone/'build'/name)
        for name in ('object-output-initialization.json','output-allocator-abi.json'):shutil.copyfile(BINARY/'cases/analysis'/name,clone/'cases/analysis'/name)
        config=json.loads((a.original/'configuration.json').read_text());graph=json.loads((a.original/'graph-audit.json').read_text());writer_evidence.BINARY=clone
        assert writer_evidence.verify(config,graph)
        for name,snippet in [
            ('pending member target invocation','void bad(C2ObjectOutput *p) { p->output_error.target(1); }'),
            ('pending member escape','void bad(C2ObjectOutput *p) { void *q = &p->output_clock; }'),
            ('pending copied error invocation','void bad(void) { output_error_handler.target(); }'),
            ('pending copied clock escape','void bad(void) { void *q = &output_clock_handler; }'),
            ('pending target direct invocation','void bad(void) { writer_callback_45477a(); }')]:
            path=clone/'src/main.c';before=path.read_bytes();path.write_bytes(before+b'\n'+snippet.encode()+b'\n')
            try:writer_evidence.verify(config,graph)
            except AssertionError as e:rejects.append({'control':name,'reason':str(e)})
            else:raise AssertionError('pending callback escape accepted')
            path.write_bytes(before)
        changed=copy.deepcopy(config);next(r for r in changed['stubs']if r['native']=='writer_callback_48511c')['pending_signature']=False
        try:writer_evidence.verify(changed,graph)
        except AssertionError:rejects.append({'control':'erased error pending signature hold','reason':'signature remains unknown'})
        else:raise AssertionError('pending signature removal accepted')
        changed=copy.deepcopy(config);next(r for r in changed['stubs']if r['native']=='allocate_output_bytes')['abi']='cdecl guess'
        try:writer_evidence.verify(changed,graph)
        except AssertionError:rejects.append({'control':'typed allocator wrong convention','reason':'complete byte-proven ABI differs'})
        else:raise AssertionError('allocator ABI change accepted')
        writer_evidence.BINARY=BINARY
        path=folder/'layout.c';path.write_text('#include "coff-output.h"\n#include <recon.h>\nvoid *ok(C2ObjectOutput *p) { return p->output_allocator(16); }\n');r=subprocess.run(flags+[str(path)],capture_output=True);assert r.returncode==0,r.stderr.decode()
        for field in ('output_error','output_clock'):
            path.write_text('#include "coff-output.h"\n#include <recon.h>\nvoid bad(C2ObjectOutput *p) { p->'+field+'(); }\n');r=subprocess.run(flags+[str(path)],capture_output=True);assert r.returncode!=0 and b'not a function'in r.stderr
            rejects.append({'control':'compile rejects '+field+' directcall','diagnostic_sha256':sha(r.stderr)})
    a.output.write_text(json.dumps({'positive':positive,'count':len(rejects),'rejected':rejects,'seconds':time.monotonic()-begin,'original_receipt_sha256':sha((a.original/'receipt.json').read_bytes()),'native_receipt_sha256':sha((a.native/'receipt.json').read_bytes()),'checker_sha256':sha(Path(__file__).read_bytes())},indent=2)+'\n')

if __name__=='__main__':main()
