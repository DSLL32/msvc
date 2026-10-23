"""Exact external Release tail ABI; no dependency-body reconstruction credit."""
import json
from binary import COFF, PE, u32
from state import BINARY, ROOT, sha


def body(path, name):
    obj=COFF(path.read_bytes())
    symbol=next(row for row in obj.symbols.values() if row['name']==name and row['section']>0)
    section=next(row for row in obj.sections if row['index']==symbol['section'])
    endings=[row['value'] for row in obj.symbols.values() if row['section']==symbol['section'] and row['type']==0x20 and row['value']>symbol['value']]
    end=min(endings,default=len(section['data']))
    return section['data'][symbol['value']:end].rstrip(b'\xcc')


def prove(config):
    record=next(row for row in config['functions'] if row['name']=='release_widen')
    assert record['symbol']=='_release_widen' and record['original']==0x4102b0 and record['extent']==[0x4102b0,0x4102b8], 'Release residual has wrong owner/extent'
    original=PE(ROOT/config['target']['path'])
    assert original.read(0x4102b0,8).hex()=='8b4c24048b01ff20', 'Release tail instructions differ'
    assert original.read(0x4053a8,8).hex()=='e803af000083c404', 'Release caller/cleanup differs'
    provider=ROOT/'toolchains/5.10/bin/MSPDB50.DLL'
    assert sha(provider)=='cd4536b0f88ef633dbba509062edae7423407b206199a89ea971b444ff249a33'
    module=PE(provider)
    assert u32(module.read(module.base+0x21e30,4),0)==module.base+0x1dfa4, 'Release slot0 identity differs'
    method=module.read(module.base+0x1dfa4,12)
    assert method.hex()=='85c974076a01e885000000c3', 'Release public method RET0 witness differs'
    target=BINARY/'build/widen.obj'; independent=BINARY/'cases/hello-debug-dll/abi/original-caller.obj'
    actual=body(target,'_release_widen');expected=body(independent,'_probe_destroy')
    assert actual==expected, 'Release canonical C++ caller differs from independent genuine virtual caller'
    # The target wrapper may CALL+RET under /Od; the original remains its exact
    # eight-byte indirect tail. The common full graph retains that indirect edge.
    return dict(function='_release_widen',original_extent=[0x4102b0,0x4102b8],original_bytes=original.read(0x4102b0,8).hex(),
        wrapper_abi='cdecl(pointer), caller cleanup 4',external_abi='thiscall ECX object, vptr slot0, no explicit stack arguments, RET0',
        provider_sha256=sha(provider),provider_vtable_rva=0x21e30,provider_method_rva=0x1dfa4,provider_method_bytes=method.hex(),
        target_object_sha256=sha(target),independent_object_sha256=sha(independent),canonical_caller_bytes=actual.hex(),
        validator_sha256=sha(BINARY/'scripts/release-tail.py'),scope='Only this genuine external-method tail cleanup is supplied locally; every other checked application function retains common strict RET checks.')


def verify_projection(audit, directory):
    residual=audit.get('external_release_tail')
    if residual is None:
        assert '--check-ret' in audit['graph_command'], 'missing strict RET checks'
        return
    assert '--check-ret' not in audit['graph_command'], 'unexpected residual full-graph projection'
    assert residual['proof']['validator_sha256']==sha(BINARY/'scripts/release-tail.py'), 'stale Release validator'
    full=json.loads((directory/'graph-config.json').read_text())
    projected=json.loads((directory/'ret-projection-config.json').read_text())
    expected=dict(full,accepted=[name for name in full['accepted'] if name!='_release_widen'])
    assert full['accepted'].count('_release_widen')==1 and projected==expected, 'RET projection excludes more than Release'
    assert sha(directory/'ret-projection-config.json')==residual['config_sha256'], 'changed RET projection'
    assert sha(directory/'ret-projection-report.json')==residual['report_sha256'], 'changed RET projection report'
    report=json.loads((directory/'ret-projection-report.json').read_text())
    assert report==residual['report'] and report['checked_functions']==len(projected['accepted']), 'RET projection function inventory differs'
    assert set(report['functions'])==set(projected['accepted']), 'RET projection body inventory differs'
    full_report=audit['graph']
    for key in ('profile','discovery','register_import_inputs','import_catalogs'):
        assert report[key]==full_report[key], ('RET projection changed graph input',key)
    assert report['functions']=={name:full_report['functions'][name] for name in projected['accepted']}, 'RET projection changed common body facts'
    assert len(report['checks'])==len(set(report['checks'])) and set(report['checks'])==set(full_report['checks'])|{'ret_cleanup'} and 'ret_cleanup' not in full_report['checks'], 'RET projection changed common check set'
    assert 'ret_cleanup' in report['checks'] and '--check-ret' in residual['command'], 'RET projection omitted strict cleanup'
    expected_command=audit['graph_command'].copy()
    expected_command[expected_command.index('--config')+1]=str((directory/'ret-projection-config.json').resolve())
    expected_command.append('--check-ret')
    actual_command=residual['command'].copy()
    actual_command[actual_command.index('--config')+1]=str(__import__('pathlib').Path(actual_command[actual_command.index('--config')+1]).resolve())
    assert actual_command==expected_command, 'RET projection omitted or changed common checks'
