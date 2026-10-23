
../../../toolchains/1.0-nt/bin/C13232.EXE:     file format pei-i386


Disassembly of section .text:

00020110 <.text+0x9110>:
   20110:	53                   	push   ebx
   20111:	83 3d 34 b2 05 00 00 	cmp    DWORD PTR ds:0x5b234,0x0
   20118:	0f 84 87 00 00 00    	je     0x201a5
   2011e:	a1 b4 d5 05 00       	mov    eax,ds:0x5d5b4
   20123:	a3 34 e4 06 00       	mov    ds:0x6e434,eax
   20128:	ff 48 04             	dec    DWORD PTR [eax+0x4]
   2012b:	a1 34 e4 06 00       	mov    eax,ds:0x6e434
   20130:	83 78 04 00          	cmp    DWORD PTR [eax+0x4],0x0
   20134:	7c 11                	jl     0x20147
   20136:	8b 00                	mov    eax,DWORD PTR [eax]
   20138:	8b 5c 24 08          	mov    ebx,DWORD PTR [esp+0x8]
   2013c:	88 18                	mov    BYTE PTR [eax],bl
   2013e:	a1 34 e4 06 00       	mov    eax,ds:0x6e434
   20143:	ff 00                	inc    DWORD PTR [eax]
   20145:	eb 13                	jmp    0x2015a
   20147:	a1 34 e4 06 00       	mov    eax,ds:0x6e434
   2014c:	8b 5c 24 08          	mov    ebx,DWORD PTR [esp+0x8]
   20150:	50                   	push   eax
   20151:	53                   	push   ebx
   20152:	e8 c9 23 03 00       	call   0x52520
   20157:	83 c4 08             	add    esp,0x8
   2015a:	68 38 b2 05 00       	push   0x5b238
   2015f:	e8 1c b4 02 00       	call   0x4b580
   20164:	83 c4 04             	add    esp,0x4
   20167:	4b                   	dec    ebx
   20168:	83 fb 0b             	cmp    ebx,0xb
   2016b:	77 38                	ja     0x201a5
   2016d:	ff 24 9d ac 01 02 00 	jmp    DWORD PTR [ebx*4+0x201ac]
   20174:	68 50 38 06 00       	push   0x63850
   20179:	e8 02 b4 02 00       	call   0x4b580
   2017e:	83 c4 04             	add    esp,0x4
   20181:	5b                   	pop    ebx
   20182:	c2 04 00             	ret    0x4
   20185:	a1 b4 04 06 00       	mov    eax,ds:0x604b4
   2018a:	50                   	push   eax
   2018b:	e8 60 b8 02 00       	call   0x4b9f0
   20190:	83 c4 04             	add    esp,0x4
   20193:	5b                   	pop    ebx
   20194:	c2 04 00             	ret    0x4
   20197:	a1 b0 04 06 00       	mov    eax,ds:0x604b0
   2019c:	50                   	push   eax
   2019d:	e8 4e b8 02 00       	call   0x4b9f0
   201a2:	83 c4 04             	add    esp,0x4
   201a5:	5b                   	pop    ebx
   201a6:	c2 04 00             	ret    0x4
   201a9:	8d 49 00             	lea    ecx,[ecx+0x0]
   201ac:	74 01                	je     0x201af
   201ae:	02 00                	add    al,BYTE PTR [eax]
   201b0:	a5                   	movs   DWORD PTR es:[edi],DWORD PTR ds:[esi]
   201b1:	01 02                	add    DWORD PTR [edx],eax
   201b3:	00 74 01 02          	add    BYTE PTR [ecx+eax*1+0x2],dh
   201b7:	00 a5 01 02 00 a5    	add    BYTE PTR [ebp-0x5afffdff],ah
   201bd:	01 02                	add    DWORD PTR [edx],eax
   201bf:	00 a5 01 02 00 74    	add    BYTE PTR [ebp+0x74000201],ah
   201c5:	01 02                	add    DWORD PTR [edx],eax
   201c7:	00 85 01 02 00 97    	add    BYTE PTR [ebp-0x68fffdff],al
   201cd:	01 02                	add    DWORD PTR [edx],eax
   201cf:	00 a5 01 02 00 a5    	add    BYTE PTR [ebp-0x5afffdff],ah
   201d5:	01 02                	add    DWORD PTR [edx],eax
   201d7:	00 74 01 02          	add    BYTE PTR [ecx+eax*1+0x2],dh
	...
