fgdemo.exe: fgdemo.obj char.obj common.obj display.obj editor.obj file.obj fundmtls.obj menu.obj misc.obj utility.obj video.obj
   tlink c0m fgdemo char common display editor file fundmtls menu misc utility video, fgdemo, nul, fglm.lib cm

fgdemo.obj: fgdemo.c
   bcc -c -mm fgdemo.c

char.obj: char.c
   bcc -c -mm char.c

common.obj: common.c
   bcc -c -mm common.c

display.obj: display.c tntmaps.h
   bcc -c -mm display.c

editor.obj: editor.c
   bcc -c -mm editor.c

file.obj: file.c
   bcc -c -mm file.c

fundmtls.obj: fundmtls.c
   bcc -c -mm fundmtls.c

menu.obj: menu.c
   bcc -c -mm menu.c

misc.obj: misc.c birdmaps.h
   bcc -c -mm misc.c

utility.obj: utility.c
   bcc -c -mm utility.c

video.obj: video.c
   bcc -c -mm video.c
