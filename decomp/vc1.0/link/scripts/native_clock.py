"""Replay independently captured clock input for registered creation cases only."""
import hashlib
import importlib.util
import json
from pathlib import Path

BINARY=Path(__file__).resolve().parents[1]
ROOT=BINARY/'cases/cross-machine-imports/clock'


def captured(record):
    path=(BINARY/record['path']).resolve()
    assert any(path.is_relative_to(root.resolve()) for root in (ROOT,BINARY/'cases/mac-loader-map/clock')), 'Clock evidence outside owned immutable captures'
    data=path.read_bytes()
    assert len(data)==record['bytes'] and hashlib.sha256(data).hexdigest()==record['sha256']
    return data


def validate(record,case):
    if case=='mac-loader-map':return validate_mac(record,case)
    proof_data=captured(record)
    proof=json.loads(proof_data)
    assert case in ('import-archive-i860-data','import-archive-mips-data')
    assert proof['case']==case and proof['status']=='pass'
    target=json.loads((BINARY/'functions.json').read_text())['target']['sha256']
    assert proof['original_sha256']==target
    files={name:captured(r) for name,r in proof['captured'].items()}
    events=[json.loads(line) for line in files['events.jsonl'].decode().splitlines()]
    observations=[event for event in events if event['event']=='clock_boundary']
    assert len(observations)==2 and observations==proof['observations']
    assert {event['caller'] for event in observations}=={'build_import_library','build_archive_header'}
    value=proof['value']
    assert isinstance(value,int) and len(str(value))==10
    for event in observations:
        assert event['result']==event['output']==value
        assert event['argument_nonnull'] and event['caller_stack_unchanged']
        assert event['errno_before']==event['errno_after']==0
    receipt=json.loads(files['receipt.json'])
    assert receipt['case']==case and receipt['mode']=='original' and receipt['original_sha256']==target
    assert receipt['summary']['failed']==receipt['summary']['root_exit']==0
    source=proof['date_suffix_source']
    assert source['path']=='src/import-archive.c'
    assert hashlib.sha256((BINARY/source['path']).read_bytes()).hexdigest()==source['sha256']
    spec=importlib.util.spec_from_file_location('clock_archive_parser',BINARY/'scripts/artifacts-cross-import.py')
    parser=importlib.util.module_from_spec(spec);spec.loader.exec_module(parser)
    machine=proof['machine'];assert machine in ('i860','mips') and case=='import-archive-'+machine+'-data'
    data=files[machine+'.lib']
    members=parser.archive_members(data)
    assert len(members)==proof['archive_member_count']
    stamps=[]
    for member in members:
        pos=member['header']
        assert data[pos+16:pos+26]==str(value).encode('ascii')
        assert data[pos+26:pos+28]==b'0 '
        if member['name'] not in ('/','//'):
            payload=data[member['offset']:member['offset']+member['bytes']]
            assert parser.coff_info(payload)['stamp']==value
            parser.timestamp_fields(payload,'coff')  # Also checks export stamp.
            stamps.append(member['offset']+4)
    assert stamps==proof['archive_coff_stamp_offsets']
    export=files[machine+'.exp']
    assert parser.coff_info(export)['stamp']==value
    parser.timestamp_fields(export,'coff')
    return dict(value=value,proof=record['path'],proof_sha256=record['sha256'],
                boundary='Existing recon_time RECON_FIXED_TIME; deterministic captured external input, no artifact mutation',
                observed_callers=sorted(event['caller'] for event in observations),
                raw_outputs={name:proof['captured'][name] for name in (machine+'.lib',machine+'.exp')})



def validate_mac(record,case):
    import struct
    proof=json.loads(captured(record))
    assert case==proof['case']=='mac-loader-map' and proof['status']=='pass'
    target=json.loads((BINARY/'functions.json').read_text())['target']['sha256']
    assert proof['original_sha256']==target
    files={n:captured(r) for n,r in proof['captured'].items()}
    events=[json.loads(line) for line in files['events.jsonl'].decode().splitlines()]
    observations=[e for e in events if e['event']=='clock_boundary']
    assert len(observations)==1 and observations==proof['observations']
    event=observations[0];value=proof['value']
    assert event['caller']=='link_passes' and event['result']==event['output']==value
    assert event['argument_nonnull'] and event['caller_stack_unchanged']
    assert event['errno_before']==event['errno_after']==0
    receipt=json.loads(files['receipt.json'])
    assert receipt['case']==case and receipt['mode']=='original' and receipt['original_sha256']==target
    assert receipt['summary']['failed']==receipt['summary']['root_exit']==0
    data=files['loader-map.exe'];pe=struct.unpack_from('<I',data,60)[0]
    assert data[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',data,pe+4)[0]==0x268
    assert struct.unpack_from('<I',data,pe+8)[0]==value
    from datetime import datetime
    from zoneinfo import ZoneInfo
    date=datetime.fromtimestamp(value,ZoneInfo('PST8PDT'))
    weekday=('Mon','Tue','Wed','Thu','Fri','Sat','Sun')[date.weekday()]
    month=('Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec')[date.month-1]
    rendered='%s %s %2d %02d:%02d:%02d %04d'%(weekday,month,date.day,date.hour,date.minute,date.second,date.year)
    line=(' Timestamp is %08x (%s)\r\n'%(value,rendered)).encode('ascii')
    assert files['loader-map.map'].count(line)==1
    return dict(value=value,proof=record['path'],proof_sha256=record['sha256'],
        boundary='Captured link_passes time(&TimeDateStamp), deterministic external input; no artifact mutation',
        observed_callers=['link_passes'],
        raw_outputs={name:proof['captured'][name] for name in ('loader-map.exe','loader-map.map')})

def install(metadata,case,environment):
    assert 'RECON_FIXED_TIME' not in environment, 'Unregistered fixed clock override'
    if 'native_clock_input' not in metadata:return None
    proof=validate(metadata['native_clock_input'],case)
    environment['RECON_FIXED_TIME']=str(proof['value'])
    return proof
