"""Own checksum entry/return and actual next public API boundary observations."""
from pathlib import Path
import json
from scaler_probe import symbols


def setup(output, binary):
    (output / 'checksum-probe-source.py').write_bytes(Path(__file__).read_bytes())
    names = symbols(binary)
    trial = json.loads((output / 'trial.json').read_text())
    code = '''python
checksum_entries=[]
checksum_returns=[]
checksum_locale_entries=[]
checksum_locale_returns=[]
checksum_next_entries=[]
checksum_binding_entries=[]
checksum_binding_returns=[]
checksum_first_entries=[]
checksum_locale_old=0
checksum_locale_result=0
def checksum_locale_state(phase,vectors=True):
    global checksum_locale_old
    old=int(gdb.parse_and_eval('recon_pctype'))
    if not checksum_locale_old:checksum_locale_old=old
    inferior=gdb.selected_inferior()
    selection=gdb.parse_and_eval('ctype_selection')
    result={'pctype':old,'old_pctype':checksum_locale_old,'static_ctype':int(gdb.parse_and_eval('&character_flags')),'mb_cur_max':int(gdb.parse_and_eval('recon_mb_cur_max')),'mb_codepage':int(gdb.parse_and_eval('recon_mb_codepage')),'mb_gate':int(gdb.parse_and_eval('recon_mb_codepage_gate')),'lc_codepage':int(gdb.parse_and_eval('recon_lc_codepage')),'lc_handle':[int(gdb.parse_and_eval('recon_lc_handle[%d]'%i))for i in range(6)],'errors':[int(gdb.parse_and_eval('*(int *)&errno')),int(gdb.parse_and_eval('recon_doserrno')),int(gdb.parse_and_eval('last_error'))],'selection':{'bound':int(selection['bound']),'calls':int(selection['calls']),'reference':selection['reference_sha256'].string(),'result':int(selection['result'])}}
    result['mbctype']=inferior.read_memory(int(gdb.parse_and_eval('&recon_mbctype')),257).tobytes().hex()
    result['ctype']=inferior.read_memory(old-2,514).tobytes().hex();result['old_ctype']=inferior.read_memory(checksum_locale_old-2,514).tobytes().hex()
    result['result_pointer']=checksum_locale_result
    result['result_bytes']=(gdb.Value(checksum_locale_result).cast(gdb.lookup_type('char').pointer()).string(encoding='latin1').encode('latin1')+b'\\0').hex()if checksum_locale_result else ''
    result['crt_argc']=int(gdb.parse_and_eval('recon_crt_argc'))
    if vectors:
        for name,expression in [('CRT-argv','recon_crt_argv'),('CRT-environment','recon_crt_environment')]:
            result[name]=system_vector(expression,'locale-native-'+phase+'-'+name+'.bin')
    result['maps']=open('/proc/%d/maps'%inferior.pid).read()
    return result
def checksum_row():
    row=scaler_context();row['guard']=scaler_guard()
    row['tables']=gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('&frontend_bss.checksum_tables')),768).tobytes().hex()
    return row
def checksum_formals(pc):
    block=gdb.block_for_pc(pc)
    while block and not block.function:block=block.superblock
    return [{'name':item.name,'type':str(item.type)}for item in block if item.is_argument]
class ChecksumReturn(gdb.Breakpoint):
    def __init__(self,address,entry_esp,rows):
        super(ChecksumReturn,self).__init__('*0x%x'%address,internal=True);self.entry_esp=entry_esp;self.rows=rows
    def stop(self):
        global checksum_locale_result
        row=checksum_row();row['entry_esp']=self.entry_esp
        if self.rows is checksum_locale_returns:
            checksum_locale_result=row['eax'];row['locale']=checksum_locale_state('after')
        if self.rows is checksum_binding_returns:row['locale']=checksum_locale_state('binding-after',False)
        self.rows.append(row);self.enabled=False;return False
class ChecksumEntry(gdb.Breakpoint):
    def stop(self):
        before_seed=checksum_row()
        if @@SEED@@:
            gdb.selected_inferior().write_memory(int(gdb.parse_and_eval('&frontend_bss.checksum_tables')),bytes([0xa5])*768)
        row=checksum_row();row['return_pc']=struct.unpack('<I',gdb.selected_inferior().read_memory(row['esp'],4))[0]
        row['formal_inventory']=checksum_formals(row['pc']);row['before_seed']=before_seed;checksum_entries.append(row)
        ChecksumReturn(row['return_pc'],row['esp'],checksum_returns);return False
class ChecksumLocaleEntry(gdb.Breakpoint):
    def stop(self):
        row=checksum_row();stack=struct.unpack('<3I',gdb.selected_inferior().read_memory(row['esp'],12));row.update(return_pc=stack[0],args=list(stack[1:]),formal_inventory=checksum_formals(row['pc']))
        row['locale_bytes']=gdb.Value(stack[2]).cast(gdb.lookup_type('char').pointer()).string(encoding='latin1')
        row['locale']=checksum_locale_state('before')
        checksum_locale_entries.append(row);ChecksumReturn(stack[0],row['esp'],checksum_locale_returns);return False
class ChecksumNextEntry(gdb.Breakpoint):
    def stop(self):
        row=checksum_row();row.update(return_pc=struct.unpack('<I',gdb.selected_inferior().read_memory(row['esp'],4))[0],formal_inventory=checksum_formals(row['pc']),locale=checksum_locale_state('next'))
        checksum_next_entries.append(row);return False
class ChecksumBindingEntry(gdb.Breakpoint):
    def stop(self):
        row=checksum_row();stack=struct.unpack('<3I',gdb.selected_inferior().read_memory(row['esp'],12))
        row.update(return_pc=stack[0],args=[gdb.Value(v).cast(gdb.lookup_type('char').pointer()).string(encoding='ascii')for v in stack[1:]],locale=checksum_locale_state('binding-before',False))
        checksum_binding_entries.append(row);ChecksumReturn(stack[0],row['esp'],checksum_binding_returns);return False
class ChecksumFirstEntry(gdb.Breakpoint):
    def stop(self):
        row=checksum_row();row['locale']=checksum_locale_state('first-entry',False);checksum_first_entries.append(row);return False
ChecksumEntry('*0x%x'%@@ENTRY@@,internal=True)
ChecksumLocaleEntry('*0x%x'%@@LOCALE@@,internal=True)
ChecksumNextEntry('*0x%x'%@@NEXT@@,internal=True)
ChecksumBindingEntry('*recon_crt_bind_empty_ctype_1252',internal=True)
ChecksumFirstEntry('*_DllMainCRTStartup',internal=True)
end
'''.replace('@@SEED@@', repr(bool(trial.get('test_seed')))).replace('@@ENTRY@@', str(names['initialize_checksums'][0])).replace('@@LOCALE@@', str(names['recon_setlocale'][0])).replace('@@NEXT@@', str(names['initialize_multibyte_categories'][0]))
    finish = 'python\n'
    for name in ('entries', 'returns', 'locale_entries', 'locale_returns', 'next_entries', 'binding_entries', 'binding_returns', 'first_entries'):
        finish += 'open(%r,"w").write(json.dumps(checksum_%s,indent=2)+"\\n")\n' % (str(output / ('checksum-' + name.replace('_', '-') + '.json')), name)
    return code, finish + 'end\n'
