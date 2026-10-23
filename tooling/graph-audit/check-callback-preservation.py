#!/usr/bin/env python3
"""Exact opaque-call preservation controls; host-only synthetic PE32 corpus."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import time

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--binary',type=Path,required=True)
p.add_argument('--scratch',type=Path,required=True)
p.add_argument('--report',type=Path,required=True)
a=p.parse_args(); binary=a.binary.resolve(); root=a.scratch.resolve()
assert root.is_relative_to(Path.home()/'.ai-tmp');root.mkdir(parents=True,exist_ok=True)
def sha(b):return hashlib.sha256(b).hexdigest()
def image(code,bss=False):
    b=bytearray(0x600)
    def w(at,n):struct.pack_into('<I',b,at,n)
    b[:2]=b'MZ';w(60,0x80);b[0x80:0x84]=b'PE\0\0';struct.pack_into('<HH',b,0x84,0x14c,2 if bss else 1);struct.pack_into('<H',b,0x94,0xe0)
    struct.pack_into('<H',b,0x98,0x10b);w(0xb4,0x400000);w(0x98+92,16);w(0x98+104,0x1100);w(0x98+108,40);b[0x178:0x17d]=b'.text'
    for off,v in [(8,0x400),(12,0x1000),(16,0x400),(20,0x200),(36,0x60000020)]:w(0x178+off,v)
    if bss:
        b[0x1a0:0x1a4]=b'.bss'
        for off,v in [(8,0x10),(12,0x2000),(36,0xc0000080)]:w(0x1a0+off,v)
    b[0x200:0x300]=b'\xcc'*0x100;b[0x200:0x200+len(code)]=code;b[0x280]=0xc3
    for at,v in [(0x300,0x1140),(0x30c,0x1180),(0x310,0x1150),(0x340,0x11c0),(0x344,0x11d0),(0x350,0x11c0),(0x354,0x11d0)]:w(at,v)
    b[0x380:0x38b]=b'msvcrt.dll\0';b[0x3c2:0x3c6]=b'Foo\0';b[0x3d2:0x3d6]=b'Bar\0';return b
seed=bytes.fromhex('8b3d50114000');callback=bytes.fromhex('ff1530124000');call=bytes.fromhex('ffd7');direct=bytes.fromhex('ff1550114000');ret=b'\xc3'
left=seed+callback+call+ret;right=callback+direct+ret
(root/'runtime.json').write_text('[]\n');(root/'packet.json').write_text('{"fixture":"opaque non-IAT call preserves EDI only"}\n')
evidence=dict(path='packet.json',sha256=sha((root/'packet.json').read_bytes()))
def metadata():
    return dict(schema=1,functions={'_caller':dict(register_imports=dict(require_all_register_calls_resolved=True))},register_imports=dict(callback_preservation_contracts=[dict(owner='_caller',original=dict(site=0x401006,slot=0x401230),rebuilt=dict(site=0x401000,slot=0x401230),preserves=['edi'],evidence=copy.deepcopy(evidence))]))
results=[]
def run(label,l=left,r=right,expected=True,meta=None,error=None,change=None,flags=None,bss=False):
    lb=image(l,bss);rb=image(r,bss)
    if change:change(lb,rb)
    (root/'original.exe').write_bytes(lb);(root/'rebuilt.exe').write_bytes(rb)
    cfg=dict(target=dict(path=str(root/'original.exe'),sha256=sha(lb)),accepted=['_caller'],driver=[dict(name='_caller',start=0x401000,end=0x401000+len(l)),dict(name='_helper',start=0x401080,end=0x401081)])
    (root/'config.json').write_text(json.dumps(cfg));(root/'metadata.json').write_text(json.dumps(meta if meta is not None else metadata()))
    (root/'fixture.map').write_text(''.join(' 0001:%08x %s %08x\n'%(v-0x400000,k,v)for k,v in [('_caller',0x401000),('_caller_end',0x401000+len(r)),('_helper',0x401080),('_helper_end',0x401081)]))
    cmd=[str(binary),'--profile','cl','--repo',str(root),'--project',str(root),'--original',str(root/'original.exe'),'--rebuilt',str(root/'rebuilt.exe'),'--config',str(root/'config.json'),'--runtime',str(root/'runtime.json'),'--rebuilt-runtime',str(root/'runtime.json'),'--metadata',str(root/'metadata.json'),'--map',str(root/'fixture.map')]
    cmd+=flags if flags is not None else ['--check-imports','--resolve-register-imports','--check-ret','--check-extents','--check-ownership']
    t=time.perf_counter();q=subprocess.run(cmd,capture_output=True,text=True);elapsed=time.perf_counter()-t
    assert (q.returncode==0)==expected,(label,q.stderr)
    if error:assert error in q.stderr,(label,error,q.stderr)
    report=json.loads(q.stdout)if q.returncode==0 else None
    results.append(dict(case=label,accepted=q.returncode==0,seconds=elapsed,diagnostic=q.stderr.strip(),report=report));return report
positive=run('opaque_callback_preserves_existing_EDI')
l=positive['functions']['_caller']['register_imports']['original'];r=positive['functions']['_caller']['register_imports']['rebuilt']
assert l['resolved_register_calls']=={str(0x40100c):dict(dll='msvcrt.dll',name='Foo')}
assert l['other_unresolved_calls']==[0x401006] and r['other_unresolved_calls']==[0x401000]
assert positive['functions']['_caller']['edges']==['indirect']
assert len(positive['functions']['_caller']['imports'])==1
assert l['callback_preservation'][str(0x401006)]['slot']==0x401230
m=metadata();m['register_imports']['callback_preservation_contracts']=[]
run('absent_contract_remains_barrier',meta=m,expected=False,error='unresolved register import calls')
m=metadata();m['functions']['_caller']['register_imports']['require_all_register_calls_resolved']=False
# A contract never creates facts, resolves the callback, or assigns it an import.
m['register_imports']['callback_preservation_contracts'][0]['original']['site']=0x401000
q=run('callback_does_not_seed_values',l=callback+call+ret,r=callback+call+ret,meta=m)
for side in ['original','rebuilt']:
    g=q['functions']['_caller']['register_imports'][side]
    assert not g['resolved_register_calls'] and g['unresolved_register_calls']==[0x401006] and g['other_unresolved_calls']==[0x401000]
# Flag-off report remains unaffected even if metadata contains an unused contract.
q=run('flag_off_report_has_no_new_fields',l=callback+ret,r=callback+ret,flags=['--check-imports'])
assert 'register_import_inputs' not in q and 'register_imports' not in q['functions']['_caller']
for label,mutator,error in [
    ('wrong_original_site',lambda c:c['original'].update(site=0x40100c),'requires exact unprefixed'),
    ('wrong_rebuilt_site',lambda c:c['rebuilt'].update(site=0x401006),'not an opaque indirect call'),
    ('site_inside_operand',lambda c:c['original'].update(site=0x401007),'absent from discovered'),
    ('site_outside_owner',lambda c:c['original'].update(site=0x401080),'outside owner'),
    ('wrong_original_slot',lambda c:c['original'].update(slot=0x401234),'slot/operand differs'),
    ('wrong_rebuilt_slot',lambda c:c['rebuilt'].update(slot=0x401234),'slot/operand differs'),
    ('wrong_owner',lambda c:c.update(owner='_helper'),'owner is not audited'),
    ('missing_owner',lambda c:c.update(owner='_missing'),'owner missing from verified index'),
    ('empty_preserves',lambda c:c.update(preserves=[]),'empty preservation contract'),
    ('unsupported_mask',lambda c:c.update(preserves=['eax']),'only ebx/esi/edi/ebp'),
    ('duplicate_mask',lambda c:c.update(preserves=['edi','edi']),'duplicate preservation register'),
    ('wrong_supported_mask_kills_EDI',lambda c:c.update(preserves=['esi']),'unresolved register import calls'),
    ('stale_evidence',lambda c:c['evidence'].update(sha256='0'*64),'stale preservation evidence'),
    ('missing_evidence',lambda c:c['evidence'].update(path='missing.json'),'resolve preservation evidence'),
    ('absolute_evidence',lambda c:c['evidence'].update(path=str(root/'packet.json')),'project-relative'),
    ('malformed_digest',lambda c:c['evidence'].update(sha256='x'*64),'invalid preservation evidence'),
    ('missing_original',lambda c:c.pop('original'),'missing field `original`'),
    ('missing_rebuilt',lambda c:c.pop('rebuilt'),'missing field `rebuilt`'),
    ('unknown_annotation',lambda c:c.update(target='invented'),'unknown field `target`'),
]:
    m=metadata();mutator(m['register_imports']['callback_preservation_contracts'][0]);run(label,meta=m,expected=False,error=error)
m=metadata();m['register_imports']['callback_preservation_contracts'].append(copy.deepcopy(m['register_imports']['callback_preservation_contracts'][0]))
run('duplicate_physical_callback_site',meta=m,expected=False,error='duplicate callback preservation')
# Segment/address/operand-size prefixes and register-relative/JMP encodings are not allowed.
for label,op,error in [
    ('segment_prefix',bytes.fromhex('2eff1530124000'),'requires exact unprefixed'),
    ('address_prefix',bytes.fromhex('67ff163012'),'requires exact unprefixed'),
    ('operand_prefix',bytes.fromhex('66ff1530124000'),'requires exact unprefixed'),
    ('indexed_operand',bytes.fromhex('ff143d30124000'),'requires exact unprefixed'),
    ('base_relative_operand',bytes.fromhex('ff9730124000'),'requires exact unprefixed'),
    ('register_operand',bytes.fromhex('ffd0'),'requires exact unprefixed'),
    ('indirect_JMP',bytes.fromhex('ff2530124000'),'not an opaque indirect call'),
]:
    run(label,l=seed+op+call+ret,expected=False,error=error)
# Decoded exact CALL but IAT identity: it must not become an opaque preservation site.
m=metadata();m['register_imports']['callback_preservation_contracts'][0]['original']['slot']=0x401150
run('IAT_call_cannot_be_callback',l=seed+direct+call+ret,meta=m,expected=False,error='not an opaque indirect call')
# Misaligned IAT overlap is also rejected, independently of known CALL identity.
m=metadata();m['register_imports']['callback_preservation_contracts'][0]['original']['slot']=0x401151
run('misaligned_IAT_overlap',l=seed+bytes.fromhex('ff1551114000')+call+ret,meta=m,expected=False,error='overlaps verified IAT')
def slot_case(label,slot,expected=False,error=None,bss=False,change=None):
    op=b'\xff\x15'+struct.pack('<I',slot);m=metadata()
    for side in ['original','rebuilt']:m['register_imports']['callback_preservation_contracts'][0][side]['slot']=slot
    return run(label,l=seed+op+call+ret,r=op+direct+ret,meta=m,expected=expected,error=error,bss=bss,change=change)
slot_case('mapped_zero_fill_BSS_slot',0x402000,True,bss=True)
slot_case('unmapped_slot',0x402000,error='not fully mapped readable')
slot_case('last_readable_DWORD',0x4013fc,True)
slot_case('straddling_readable_section_end',0x4013fd,error='not fully mapped readable')
slot_case('overflowing_slot',0xfffffffe,error='slot overflow')
def unreadable(l,r):
    for b in [l,r]:struct.pack_into('<I',b,0x178+36,0x20000020)
slot_case('mapped_but_unreadable',0x401230,error='not fully mapped readable',change=unreadable)
# A live value changed after the callback is not protected by its contract.
run('later_EDI_write_kills_fact',l=seed+callback+bytes.fromhex('66bf0000')+call+ret,expected=False,error='unresolved register import calls')
# The owner/site association is physical, not a license to annotate unreachable bytes.
m=metadata();m['register_imports']['callback_preservation_contracts'][0]['original']['site']=0x401008
run('unreachable_callback_site',l=seed+b'\xeb\x06'+callback+call+ret,meta=m,expected=False,error='absent from discovered')
a.report.write_text(json.dumps(dict(scope=__doc__,binary_sha256=sha(binary.read_bytes()),results=results),indent=2)+'\n')
print(f'Callback preservation CLI: {len(results)} controls passed.')
