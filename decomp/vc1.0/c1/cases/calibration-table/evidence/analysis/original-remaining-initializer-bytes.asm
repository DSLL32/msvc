
../../../toolchains/1.0-nt/bin/C13232.EXE:     file format pei-i386


Disassembly of section .text:

00031030 <.text+0x1a030>:
   31030:	53                   	push   ebx
   31031:	56                   	push   esi
   31032:	57                   	push   edi
   31033:	55                   	push   ebp
   31034:	a1 58 02 06 00       	mov    eax,ds:0x60258
   31039:	83 3d 14 cd 06 00 00 	cmp    DWORD PTR ds:0x6cd14,0x0
   31040:	8b 30                	mov    esi,DWORD PTR [eax]
   31042:	7d 0c                	jge    0x31050
   31044:	2b c0                	sub    eax,eax
   31046:	5d                   	pop    ebp
   31047:	5f                   	pop    edi
   31048:	5e                   	pop    esi
   31049:	5b                   	pop    ebx
   3104a:	c3                   	ret
   3104b:	05 00 00 00 00       	add    eax,0x0
   31050:	8b 4e 0c             	mov    ecx,DWORD PTR [esi+0xc]
   31053:	8b 41 04             	mov    eax,DWORD PTR [ecx+0x4]
   31056:	0b c0                	or     eax,eax
   31058:	74 38                	je     0x31092
   3105a:	66 8b 10             	mov    dx,WORD PTR [eax]
   3105d:	80 e2 07             	and    dl,0x7
   31060:	80 fa 02             	cmp    dl,0x2
   31063:	75 2d                	jne    0x31092
   31065:	8b 70 08             	mov    esi,DWORD PTR [eax+0x8]
   31068:	0b f6                	or     esi,esi
   3106a:	75 07                	jne    0x31073
   3106c:	2b c0                	sub    eax,eax
   3106e:	5d                   	pop    ebp
   3106f:	5f                   	pop    edi
   31070:	5e                   	pop    esi
   31071:	5b                   	pop    ebx
   31072:	c3                   	ret
   31073:	50                   	push   eax
   31074:	8b 41 08             	mov    eax,DWORD PTR [ecx+0x8]
   31077:	50                   	push   eax
   31078:	66 8b 01             	mov    ax,WORD PTR [ecx]
   3107b:	50                   	push   eax
   3107c:	e8 8f 1b 00 00       	call   0x32c10
   31081:	8b 0d 58 02 06 00    	mov    ecx,DWORD PTR ds:0x60258
   31087:	5d                   	pop    ebp
   31088:	2b 71 08             	sub    esi,DWORD PTR [ecx+0x8]
   3108b:	0f af c6             	imul   eax,esi
   3108e:	5f                   	pop    edi
   3108f:	5e                   	pop    esi
   31090:	5b                   	pop    ebx
   31091:	c3                   	ret
   31092:	f6 41 01 04          	test   BYTE PTR [ecx+0x1],0x4
   31096:	0f 84 d5 00 00 00    	je     0x31171
   3109c:	83 3d 50 02 06 00 00 	cmp    DWORD PTR ds:0x60250,0x0
   310a3:	74 13                	je     0x310b8
   310a5:	a1 4c 02 06 00       	mov    eax,ds:0x6024c
   310aa:	a3 c8 ff 05 00       	mov    ds:0x5ffc8,eax
   310af:	8b 46 28             	mov    eax,DWORD PTR [esi+0x28]
   310b2:	50                   	push   eax
   310b3:	e8 c8 03 00 00       	call   0x31480
   310b8:	2b db                	sub    ebx,ebx
   310ba:	8b 7e 28             	mov    edi,DWORD PTR [esi+0x28]
   310bd:	3b fb                	cmp    edi,ebx
   310bf:	0f 84 94 00 00 00    	je     0x31159
   310c5:	2b ed                	sub    ebp,ebp
   310c7:	8a 07                	mov    al,BYTE PTR [edi]
   310c9:	24 1f                	and    al,0x1f
   310cb:	3c 01                	cmp    al,0x1
   310cd:	72 20                	jb     0x310ef
   310cf:	3c 0d                	cmp    al,0xd
   310d1:	77 0a                	ja     0x310dd
   310d3:	3c 05                	cmp    al,0x5
   310d5:	74 06                	je     0x310dd
   310d7:	f6 47 15 08          	test   BYTE PTR [edi+0x15],0x8
   310db:	75 6b                	jne    0x31148
   310dd:	3c 01                	cmp    al,0x1
   310df:	72 0e                	jb     0x310ef
   310e1:	3c 0d                	cmp    al,0xd
   310e3:	77 0a                	ja     0x310ef
   310e5:	3c 05                	cmp    al,0x5
   310e7:	74 06                	je     0x310ef
   310e9:	f6 47 15 10          	test   BYTE PTR [edi+0x15],0x10
   310ed:	75 6a                	jne    0x31159
   310ef:	3c 02                	cmp    al,0x2
   310f1:	75 32                	jne    0x31125
   310f3:	8b 47 04             	mov    eax,DWORD PTR [edi+0x4]
   310f6:	50                   	push   eax
   310f7:	8b 47 20             	mov    eax,DWORD PTR [edi+0x20]
   310fa:	50                   	push   eax
   310fb:	e8 90 fe ff ff       	call   0x30f90
   31100:	3b c5                	cmp    eax,ebp
   31102:	74 04                	je     0x31108
   31104:	8b f8                	mov    edi,eax
   31106:	eb eb                	jmp    0x310f3
   31108:	39 2d 50 02 06 00    	cmp    DWORD PTR ds:0x60250,ebp
   3110e:	74 0a                	je     0x3111a
   31110:	89 2d 50 02 06 00    	mov    DWORD PTR ds:0x60250,ebp
   31116:	8b c5                	mov    eax,ebp
   31118:	eb 23                	jmp    0x3113d
   3111a:	8b 47 0c             	mov    eax,DWORD PTR [edi+0xc]
   3111d:	66 8b 00             	mov    ax,WORD PTR [eax]
   31120:	83 e0 0f             	and    eax,0xf
   31123:	eb 18                	jmp    0x3113d
   31125:	a1 80 b6 06 00       	mov    eax,ds:0x6b680
   3112a:	39 47 08             	cmp    DWORD PTR [edi+0x8],eax
   3112d:	74 0b                	je     0x3113a
   3112f:	8b 47 0c             	mov    eax,DWORD PTR [edi+0xc]
   31132:	50                   	push   eax
   31133:	e8 18 1c 00 00       	call   0x32d50
   31138:	eb 03                	jmp    0x3113d
   3113a:	8b 47 20             	mov    eax,DWORD PTR [edi+0x20]
   3113d:	03 d8                	add    ebx,eax
   3113f:	8b 7f 04             	mov    edi,DWORD PTR [edi+0x4]
   31142:	3b fd                	cmp    edi,ebp
   31144:	75 81                	jne    0x310c7
   31146:	eb 11                	jmp    0x31159
   31148:	8b 47 0c             	mov    eax,DWORD PTR [edi+0xc]
   3114b:	50                   	push   eax
   3114c:	e8 ff 1b 00 00       	call   0x32d50
   31151:	03 d8                	add    ebx,eax
   31153:	57                   	push   edi
   31154:	e8 77 fe ff ff       	call   0x30fd0
   31159:	c7 46 28 00 00 00 00 	mov    DWORD PTR [esi+0x28],0x0
   31160:	c7 05 50 02 06 00 00 	mov    DWORD PTR ds:0x60250,0x0
   31167:	00 00 00 
   3116a:	8b c3                	mov    eax,ebx
   3116c:	5d                   	pop    ebp
   3116d:	5f                   	pop    edi
   3116e:	5e                   	pop    esi
   3116f:	5b                   	pop    ebx
   31170:	c3                   	ret
   31171:	51                   	push   ecx
   31172:	e8 d9 1b 00 00       	call   0x32d50
   31177:	5d                   	pop    ebp
   31178:	5f                   	pop    edi
   31179:	5e                   	pop    esi
   3117a:	5b                   	pop    ebx
   3117b:	c3                   	ret
   3117c:	8d                   	.byte 0x8d
