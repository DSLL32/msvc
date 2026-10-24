"""Native-only exact OMF artifact/record/input-inventory oracle controls."""
from pathlib import Path
import copy,importlib.util,json,shutil,tempfile,time
from artifacts import normalize
from omf_records import records
from case_paths import C2_SCRATCH
spec=importlib.util.spec_from_file_location('omf_native_check',Path(__file__).with_name('check-native.py'));native=importlib.util.module_from_spec(spec);spec.loader.exec_module(native)
started=time.perf_counter();fixture=Path('../fixtures/c2-omf-comdat-communal');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2'];row=phase['outputs'][0];original=(fixture/row['path']).read_bytes();parsed=records(original);assert not row['ignored_fields'];rejected=[]
with tempfile.TemporaryDirectory(prefix='omf-artifact-oracles-',dir=C2_SCRATCH)as d:
    work=Path(d)
    for r in phase['inputs']:shutil.copyfile(fixture/r['path'],work/Path(r['path']).name)
    object_path=work/'packets.obj';object_path.write_bytes(original);native.outputs(work,phase,fixture)
    changes=[('OMF bytes4..7 treated as clock',4)]
    for kind in ['0xc3','0xc5','0xb0','0xa3']:
        r=next(r for r in parsed if r['type']==kind);changes.append(('corrupt record '+kind,r['offset']+3))
    for label,offset in changes:
        data=bytearray(original);data[offset]^=1;object_path.write_bytes(data)
        try:native.outputs(work,phase,fixture)
        except AssertionError:rejected.append(label)
        else:raise AssertionError('Accepted '+label)
    object_path.write_bytes(original);bad=copy.deepcopy(row);bad['ignored_fields']=[dict(offset=4,bytes=4)]
    try:normalize(original,bad)
    except AssertionError:rejected.append('unproved COFF clock exemption on OMF')
    else:raise AssertionError('Accepted OMF clock exemption')
    (work/'msvc.pdb').write_bytes(b'unexpected')
    try:native.outputs(work,phase,fixture)
    except AssertionError:rejected.append('unexpected debug artifact')
    else:raise AssertionError('Accepted unexpected PDB')
    (work/'msvc.pdb').unlink()
    for r in phase['inputs']:
        p=work/Path(r['path']).name;data=p.read_bytes();p.write_bytes(bytes([data[0]^1])+data[1:])
        try:native.captured(p,r)
        except AssertionError:rejected.append('changed '+p.name)
        else:raise AssertionError('Accepted changed OMF input')
        p.write_bytes(data)
    for label,data in [('truncated MODEND',original[:-1]),('trailing data',original+b'\0'),('bad checksum',original[:4]+bytes([original[4]^1])+original[5:])]:
        try:records(data)
        except AssertionError:rejected.append(label)
        else:raise AssertionError('Accepted malformed OMF '+label)
Path('cases/comdat-artifact-validation.json').write_text(json.dumps(dict(method='Synthetic native-only oracle checks, no original execution',positive_bytes=len(original),positive_records=len(parsed),ignored_fields=[],rejected=rejected,seconds=time.perf_counter()-started,scope='Exact whole OMF bytes/checksums, no COFF timestamp exemption, all5 IR inputs+source hash and artifact absence. Shape parser is a bounded container/checksum check, not an OMF semantic implementation.'),indent=2)+'\n');print('Exact OMF positive and%d artifact/input/record rejections pass'%len(rejected))
