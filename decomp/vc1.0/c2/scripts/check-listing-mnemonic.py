"""Typed mnemonic/condition values and whole original/C writer control."""
from pathlib import Path
import hashlib,json,re,struct,subprocess,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP
from pe_indexes import indexes,publics
started=time.perf_counter();pe,canonical,config,left,right=indexes();symbols=publics()
names=re.findall(r'^OP\((\w+),',Path('src/machine-opcodes.def').read_text(),re.M);mn=re.findall(r'^MN\((\w+), "([^"]+)"\)',Path('src/listing-mnemonics.def').read_text(),re.M)
assert [n for n,s in mn]==names
def string(image,a):return image.read(a,64).split(b'\0')[0]
data=[]
for index,(name,spelling)in enumerate(mn):
    a=struct.unpack('<I',pe.read(0x811c0+4*index,4))[0];b=struct.unpack('<I',canonical.read(symbols['_listing_mnemonics']+4*index,4))[0]
    assert string(pe,a)==string(canonical,b)==spelling.encode(),(index,name,spelling)
    data.append(dict(operation=name,spelling=spelling,original_pointer=hex(a),canonical_pointer=hex(b)))
conditions=['o','no','b','ae','e','ne','be','a','s','ns','p','np','l','ge','le','g']
for index,name in enumerate(conditions):
    a=struct.unpack('<I',pe.read(0x82bf8+4*index,4))[0];b=struct.unpack('<I',canonical.read(symbols['_listing_condition_names']+4*index,4))[0]
    assert string(pe,a)==string(canonical,b)==name.encode(),(index,name)
row=next(r for r in config['driver']if r['name']=='@write_instruction_mnemonic@4');assert row['end']==0x2a36a and row['shared']==[[0x2a36c,0x2a3d8]]
native=subprocess.check_output(['build/native/check-listing-mnemonic'],text=True).splitlines();rows=[]
for actual in native:
    operation,width,mode,condition=[int(v)for v in actual.split()[:4]]
    u=Uc(UC_ARCH_X86,UC_MODE_32)
    for a,n in [(0x10000,0x90000),(0x200000,0x10000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
    for s in pe.sections:
        if s['size']:u.mem_write(pe.base+s['rva'],pe.data[s['off']:s['off']+s['size']])
    def word(a):return struct.unpack('<I',u.mem_read(a,4))[0]
    def put(a,*v):u.mem_write(a,struct.pack('<'+'I'*len(v),*[x&0xffffffff for x in v]))
    put(0x200000,0,0,(operation<<8)|(width<<4),0xe000 if mode else 0,2 if mode==2 else 1,0,condition,0);put(0x80d40,24);put(0x300800,0x400000)
    u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,0x200000);output=bytearray();calls=[0,0]
    def hook(m,a,size,unused):
        if a==0x400000:m.emu_stop();return
        if a not in [0x2a840,0x29490,0x71b40]:return
        esp=m.reg_read(UC_X86_REG_ESP)
        if a in [0x2a840,0x29490]:
            index=int(a==0x29490);value=m.reg_read(UC_X86_REG_ECX);assert not calls[index];calls[index]=value+1
            m.mem_write(0x202000,('%s%u\0'%('d'if index else 's',value)).encode());m.reg_write(UC_X86_REG_EAX,0x202000)
        else:
            fmt=bytes(m.mem_read(word(esp+4),16)).split(b'\0')[0]
            if fmt==b'%s':piece=bytes(m.mem_read(word(esp+8),64)).split(b'\0')[0]
            else:assert fmt in [b' ',b'\t'];piece=fmt
            output.extend(piece);m.reg_write(UC_X86_REG_EAX,len(piece))
        m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+4)
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(0x2a140,0,count=100000);assert u.reg_read(UC_X86_REG_ESP)==0x300804
    expected='%u %u %u %u %u %u %u %s'%(operation,width,mode,condition,word(0x80d40),*calls,output.hex());assert actual==expected,(names[operation],width,mode,condition,actual,expected)
    rows.append(dict(operation=names[operation],bytes=width,mode=mode,condition=condition,result=expected))
assert len(rows)==915
Path('cases/listing-mnemonic-validation.json').write_text(json.dumps(dict(mnemonics=data,conditions=conditions,cases=rows,seconds=time.perf_counter()-started,source_sha256=hashlib.sha256(Path('src/listing-mnemonics.def').read_bytes()).hexdigest(),scope='All231 typed enum-indexed spellings and16 condition spellings independently equal original/canonical linked values. Whole writer and actual relative-spacing C versus original2a140..2a36a with27-way switch data2a36c..2a3d8. Named size-suffix/data-directive children mocked, ASCII printf contract intercepted. Type/conditional/pseudo families and selected ordinary operations exercised; no child implementation, arbitrary opcode validity, printf failure or actual compile branch coverage claim.'),indent=2)+'\n');print(len(data),'typed mnemonic values,16 conditions and',len(rows),'whole writer states match')
