"""Mutate an isolated current C2 audit snapshot; never run a target or Wine."""
import argparse
from contextlib import contextmanager
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import time
from binary import PE
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from case_paths import C2_SCRATCH, ROOT

BINARY = Path(__file__).resolve().parents[1]
def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

@contextmanager
def modified(paths):
    saved = {path:path.read_bytes() for path in paths}
    try:
        yield
    finally:
        for path,data in saved.items():
            path.write_bytes(data)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary',type=Path,required=True)
    args = parser.parse_args()
    tool = args.binary.resolve()
    start = time.perf_counter()
    snapshot = Path(tempfile.mkdtemp(prefix='rust-graph-negatives-',dir=C2_SCRATCH))
    project = snapshot/'decomp/vc1.0/c2'
    paths = ['functions.json','runtime.json','build/c2.exe','build/c2.map','build/runtime.json']
    frozen = {p:sha(BINARY/p) for p in paths}
    for relative in paths:
        target = project/relative
        target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(BINARY/relative,target)
    shutil.copytree(BINARY/'scripts',project/'scripts')
    (project/'cases').mkdir()
    original_path = snapshot/'toolchains/1.0-nt/bin/C23232.EXE'
    original_path.parent.mkdir(parents=True)
    shutil.copyfile(ROOT/'toolchains/1.0-nt/bin/C23232.EXE',original_path)
    config = project/'functions.json'
    exe, mapfile, runtime = project/'build/c2.exe', project/'build/c2.map', project/'runtime.json'
    info = json.loads(subprocess.check_output([sys.executable,str(ROOT/'tooling/graph-audit/mutation-info.py'),'c2'],cwd=project))
    command = [str(tool),'--repo',str(snapshot),'--project',str(project),'--profile','c2',
        '--discovery','cfg','--check-ownership','--check-extents','--check-accepted-extents']
    records = []
    def run(command):
        phase = time.perf_counter()
        result = subprocess.run(command,cwd=project,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=30)
        return dict(status=result.returncode,seconds=time.perf_counter()-phase,
            stdout_sha256=hashlib.sha256(result.stdout).hexdigest(),
            diagnostic=result.stderr.decode(errors='replace')[-1600:])
    def reject(label):
        python = run([sys.executable,'scripts/audit-graph.py'])
        rust = run(command)
        assert python['status'] != 0 and rust['status'] != 0,(label,python,rust)
        records.append(dict(mutation=label,python=python,rust=rust))
    baseline = run(command)
    assert baseline['status'] == 0, baseline
    with modified([exe]):
        data = bytearray(exe.read_bytes());call = info['call']
        struct.pack_into('<I',data,call['offset']+1,(0xdeadbeef-call['address']-5)&0xffffffff)
        exe.write_bytes(data);reject('wrong direct callee')
    with modified([exe]):
        data = bytearray(exe.read_bytes())
        for call in info['call']['all_calls']:
            data[call['offset']:call['offset']+5] = b'\x90'*5
        exe.write_bytes(data);reject('removed direct call edges')
    with modified([mapfile]):
        name = re.escape(info['first']['name'])
        changed,count = re.subn(r'(?m)^(\s+[0-9a-f]{4}:[0-9a-f]{8}\s+)'+name+r'(\s+[0-9a-f]{8}.*)$',
            r'\1__c2_audit_missing_public\2',mapfile.read_text())
        assert count
        mapfile.write_text(changed);reject('missing application public mapping')
    with modified([config]):
        data = json.loads(config.read_text())
        row = next(r for r in data['driver'] if r['name'] == info['truncated']['name'])
        row['end'] = info['truncated']['end'];config.write_text(json.dumps(data))
        reject('split application RET operand')
    with modified([runtime]):
        data = json.loads(runtime.read_text());accepted = set(json.loads(config.read_text())['accepted'])
        original = PE(original_path);decoder = Cs(CS_ARCH_X86,CS_MODE_32)
        mutation = None
        for row in data:
            if row['name'] not in accepted:continue
            call = next((i for i in decoder.disasm(original.read(row['start'],row['end']-row['start']),row['start'])
                         if i.mnemonic == 'call' and i.size > 1),None)
            if call is not None:
                mutation = row['name'];row['end'] = call.address+1;break
        assert mutation
        runtime.write_text(json.dumps(data));reject('split accepted CRT call operand: '+mutation)
    # Global RET inference cannot prove the finish_debugging indirect tail.
    # Its existing narrow application-ABI proof must independently reject a
    # changed cleanup operand even when a call-graph-only comparison passes.
    with modified([exe]):
        data = bytearray(exe.read_bytes());image = PE(exe)
        publics = {}
        for line in mapfile.read_text().splitlines():
            match = re.match(r'\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',line)
            if match:publics[match[1]] = int(match[2],16)
        address = publics['@set_generated_intrinsic_call@16']
        instruction = next(i for i in Cs(CS_ARCH_X86,CS_MODE_32).disasm(image.read(address,256),address) if i.mnemonic=='ret')
        assert instruction.op_str == '8'
        struct.pack_into('<H',data,image.offset(instruction.address)+1,4);exe.write_bytes(data)
        result = run([sys.executable,'scripts/check-frontier-abi.py'])
        assert result['status'] != 0
        records.append(dict(mutation='changed generated-call RET8 to RET4',narrow_python_abi=result))
    final = run(command)
    assert final['status'] == 0 and {p:sha(BINARY/p) for p in paths} == frozen
    receipt = dict(method='Current isolated canonical C2 snapshot mutations; no Wine or target execution',
        input_sha256=frozen,tool_sha256=sha(tool),command_flags=command[command.index('--profile'):],
        baseline=baseline,checks=records,restored_positive=final,seconds=time.perf_counter()-start,
        snapshot=str(snapshot),ret_scope='15 evidence-backed application ABIs; global indirect-tail ABI unresolved')
    (BINARY/'cases/rust-audit-negatives.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print('%d current C2 Rust/Python or narrow-ABI negative checks pass'%len(records))

if __name__ == '__main__':main()
