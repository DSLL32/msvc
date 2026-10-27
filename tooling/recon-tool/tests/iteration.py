#!/usr/bin/env python3
"""Authored orchestration controls; synthetic receipts are not original evidence.

No Wine, original target, reconstruction worker or real reference is executed.
The existing native executor/comparator is real; Make compiler/auditor controls
are explicit stand-ins that expose ordering/failure/freshness behavior.
"""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import struct
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
REPO = Path(os.environ.get("RECON_TOOL_TEST_REPO", ROOT.parents[1]))
TOOL = Path(os.environ.get('RECON_TOOL_TEST_EXE', ROOT/'target/i686-unknown-linux-gnu/debug/recon-tool')).resolve()
WORK = Path(tempfile.mkdtemp(prefix='iteration-controls-', dir=Path.home()/'.ai-tmp'))
PROJECT = WORK/'project'
PROJECT.mkdir()
STATE = WORK/'journal'
CASE = PROJECT/'cases/0001-progress'
CASE.mkdir(parents=True)
start = time.monotonic()

def write(path, text):
    path = PROJECT/path
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)
    return path

def sha(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def binding(path): return dict(path=str(path.resolve()), sha256=sha(path))
def invoke(args, ok=True, error=None):
    result = subprocess.run([str(TOOL), *map(str,args)],cwd=PROJECT,text=True,capture_output=True)
    assert (result.returncode==0)==ok, (args,result.stdout,result.stderr)
    if error: assert error in result.stdout+result.stderr,(error,result.stdout,result.stderr)
    return result

def records():
    return sorted((p for p in STATE.glob('*/result.json')),reverse=True)
def latest(): return json.loads(next(p for p in records() if 'synthetic' not in str(p)).read_text())
def attempt(*args, ok=True,error=None):
    result=invoke(['original','iterate','cases/0001-progress','--state',STATE,'--repo',REPO,*args],ok,error)
    return latest(),result

try:
    # Valid minimal PE container for static staging-closure controls; never run.
    image=bytearray(1024);image[:2]=b'MZ';struct.pack_into('<I',image,0x3c,0x80)
    image[0x80:0x84]=b'PE\0\0';struct.pack_into('<HH',image,0x84,0x14c,1)
    struct.pack_into('<H',image,0x94,224);struct.pack_into('<H',image,0x98,0x10b)
    struct.pack_into('<I',image,0xa8,0x1000)
    struct.pack_into('<IIII',image,0x178+8,512,0x1000,512,512)
    struct.pack_into('<I',image,0x178+36,0x60000020)
    (PROJECT/'target.exe').write_bytes(image)
    target=PROJECT/'target.exe'
    write('src/app.c', '''int pending(void){ __asm__ volatile("int3");return 0; }
int owner(void){return pending();} int other(void){return 11;}\n''')
    binary='''#include "recon-tool.h"
static const ReconExtent a[]={{0x1000,0x1010}},b[]={{0x1100,0x1103}},c[]={{0x1200,0x1210}};
static const ReconFunction f[]={
{.name="owner",.entry_rva=0x1000,.code=a,.code_count=1,.definition=RECON_APP,.implementation=RECON_IMPLEMENTED,.native_symbol="owner"},
{.name="pending",.entry_rva=0x1100,.code=b,.code_count=1,.definition=RECON_APP,.implementation=RECON_UNIMPLEMENTED,.native_symbol="pending"},
{.name="other",.entry_rva=0x1200,.code=c,.code_count=1,.definition=RECON_APP,.implementation=RECON_IMPLEMENTED,.native_symbol="other"}};
const ReconBinary binary_descriptor={.abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconBinary),
.name="iteration-authored",.original_path="TARGET",.original_sha256="HASH",.original_entry_rva=0x1000,.functions=f,.function_count=3};
'''.replace('TARGET',str(target)).replace('HASH',sha(target))
    write('binary.c',binary)
    case='''#include "recon-tool.h"
#include <stdio.h>
extern int owner(void);
static int io(int argc,char **argv){ BAD return owner(); }
static int unit(void){return 1;}
static const ReconTest tests[]={{.name="only",.fork=0,.exec=unit}};
const ReconCase case_descriptor={.abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),
.name="iteration-progress",.description="authored stale descriptor",.execution=RECON_IO,.io=io,
.terminal=RECON_FRONTIER,.frontier_rva=0x1000,.tests=tests,.test_count=1};
'''
    write('cases/0001-progress/case.c',case.replace('BAD','fputs("bad\\n",stdout);fflush(stdout);'))
    policy={'accepted_scopes':{'one':['owner']},'ret_checks':{'schema':1,'functions':{
        'owner':{'expected_cleanup':[0]},'other':{'expected_cleanup':[0]}}},
        'graph_metadata':{'schema':1,'functions':{'owner':{'register_imports':{'enabled':True}},
        'other':{'register_imports':{'enabled':True}}},'callback_tables':[],
        'register_imports':{'preservation_contracts':[]}}}
    write('policy.json',json.dumps(policy))
    graph=write('graph', '''#!/usr/bin/env python3
import json,sys
from pathlib import Path
args=sys.argv[1:]
get=lambda flag:Path(args[args.index(flag)+1])
c=json.loads(get('--config').read_text());m=json.loads(get('--metadata').read_text());r=json.loads(get('--ret-checks').read_text()) if '--ret-checks' in args else {'functions':{}}
assert len(c['function_registry'])==3 and len(c['driver'])==3
assert 'register_imports' in m and 'callback_tables' in m
for name in ('owner','other'):
 assert ('register_imports' in m['functions'][name])==(name in c['accepted'])
assert set(r['functions'])==set(c['accepted'])
if Path('fail-graph').exists():sys.exit(8)
get('--report').write_text(json.dumps({'authored_gate_control':True,'accepted':c['accepted']}))
''');graph.chmod(0o755)
    write('Makefile',f'''RECON_TOOL_DIR := {ROOT}
RECON_TOOL := {TOOL}
BINARY_SOURCES := src/app.c
BINARY_SUPPORT_SOURCES := {REPO}/lib/recon/src/trace.c
NATIVE_CPPFLAGS := -DRECON_NATIVE_TEST
ORIGINAL_OUTPUT := build/rebuilt.exe
ORIGINAL_MAP := build/rebuilt.map
RECON_POLICY := policy.json
RECON_IMAGE_BASE := 4194304
RECON_ITERATION_ENABLED := 1
RECON_ITERATION_INPUT := inputs.bin
RECON_AUDIT_PROFILE := cl
RECON_AUDIT_REGISTRY_METADATA := 1
RECON_AUDIT_REGISTRY_RET_CHECKS := 1
RECON_ITERATION_METADATA_PREREQUISITES :=
RECON_AUDIT_FLAGS := --discovery cfg --check-ownership --check-extents --original {target} --rebuilt $(ORIGINAL_OUTPUT) --map $(ORIGINAL_MAP)
include $(RECON_TOOL_DIR)/recon-tool.mk
GRAPH_AUDIT_BIN := {graph}
$(ORIGINAL_OUTPUT) $(ORIGINAL_MAP) &: target.exe
	@mkdir -p build
	@cp target.exe $(ORIGINAL_OUTPUT)
	@echo authored-map > $(ORIGINAL_MAP)
	@echo period-control >> period.log
.PHONY: mutate-consumed
mutate-consumed:
	@echo drift >> target.exe
''')
    write('inputs.json',json.dumps({'args':['fixture'],'partial':{'stdout':'text:','stderr':'text:','reject_unexpected_files':True}}))
    invoke(['capture','pack','inputs.json','inputs.bin'])
    # Explicit baseline only; first narrow use must stop, not silently run a full gate.
    r,_=attempt('--owner','owner','--check-only',ok=False,error='no checked static baseline')
    assert r['original_launches']==0 and not r['checked']
    r,_=attempt('--all-restored','--check-only')
    assert r['checked'] and r['owners']==['owner','other'] and r['original_launches']==0
    assert set(r['plan'])=={'sha256','repo'} and 'functions' not in json.dumps(r['plan'])
    period_before=(PROJECT/'period.log').read_text()
    invoke(['registry-export',CASE,'--image-base','4194304','--policy','policy.json'],False,'frontier must name an unimplemented')
    invoke(['registry-export',CASE,'--progress','--image-base','4194304','--policy','policy.json','--owner','owner','--owner','other','--output',WORK/'both.json'])
    assert json.loads((WORK/'both.json').read_text())['accepted']==['owner','other']
    invoke(['registry-export',CASE,'--progress','--image-base','4194304','--owner','pending'],False,'unaccepted/unknown/duplicate')
    invoke(['registry-export',CASE,'--progress','--image-base','4194304','--owner','owner','--owner','owner'],False,'unaccepted/unknown/duplicate')
    # Named and transient scopes use the same validator and retain identities.
    r,_=attempt('--accepted-scope','one','--check-only')
    assert r['owners']==['owner'] and r['checked']
    # Existing metadata/RET flags without dedicated Make variables remain inputs.
    write('external-metadata.json',json.dumps(policy['graph_metadata']))
    write('external-ret.json',json.dumps(policy['ret_checks']))
    write('filtered-runtime.json','[]')
    mf=(PROJECT/'Makefile').read_text().replace('RECON_AUDIT_REGISTRY_METADATA := 1','RECON_AUDIT_REGISTRY_METADATA :=')
    mf=mf.replace('RECON_AUDIT_REGISTRY_RET_CHECKS := 1','RECON_AUDIT_REGISTRY_RET_CHECKS :=')
    mf+='\nRECON_AUDIT_FLAGS += --metadata external-metadata.json --ret-checks external-ret.json --runtime filtered-runtime.json\n'
    write('Makefile',mf)
    r,_=attempt('--owner','owner','--check-only')
    bound={Path(v['path']).name for v in r['consumed']}
    assert {'external-metadata.json','external-ret.json','filtered-runtime.json'}<=bound
    graph_phase=next(v for v in r['phases'] if v['name']=='graph')
    assert graph_phase['argv'][graph_phase['argv'].index('--runtime')+1]=='filtered-runtime.json'
    # A restoration outside that selected scope cannot inherit baseline coverage.
    write('binary.c',binary.replace('0x1210','0x1211'))
    r,_=attempt('--owner','owner','--check-only',ok=False,error='outside checked selection')
    assert r['original_launches']==0
    write('binary.c',binary)
    r,_=attempt('--owner','owner','--check-only')
    # Full actual artifact hashes protect the interval between gates and capture.
    write('fail-graph','1')
    r,_=attempt('--owner','owner',ok=False,error='graph failed')
    assert r['original_launches']==0 and r['capture'] is None
    (PROJECT/'fail-graph').unlink()
    write('src/app.c','this cannot compile\n')
    r,_=attempt('--owner','owner',ok=False,error='native-build failed')
    assert r['original_launches']==0
    write('src/app.c','int pending(void){__asm__ volatile("int3");return 0;} int owner(void){return pending();} int other(void){return 11;}\n')
    r,_=attempt('--owner','owner','--check-only')
    # A finite residual hook cannot mutate an already-checked consumed input.
    before_make=(PROJECT/'Makefile').read_text()
    before_target=(PROJECT/'target.exe').read_bytes()
    write('Makefile',before_make+'\nRECON_ITERATION_CHECK_TARGETS := mutate-consumed\n')
    r,_=attempt('--owner','owner',ok=False,error='consumed artifact changed')
    assert r['original_launches']==0 and r['capture'] is None
    (PROJECT/'target.exe').write_bytes(before_target)
    write('Makefile',before_make)
    r,_=attempt('--owner','owner','--check-only')
    period_before=(PROJECT/'period.log').read_text()
    # Seed an explicitly synthetic bound reference from the checked plan. This
    # tests journal selection/native execution without pretending an original ran.
    seed=STATE/'999999999999999999999-synthetic'
    seed.mkdir()
    shutil.copy(PROJECT/'inputs.bin',seed/'original.bin')
    shutil.copy(PROJECT/'inputs.bin',seed/'request.bin')
    info=json.loads(invoke(['inspect',CASE,'--progress']).stdout)
    receipt={'case':info['case'],'oracle_written':True,'terminal':'frontier:1100','first_entry_order':[0x1000,0x1100],
             'identity':{k:info[k] for k in ['case','binary','original_path','original_sha256','execution','first_entry_seen']},
             'capture_sha256':sha(seed/'original.bin'),'purpose':'synthetic control; NO original execution'}
    (seed/'original.json').write_text(json.dumps(receipt))
    ref=r.copy();ref.update(step=seed.name,kind='original',capture=binding(seed/'original.bin'),original_report=binding(seed/'original.json'),native_pass=False)
    shutil.copy(records()[0].parent/'make.conf',seed/'make.conf')
    (seed/'result.json').write_text(json.dumps(ref))
    # Latest includes a captured reference whose native failed and skips failures
    # without a capture. Replay cannot invoke a period build or Wine.
    invoke(['iterate','latest',CASE,'--state',STATE])
    invoke(['iterate','replay',CASE,'--state',STATE],False,'native failed')
    assert (PROJECT/'period.log').read_text()==period_before
    assert latest()['original_launches']==0
    write('cases/0001-progress/case.c',case.replace('BAD',''))
    invoke(['iterate','replay',CASE,'--state',STATE])
    assert latest()['native_pass'] is True and (PROJECT/'period.log').read_text()==period_before
    invoke(['test',CASE,'--capture',seed/'original.bin','--original-report',seed/'original.json'])
    invoke(['run',CASE,'--capture',seed/'original.bin'],False,'frontier must name an unimplemented')
    invoke(['iterate','export',CASE,'--state',STATE,'--output',WORK/'expectations.json'])
    delta=json.loads((WORK/'expectations.json').read_text())
    assert delta['review_required'] and delta['not_case_completion'] and delta['native_pass'] is False
    # Explicit original-report validation stays strict in progress setup.
    old=(seed/'original.json').read_text();bad=json.loads(old);bad.pop('identity');bad.pop('capture_sha256')
    (seed/'original.json').write_text(json.dumps(bad));ref['original_report']=binding(seed/'original.json');(seed/'result.json').write_text(json.dumps(ref))
    invoke(['iterate','replay',CASE,'--state',STATE],False,'requires a bound')
    (seed/'original.json').write_text(old);ref['original_report']=binding(seed/'original.json');(seed/'result.json').write_text(json.dumps(ref))
    # Native relinks are reusable; changed ORIGINAL restoration is not.
    write('binary.c',binary.replace('.name="pending",.entry_rva=0x1100,.code=b,.code_count=1,.definition=RECON_APP,.implementation=RECON_UNIMPLEMENTED',
        '.name="pending",.entry_rva=0x1100,.code=b,.code_count=1,.definition=RECON_APP,.implementation=RECON_IMPLEMENTED'))
    invoke(['iterate','replay',CASE,'--state',STATE],False,'no compatible captured reference')
    assert latest()['original_launches']==0
    write('binary.c',binary)
    write('inputs-observed.json',json.dumps({'args':['fixture'],'raw_command_line':'fixture'}))
    invoke(['capture','pack','inputs-observed.json','inputs.bin'])
    r,_=attempt('--owner','owner',ok=False,error='observed launch inputs')
    assert r['original_launches']==0
    # Explicit bootstrap baseline with no implemented App owner is legal.
    invoke(['capture','pack','inputs.json','inputs.bin'])
    write('binary.c',binary.replace('.implementation=RECON_IMPLEMENTED','.implementation=RECON_UNIMPLEMENTED'))
    r,_=attempt('--all-restored','--check-only')
    assert r['checked'] and r['owners']==[] and r['original_launches']==0
    print(f'iteration authored Make/native/journal/scope/freshness/progress controls PASS ({time.monotonic()-start:.3f} s); no Wine/original execution')
finally:
    if os.environ.get('RECON_KEEP_TEST_SCRATCH'):print('scratch:',WORK)
    else:shutil.rmtree(WORK)
