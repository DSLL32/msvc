break recon_close
run > /root/.ai-tmp/vc1.0-c1xx/close-boundary-q8141zsp/native-stdout.bin 2> /root/.ai-tmp/vc1.0-c1xx/close-boundary-q8141zsp/native-stderr.bin
python
import gdb,json
descriptor=int(gdb.parse_and_eval("fd"))
before=int(gdb.parse_and_eval("recon_osfile[fd]"))
end
finish
python
observed={"descriptor":descriptor,"open_flag":before,"result":int(gdb.parse_and_eval("(int)$eax")),"closed_flag":int(gdb.parse_and_eval("recon_osfile[%d]"%descriptor)),"caller":gdb.newest_frame().name()}
open('/root/.ai-tmp/vc1.0-c1xx/close-boundary-q8141zsp/native-observation.json',"w").write(json.dumps(observed,indent=2)+"\n")
end
