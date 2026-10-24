"""Bind the actual original GetSystemInfo response for a native-only adapter."""
import importlib.util,json,struct
from pathlib import Path
from provenance import PROJECT,ROOT,sha,freeze
from allocation_imports import export_rva

def encode(directory,output,candidate_path):
 directory=directory.resolve();receipt=json.loads((directory/'receipt.json').read_text())
 assert receipt['kind']=='explicit-original-capture'and receipt['mode']=='masked'and receipt['exit']==125
 for name,row in receipt['artifacts'].items():
  path=directory/name;assert path.is_file()and path.stat().st_size==row['size']and sha(path)==row['sha256']
 assert sha(directory/'candidate.json')==sha(candidate_path)
 assert receipt['target_sha256']==sha(ROOT/'toolchains/5.10/bin/C1.DLL')==sha(directory/'loaded-target.bin')
 assert receipt['canonical_crt_sha256']==sha(ROOT/'toolchains/5.10/bin/MSVCRT.DLL')==sha(directory/'loaded-msvcrt.bin')
 case=json.loads((directory/'candidate.json').read_text());assert case['case']=='minimal'and case['expected_frontier']['kind']in ('post-system-info','arena-initialization','arena-reservation','arena-size-calculation','arena-initialized','arenas-complete','default-pool-arena','pool-refill','arena-page-allocation','arena-commit','command-line-entry','command-line-helper','public-crt-entry','multibyte-scanner-complete','path-fold-complete','environment-arguments-complete','option-vectors-complete','argument-cursor-complete','option-match-action','option-action-integer','option-integers-complete','option-path-normalizer','path-normalizer-complete','packing-initializer-allocator','packing-allocation-complete','packing-frame-complete')
 assert receipt['probe']in ('normal','restore')
 raw=(directory/'system-info-after-000.bin').read_bytes();assert len(raw)==36
 assert sha(directory/'test.c')==case['input_files'][0]['sha256']
 # Provider/argument/error/neighbor proof belongs to the target adapter; the
 # shared encoder supplies only its tagged byte format, never a host query.
 import re
 events=(directory/'events.txt').read_text();module=int(re.findall(r'^module name=c1n32_l.dll actual=([0-9a-f]+) ',events,re.M)[0],16)
 kernel=int(re.findall(r'^module name=KERNEL32.dll actual=([0-9a-f]+) ',events,re.M)[0],16)
 provider=kernel+export_rva(directory/'loaded-kernel32.bin','GetSystemInfo')
 before=re.findall(r'^systeminfo-before seq=(\d+) pc=([0-9a-f]+) esp=([0-9a-f]+) argument=([0-9a-f]+) provider=([0-9a-f]+) error=(\d+) fpu=([0-9a-f]+)$',events,re.M)
 after=re.findall(r'^systeminfo-after seq=(\d+) pc=([0-9a-f]+) esp=([0-9a-f]+) before-esp=([0-9a-f]+) error=(\d+) before-error=(\d+) fpu=([0-9a-f]+)$',events,re.M)
 assert len(before)==len(after)==1;b,a=before[0],after[0]
 assert int(b[1],16)==module+0x127ec and int(a[1],16)==module+0x127f2 and int(b[3],16)==module+0x5f508 and int(b[4],16)==provider
 assert int(a[2],16)==int(b[2],16)+4 and int(a[3],16)==int(b[2],16)
 assert int(b[5])==int(a[4])==int(a[5])==0 and int(b[6],16)&0xffff==int(a[6],16)&0xffff==0x027f
 first=(directory/('compiler-data-%03d.bin'%int(b[0]))).read_bytes();last=(directory/('compiler-data-%03d.bin'%int(a[0]))).read_bytes();wanted=bytearray(first);wanted[0x1508:0x152c]=raw;assert bytes(wanted)==last
 for region in ('compiler-bss','compiler-snapshot','state'):
  assert(directory/('%s-%03d.bin'%(region,int(b[0])))).read_bytes()==(directory/('%s-%03d.bin'%(region,int(a[0])))).read_bytes()
 encoder=ROOT/'lib/recon/system_profile.py';frozen=freeze([encoder,Path(__file__)],output.parent/'profile-controller-snapshot.json');spec=importlib.util.spec_from_file_location('recon_system_profile',encoder);shared=importlib.util.module_from_spec(spec);spec.loader.exec_module(shared)
 reference=sha(directory/'receipt.json');shared.write_system_info_profile(output,reference,raw)
 assert all(sha(ROOT/name)==row['sha256']for name,row in frozen.items())
 assert output.read_bytes()==b'RSI1'+reference.encode('ascii')+raw
 binding=dict(kind='actualGetSystemInfo-only originalresponse closure',original_directory=str(directory),original_receipt_sha256=reference,candidate_sha256=sha(candidate_path),environment_sha256=sha(directory/'invoke-environment.bin'),canonical_target_sha256=receipt['target_sha256'],canonical_crt_sha256=receipt['canonical_crt_sha256'],provider_hfile_sha256=sha(directory/'loaded-kernel32.bin'),provider_export_rva=provider-kernel,system_info_sha256=sha(directory/'system-info-after-000.bin'),profile_sha256=sha(output),profile_bytes=104,encoder_path=str(encoder.relative_to(ROOT)),encoder_sha256=sha(encoder),factory_sha256=sha(Path(__file__)),memory_status_supplied=False)
 (output.parent/'system-info-binding.json').write_text(json.dumps(binding,indent=2)+'\n');return reference
