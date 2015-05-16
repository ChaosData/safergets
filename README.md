# safergets

`safergets` is a "wonderful harmonious safer `gets(3)` for justice and peace," as it says on the tin. It has magic powers that prevent most `gets(3)`-related injuries. These powers include:
- hooking all of your memory allocation/freeing operations
  - to determine if pointers go anywhere (and to decrease performance)
- storing all memory allocations in a (secret) global hash table
  - to determine if pointers go anywhere (and to abuse hash tables for things they weren't meant for)
- dynamically finding and overwriting the implementation of `gets(3)` (really IO_gets) in eglibc memory
  - to kill it dead and replace it with a newer safer `gets(3)` implementation
  - with support for various combinations of RELRO (partial, full) and PIE
- doing things in the least right way
  - for maximum awesome

### Features
`safergets` will overrite your `gets(3)` implementation to prevent stack based buffer overflows and heap block overflows. Following the documentation of `gets(3)` it will return a pointer to the buffer containing the input from `stdin`. This pointer
may or may not be different than the one you passed to `gets(3)`. If it's different, it's going to be heap allocated. You should check that the return value doesn't match the input value and `free(3)` it at some point (memory leaks are bad, mmkay).

### How to use
```c
#include "safergets.h"
```

### Dependencies
- Ubuntu 14.04 x86_64
  - This is partially specific to the version of eglibc used in this Ubuntu version and also x86_64 in general
- Apache Portable Runtime (hash table)
  - `sudo apt-get install libapr1-dev`

### Example Usage
```bash
$ clang -std=c99 -Wall -pedantic -masm=intel -Wno-deprecated-declarations -fstack-protector-all -I/usr/include/apr-1.0 -lapr-1 -g -o foo foo.c
```
```
/tmp/jtd/foo-91ab6e.o: In function `notmain':
/home/jtd/c/safergets/./safergets.h:434: warning: the `gets' function is dangerous and should not be used.
```
```bash
$ perl -e 'print "A"x256 . "\n" . "B"x256 . "\n" . "C"x120 . "\n" . "D"x512 . "\n"' | valgrind ./foo
```
```
==82251== Memcheck, a memory error detector
==82251== Copyright (C) 2002-2013, and GNU GPL'd, by Julian Seward et al.
==82251== Using Valgrind-3.10.0.SVN and LibVEX; rerun with -h for copyright info
==82251== Command: ./foo
==82251== 
Vanilla:
	buf: 60, NO! YOU'RE SUPPOSED TO USE THE RETURN VALUE OF gets(3) HERE!
	buf2: 256, AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
====
Footgun:
	f:
		f.a: 12345678
		f.b: NO! YOU'RE SUPPOSED TO USE THE RETURN VALUE OF gets(3) HERE!
		f.c: 12345678
		fb2:  BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB
	f2:
		f2->a: 12345678
		f2->b: CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
		f2->c: 12345678
		f2b2:  CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
	f3:
		f3->a: 12345678
		f3->b: DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD
		f3->c: 44444444
		f3b2:  DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD
==82251== 
==82251== HEAP SUMMARY:
==82251==     in use at exit: 0 bytes in 0 blocks
==82251==   total heap usage: 21 allocs, 21 frees, 3,232 bytes allocated
==82251== 
==82251== All heap blocks were freed -- no leaks are possible
==82251== 
==82251== For counts of detected and suppressed errors, rerun with: -v
==82251== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

Now with support for full RELRO and PIE:
```bash
$ clang -std=c99 -Wall -pedantic -masm=intel -Wno-deprecated-declarations -fstack-protector-all -pie -fPIE -Wl,-z,relro,-z,now -I/usr/include/apr-1.0 -lapr-1 -g -o foo foo.c
```
```
/tmp/jtd/foo-18b8d3.o: In function `notmain':
/home/jtd/c/safergets/./safergets.h:434: warning: the `gets' function is dangerous and should not be used.
```
```bash
$ perl -e 'print "A"x256 . "\n" . "B"x256 . "\n" . "C"x120 . "\n" . "D"x512 . "\n"' | valgrind ./foo
```
```
==82739== Memcheck, a memory error detector
==82739== Copyright (C) 2002-2013, and GNU GPL'd, by Julian Seward et al.
==82739== Using Valgrind-3.10.0.SVN and LibVEX; rerun with -h for copyright info
==82739== Command: ./foo
==82739== 
Vanilla:
	buf: 60, NO! YOU'RE SUPPOSED TO USE THE RETURN VALUE OF gets(3) HERE!
	buf2: 256, AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
====
Footgun:
	f:
		f.a: 12345678
		f.b: NO! YOU'RE SUPPOSED TO USE THE RETURN VALUE OF gets(3) HERE!
		f.c: 12345678
		fb2:  BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB
	f2:
		f2->a: 12345678
		f2->b: CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
		f2->c: 12345678
		f2b2:  CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
	f3:
		f3->a: 12345678
		f3->b: DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD
		f3->c: 44444444
		f3b2:  DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD
==82739== 
==82739== HEAP SUMMARY:
==82739==     in use at exit: 0 bytes in 0 blocks
==82739==   total heap usage: 21 allocs, 21 frees, 3,232 bytes allocated
==82739== 
==82739== All heap blocks were freed -- no leaks are possible
==82739== 
==82739== For counts of detected and suppressed errors, rerun with: -v
==82739== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

### Is `safergets` right for you?
a) No.

b) No! No! No! No! No! No! No! No! No! No! No!

c) ![NO NO NO NO](https://i.imgur.com/Sgevtul.jpg)

d) All of the above

`gets(3)` is officially dead as of C11 (current versions of most C11 compliant compilers won't let use use it in C11 mode), but you should have never used it to begin with. It is literally a function without a safe method of invocation within the context of a single program. Using gets(3) is like attacking a unicorn. You shouldn't do it; and if you tried, people would think you were crazy, or high, possibly both.

This magic header file can only do so much to hold back the darkness that is `gets(3)` short of neutering it completely. It will prevent buffer overflows of statically allocated buffers and heap allocated blocks (note I didn't say "buffers" for that one).
If for some insane reason, you decided to allocate space for a struct containing a buffer followed by any other variable (like a function pointer, because you like to be evil) and then `gets(3)` into it, this won't save you. It will stop overwrites past the end of the heap block though.


