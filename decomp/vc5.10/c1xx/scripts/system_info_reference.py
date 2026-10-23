"""Read-only closure for this target's captured CRT inputs and GetSystemInfo."""
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import struct


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def prepare(project,original,output,trial):
    original=original.resolve();root=project.parents[2]
    receipt=json.loads((original/'receipt.json').read_text())
    if (receipt['kind'],receipt['mode'],receipt['exit'])!=('explicit-original-capture','masked',125):raise ValueError('not a completed original masked frontier')
    for name,row in receipt['artifacts'].items():
        file=original/name
        if Path(name).name!=name or not file.is_file() or sha(file)!=row['sha256'] or file.stat().st_size!=row['size']:raise ValueError('original response artifact drift '+name)
    if (original/'trial.json').read_bytes()!=(output/'trial.json').read_bytes():raise ValueError('original response belongs to different trial')
    for side in ('target','msvcrt'):
        canonical=root/('toolchains/5.10/bin/C1XX.DLL' if side=='target' else 'toolchains/5.10/bin/MSVCRT.DLL')
        key='target_sha256' if side=='target' else 'canonical_crt_sha256'
        if sha(original/('loaded-'+side+'.bin'))!=receipt[key] or receipt[key]!=sha(canonical):raise ValueError('original provider identity differs')
    for item,digest in trial['case_inputs'].items():
        file=original/Path(item).name
        if sha(file)!=digest:raise ValueError('original case input differs '+item)
    from allocation_imports import export_rva
    events=(original/'events.txt').read_text();kernel=int(re.search(r'^module name=KERNEL32.dll actual=([0-9a-f]+) ',events,re.M)[1],16)
    before=re.findall(r'^system-info-before pc=(\w+) esp=(\w+) argument=(\w+) provider=(\w+) edi=(\w+) esi=(\w+) ebx=(\w+) error=(\d+) fpu=(\w+)$',events,re.M)
    after=re.findall(r'^system-info-after pc=(\w+) esp=(\w+) edi=(\w+) esi=(\w+) ebx=(\w+) error=(\d+) fpu=(\w+)$',events,re.M)
    if len(before)!=1 or len(after)!=1:raise ValueError('not one actual system info call')
    b,a=before[0],after[0]
    if [int(b[i],16) for i in (0,2,3)]!=[0x10438c42,0x104cd7b8,kernel+export_rva(original/'loaded-kernel32.bin','GetSystemInfo')] or int(a[0],16)!=0x10438c48 or int(a[1],16)!=int(b[1],16)+4 or a[2:5]!=b[4:7]:raise ValueError('original GetSystemInfo ABI/provider differs')
    if int(b[7])!=203 or int(a[5])!=203 or int(b[8],16)&0xffff!=0x27f or int(a[6],16)&0xffff!=0x27f:raise ValueError('original API error/FPU differs')
    raw=(original/'system-info-after.bin').read_bytes()
    if len(raw)!=36:raise ValueError('not a complete SDK output')
    data=bytearray((original/'frontend-data-system-info-before.bin').read_bytes());data[0x17b8:0x17dc]=raw
    if bytes(data)!=(original/'frontend-data-system-info-after.bin').read_bytes():raise ValueError('original API changed adjacent initialized bytes')
    for name in ('frontend-snapshot','system-info-bss'):
        prefix=name+'-' if name=='system-info-bss' else name+'-system-info-'
        if (original/(prefix+'before.bin')).read_bytes()!=(original/(prefix+'after.bin')).read_bytes():raise ValueError('original API changed '+name)
    for name in ('original-crt-argv.bin','original-crt-environment.bin'):
        (output/name).write_bytes((original/name).read_bytes())
    encoder=root/'lib/recon/system_profile.py';factory=Path(__file__);controller={}
    for file,name in ((encoder,'system-profile-encoder.py'),(factory,'system-profile-factory.py')):
        blob=file.read_bytes();(output/name).write_bytes(blob);controller[str(file.relative_to(root))]=dict(sha256=sha(file),artifact=name)
    spec=importlib.util.spec_from_file_location('system_profile_encoder',encoder);shared=importlib.util.module_from_spec(spec);spec.loader.exec_module(shared)
    reference=sha(original/'receipt.json');profile=output/'system-info-profile.bin';shared.write_system_info_profile(profile,reference,raw)
    binding=dict(kind='C1XX actual GetSystemInfo-only response and original HOST CRT vectors; Invoke argv remains separate',original_directory=str(original),original_receipt_sha256=reference,trial_sha256=sha(original/'trial.json'),target_sha256=receipt['target_sha256'],canonical_crt_sha256=receipt['canonical_crt_sha256'],provider_hfile_sha256=sha(original/'loaded-kernel32.bin'),provider_export_rva=export_rva(original/'loaded-kernel32.bin','GetSystemInfo'),raw_output_sha256=sha(original/'system-info-after.bin'),profile_sha256=sha(profile),profile_bytes=104,GlobalMemoryStatus_supplied=False,original_crt_inputs={name:sha(original/name) for name in ('original-crt-argv.bin','original-crt-environment.bin')},controllers=controller)
    (output/'system-info-binding.json').write_text(json.dumps(binding,indent=2)+'\n')
    if any(sha(root/path)!=row['sha256'] for path,row in controller.items()):raise ValueError('profile factory changed during preparation')
    return reference
