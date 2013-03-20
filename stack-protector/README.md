Part 1. -fstack-protector
----

```c
static void fun() {
	char *buf;
	buf = alloca(0x100);
}
```

```
$ objdump -d -M intel unprotected | awk '/<fun>:/,/ret/'

08048404 <fun>:
 8048404:       55                      push   ebp
 8048405:       89 e5                   mov    ebp,esp
 8048407:       83 ec 38                sub    esp,0x38
 804840a:       8d 44 24 0f             lea    eax,[esp+0xf]
 804840e:       83 e0 f0                and    eax,0xfffffff0
 8048411:       89 45 f4                mov    DWORD PTR [ebp-0xc],eax
 8048414:       c9                      leave
 8048415:       c3                      ret
```

```
$ objdump -d -M intel protected | awk '/<fun>:/,/ret/'

08048464 <fun>:
 8048464:       55                      push   ebp
 8048465:       89 e5                   mov    ebp,esp
 8048467:       83 ec 38                sub    esp,0x38
 804846a:       65 a1 14 00 00 00       mov    eax,gs:0x14
 8048470:       89 45 f4                mov    DWORD PTR [ebp-0xc],eax
 8048473:       31 c0                   xor    eax,eax
 8048475:       8d 44 24 0f             lea    eax,[esp+0xf]
 8048479:       83 e0 f0                and    eax,0xfffffff0
 804847c:       89 45 f0                mov    DWORD PTR [ebp-0x10],eax
 804847f:       8b 45 f4                mov    eax,DWORD PTR [ebp-0xc]
 8048482:       65 33 05 14 00 00 00    xor    eax,DWORD PTR gs:0x14
 8048489:       74 05                   je     8048490 <fun+0x2c>
 804848b:       e8 b0 fe ff ff          call   8048340 <__stack_chk_fail@plt>
 8048490:       c9                      leave
 8048491:       c3                      ret
```

Part 2. -D_FORTIFY_SOURCE=2
----

```c
void fun(char *s) {
	char buf[0x100];
	strcpy(buf, s);
}
```

```
$ objdump -d -M intel unfortified | awk '/<fun>:/,/ret/'
08048454 <fun>:
 8048454:       55                      push   ebp
 8048455:       89 e5                   mov    ebp,esp
 8048457:       81 ec 10 01 00 00       sub    esp,0x110
 804845d:       50                      push   eax
 804845e:       8d 85 f8 fe ff ff       lea    eax,[ebp-0x108]
 8048464:       50                      push   eax
 8048465:       e8 b6 fe ff ff          call   8048320 <strcpy@plt>
 804846a:       83 c4 10                add    esp,0x10
 804846d:       c9                      leave
 804846e:       c3                      ret
```

```
$ objdump -d -M intel fortified | awk '/<fun>:/,/ret/'
08048474 <fun>:
 8048474:       55                      push   ebp
 8048475:       89 e5                   mov    ebp,esp
 8048477:       81 ec 0c 01 00 00       sub    esp,0x10c
 804847d:       68 00 01 00 00          push   0x100
 8048482:       50                      push   eax
 8048483:       8d 85 f8 fe ff ff       lea    eax,[ebp-0x108]
 8048489:       50                      push   eax
 804848a:       e8 e1 fe ff ff          call   8048370 <__strcpy_chk@plt>
 804848f:       83 c4 10                add    esp,0x10
 8048492:       c9                      leave
 8048493:       c3                      ret
```

