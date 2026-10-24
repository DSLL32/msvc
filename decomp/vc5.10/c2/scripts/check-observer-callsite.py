#!/usr/bin/env python3
"""Current ELF observer bytes/owner bounds and authenticated receipt controls."""
import argparse
import importlib.util
import json
from pathlib import Path
import shutil
import tempfile
import time
from compare import compare
from state import BINARY,sha


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('original','native','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();begin=time.monotonic();positive=compare(a.original,a.native);passed=[]
    spec=importlib.util.spec_from_file_location('observer',BINARY/'scripts/native-proof.py');mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
    elf=BINARY/'build/native/c2';actual=mod.verify_callsite(elf,'write_output_directives',73,b'\xff\xd0')
    assert actual==json.loads((a.native/'receipt.json').read_text())['directive_callsite_proof']
    with tempfile.TemporaryDirectory(prefix='observer-callsite-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        for i,(name,field,value)in enumerate([('wrong current ELF hash','elf_sha256','0'*64),('wrong named observer owner','owner','select_output_section'),('wrong current symbol address','symbol_start',actual['symbol_start']+1),('wrong symbol bounds','symbol_size',actual['symbol_size']+1),('wrong instruction offset','offset',74),('wrong instruction bytes','bytes','ffd1')]):
            root=Path(folder)/str(i);shutil.copytree(a.native,root/'n');path=root/'n/receipt.json';r=json.loads(path.read_text());assert r['directive_callsite_proof'][field]!=value;r['directive_callsite_proof'][field]=value;path.write_text(json.dumps(r,indent=2)+'\n')
            try:compare(a.original,root/'n')
            except (AssertionError,KeyError,ValueError)as error:passed.append({'control':name,'reason':str(error)})
            else:raise AssertionError('observer receipt mutation accepted: '+name)
        for name,owner,offset,expected in [('unreal observer owner','no_such_function',73,b'\xff\xd0'),('outside owner bounds','write_output_directives',actual['symbol_size'],b'\xff\xd0'),('instruction shifted one byte','write_output_directives',74,b'\xff\xd0'),('actual CALL EAX is not CALL ECX','write_output_directives',73,b'\xff\xd1')]:
            try:mod.verify_callsite(elf,owner,offset,expected)
            except AssertionError as error:passed.append({'control':name,'reason':str(error)})
            else:raise AssertionError('observer predicate mutation accepted: '+name)
    a.output.write_text(json.dumps({'positive':positive,'current_instruction':actual,'count':len(passed),'rejected':passed,'seconds':time.monotonic()-begin,'checker_sha256':sha(Path(__file__).read_bytes()),'predicate_sha256':sha((BINARY/'scripts/native-proof.py').read_bytes()),'original_receipt_sha256':sha((a.original/'receipt.json').read_bytes()),'native_receipt_sha256':sha((a.native/'receipt.json').read_bytes()),'scope':'Finite observerinstruction only. Full ELF execution/state/target/bias provenance remains independently checked; historical contract ELF reference retained unchanged.'},indent=2)+'\n')

if __name__=='__main__':main()
