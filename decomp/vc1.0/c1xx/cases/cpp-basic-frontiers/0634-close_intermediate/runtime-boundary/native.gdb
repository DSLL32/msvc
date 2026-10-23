break close_intermediate
run > /root/.ai-tmp/vc1.0-c1xx/stream-boundary-xoexih0v/native-stdout.bin 2> /root/.ai-tmp/vc1.0-c1xx/stream-boundary-xoexih0v/native-stderr.bin
break recon_fflush
continue
python
import gdb,json
stream=gdb.parse_and_eval("s")
def snapshot(): return {"descriptor":int(stream["_file"]),"flags":int(stream["_flag"]),"count":int(stream["_cnt"]),"used":int(stream["_ptr"])-int(stream["_base"]),"size":int(stream["_bufsiz"])}
before=snapshot()
end
finish
python
observed={"before":before,"after":snapshot(),"result":int(gdb.parse_and_eval("(int)$eax")),"caller":gdb.newest_frame().name()}
open('/root/.ai-tmp/vc1.0-c1xx/stream-boundary-xoexih0v/native-observation.json',"w").write(json.dumps(observed,indent=2)+"\n")
end
