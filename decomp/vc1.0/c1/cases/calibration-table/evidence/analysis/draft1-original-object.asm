
cases/calibration-table/evidence/probes/draft1/calibration.obj:     file format pe-i386


Disassembly of section .text:

00000000 <_calibrate>:
   0:	55                   	push   ebp
   1:	8b ec                	mov    ebp,esp
   3:	83 ec 08             	sub    esp,0x8
   6:	53                   	push   ebx
   7:	56                   	push   esi
   8:	57                   	push   edi
   9:	64 a1 00 00 00 00    	mov    eax,fs:0x0
			b: dir32	__tls_array
   f:	8b 0d 00 00 00 00    	mov    ecx,DWORD PTR ds:0x0
			11: dir32	__tls_index
  15:	8b 04 88             	mov    eax,DWORD PTR [eax+ecx*4]
  18:	83 b8 00 00 00 00 00 	cmp    DWORD PTR [eax+0x0],0x0
			1a: secrel32	.tls$
  1f:	0f 85 4c 00 00 00    	jne    71 <_calibrate+0x71>
  25:	64 a1 00 00 00 00    	mov    eax,fs:0x0
			27: dir32	__tls_array
  2b:	8b 0d 00 00 00 00    	mov    ecx,DWORD PTR ds:0x0
			2d: dir32	__tls_index
  31:	8b 04 88             	mov    eax,DWORD PTR [eax+ecx*4]
  34:	83 b8 08 00 00 00 00 	cmp    DWORD PTR [eax+0x8],0x0
			36: secrel32	.tls$
  3b:	0f 85 26 00 00 00    	jne    67 <_calibrate+0x67>
  41:	64 a1 00 00 00 00    	mov    eax,fs:0x0
			43: dir32	__tls_array
  47:	8b 0d 00 00 00 00    	mov    ecx,DWORD PTR ds:0x0
			49: dir32	__tls_index
  4d:	8b 04 88             	mov    eax,DWORD PTR [eax+ecx*4]
  50:	dd 80 10 00 00 00    	fld    QWORD PTR [eax+0x10]
			52: secrel32	.tls$
  56:	dc 1d 00 00 00 00    	fcomp  QWORD PTR ds:0x0
			58: dir32	.rdata
  5c:	df e0                	fnstsw ax
  5e:	f6 c4 40             	test   ah,0x40
  61:	0f 85 0a 00 00 00    	jne    71 <_calibrate+0x71>
  67:	b8 ff ff ff ff       	mov    eax,0xffffffff
  6c:	e9 a1 00 00 00       	jmp    112 <_calibrate+0x112>
  71:	64 a1 00 00 00 00    	mov    eax,fs:0x0
			73: dir32	__tls_array
  77:	8b 0d 00 00 00 00    	mov    ecx,DWORD PTR ds:0x0
			79: dir32	__tls_index
  7d:	8b 04 88             	mov    eax,DWORD PTR [eax+ecx*4]
  80:	ff 80 00 00 00 00    	inc    DWORD PTR [eax+0x0]
			82: secrel32	.tls$
  86:	8b 45 08             	mov    eax,DWORD PTR [ebp+0x8]
  89:	64 8b 0d 00 00 00 00 	mov    ecx,DWORD PTR fs:0x0
			8c: dir32	__tls_array
  90:	8b 15 00 00 00 00    	mov    edx,DWORD PTR ds:0x0
			92: dir32	__tls_index
  96:	8b 0c 91             	mov    ecx,DWORD PTR [ecx+edx*4]
  99:	01 81 08 00 00 00    	add    DWORD PTR [ecx+0x8],eax
			9b: secrel32	.tls$
  9f:	8b 45 08             	mov    eax,DWORD PTR [ebp+0x8]
  a2:	89 45 f8             	mov    DWORD PTR [ebp-0x8],eax
  a5:	c7 45 fc 00 00 00 00 	mov    DWORD PTR [ebp-0x4],0x0
  ac:	df 6d f8             	fild   QWORD PTR [ebp-0x8]
  af:	64 a1 00 00 00 00    	mov    eax,fs:0x0
			b1: dir32	__tls_array
  b5:	8b 0d 00 00 00 00    	mov    ecx,DWORD PTR ds:0x0
			b7: dir32	__tls_index
  bb:	8b 04 88             	mov    eax,DWORD PTR [eax+ecx*4]
  be:	dd 98 10 00 00 00    	fstp   QWORD PTR [eax+0x10]
			c0: secrel32	.tls$
  c4:	64 a1 00 00 00 00    	mov    eax,fs:0x0
			c6: dir32	__tls_array
  ca:	8b 0d 00 00 00 00    	mov    ecx,DWORD PTR ds:0x0
			cc: dir32	__tls_index
  d0:	8b 04 88             	mov    eax,DWORD PTR [eax+ecx*4]
  d3:	dd 80 10 00 00 00    	fld    QWORD PTR [eax+0x10]
			d5: secrel32	.tls$
  d9:	e8 00 00 00 00       	call   de <_calibrate+0xde>
			da: DISP32	__ftol
  de:	64 8b 0d 00 00 00 00 	mov    ecx,DWORD PTR fs:0x0
			e1: dir32	__tls_array
  e5:	8b 15 00 00 00 00    	mov    edx,DWORD PTR ds:0x0
			e7: dir32	__tls_index
  eb:	8b 0c 91             	mov    ecx,DWORD PTR [ecx+edx*4]
  ee:	8b 89 08 00 00 00    	mov    ecx,DWORD PTR [ecx+0x8]
			f0: secrel32	.tls$
  f4:	03 c8                	add    ecx,eax
  f6:	64 a1 00 00 00 00    	mov    eax,fs:0x0
			f8: dir32	__tls_array
  fc:	8b 15 00 00 00 00    	mov    edx,DWORD PTR ds:0x0
			fe: dir32	__tls_index
 102:	8b 04 90             	mov    eax,DWORD PTR [eax+edx*4]
 105:	03 88 00 00 00 00    	add    ecx,DWORD PTR [eax+0x0]
			107: secrel32	.tls$
 10b:	8b c1                	mov    eax,ecx
 10d:	e9 00 00 00 00       	jmp    112 <_calibrate+0x112>
 112:	5f                   	pop    edi
 113:	5e                   	pop    esi
 114:	5b                   	pop    ebx
 115:	c9                   	leave
 116:	c3                   	ret

00000117 <_main>:
 117:	55                   	push   ebp
 118:	8b ec                	mov    ebp,esp
 11a:	83 ec 0c             	sub    esp,0xc
 11d:	53                   	push   ebx
 11e:	56                   	push   esi
 11f:	57                   	push   edi
 120:	c7 45 f8 00 00 00 00 	mov    DWORD PTR [ebp-0x8],0x0
 127:	c7 45 f4 00 00 00 00 	mov    DWORD PTR [ebp-0xc],0x0
 12e:	e9 03 00 00 00       	jmp    136 <_main+0x1f>
 133:	ff 45 f4             	inc    DWORD PTR [ebp-0xc]
 136:	83 7d f4 30          	cmp    DWORD PTR [ebp-0xc],0x30
 13a:	0f 83 12 00 00 00    	jae    152 <_main+0x3b>
 140:	8b 45 f4             	mov    eax,DWORD PTR [ebp-0xc]
 143:	8b 04 85 00 00 00 00 	mov    eax,DWORD PTR [eax*4+0x0]
			146: dir32	.data
 14a:	01 45 f8             	add    DWORD PTR [ebp-0x8],eax
 14d:	e9 e1 ff ff ff       	jmp    133 <_main+0x1c>
 152:	81 7d f8 98 04 00 00 	cmp    DWORD PTR [ebp-0x8],0x498
 159:	0f 84 0a 00 00 00    	je     169 <_main+0x52>
 15f:	b8 01 00 00 00       	mov    eax,0x1
 164:	e9 8c 01 00 00       	jmp    2f5 <_main+0x1de>
 169:	33 c0                	xor    eax,eax
 16b:	66 a1 e0 00 00 00    	mov    ax,ds:0xe0
			16d: dir32	.data
 171:	83 f8 03             	cmp    eax,0x3
 174:	0f 85 67 00 00 00    	jne    1e1 <_main+0xca>
 17a:	f6 05 e2 00 00 00 07 	test   BYTE PTR ds:0xe2,0x7
			17c: dir32	.data
 181:	0f 85 5a 00 00 00    	jne    1e1 <_main+0xca>
 187:	a1 e2 00 00 00       	mov    eax,ds:0xe2
			188: dir32	.data
 18c:	c1 e8 03             	shr    eax,0x3
 18f:	a8 1f                	test   al,0x1f
 191:	0f 85 4a 00 00 00    	jne    1e1 <_main+0xca>
 197:	83 3d e6 00 00 00 00 	cmp    DWORD PTR ds:0xe6,0x0
			199: dir32	.data
 19e:	0f 85 3d 00 00 00    	jne    1e1 <_main+0xca>
 1a4:	33 c0                	xor    eax,eax
 1a6:	66 a1 02 01 00 00    	mov    ax,ds:0x102
			1a8: dir32	.data
 1ac:	83 f8 05             	cmp    eax,0x5
 1af:	0f 85 2c 00 00 00    	jne    1e1 <_main+0xca>
 1b5:	a1 04 01 00 00       	mov    eax,ds:0x104
			1b6: dir32	.data
 1ba:	24 07                	and    al,0x7
 1bc:	3c 02                	cmp    al,0x2
 1be:	0f 85 1d 00 00 00    	jne    1e1 <_main+0xca>
 1c4:	a1 04 01 00 00       	mov    eax,ds:0x104
			1c5: dir32	.data
 1c9:	c1 e8 03             	shr    eax,0x3
 1cc:	a8 1f                	test   al,0x1f
 1ce:	0f 85 0d 00 00 00    	jne    1e1 <_main+0xca>
 1d4:	83 3d 08 01 00 00 00 	cmp    DWORD PTR ds:0x108,0x0
			1d6: dir32	.data
 1db:	0f 84 0a 00 00 00    	je     1eb <_main+0xd4>
 1e1:	b8 02 00 00 00       	mov    eax,0x2
 1e6:	e9 0a 01 00 00       	jmp    2f5 <_main+0x1de>
 1eb:	c7 45 f4 00 00 00 00 	mov    DWORD PTR [ebp-0xc],0x0
 1f2:	e9 03 00 00 00       	jmp    1fa <_main+0xe3>
 1f7:	ff 45 f4             	inc    DWORD PTR [ebp-0xc]
 1fa:	83 7d f4 02          	cmp    DWORD PTR [ebp-0xc],0x2
 1fe:	0f 83 54 00 00 00    	jae    258 <_main+0x141>
 204:	c7 45 fc 00 00 00 00 	mov    DWORD PTR [ebp-0x4],0x0
 20b:	e9 03 00 00 00       	jmp    213 <_main+0xfc>
 210:	ff 45 fc             	inc    DWORD PTR [ebp-0x4]
 213:	83 7d fc 03          	cmp    DWORD PTR [ebp-0x4],0x3
 217:	0f 83 36 00 00 00    	jae    253 <_main+0x13c>
 21d:	8b 45 fc             	mov    eax,DWORD PTR [ebp-0x4]
 220:	8b 4d f4             	mov    ecx,DWORD PTR [ebp-0xc]
 223:	8b d1                	mov    edx,ecx
 225:	c1 e1 05             	shl    ecx,0x5
 228:	03 ca                	add    ecx,edx
 22a:	03 ca                	add    ecx,edx
 22c:	dd 84 c1 ea 00 00 00 	fld    QWORD PTR [ecx+eax*8+0xea]
			22f: dir32	.data
 233:	dc 1d 00 00 00 00    	fcomp  QWORD PTR ds:0x0
			235: dir32	.rdata
 239:	df e0                	fnstsw ax
 23b:	f6 c4 40             	test   ah,0x40
 23e:	0f 85 0a 00 00 00    	jne    24e <_main+0x137>
 244:	b8 03 00 00 00       	mov    eax,0x3
 249:	e9 a7 00 00 00       	jmp    2f5 <_main+0x1de>
 24e:	e9 bd ff ff ff       	jmp    210 <_main+0xf9>
 253:	e9 9f ff ff ff       	jmp    1f7 <_main+0xe0>
 258:	dd 05 c0 00 00 00    	fld    QWORD PTR ds:0xc0
			25a: dir32	.data
 25e:	dc 1d 08 00 00 00    	fcomp  QWORD PTR ds:0x8
			260: dir32	.rdata
 264:	df e0                	fnstsw ax
 266:	f6 c4 40             	test   ah,0x40
 269:	0f 84 45 00 00 00    	je     2b4 <_main+0x19d>
 26f:	dd 05 c8 00 00 00    	fld    QWORD PTR ds:0xc8
			271: dir32	.data
 275:	dc 1d 10 00 00 00    	fcomp  QWORD PTR ds:0x10
			277: dir32	.rdata
 27b:	df e0                	fnstsw ax
 27d:	f6 c4 40             	test   ah,0x40
 280:	0f 84 2e 00 00 00    	je     2b4 <_main+0x19d>
 286:	dd 05 d0 00 00 00    	fld    QWORD PTR ds:0xd0
			288: dir32	.data
 28c:	dc 1d 18 00 00 00    	fcomp  QWORD PTR ds:0x18
			28e: dir32	.rdata
 292:	df e0                	fnstsw ax
 294:	f6 c4 40             	test   ah,0x40
 297:	0f 84 17 00 00 00    	je     2b4 <_main+0x19d>
 29d:	d9 05 d8 00 00 00    	fld    DWORD PTR ds:0xd8
			29f: dir32	.data
 2a3:	d8 1d 20 00 00 00    	fcomp  DWORD PTR ds:0x20
			2a5: dir32	.rdata
 2a9:	df e0                	fnstsw ax
 2ab:	f6 c4 40             	test   ah,0x40
 2ae:	0f 85 0a 00 00 00    	jne    2be <_main+0x1a7>
 2b4:	b8 04 00 00 00       	mov    eax,0x4
 2b9:	e9 37 00 00 00       	jmp    2f5 <_main+0x1de>
 2be:	6a 03                	push   0x3
 2c0:	e8 00 00 00 00       	call   2c5 <_main+0x1ae>
			2c1: DISP32	_calibrate
 2c5:	83 c4 04             	add    esp,0x4
 2c8:	83 f8 07             	cmp    eax,0x7
 2cb:	0f 85 13 00 00 00    	jne    2e4 <_main+0x1cd>
 2d1:	6a 05                	push   0x5
 2d3:	e8 00 00 00 00       	call   2d8 <_main+0x1c1>
			2d4: DISP32	_calibrate
 2d8:	83 c4 04             	add    esp,0x4
 2db:	83 f8 0f             	cmp    eax,0xf
 2de:	0f 84 0a 00 00 00    	je     2ee <_main+0x1d7>
 2e4:	b8 05 00 00 00       	mov    eax,0x5
 2e9:	e9 07 00 00 00       	jmp    2f5 <_main+0x1de>
 2ee:	2b c0                	sub    eax,eax
 2f0:	e9 00 00 00 00       	jmp    2f5 <_main+0x1de>
 2f5:	5f                   	pop    edi
 2f6:	5e                   	pop    esi
 2f7:	5b                   	pop    ebx
 2f8:	c9                   	leave
 2f9:	c3                   	ret
