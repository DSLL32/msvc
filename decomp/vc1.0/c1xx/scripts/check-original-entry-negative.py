"""Explicit original-only rejection of an incorrect cached first-entry order."""
import argparse
import fcntl
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import time
from case_paths import BINARY,CAPTURE_ROOT,ROOT,SCRATCH,canonical_parts,paths,windows


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture',type=Path)
    args=parser.parse_args()
    before=json.loads((args.capture/'receipt.json').read_text())
    assert 'first_entry_projection' in before
    spec=importlib.util.spec_from_file_location('c1xx_original_tool',BINARY/'scripts/wine-tool.py')
    tool=importlib.util.module_from_spec(spec);spec.loader.exec_module(tool)
    config=json.loads((BINARY/'functions.json').read_text())
    _,fixture,_,_=paths(config,before['case'])
    phase=json.loads((fixture/'manifest.json').read_text())['phases']['c1']
    canonical=phase['canonical_working_directory']
    directory=Path(tempfile.mkdtemp(prefix='wrong-original-entry-',dir=CAPTURE_ROOT))
    rows=(args.capture/'entries.plan').read_text().splitlines()
    assert len(rows)>2
    rows[0],rows[1]=rows[1],rows[0]
    plan=directory/'wrong.plan';plan.write_text('\n'.join(rows)+'\n')
    work=SCRATCH.joinpath(*canonical_parts(canonical))
    with (CAPTURE_ROOT/'capture.lock').open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX)
        for p in work.iterdir():
            assert p.is_file() and not p.is_symlink();p.unlink()
        for item in phase['inputs']:shutil.copyfile(fixture/item['path'],work/Path(item['path']).name)
        flags=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(tool.TC/'bin'),include=windows(tool.TC/'include'))
        command=['wine',windows(BINARY/'build/debug-run.exe'),windows(args.capture/'masked-original.exe'),'',
            windows(directory/'stdout.bin'),windows(directory/'stderr.bin'),windows(directory/'events.txt'),canonical,
            'MSC_CMD_FLAGS='+flags,phase['command_line_template'].format(bin=windows(tool.TC/'bin')),
            ','.join('%x'%(s['original']-0x10000) for s in config['buffered_streams']),
            ','.join('%x:%s'%(s['original']-0x10000,'p' if s.get('pointer') else 'v') for s in config['checkpoint_state']),windows(plan)]
        started=time.perf_counter()
        result=subprocess.run(command,env=tool.environment(),stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=90)
        elapsed=time.perf_counter()-started
    raw=(directory/'events.txt').read_text()
    assert result.returncode==5 and 'entry-divergence expected ' in raw,(result.returncode,raw)
    receipt=dict(status='pass',scope='Wrong first-entry plan rejected by fresh original debugger; no frontend acceptance',
        helper_status=result.returncode,original_execution_seconds=elapsed,source_capture=str(args.capture),
        first_expected=rows[0],first_actual=(args.capture/'entries.plan').read_text().splitlines()[0])
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(directory/'receipt.json')


if __name__=='__main__':main()
