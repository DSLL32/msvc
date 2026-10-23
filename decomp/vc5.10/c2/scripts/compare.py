#!/usr/bin/env python3
"""Compare bound fresh original/native frontier evidence; no acceptance mutation."""
import argparse
import json
import re
from pathlib import Path
from state import BINARY, CANONICAL_CRT, load, receipt_bound, sha, masked, offset, trial_configuration, audit_input_hashes

def compare(original,native,validation=None):
    base,original_bytes,inventory=load()
    config,_=trial_configuration((BINARY/'functions.json').read_bytes())
    receipts=[]
    for path,is_native in ((original,False),(native,True)):
        receipt=json.loads((path/'receipt.json').read_text())
        assert (path/'base_configuration.json').read_bytes()==(BINARY/'functions.json').read_bytes(),'base configuration changed'
        receipt_bound(receipt,(path/'configuration.json').read_bytes(),is_native)
        for stream in ('stdout','stderr'):
            data=(path/(stream+'.bin')).read_bytes()
            assert receipt[stream]=={'bytes':len(data),'sha256':sha(data)},'stream evidence changed'
        receipts.append(receipt)
    assert receipts[1]['resource_sha256']==sha((BINARY/'build/messages.rmr').read_bytes()),'prepared native resources changed'
    assert receipts[0]['candidates']==receipts[1]['candidates']==config['trial'],'different candidate sets'
    if validation is None:
        audit_file=original/'audit-receipt.json';report_file=original/'graph-audit.json'
        assert sha(audit_file.read_bytes())==receipts[0]['audit_receipt_sha256'],'initial audit receipt changed'
        audit=json.loads(audit_file.read_text())
    else:
        audit_file=Path(validation);report_file=audit_file.parent/'graph-audit.json'
        audit=json.loads(audit_file.read_text())
        assert audit['original_execution_receipt_sha256']==sha((original/'receipt.json').read_bytes()),'reaudit belongs to different execution'
    win_provenance=json.loads((report_file.parent/'win32-inputs.json').read_text())
    assert sha((report_file.parent/'win32-inputs.json').read_bytes())==audit['provenance_sha256'],'Win32 input provenance changed'
    assert win_provenance['binary_sha256']==audit['rebuilt_sha256']
    for name,digest in win_provenance['inputs'].items():
        assert sha((report_file.parent/'win32-inputs'/name).read_bytes())==digest,'immutable Win32 compiler input changed'
    graph=json.loads(report_file.read_text())
    from import_evidence import verify
    residual_file=report_file.parent/'import-residuals.json'
    assert sha(residual_file.read_bytes())==audit['import_residuals_sha256']
    assert json.loads(residual_file.read_text())==verify(config,graph),'cached-import identity evidence differs'
    for support in config['support']:
        if support['kind']=='import'and support['name']in config['accepted']:
            identity=graph['import_catalogs']['original']['slots'][str(support['iat'])]
            assert identity=={'dll':support['dll'].lower(),'name':support['symbol']},'import-support slot identity differs'
    assert audit['configuration_sha256']==receipts[0]['configuration_sha256'],'trial audit configuration differs'
    assert audit['rebuilt_sha256']==sha((BINARY/'build/c2.exe').read_bytes()),'current trial reconstruction differs'
    assert audit['rebuilt_map_sha256']==audit_input_hashes()['build/c2.map'],'audit MAP differs'
    assert audit['runtime_sha256']==audit_input_hashes()['runtime.json'],'audit original runtime index differs'
    assert audit['rebuilt_runtime_sha256']==audit_input_hashes()['build/runtime.json'],'audit rebuilt runtime index differs'
    assert audit['metadata_sha256']==audit_input_hashes()['build/graph-metadata.json'],'audit roots/chunks metadata differs'
    assert audit['report_sha256']==sha(report_file.read_bytes()),'graph report changed'
    assert audit['tool_sha256']==audit_input_hashes()['graph-audit'],'audit tool changed: explicit reaudit required, not original replay'
    assert not receipts[0]['unmasked'],'frontier requires masked original'
    assert sha((original/'events.txt').read_bytes())==receipts[0]['events_sha256'],'original events changed'
    assert sha((native/'state.json').read_bytes())==receipts[1]['state_sha256'],'native state changed'
    assert sha((original/'loaded-msvcrt.bin').read_bytes())==CANONICAL_CRT,'provider file changed'
    expected=bytearray(masked(config,original_bytes,inventory,receipts[0]['candidates']))
    for record in config['driver']+config['support']:
        if record['name']in config['accepted']+receipts[0]['candidates']:
            expected[offset(inventory,record['start'])]=0xcc
    assert (original/'C2.EXE').read_bytes()==bytes(expected),'masked image differs from configuration'
    assert sha(bytes(expected))==receipts[0]['executable_sha256']
    for name,digest in receipts[0]['execution_inputs'].items():
        assert sha((original/name).read_bytes())==digest,'raw execution input changed'
    child_env=b'\0'.join(v.encode()for v in config['case']['child_environment_input'])+b'\0\0'
    assert (original/'child-environment-input.bin').read_bytes()==child_env,'original actual child environment changed'
    assert not any(v.startswith(b'MSC_CMD_FLAGS=')for v in child_env.split(b'\0'))
    assert (original/'child-command-input.bin').read_bytes()==b'"R:\\toolchains\\5.10\\bin\\C2.EXE" /?\0','original actual command changed'
    assert all(r['helper_status']==0 for r in receipts),'capture helper failed'
    text=(original/'events.txt').read_text()
    assert re.findall(r'^image-base ([0-9a-f]+)$',text,re.M)==['00400000'],'unexpected original image base'
    assert re.findall(r'^module ([0-9a-f]+) [0-9a-f]+ MSVCRT.dll$',text,re.M)==['78000000'],'unexpected canonical CRT load base'
    assert 'timeout'not in text,'original capture timed out'
    traps=re.findall(r'^trap ([0-9a-f]+) ([0-9a-f]+) ',text,re.M)
    assert len(traps)==1 and int(traps[0][0],16)==0x80000003,'original did not stop at one INT3 frontier'
    address=int(traps[0][1],16)
    matching=[r for r in config['stubs']if r['start']==address]
    assert len(matching)==1,'unknown original frontier'
    expected=matching[0]
    if expected['name']!='_c2_crt_startup':
        assert expected.get('state_policy')in ('pre-main-no-arguments','main-arguments-and-initial-state','exit-callback','command-line-state','environment-flags','match-option')and expected.get('state'),'new frontier needs explicit state decoder'
        if expected['state_policy']=='pre-main-no-arguments':
            assert expected['abi']=='cdecl, no parameters','parameter ABI has no decoder'
    for group in ('gdb_inputs','gdb_outputs'):
        for name,digest in receipts[1][group].items():
            assert sha((native/name).read_bytes())==digest,'GDB input/output provenance changed'
    provenance=json.loads((native/'compiled-inputs.json').read_text())
    assert sha((native/'compiled-inputs.json').read_bytes())==receipts[1]['provenance_sha256']
    assert provenance['binary_sha256']==receipts[1]['native_sha256']
    source_prefix=str((BINARY/'src').relative_to(BINARY.parents[2]))+'/'
    assert {n:v for n,v in provenance['inputs'].items()if n.startswith(source_prefix)}=={n:v for n,v in win_provenance['inputs'].items()if n.startswith(source_prefix)},'Win32/native reconstructed source snapshots differ'
    for name,digest in provenance['inputs'].items():
        assert sha((native/'compiled-inputs'/name).read_bytes())==digest,'immutable compiled input changed'
    state=json.loads((native/'state.json').read_text())
    import importlib.util
    module_spec=importlib.util.spec_from_file_location('c2_native_elf_proof',BINARY/'scripts/native-proof.py')
    proof=importlib.util.module_from_spec(module_spec);module_spec.loader.exec_module(proof)
    native_proof=proof.verify(BINARY/'build/native/c2',(native/'native-maps.txt').read_text(),(native/'gdb.stdout').read_text(),state,config)
    assert state['arguments']==config['case']['arguments'],'native actual arguments changed'
    assert state['MSC_CMD_FLAGS']is None,'native actual environment inherited backend flags'
    assert state['function']==expected['native'],'logical frontier mismatch'
    assert 'SIGTRAP' in state['signal'],'native not stopped at INT3'
    declared=[r['name']for r in config['driver']+config['support']if r['name']in config['accepted']and r.get('kind')!='import']
    entry_names={r['start']:r['name']for r in config['driver']+config['support']if r['name']in declared}
    original_trace=[entry_names[int(a,16)]for a in re.findall(r'^entry ([0-9a-f]+)$',text,re.M)if int(a,16)in entry_names]
    assert original_trace==state['trace'],'accepted source-entry order differs'
    if expected.get('state'):
        section=text.rsplit('observe %08x '%address,1)
        assert len(section)==2,'frontier state is missing'
        raw={int(a,16):int(v,16)for a,v in re.findall(r'^global ([0-9a-f]+) ([0-9a-f]+)$',section[1],re.M)}
        required={view['original']:raw[int(view['original'],16)]for view in expected['state']}
        assert state['globals']==required,'frontier globals/runtime state differs'
    if expected.get('state_policy')in ('command-line-state','environment-flags','match-option'):
        segment=text.rsplit('observe %08x '%address,1)[1]
        values=re.findall(r'^command-state argc=(\d+) argv=([0-9a-f]+) input=([0-9a-f]+)$',segment,re.M)
        assert len(values)==1
        expected_argv=(BINARY/'cases/unsupported-help-diagnostic/argv.bin').read_bytes()
        consumed=expected.get('argv_consumed',0)
        strings=expected_argv[:-2].split(b'\0')[consumed:]
        assert (original/'command-argv.bin').read_bytes()==b'\0'.join(strings)+b'\0\0'
        assert int(values[0][0])==2-consumed
        assert state['command_state']=={'argc':2-consumed,'argv':[v.decode()for v in strings]}
        if expected['state_policy']=='environment-flags':
            assert int(values[0][2],16)==0 and state['flags']is None,'absent MSC_CMD_FLAGS input is not NULL'
    if expected.get('state_policy')=='match-option':
        raw=re.findall(r'^matcher-state table=([0-9a-f]+) input=([0-9a-f]+) callback=([0-9a-f]+) mode=(\d+)$',text,re.M)
        assert len(raw)==1
        pending=next(r for r in config['pending_data']if r['name']=='c2_options')
        assert int(raw[0][0],16)==pending['original'] and int(raw[0][2],16)==0x452bb7 and int(raw[0][3])==0
        assert (original/'matcher-input.bin').read_bytes()==b'/?\0'
        table,size=proof.symbols(BINARY/'build/native/c2',data=True)['c2_options']
        bias=int(native_proof['load_bias'],16)
        callback=proof.symbols(BINARY/'build/native/c2')['c2_next_argument'][0]+bias
        assert size==pending['storage_records']*12
        assert state['matcher']=={'table':table+bias,'input':'/?','next':callback,'mode':0,
                                  'record_size':12,'storage_records':pending['storage_records']},'matcher ABI/logical table identity differs'
        line='C2-MATCHER table=%s next=%s mode=0'%(hex(table+bias),hex(callback))
        assert re.findall(r'^C2-MATCHER .*$',(native/'gdb.stdout').read_text(),re.M)==[line]
    if expected.get('state_policy')=='exit-callback':
        callback=next(r for r in config['stubs']if r['name']=='_c2_cleanup')
        assert re.findall(r'^exit-callback ([0-9a-f]+)$',text.rsplit('observe %08x '%address,1)[1],re.M)==['%08x'%callback['start']]
        callback_native=proof.symbols(BINARY/'build/native/c2')[callback['native']][0]+int(native_proof['load_bias'],16)
        assert state['callback']==callback_native,'exit callback address is not the named native function'
        assert re.findall(r'^C2-CALLBACK (0x[0-9a-f]+)$',(native/'gdb.stdout').read_text(),re.M)==[hex(callback_native)]
    if expected.get('state_policy')=='main-arguments-and-initial-state':
        raw_main=re.findall(r'^main-input argc=(\d+) argv=([0-9a-f]+) environment=([0-9a-f]+)$',text,re.M)
        assert len(raw_main)==1 and int(raw_main[0][0])==2
        def vector_bytes(name):
            data=(original/name).read_bytes();assert data.endswith(b'\0\0')
            return [v.decode()for v in data[:-2].split(b'\0')]
        expected_main={'argc':2,'argv':vector_bytes('main-argv.bin'),'environment':vector_bytes('main-environment.bin'),
                       'initenv_alias':1,'x87_control':int(re.findall(r'^x87-control ([0-9a-f]+)$',text,re.M)[-1],16)&0xffff}
        assert re.findall(r'^main-initenv-alias (\d+)$',text,re.M)==['1']
        assert (original/'main-argv.bin').read_bytes()==(BINARY/'cases/unsupported-help-diagnostic/argv.bin').read_bytes()
        assert (original/'main-environment.bin').read_bytes()==(BINARY/'cases/unsupported-help-diagnostic/environment.bin').read_bytes()
        assert state['main']==expected_main,'main argument vectors/alias/x87 differ'
    assert all((original/(s+'.bin')).read_bytes()==(native/(s+'.bin')).read_bytes()for s in ('stdout','stderr')),'partial stream mismatch'
    return {'validation':str(audit_file),'graph_report':str(report_file),'matched':expected['name'],'original':hex(address),'native':state['function'],'argument_state':expected.get('state_policy','no arguments at initial entry'),'native_elf_proof':native_proof,'source_trace':state['trace'],'candidates':config['trial'],'acceptance':False}

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--original',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--validation',type=Path)
    a=p.parse_args();print(json.dumps(compare(a.original,a.native,a.validation),indent=2))
