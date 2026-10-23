"""Reject incorrect compiler-table provenance without running either executable."""
import argparse
from importlib.machinery import SourceFileLoader
import json
from pathlib import Path
import re
import struct
import tempfile
import time

HERE=Path(__file__).resolve().parents[1]
checker=SourceFileLoader('dispatcher_switch',str(HERE/'scripts/check-dispatcher-switch.py')).load_module()
def run():
    started=time.perf_counter();proof=checker.verify();controls=[]
    with tempfile.TemporaryDirectory(prefix='switch-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/rcdll')as scratch:
        root=Path(scratch);obj=root/'preprocess.obj';pe=root/'RCDLL.DLL';mapping=root/'rcdll.map'
        original_obj=(HERE/'build/preprocess.obj').read_bytes();original_pe=(HERE/'build/RCDLL.DLL').read_bytes();original_map=(HERE/'build/rcdll.map').read_bytes()
        obj.write_bytes(original_obj);pe.write_bytes(original_pe);mapping.write_bytes(original_map)
        assert checker.verify(obj,pe,mapping)==proof
        def reject(name,path,raw,at):
            changed=bytearray(raw);changed[at]^=0x40;path.write_bytes(changed)
            try:checker.verify(obj,pe,mapping)
            except AssertionError:controls.append(name)
            else:raise AssertionError('bad table passed: '+name)
            path.write_bytes(raw)
        section=20+40*(proof['object_section']-1);raw=struct.unpack_from('<I',original_obj,section+20)[0]
        reject('wrong-COFF-selector-index',obj,original_obj,raw+proof['object_remap'][0])
        pe_header=struct.unpack_from('<I',original_pe,60)[0];optional=pe_header+24;base=struct.unpack_from('<I',original_pe,optional+28)[0]
        def offset(address):
            for index in range(struct.unpack_from('<H',original_pe,pe_header+6)[0]):
                at=optional+struct.unpack_from('<H',original_pe,pe_header+20)[0]+40*index
                _,start,size,raw=struct.unpack_from('<IIII',original_pe,at+8)
                if start<=address-base<start+size:return raw+address-base-start
            raise AssertionError('unbacked table')
        table=proof['text_data'][0][0];remap=table+proof['object_remap'][0]-proof['object_table'][0]
        reject('wrong-linked-switch-target',pe,original_pe,offset(table))
        reject('wrong-linked-remap-byte',pe,original_pe,offset(remap))
        text=original_map.decode();match=re.search(r'(_rc_option_number@4\s+)([0-9a-fA-F]+)',text);assert match
        text=text[:match.start(2)]+('%08x'%(int(match[2],16)+1))+text[match.end(2):];mapping.write_text(text)
        try:checker.verify(obj,pe,mapping)
        except AssertionError:controls.append('wrong-following-MAP-public')
        else:raise AssertionError('wrong following public passed')
        mapping.write_bytes(original_map);assert checker.verify(obj,pe,mapping)==proof
    return dict(proof=proof,negative_controls=controls,seconds=time.perf_counter()-started,checker_sha256=checker.sha(HERE/'scripts/check-dispatcher-switch.py'),controls_sha256=checker.sha(Path(__file__)))
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--output',type=Path);args=p.parse_args();result=run()
    if args.output:args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
