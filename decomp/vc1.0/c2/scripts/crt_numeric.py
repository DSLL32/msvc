"""Observed finite numeric printf support: original internals, native public seam."""
from fractions import Fraction
import hashlib,json,struct
from pathlib import Path
from crt_stdio import identity
PUBLIC_BOUNDARY='Public printf numeric seam only; original internal CRT ABIs observed, never independently matched to fabricated native entries; zero application credit'
CONTRACTS={
 '__cfltcvt':'Original cdecl(double*,char*,format,precision,caps), encountered g/6/0; output1.25000',
 '__cftog':'Original g conversion of1.25 at precision6; output1.25000',
 '__cftof_g':'Original g-to-fixed wrapper, precision6, shared return address with cftof',
 '__cftof':'Original fixed conversion writes1.25000; returned buffer identity observed',
 '__cropzeros':'Original in-place1.25000→1.25, decimal point and terminator retained',
 '__fltout':'Original by-value double1.25 returns sign/decimal/flags/digits metadata',
 '___dtold':'Original pointer conversion of binary64 1.25 into ten-byte extended1.25',
 '_$I10_OUTPUT':'Original by-value ten-byte1.25/precision17/flags0 yields decimal1,sign space,digits125',
 '__fptostr':'Original precision6 decimal digit output125000 with unchanged input metadata',
 '__shift':'Original overlapping positive-one byte shift preserves input prefix; memcpy call observed',
 '_memcpy':'Original encountered6-byte overlapping source/destination copies prior source bytes and returns destination',
 '___multtenpow12':'Original twelve-byte1.25 scaled by10^-1 becomes0.125, flags1',
 '___ld12mul':'Original twelve-byte multiplication writes left operand; right retained; encountered product rounded within one mantissa ULP',
 '___addl':'Original DWORD addition stores modulo32-bit sum and returns carry',
 '___add_12':'Original96-bit addition writes left operand and retains right',
 '___shl_12':'Original96-bit left shift by one',
 '___shr_12':'Original96-bit unsigned right shift by one'}
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def u32hex(text):return struct.unpack('<I',bytes.fromhex(text))[0]
def unsigned(text):return int.from_bytes(bytes.fromhex(text),'little')
def ld12(text):
    data=bytes.fromhex(text);assert len(data)==12
    mantissa=int.from_bytes(data[:10],'little');exponent=int.from_bytes(data[10:],'little')
    assert not exponent&0x8000 and 0<exponent<0x7fff and mantissa>>79==1
    power=exponent-0x3fff-79
    return Fraction(mantissa)*Fraction(2)**power,Fraction(2)**power

def validate_observations(name,receipt):
    assert name in CONTRACTS and receipt['matched_public_seam'] and receipt['status']=='MATCHED'
    assert receipt['public_args']==dict(format_hex=b'; %g\0'.hex(),double_hex=struct.pack('<d',1.25).hex())
    assert receipt['public_original_result']==receipt['public_native_result']==6
    left,right=receipt['public_original_file'],receipt['public_native_file'];assert left==right and len(left)==2
    for r in left:
        assert(r['flags'],r['fd'],r['size'],r['null'],r['position'])==(10,1,4096,0,0)
        assert r['count']==4096-r['used'] and len(bytes.fromhex(r['buffer_hex']))==r['used']
    assert [r['used']for r in left]==[773,779] and bytes.fromhex(left[0]['buffer_hex']).startswith(b'\tTITLE\tdbglist.c\n')
    assert bytes.fromhex(left[1]['buffer_hex'])==bytes.fromhex(left[0]['buffer_hex'])+b'; 1.25'
    assert receipt['public_original_floating']==receipt['public_native_floating']==[dict(cw=0x27f,sw=0)]*2
    inventory=receipt['observations'];assert inventory and {r['name']for r in inventory}==set(CONTRACTS)|{'_printf'},'Unknown or missing numeric internal inventory'
    rows=[r for r in inventory if r['name']==name];assert [r['stage']for r in rows]==['enter','return']
    before,after=rows;args=before['args'];assert after['args']==args
    a,b=before['extra'],after['extra'];assert before['file']==after['file'],'Numeric internals unexpectedly wrote FILE'
    assert(before['file']['used'],before['file']['count'])==(775,3321)
    for r in rows:assert r['registers']['cw']&0xffff==0x27f and r['registers']['sw']&0xffff==0
    if name in ['__cfltcvt','__cftog','__cftof','__cftof_g']:
        assert a['input_hex']==b['input_hex']==struct.pack('<d',1.25).hex()
        assert b['output_hex']==b'1.25000\0'.hex() and after['eax']==args[1]
        if name=='__cfltcvt':assert args[2:]==[ord('g'),6,0]
        elif name=='__cftog':assert args[2:]==[6,0]
        else:assert args[2:]==[6]
    elif name=='__cropzeros':assert a['output_hex']==b'1.25000\0'.hex()and b['output_hex']==b'1.25\0'.hex()
    elif name=='__fltout':
        assert args==list(struct.unpack('<II',struct.pack('<d',1.25)))
        assert struct.unpack('<III',bytes.fromhex(b['float_info_hex']))==(32,1,1) and b['digits_hex']==b'125\0'.hex()
    elif name=='___dtold':assert a['input_hex']==b['input_hex']==struct.pack('<d',1.25).hex()and b['extended_hex']==struct.pack('<IIH',0,0xa0000000,0x3fff).hex()
    elif name=='_$I10_OUTPUT':
        assert args[:2]==[0,0xa0000000]and args[2]&65535==0x3fff and args[3:5]==[17,0]and after['eax']==1
        assert struct.unpack('<hBB',bytes.fromhex(b['decimal_header_hex']))==(1,32,3)and b['digits_hex']==b'125\0'.hex()
    elif name=='__fptostr':
        assert args[1]==6 and a['float_info_hex']==b['float_info_hex']==struct.pack('<III',32,1,1).hex()
        assert a['digits_hex']==b['digits_hex']==b'125\0'.hex()and b['output_hex']==b'125000\0'.hex()
    elif name=='__shift':
        assert args[1]==1
        text=bytes.fromhex(a['output_hex']);assert text==b'25000\0'and bytes.fromhex(b['output_hex'])==text[:1]+text
    elif name=='_memcpy':assert args[0]==args[1]+1 and args[2]==6 and a['input_hex']==b['output_hex']==b'25000\0'.hex()and after['eax']==args[0]
    elif name=='___addl':
        total=args[0]+args[1];assert u32hex(b['sum_hex'])==total&0xffffffff and after['eax']==total>>32
    elif name=='___add_12':assert b['right_hex']==a['right_hex']and unsigned(b['left_hex'])==(unsigned(a['left_hex'])+unsigned(a['right_hex']))&((1<<96)-1)
    elif name=='___shl_12':assert unsigned(b['extended_hex'])==(unsigned(a['extended_hex'])<<1)&((1<<96)-1)
    elif name=='___shr_12':assert unsigned(b['extended_hex'])==unsigned(a['extended_hex'])>>1
    elif name=='___multtenpow12':assert args[1:]==[0xffffffff,1]and ld12(a['extended_hex'])[0]==Fraction(5,4)and ld12(b['extended_hex'])[0]==Fraction(1,8)
    elif name=='___ld12mul':
        assert b['right_hex']==a['right_hex'];result,ulp=ld12(b['left_hex']);assert abs(result-ld12(a['left_hex'])[0]*ld12(a['right_hex'])[0])<=ulp
    return rows

def validate(evidence,record,before):
    assert evidence['identity']==identity(record)
    path=Path(evidence['observation_path']);assert sha(path)==evidence['observation_sha256']
    observation=json.loads(path.read_text());assert evidence['internal_observations']==validate_observations(record['name'],observation)
    assert evidence['contract']==CONTRACTS[record['name']]and evidence['native_boundary']==PUBLIC_BOUNDARY
    assert evidence['source_case']=='dbglist numeric printf1.25, finite C locale, plain%g'
