"""Contextual register spellings, shared segment identity and whole lookup."""
from pathlib import Path
import hashlib,json,struct,subprocess,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP
from pe_indexes import indexes,publics
started=time.perf_counter();pe,canonical,config,left,right=indexes();symbols=publics();tables=[('listing_registers32',0x82b08,14),('listing_registers16',0x82b40,14),('listing_registers8',0x82b78,8),('listing_system_registers',0x82b98,24)];rows=[];values={}
def word(image,a):return struct.unpack('<I',image.read(a,4))[0]
def spelling(image,a):return image.read(a,32).split(b'\0')[0].decode()if a else None
for name,address,count in tables:
    values[name]=[]
    for reg in range(count):
        a=word(pe,address+4*reg);b=word(canonical,symbols['_'+name]+4*reg);s=spelling(pe,a);assert spelling(canonical,b)==s,(name,reg,s)
        values[name].append(dict(reg=reg,spelling=s,original=hex(a),canonical=hex(b)))
for reg in range(8,14):
    for image,a,b in [(pe,0x82b08,0x82b40),(canonical,symbols['_listing_registers32'],symbols['_listing_registers16'])]:assert word(image,a+4*reg)==word(image,b+4*reg)
native=subprocess.check_output(['build/native/check-register-names'],text=True).splitlines()
for actual in native:
    width,reg,unused=actual.split();width=int(width);reg=int(reg);u=Uc(UC_ARCH_X86,UC_MODE_32)
    for a,n in [(0x10000,0x90000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
    for s in pe.sections:
        if s['size']:u.mem_write(pe.base+s['rva'],pe.data[s['off']:s['off']+s['size']])
    u.mem_write(0x300800,struct.pack('<I',0x400000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,reg);u.reg_write(UC_X86_REG_EDX,width)
    u.hook_add(UC_HOOK_CODE,lambda m,a,s,x:m.emu_stop()if a==0x400000 else None);u.emu_start(0x35870,0,count=100);assert u.reg_read(UC_X86_REG_ESP)==0x300804
    pointer=u.reg_read(UC_X86_REG_EAX);name=bytes(u.mem_read(pointer,32)).split(b'\0')[0].decode()if pointer else 'NULL';assert actual=='%u %u %s'%(width,reg,name),(actual,width,reg,name)
    rows.append(dict(width=width,reg=reg,spelling=None if not pointer else name,original_return=hex(pointer)))
# Enumerate direct-call encodings as an analysis cross-check, then require every
# candidate to belong to an already decoded accepted caller graph. Not a proof
# that arbitrary computed code addresses cannot exist elsewhere in the image.
code=pe.read(0x17000,0x7894d-0x17000);calls=[]
for i in range(len(code)-4):
    a=0x17000+i
    if code[i]==0xe8 and a+5+struct.unpack_from('<i',code,i+1)[0]==0x35870:
        owners=[r['name']for r in config['driver']if r['start']<=a<r['end']and r['name']in config['accepted']];assert len(owners)==1;calls.append(dict(address=hex(a),owner=owners[0]))
assert len(calls)==10
r=dict(tables=values,shared_segment_identities=6,cases=rows,direct_call_candidates=calls,seconds=time.perf_counter()-started,scope='60 contextual typed table rows including originalNULL slots; original/canonical values and6 segment aliases checked. Whole lookup35870..358a8 versus actual C for all in-class registers at widths0/1/2/3/4/6/8/10. Actual live entry independently observes reg5,width4. Original assumes class-appropriate register numbers; no new range guard or out-of-class memory semantics claim. Direct-call byte candidates are constrained to accepted caller extents, not a universal pointer-escape proof.')
Path('cases/register-names-validation.json').write_text(json.dumps(r,indent=2)+'\n');print('60 register rows,6 shared identities and',len(rows),'whole lookup states match')
