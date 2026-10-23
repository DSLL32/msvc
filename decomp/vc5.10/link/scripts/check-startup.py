#!/usr/bin/env python3
"""Raw typed startup-boundary and mutation checks; never invokes Wine."""
import argparse
import copy
import json
from pathlib import Path
import struct
import tempfile
from accept import BINARY,bind_capture,verify_frontier
from boundary_state import compare


def clone(source, changed, data):
    directory=Path(tempfile.mkdtemp(prefix='state-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/link'))
    for path in source.iterdir():
        if path.is_file():
            if path.name==changed:(directory/path.name).write_bytes(data)
            else:(directory/path.name).symlink_to(path)
    return directory


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original',type=Path,required=True)
    parser.add_argument('--native',type=Path,required=True)
    args=parser.parse_args()
    trial=json.loads((args.original/'functions.json').read_text())
    bind_capture(args.original,trial,'original');bind_capture(args.native,trial,'native')
    verify_frontier(args.original,args.native,'_initialize_buffer12',trial)
    events=[json.loads(line) for line in (args.original/'events.jsonl').read_text().splitlines()]
    original=next(event for event in events if event['event']=='trap')
    native=json.loads((args.native/'state.json').read_text())
    checks=[]
    def negative(label,orig_state=original,native_state=native,orig_dir=args.original):
        try:compare('_initialize_buffer12',orig_dir,args.native,orig_state,native_state)
        except (AssertionError,KeyError):checks.append(label);return
        raise AssertionError('mutation accepted: '+label)
    for key,value in [('x87_control_word',0x037f),('crt_fmode',0x4000),('crt_commode',1),
                      ('crt_adjust_fdiv',1),('initerm_current_index',2),('initerm_end_index',4),
                      ('crt_argv',['LINK.EXE','/?'])]:
        state=copy.deepcopy(native);state[key]=value;negative('wrong '+key,native_state=state)
    state=copy.deepcopy(native);state['initializers'][1],state['initializers'][2]=state['initializers'][2],state['initializers'][1]
    negative('reordered native initializer slots',native_state=state)
    state=copy.deepcopy(native);state['initerm_calls'].reverse();negative('reordered imported calls',native_state=state)
    state=copy.deepcopy(native);state['initerm_calls'].insert(1,[]);negative('extra imported call',native_state=state)
    state=copy.deepcopy(native);state['app_globals']['buffer12']='01'+state['app_globals']['buffer12'][2:]
    negative('changed native constructor state',native_state=state)
    state=copy.deepcopy(original);state['esi']+=4;negative('wrong original callback cursor',orig_state=state)
    directory=clone(args.original,'crt-fmode.bin',struct.pack('<i',0x4000));negative('changed original consumed CRT state',orig_dir=directory)
    values=list(struct.unpack('<5I',(args.original/'initializers.bin').read_bytes()));values[1],values[2]=values[2],values[1]
    directory=clone(args.original,'initializers.bin',struct.pack('<5I',*values));negative('reordered original callback slots',orig_dir=directory)
    report=dict(positive='fresh first-constructor raw state and import-call comparisons pass',negative_count=len(checks),negative_checks=checks,accepted_application_functions=0)
    (BINARY/'cases/help/startup-boundary/negative-validation.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
