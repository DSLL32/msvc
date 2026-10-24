import gdb,json
rows=[]
def reg(n):return int(gdb.parse_and_eval('$'+n))&0xffffffff
class Entry(gdb.Breakpoint):
 def stop(self):
  caller=gdb.newest_frame().older();caller.select();name=caller.name();producer=int(gdb.parse_and_eval('record' if name=='omf_write_segment_definition' else 'value'))&0xffffffff;gdb.newest_frame().select()
  row=dict(eax=reg('eax'),ecx=reg('ecx'),esp=reg('esp'),caller=name,producer=producer,nonvol={r:reg(r)for r in ['ebx','esi','edi','ebp']});rows.append(row);return False
class Returned(gdb.FinishBreakpoint):
 def __init__(self,row):super().__init__(internal=True);self.row=row
 def stop(self):
  self.row.update(return_eax=reg('eax'),return_esp=reg('esp'),return_nonvol={r:reg(r)for r in ['ebx','esi','edi','ebp']});return False
class Body(gdb.Breakpoint):
 def stop(self):
  row=rows[-1];row.update(incoming_eax=int(gdb.parse_and_eval('incoming_eax'))&0xffffffff,alignment=int(gdb.parse_and_eval('alignment'))&0xffffffff,pool=int(gdb.parse_and_eval('&omf_record_storage.records[0]')),slots=[int(gdb.parse_and_eval('&omf_record_storage.records[%d]'%i))for i in range(4)]);Returned(row);return False
Entry('*omf_alignment',internal=True)
Body('src/omf-output.c:686',internal=True)
gdb.execute('run > inferior.stdout')
print('C2_ALIGNMENT_ROWS '+json.dumps(rows))
