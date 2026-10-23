"""Exact original library identities and bounded actual listing-input seams."""
from pathlib import Path
import hashlib,json
from crt_stdio import identity
CONTRACTS={'_fgets':'Actual fgets(destination,256,sourceFILE), first27-byte source line plus NUL, returned destination;1690-byte binary input buffer and descriptor state observed',
 '_strchr':'Actual strchr(first source line,LF), returned original input pointer+26; entire destination and FILE input state unchanged'}
PUBLIC_BOUNDARY='Actual public fgets/strchr calls only; zero application credit; no fabricated native internal CRT entries'
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def validate_observations(name,receipt):
    assert name in CONTRACTS and receipt['matched_public_seam']and receipt['status']=='MATCHED'
    fixture=Path('../fixtures/c2-debug-asm-listing');manifest=json.loads((fixture/'manifest.json').read_text());phase=manifest['phases']['c2'];row=next(r for r in phase['inputs']if r['path']=='c2/dbglist.c')
    source=(fixture/row['path']).read_bytes();assert sha(fixture/row['path'])==row['sha256']==receipt['source_sha256']and len(source)==1690
    prefix=source[:source.index(b'\n')+1]+b'\0';assert len(prefix)==receipt['defined_destination_bytes']==28
    originals=receipt['original_observations'];native=receipt['native_observations'];assert len(native)==4 and [(r['name'],r['stage'])for r in originals]==[('_fgets','enter'),('_fgets','return'),('_strchr','enter'),('_strchr','return')]
    args=originals[0]['args'];nargs=receipt['native_fgets_args'];assert len(args)==len(nargs)==3 and args[1]==nargs[1]==256
    assert originals[1]['args']==args and originals[1]['eax']==args[0]and receipt['native_fgets_result']==nargs[0]
    assert all(r['input']['stream']==args[2]for r in originals)and all(r['stream']==nargs[2]for r in native)
    assert originals[2]['args']==originals[3]['args']==[args[0],10]and receipt['native_strchr_args']==[nargs[0],10]
    assert originals[3]['eax']==args[0]+26 and receipt['native_strchr_result']==nargs[0]+26 and receipt['strchr_result_offset']==26
    for i,(left,right)in enumerate(zip(originals,native)):
        expected=dict(cw=0x27f,sw=0,flags=1 if i==0 else 9,count=0 if i==0 else 1663,fd=7,size=0 if i==0 else 4096,null=int(i==0),used=0 if i==0 else 27,valid=0 if i==0 else 1690,position=0 if i==0 else 1690,osfile=1,file_buffer_hex=''if i==0 else source.hex())
        assert {k:left['input'][k]for k in expected}=={k:right[k]for k in expected}==expected
    for rows in [[r['input']for r in originals],native]:
        destination=[bytes.fromhex(r['destination_hex'])for r in rows];assert all(len(x)==256 for x in destination)
        assert destination[1][:28]==prefix and destination[0][28:]==destination[1][28:],'Destination defined prefix or own untouched tail changed'
        assert destination[1]==destination[2]==destination[3],'strchr modified destination'
    stdout=[r['stdout']for r in originals];assert all(r==stdout[0]for r in stdout)
    expected_listing=next(r for r in phase['outputs']if r['path'].endswith('.cod'));data=(fixture/expected_listing['path']).read_bytes();assert sha(fixture/expected_listing['path'])==expected_listing['sha256']
    logical=bytes.fromhex(stdout[0]['buffer_hex']);assert stdout[0]['used']==len(logical)==904
    # parse_options opens the listing with "wt". Buffered FILE bytes are LF;
    # the physical output gets the CRT's CRLF write expansion. This assertion
    # checks that encoding, not an ignored/normalized artifact field: the final
    # fixture still requires every original .cod byte, including all CR bytes.
    encoded=logical.replace(b'\n',b'\r\n');assert data[:len(encoded)]==encoded
    return [r for r in originals if r['name']==name]
def validate(evidence,record,before):
    assert evidence['identity']==identity(record)
    path=Path(evidence['observation_path']);assert sha(path)==evidence['observation_sha256']
    assert evidence['observations']==validate_observations(record['name'],json.loads(path.read_text()))
    assert evidence['contract']==CONTRACTS[record['name']]and evidence['native_boundary']==PUBLIC_BOUNDARY
    assert evidence['source_case']=='dbglist first binary listing-source fgets256/strchrLF'
