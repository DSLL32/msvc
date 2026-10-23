"""Explicit98 counter/frame/return/target/scope and genuine-next-input negatives."""
import copy,json,re
from pathlib import Path
from unittest.mock import patch
from state import BINARY,load
from scalar_block import compare,progression,validate

def check(original,native,progress_original,progress_native):
    config,pe=load();events=(Path(original)/'events').read_text();state=json.loads((Path(native)/'scalar-block.json').read_text());pevents=(Path(progress_original)/'events').read_text();pstate=json.loads((Path(progress_native)/'gdb.json').read_text());trace=(Path(progress_native)/'trace').read_text().splitlines();compare(config,pe,events,state);progression(config,pe,pevents,pstate,trace)
    def reject(label,action):
        try:action()
        except(AssertionError,KeyError,IndexError,StopIteration):print('rejected:',label)
        else:raise AssertionError('accepted '+label)
    def run(change,old=events):
        s=copy.deepcopy(state);change(s);return compare(config,pe,old,s)
    for key,value,label in(('before',[state['cursor'],1,0,state['output_cursor']],'suppressed-entry mode'),('after',[state['cursor'],0,2,state['output_cursor']],'wrong scope increment'),('after',[state['cursor'],1,1,state['output_cursor']],'unexpected suppression write'),('after',[state['cursor']+1,0,1,state['output_cursor']],'unexpected input-pointer write'),('after',[state['cursor'],0,1,state['output_cursor']+1],'unexpected output-pointer write'),('return_pc',state['caller_pc']+1,'stale return PC'),('return_fp',state['caller_fp']+4,'stale owning frame'),('return_thread',[0,0,0],'wrong return thread'),('return_function','main','wrong returning caller'),('caller_line',1,'wrong source callsite'),('conversion_index',19,'wrong callback index'),('conversion_target',state['function_address']+1,'wrong callback target'),('conversion',state['conversion']+8,'wrong callback table owner'),('call_bytes','0000000000000000','wrong actual indirect CALL'),('later_heap_equality',True,'invented heap equivalence'),('signal',5,'invented stub/mode')):
        reject('scalar98 '+label,lambda key=key,value=value:run(lambda s:s.update({key:value})))
    reject('scalar98 undeclared heap write effect',lambda:run(lambda s:s.update(heap_writes=[dict(offset=2,value=8)])))
    reject('scalar98 no callback rows on both sides',lambda:run(lambda s:s.clear(),re.sub(r'^scalar-block .*\n','',events,flags=re.M)))
    for label,old in(('missing actual return',re.sub(r'^scalar-block return .*\n','',events,flags=re.M)),('duplicate invocation',events+next(x for x in events.splitlines(True)if x.startswith('scalar-block entry '))),('invented completion',events.replace('observed-scalar-completion','invented-scalar-completion')),('wrong return site',events.replace('scalar-block return '+next(x.split()[2]for x in events.splitlines()if x.startswith('scalar-block return '))+' 00025172','scalar-block return 00000000 00025171')),('wrong target',events.replace('scalar-block target 00025450','scalar-block target 00025480')),('wrong original increment',re.sub(r'^(scalar-block return .* )00000001 ([0-9a-f]+)$',r'\g<1>00000002 \2',events,flags=re.M))):
        assert old!=events,label;reject('scalar98 '+label,lambda old=old:compare(config,pe,old,state))
    metadata=json.loads((BINARY/config['scalar_block_observations']['path']).read_text())
    for key,value,label in(('scope','generic-scalar-slice','generic waiver'),('sequence',99,'future function'),('allowed_effects',{'symbol_scope_depth':[0,1],'heap_write':[2,8]},'additional memory effect')):
        changed=copy.deepcopy(metadata);changed[key]=value
        with patch('scalar_block.json.loads',return_value=changed):reject('scalar98 '+label,lambda:validate(config,pe))
    def progress(change):
        s=copy.deepcopy(pstate);change(s);return progression(config,pe,pevents,s,trace)
    for key,value,label in(('parameters',{},'missing sole formal'),('parameters',{'handle':14},'wrong actual handle'),('input_file',14,'wrong global handle'),('scope_depth',1,'wrong scope'),('signal',0,'missing real trap'),('caller_pc',pstate['caller_pc']+1,'wrong caller return'),('function_address',pstate['function_address']+1,'wrong called target'),('later_heap_equality',True,'unobserved heap credit')):reject('98 genuine next '+label,lambda key=key,value=value:progress(lambda s:s.update({key:value})))
    reject('98 genuine next missing begin entry',lambda:progression(config,pe,pevents,pstate,[x for x in trace if x!='begin_symbol_block']))
    reject('98 genuine next original ABI wrong',lambda:progression(config,pe,pevents.replace('stack 1 0000000f','stack 1 0000000e'),pstate,trace))
    reject('98 genuine next fabricated masked entry',lambda:progression(config,pe,pevents.replace('va 00028240','va 00028241'),pstate,trace))
