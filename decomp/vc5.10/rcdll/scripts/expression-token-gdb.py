"""Opt-in production-ELF store probe, outside acceptance/case completion."""
import gdb,json,os
from pathlib import Path
out=Path(os.environ['RCDLL_NATIVE_SNAPSHOT'])
def state():
    return dict(counter=int(gdb.parse_and_eval('rc_expression_parentheses')),lookahead=int(gdb.parse_and_eval('rc_expression_token')),lexer=int(gdb.parse_and_eval('rc_token_kind')))
try:
    assert gdb.newest_frame().name()=='rc_evaluate_condition'
    addresses={n:int(gdb.parse_and_eval('&'+n))for n in ['rc_expression_parentheses','rc_expression_token','rc_token_kind']}
    widths={n:gdb.parse_and_eval(n).type.sizeof for n in addresses}
    assert len(set(addresses.values()))==3 and list(widths.values())==[4,1,1]
    gdb.execute('set variable rc_expression_parentheses=0x11223344')
    gdb.execute('set variable rc_expression_token=0xa5')
    gdb.execute('set variable rc_token_kind=0x5a')
    before=state();assert before==dict(counter=0x11223344,lookahead=0xa5,lexer=0x5a)
    gdb.Breakpoint('rc_parse_conditional_expression')
    gdb.execute('continue')
    frame=gdb.newest_frame();after=state()
    assert frame.name()=='rc_parse_conditional_expression'and frame.older().name()=='rc_evaluate_condition'
    assert after==dict(counter=0,lookahead=1,lexer=0x5a)
    report=dict(kind='expression-store-probe',status='pass',function=frame.name(),pc=int(gdb.parse_and_eval('$pc')),addresses=addresses,widths=widths,before=before,after=after,scope='Auxiliary debugger-written noncoincident inputs; stops at first child before parsing. Not case completion or function acceptance.')
except Exception as error:
    report=dict(kind='error',error=str(error))
out.write_text(json.dumps(report,indent=2)+'\n')
