"""Whole byte/relative-column bodies, with explicit ASCII printf interception."""
from pathlib import Path
import argparse,itertools,json,struct,subprocess,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_EIP
from binary import PE
parser=argparse.ArgumentParser(description=__doc__);group=parser.add_mutually_exclusive_group();group.add_argument('--comment-prefix',action='store_true');group.add_argument('--displacement',action='store_true');args=parser.parse_args()
started=time.perf_counter();pe=PE('../../../toolchains/1.0-nt/bin/C23232.EXE');rows=[]
cases=itertools.product([0],[0,57],[-2147483648,-99,-9,-1,0,1,9,10,99,2147483647],[-1,0,1,2,3,4,5,7])if args.displacement else itertools.product([0],[-1,0,1,7,8,9,55,56,57,100],[-3,0,1],[0])if args.comment_prefix else itertools.chain(
    itertools.product([0],[-1,0,1,7,8,9,15,16,23,24],[-3,0,1,2,7,8,9,16,25],[0]),
    itertools.product([1],[0,7,8,24],[-1,0,1,5,6,10,11,20],[0,0xfffff]))
for mode,column,count,offset in cases:
    u=Uc(UC_ARCH_X86,UC_MODE_32)
    for a,n in [(0x10000,0x90000),(0x200000,0x10000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
    for s in pe.sections:
        if s['size']:u.mem_write(pe.base+s['rva'],pe.data[s['off']:s['off']+s['size']])
    def word(a):return struct.unpack('<I',u.mem_read(a,4))[0]
    def put(a,*v):u.mem_write(a,struct.pack('<'+'I'*len(v),*[x&0xffffffff for x in v]))
    put(0x80d40,column);put(0x300800,0x400000,0x200000);u.mem_write(0x200000,bytes((i*13+7)&255 for i in range(20)))
    u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,offset if mode else count&0xffffffff);u.reg_write(UC_X86_REG_EDX,(offset if args.displacement else count)&0xffffffff);output=bytearray()
    def hook(m,a,size,unused):
        if a==0x400000:m.emu_stop();return
        if a!=0x71b40:return
        esp=m.reg_read(UC_X86_REG_ESP);fmt=bytes(m.mem_read(word(esp+4),16)).split(b'\0')[0]
        if fmt==b'%05lx':piece=('%05x'%word(esp+8)).encode()
        elif fmt==b'%02x':piece=('%02x'%word(esp+8)).encode()
        elif fmt in [b'%-+ld',b'%-+.2ld',b'%-+.4ld']:
            value=struct.unpack('<i',m.mem_read(esp+8,4))[0];precision=2 if fmt==b'%-+.2ld'else 4 if fmt==b'%-+.4ld'else 0
            piece=(('-'if value<0 else'+')+str(abs(value)).zfill(precision)).encode()
        else:assert fmt in [b' ',b'\t',b'\n',b'; ',b', '];piece=fmt
        output.extend(piece);m.reg_write(UC_X86_REG_EAX,len(piece));m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+4)
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(0x2a7c0 if args.displacement else 0x28280 if args.comment_prefix else 0x29a60 if mode else 0x29990,0,count=100000)
    assert u.reg_read(UC_X86_REG_ESP)==0x300808 if mode else u.reg_read(UC_X86_REG_ESP)==0x300804
    if args.comment_prefix:assert u.reg_read(UC_X86_REG_EAX)==0
    actual=subprocess.check_output(['build/native/check-listing-'+('displacement'if args.displacement else'comment-prefix'if args.comment_prefix else'bytes'),str(mode),str(column&0xffffffff),str(count),str(offset)],text=True).splitlines()
    expected=[str(word(0x80d40)),output.hex()];assert actual==expected,(mode,column,count,offset,actual,expected)
    rows.append(dict(bytes_mode=mode,column=column,count=count,offset=offset,end_column=word(0x80d40),output_hex=output.hex()))
scope='Actual unchanged C displacement helper versus whole original2a7c0..2a83d; printf ASCII contract intercepted. Signed32 extremes, zero guard, all hint classes and minimum precision/sign checked. Actual initial live displacement0/hint0 does not exercise formatting; no printf failure or universal branch claim.'if args.displacement else'Actual C comment prefix28280..282ce and real absolute-column padding versus original; ASCII printf contract intercepted. Zero/nonzero/negative first flags, column/tab/at-or-beyond-target behavior and actual zero return tested. No printf failure or real-case branch coverage claim.'if args.comment_prefix else'Actual extracted unchanged C bodies versus whole original29990..29a55 and29a60..29b2b plus original absolute-column helper28160..28225. ASCII printf contract intercepted; tabs at logical column8, relative/nonpositive counts, five-byte continuations, signed count and RET4 checked. No arbitrary pointer safety, CRT failure or unvisited real compile branch claim.'
Path('cases/listing-'+('displacement'if args.displacement else'comment-prefix'if args.comment_prefix else'bytes')+'-validation.json').write_text(json.dumps(dict(cases=rows,seconds=time.perf_counter()-started,scope=scope),indent=2)+'\n')
print(len(rows),'whole byte/spacing states match')
