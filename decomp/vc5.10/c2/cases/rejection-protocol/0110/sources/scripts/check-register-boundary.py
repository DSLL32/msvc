#!/usr/bin/env python3
"""Nonzero partial-table storage, canary, alias and unknown-consumer controls."""
import argparse,copy,importlib.util,json,shutil,subprocess,tempfile,time
from pathlib import Path
from state import BINARY,ROOT,sha
from register_data import model,source_guard,owner_guard,compare_states
from compare import compare

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for n in ('original','native','output'):p.add_argument('--'+n,type=Path,required=True)
    a=p.parse_args();start=time.monotonic();positive=compare(a.original,a.native);config=json.loads((a.original/'configuration.json').read_text());value=model(config);passed=[]
    def reject(name,fn):
        t=time.monotonic()
        try:fn()
        except(AssertionError,KeyError,ValueError)as e:passed.append({'control':name,'reason':str(e),'seconds':time.monotonic()-t})
        else:raise AssertionError('mutation accepted: '+name)
    text=(BINARY/'src/registers.c').read_text()
    with tempfile.TemporaryDirectory(prefix='register-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as temp:
        folder=Path(temp);harness=folder/'nonzero.c';harness.write_text('''#include "registers.h"
#include <recon.h>
struct Guard { C2MachineRegister before; C2MachineRegister rows[75]; C2MachineRegister after; };
extern struct Guard register_guard;
static unsigned char saved[6160];
static unsigned char *aliases[75];
int main(int argc,char **argv,char **environment) {
    unsigned char *raw=(unsigned char *)&register_guard; unsigned i,j;
    for(i=0;i<sizeof(register_guard);++i)raw[i]=(unsigned char)(17+37*i);
    register_guard.before.flags=255; register_guard.after.flags=255;
    for(i=0;i<75;++i){register_guard.rows[i].flags=(unsigned char)(i*7+19);aliases[i]=&register_guard.rows[i].flags;}
    for(i=0;i<sizeof(register_guard);++i)saved[i]=raw[i];
    reset_register_marks();
    for(i=0;i<sizeof(register_guard);++i){
        unsigned char expected=saved[i];
        if(i>=85 && i<6080 && ((i-85)%80)==0)expected &= ~16;
        if(raw[i]!=expected)return 1;
    }
    for(j=0;j<75;++j)if(aliases[j]!=raw+85+j*80 || *aliases[j]!=(unsigned char)(saved[85+j*80]&~16))return 2;
    return 0;
}
''')
        declaration='C2MachineRegister machine_registers[C2_MACHINE_REGISTER_COUNT];'
        storage='struct Guard { C2MachineRegister before; C2MachineRegister rows[75]; C2MachineRegister after; } register_guard;\n#define machine_registers register_guard.rows'
        assert text.count(declaration)==1
        prefix=['clang','-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-DRECON_NATIVE_TEST','-Wno-ignored-attributes','-O0','-g','-I'+str(ROOT/'lib/recon/include'),'-I'+str(ROOT/'lib/recon/native'),'-I'+str(BINARY/'src')]
        def run(name,source):
            file=folder/(name+'.c');file.write_text(source.replace(declaration,storage,1));exe=folder/name;t=time.monotonic();r=subprocess.run(prefix+[str(file),str(harness),'-o',str(exe)],capture_output=True);assert r.returncode==0,r.stderr.decode();build=time.monotonic()-t;t=time.monotonic();r=subprocess.run([str(exe)],capture_output=True);return r.returncode,build,time.monotonic()-t
        result,build,execution=run('actual',text);assert result==0
        passed.append({'control':'actual75 nonzero flags/5925pendingbytes/canary/livealias','build_seconds':build,'execution_seconds':execution})
        mutations=[('extra row','reg < machine_registers + C2_MACHINE_REGISTER_COUNT','reg <= machine_registers + C2_MACHINE_REGISTER_COUNT'),('row before','reg = machine_registers;','reg = machine_registers - 1;'),('wrong mask','~0x10','~0x20'),('clobber other bits','reg->flags &= ~0x10;','reg->flags = 0;'),('pending write','reg->flags &= ~0x10;','reg->pending00[0] = 0;')]
        for name,before,after in mutations:
            assert before in text;mutated=text.replace(before,after,1);assert mutated!=text;result,build,execution=run('mutant'+str(len(passed)),mutated);assert result!=0,name;passed.append({'control':name,'build_seconds':build,'execution_seconds':execution})
        root=folder/'source';(root/'src').mkdir(parents=True)
        for name in value['source_files']:shutil.copyfile(BINARY/name,root/name)
        source_guard(root,value)
        for expression in ['machine_registers[0].pending00[0]','machine_registers[0].pending06[0]','return machine_registers;','C2MachineRegister *escaped;']:
            (root/'src/unknown.c').write_text(expression);reject('unknown use '+expression,lambda:source_guard(root,value))
        (root/'src/unknown.c').unlink()
        for witness in value['actual_row_base_witnesses']:
            at=witness['instruction']
            if 0x428642<=at<0x42865a:continue
            changed=copy.deepcopy(config);changed['driver'].append({'name':'_unknown_table_consumer','start':at,'end':at+1});changed['accepted'].append('_unknown_table_consumer');reject('unmodeled consumer %08x'%at,lambda c=changed:owner_guard(c,value))
        spec=importlib.util.spec_from_file_location('register_proof',BINARY/'scripts/native-proof.py');proof=importlib.util.module_from_spec(spec);spec.loader.exec_module(proof);state=json.loads((a.native/'state.json').read_text());native_proof=positive['native_elf_proof']
        for side,file,index in [('n','registers-after.bin',0),('o','registers-after.bin',0),('n','registers-after.bin',5),('o','registers-after.bin',5),('n','registers-before.bin',85),('o','registers-before.bin',85)]:
            case=folder/('raw'+str(len(passed)))
            for key,src in [('o',a.original),('n',a.native)]:
                dest=case/key;dest.mkdir(parents=True)
                for name in ['receipt.json','register-model.json','registers-before.bin','registers-after.bin']+(['events.txt']if key=='o'else['gdb.stdout']):shutil.copyfile(src/name,dest/name)
            path=case/side/file;raw=bytearray(path.read_bytes());raw[index]^=1;path.write_bytes(raw);receipt=json.loads((case/side/'receipt.json').read_text());receipt['register_state_hashes'][file]=sha(raw);(case/side/'receipt.json').write_text(json.dumps(receipt));reject('raw %s/%s byte%d'%(side,file,index),lambda:compare_states(case/'o',case/'n',config,state,native_proof,proof))
        bad=copy.deepcopy(state);bad['register_state']['base']+=80;reject('wrong live table alias',lambda:compare_states(a.original,a.native,config,bad,native_proof,proof))
    a.output.write_text(json.dumps({'positive':positive,'count':len(passed),'controls':passed,'seconds':time.monotonic()-start,'checker_sha256':sha(Path(__file__).read_bytes()),'model_sha256':config['register_model_sha256'],'scope':'Actual compiled loop with declaration-only auxiliary storage redirection. Only75flags credit;5925pendingbytes own-side untouched, nonzero/canary/livealias proof. All finite unmodeled consumers held.'},indent=2)+'\n')
if __name__=='__main__':main()
