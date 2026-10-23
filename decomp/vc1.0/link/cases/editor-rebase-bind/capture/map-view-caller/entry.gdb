set pagination off
set confirm off
python
import gdb
class QualifiedBreakpoint(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame().older()
        if frame is None or frame.name() is None:
            raise gdb.GdbError('Missing native API caller')
        if frame.name()=='map_rebase_image':return True
        if frame.name() in ['map_file']:
            gdb.write('NATIVE_REVIEWED_EARLIER '+frame.name()+'\n')
            return False
        raise gdb.GdbError('Unexpected native API caller: '+frame.name())
QualifiedBreakpoint('*recon_MapViewOfFile')
end
run -edit /rebase:base=0x500000 client.exe >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/editor-rebase-recon_MapViewOfFile-zajwkh0f/native.stdout 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/editor-rebase-recon_MapViewOfFile-zajwkh0f/native.stderr
printf "NATIVE_MATCH recon_MapViewOfFile\n"
x/6wx $esp
bt 5
quit
