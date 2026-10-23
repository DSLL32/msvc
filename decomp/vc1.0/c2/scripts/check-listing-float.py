"""Bounded actual-C/original listing widths and default-g conversion checks."""
from pathlib import Path
import itertools,json,struct,subprocess,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP
from binary import PE
started=time.perf_counter();pe=PE('../../../toolchains/1.0-nt/bin/C23232.EXE')
def machine():
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x90000);u.mem_map(0x200000,0x10000);u.mem_map(0x300000,0x10000);u.mem_map(0x400000,0x1000)
    for section in pe.sections:
        if section['size']:u.mem_write(pe.base+section['rva'],pe.data[section['off']:section['off']+section['size']])
    return u
def word(u,address):return struct.unpack('<I',u.mem_read(address,4))[0]
def put(u,address,*v):u.mem_write(address,struct.pack('<'+'I'*len(v),*v))
def string(u,address):return bytes(u.mem_read(address,256)).split(b'\0')[0]
# The observed public default-g recipe is cfltcvt(g,precision6) then cropzeros.
# Execute those ORIGINAL conversion bodies, not a host/Python float formatter.
converted={}
def decimal(bits):
    if bits in converted:return converted[bits]
    u=machine();u.mem_write(0x200000,bits);u.mem_write(0x201000,b'\xcc'*512)
    u.hook_add(UC_HOOK_CODE,lambda m,a,s,x:m.emu_stop()if a==0x400000 else None)
    put(u,0x300800,0x400000,0x200000,0x201000,ord('g'),6,0);u.reg_write(UC_X86_REG_ESP,0x300800);u.emu_start(0x74800,0,count=1000000)
    put(u,0x300800,0x400000,0x201000);u.reg_write(UC_X86_REG_ESP,0x300800);u.emu_start(0x743f0,0,count=100000)
    result=string(u,0x201000);assert result and u.mem_read(0x201100,256)==b'\xcc'*256,'Conversion escaped owned buffer prefix'
    converted[bits]=result;return result
payloads=[]
for value in [0.0,1.25,-2.5]:
    payloads.append((0x1004,struct.pack('<f',value)+b'\xcc'*8))
    payloads.append((0x1008,struct.pack('<d',value)+b'\xcc'*4))
for low,high,exponent in [(0,0,0),(0,0xa0000000,0x3fff),(0,0x80000000,0x3fff)]:payloads.append((0x100a,struct.pack('<IIHH',low,high,exponent,0x100a)))
payloads.append((0x1005,b'\xcc'*12));rows=[]
for (kind,payload),repeat,column in itertools.product(payloads,[-1,0,1,2],[0,9,73]):
    u=machine();u.mem_write(0x200000,payload);put(u,0x80d40,column);put(u,0x300800,0x400000,repeat&0xffffffff)
    u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,kind);u.reg_write(UC_X86_REG_EDX,0x200000);output=bytearray();formats=[]
    def hook(m,address,size,unused):
        if address==0x400000:m.emu_stop();return
        if address!=0x71b40:return
        stack=m.reg_read(UC_X86_REG_ESP);fmt=string(m,word(m,stack+4));formats.append(fmt.decode());args=stack+8
        if fmt==b'%ld DUP(':piece=('%d DUP('%struct.unpack('<i',m.mem_read(args,4))[0]).encode()
        elif fmt==b'0%.8lxr':piece=('0%08xr'%word(m,args)).encode()
        elif fmt==b'0%.8lx%.8lxr':piece=('0%08x%08xr'%(word(m,args),word(m,args+4))).encode()
        elif fmt==b'0%.8lx%.8lx%.4xr':piece=('0%08x%08x%04xr'%(word(m,args),word(m,args+4),word(m,args+8))).encode()
        elif fmt in [b'; %g',b'; %Lg']:piece=b'; '+decimal(bytes(m.mem_read(args,8)))
        else:assert b'%'not in fmt,fmt;piece=fmt
        output.extend(piece);m.reg_write(UC_X86_REG_EAX,len(piece));m.reg_write(UC_X86_REG_ESP,stack+4);m.reg_write(UC_X86_REG_EIP,word(m,stack))
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(0x290b0,0,count=100000)
    result=subprocess.check_output(['build/native/check-listing-float',str(kind),payload.hex(),str(repeat),str(column)],text=True).splitlines()
    assert result==[str(word(u,0x80d40)),output.hex()],(kind,payload,repeat,column,result,output)
    rows.append(dict(type=hex(kind),payload_hex=payload.hex(),repeat=repeat,start_column=column,end_column=word(u,0x80d40),formats=formats,output_hex=output.hex()))
Path('cases/listing-float-validation.json').write_text(json.dumps(dict(cases=rows,seconds=time.perf_counter()-started,conversion_values=[dict(double_hex=k.hex(),original_g=v.decode())for k,v in converted.items()],
 method='Actual compiled native listing C and shared public printf versus whole original290b0..2933e with printf contract interception. Default g executes real originalcfltcvt74800/cropzeros743f0 at precision6, with512-byte owned sentinel buffer and untouched tail. The native public shim writes an owned1000-byte FILE buffer with the entire unused tail checked unchanged. Public actual1.25 printf seam is independently observed by the live case observer.',
 limits='Bounded finite C-locale values and4/8/10-byte widths only; not universal floating/locale/exception equivalence. FPU environment of the public shim is separately gated. Original type100a decimal comment uses the first8 bytes, not an80-bit numeric reinterpretation.'),indent=2)+'\n');print(len(rows),'whole listing width/repetition/column states match actual C and original default-g conversions')
