#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "safergets.h"

typedef struct foo {
  uint32_t a;
  char b[128];
  uint32_t c;
} foo_t;

int main() {

  puts("Vanilla:");
  char buf[128] = {0};
  char* buf2 = gets(buf);
  printf("\tbuf: %lu, %s\n", strlen(buf), buf);
  printf("\tbuf2: %lu, %s\n", strlen(buf2), buf2);
  free(buf2);


  puts("====\nFootgun:");
  foo_t f = {0};
  f.a = 0x12345678;
  f.c = 0x12345678;
  char* fb2 = gets(f.b);
  printf("\tf:\n"
             "\t\tf.a: %x\n"
             "\t\tf.b: %s\n"
             "\t\tf.c: %x\n"
         "\t\tfb2:  %s\n",
         f.a, f.b, f.c, fb2);
  free(fb2);


  foo_t* f2 = calloc(1, sizeof(foo_t));
  f2->a = 0x12345678;
  f2->c = 0x12345678;
  char* f2b2 = gets(f2->b);
  if (f2b2 != f2->b) {
    f2 = (foo_t*)(f2b2 - (size_t)4);
  }
  printf("\tf2:\n"
             "\t\tf2->a: %x\n"
             "\t\tf2->b: %s\n"
             "\t\tf2->c: %x\n"
         "\t\tf2b2:  %s\n",
         f2->a, f2->b, f2->c, f2b2);
  free((void*)((size_t)f2b2 - 4));

  foo_t* f3 = calloc(1, sizeof(foo_t));
  f3->a = 0x12345678;
  f3->c = 0x12345678;
  char* f3b2 = gets(f3->b);
  if (f3b2 != f3->b) {
    f3 = (foo_t*)(f3b2 - (size_t)4);
  }
  printf("\tf3:\n"
             "\t\tf3->a: %x\n"
             "\t\tf3->b: %s\n"
             "\t\tf3->c: %x\n"
         "\t\tf3b2:  %s\n",
         f3->a, f3->b, f3->c, f3b2);
  free((void*)((size_t)f3b2 - 4));

  return 0;
}
