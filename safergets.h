#ifndef _SAFERGETS_H_
#define _SAFERGETS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>


#include <linux/limits.h>
#include <apr.h>
#include <apr_strings.h>
#include <apr_hash.h>
#include <apr_time.h>

typedef struct hash_stuff {
  apr_pool_t* p;
  apr_hash_t* ht;
} hash_stuff_t;
static hash_stuff_t h = {NULL, NULL};


void hash_cleanup() {
  h.ht = NULL;
  apr_terminate();
}

void hash_init() {
  int fake_argc = 1;
  char fake_argv[1][4] = {{"LOL"}};
  apr_app_initialize(&fake_argc, (const char *const **)&fake_argv, NULL);
  atexit(hash_cleanup);
  apr_pool_create(&h.p, NULL);
  h.ht = apr_hash_make(h.p);
}


typedef struct keyval {
  void* ptr_key;
  size_t size_val;
} keyval_t;


#define VA_NARGS_IMPL(_1, _2, _3, _4, _5, N, ...) N
#define VA_NARGS(...) VA_NARGS_IMPL(X, __VA_ARGS__, 4, 3, 2, 1, 0)
#define VARARG_IMPL2(base, count, ...) base##count(__VA_ARGS__)
#define VARARG_IMPL(base, count, ...) VARARG_IMPL2(base, count, __VA_ARGS__)
#define VARARG(base, ...) VARARG_IMPL(base, VA_NARGS(__VA_ARGS__), __VA_ARGS__)

#define NAMES0()
#define NAMES2(name, type_name) name
#define NAMES4(name, type_name, ...) name, NAMES2(__VA_ARGS__)
#define NAMES6(name, type_name, ...) name, NAMES4(__VA_ARGS__)
#define NAMES(...) VARARG(NAMES, __VA_ARGS__)

#define DEFS0()
#define DEFS2(name, type_name) type_name
#define DEFS4(name, type_name, ...) type_name, DEFS2(__VA_ARGS__)
#define DEFS6(name, type_name, ...) type_name, DEFS4(__VA_ARGS__)
#define DEFS(...) VARARG(DEFS, __VA_ARGS__)


#define ARG(type, name) name, type name
#define HOOK_ALLOC(name, block, ...)\
extern void* __libc_##name (DEFS(__VA_ARGS__));\
int name##_hook_active = 1;\
void* name##_hook (DEFS(__VA_ARGS__), void* caller) {\
  void* result = NULL;\
  name##_hook_active = 0;\
  block\
  name##_hook_active = 1;\
  return result;\
}\
\
void* name (DEFS(__VA_ARGS__)) {\
  void* caller = __builtin_return_address(0);\
  if (name##_hook_active) {\
    return name##_hook (NAMES(__VA_ARGS__), caller);\
  }\
  return __libc_##name (NAMES(__VA_ARGS__));\
}

extern void *__libc_free(void* ptr) __THROW;
int free_hook_active = 1;
void free_hook (void* ptr, void *caller) {
  free_hook_active = 0;
  if (h.ht != NULL) {
    keyval_t* old_kv = (keyval_t*)apr_hash_get(h.ht, &ptr, sizeof(void*));
    if (old_kv == NULL) {
      // ???
    } else {
      apr_hash_set(h.ht, &ptr, sizeof(size_t), NULL);
      __libc_free(old_kv);
    }
  } else {
  }
  __libc_free(ptr);
  free_hook_active = 1;
  return;
}


void free (void* ptr) {
  void *caller = __builtin_return_address(0);
  if (free_hook_active) {
    free_hook(ptr, caller);
    return;
  }
  __libc_free(ptr);
  return;
}


HOOK_ALLOC(malloc, {
  result = malloc(size);
  if (h.ht != NULL) {
    keyval_t* kv = __libc_malloc(sizeof(keyval_t));
    kv->ptr_key = result;
    kv->size_val = size;
    apr_hash_set(h.ht, &(kv->ptr_key), sizeof(size_t), (void*)kv);
  } else {
  }
}, ARG(size_t, size))

HOOK_ALLOC(calloc, {
  result = calloc(nmemb, size);
  if (h.ht != NULL) {
    keyval_t* kv = __libc_malloc(sizeof(keyval_t));
    kv->ptr_key = result;
    kv->size_val = nmemb * size; //OVERFLOW
    apr_hash_set(h.ht, &(kv->ptr_key), sizeof(size_t), (void*)kv);
  } else {
  }
}, ARG(size_t, nmemb), ARG(size_t, size))

HOOK_ALLOC(realloc, {
  if (h.ht != NULL) {
    keyval_t* old_kv = (keyval_t*)apr_hash_get(h.ht, &ptr, sizeof(void*));
    apr_hash_set(h.ht, &ptr, sizeof(void*), NULL);
    result = realloc(ptr, size);

    if (ptr != result) { // if the realloc had to move it elsewhere
      if (old_kv == NULL) {
        //???
      } else {
        __libc_free(old_kv);
      }
      keyval_t* kv = __libc_malloc(sizeof(keyval_t));
      kv->ptr_key = result;
      kv->size_val = size;
      apr_hash_set(h.ht, &(kv->ptr_key), sizeof(size_t), (void*)kv);
    } else {
      if (old_kv != NULL) {
        // update the size value for the original keyval_t
        old_kv->size_val = size;
        apr_hash_set(h.ht, &(old_kv->ptr_key), sizeof(size_t), (void*)old_kv);
      } else {
        //???
      }
    }
  } else {
  }
}, ARG(void*, ptr), ARG(size_t, size))


void dumpHashTable() {
  apr_hash_index_t* hidx = NULL;
  void** key = NULL;
  keyval_t* val = NULL;
  puts("<dumpHashTable>");
  for (hidx = apr_hash_first(h.p, h.ht);
       hidx != NULL;
       hidx = apr_hash_next(hidx)) {
    apr_hash_this(hidx, (const void**)&key, NULL, (void**) &val);
    printf("%p (%p): %lu\n", *key, val->ptr_key, val->size_val);
  }
  puts("</dumpHashTable>");
}

apr_hash_index_t* heapOrNot(void* ptr) {
  apr_hash_index_t* hidx = NULL;
  void** key = NULL;
  keyval_t* val = NULL;
  for (hidx = apr_hash_first(h.p, h.ht);
       hidx != NULL;
       hidx = apr_hash_next(hidx)) {
    apr_hash_this(hidx, (const void**)&key, NULL, (void**) &val);
    if (val->ptr_key == ptr
       || (val->ptr_key <= ptr
           && (size_t)ptr < ((size_t)val->ptr_key + val->size_val)
          )
       ) {
      return hidx;
    }
  }
  return NULL;
}




/*
Strategy:

- not doing:
  - safer macro wrapper around gets
    - not crazy enough

- pre main:
  - parse plt for gets
  - force non executing plt link for gets
  - get address to real gets
  - overwrite gets implementation to jmp to alternative implementation
  - hook all malloc/calloc/realloc/free to keep track of heap allocations
  - have a safegets function:
    - c preamble
      - if pointer is on the heap
        - attempt to find heap block (via heap allocation tracking)
          - walk through all of them from the beginning/end
          - reallocate as necessary
      - else
        - allocate space
        - read into space
        - if not enough, reallocate bigger space
    - c epilogue
      - if not on the heap and safely limited:
        - copy "NO! YOU'RE SUPPOSED TO USE THE RETURN VALUE OF gets(3) HERE!" to original pointer
      - return pointer to written to location
*/



void* get_got_entry(void* plt_func_stub) {
  uint32_t got_jmp_offset = *((uint32_t*)((uint64_t)plt_func_stub+2));
  uint64_t got_entry_addr = (uint64_t)plt_func_stub + 0x6 + (uint64_t)got_jmp_offset;
  void* got_entry = (void*)*((uint64_t*)got_entry_addr);
  return got_entry;
}

uint32_t get_plt_push(void* plt_func_stub) {
  uint32_t got_push_offset = *((uint32_t*)((uint64_t)plt_func_stub+7));
  return got_push_offset;
}

void* get_plt_init_jmp_addr(void* plt_func_stub) {
  void* init_jmp_addr  = (void*)((uint64_t)plt_func_stub + 16 + *((int32_t*)((uint64_t)plt_func_stub+12)));
  return init_jmp_addr;
}

uint64_t get_got_magic(void* plt_head) {
  uint32_t got_magic_push_offset = *((uint32_t*)((uint64_t)plt_head+2));
  uint64_t* got_magic_addr = (uint64_t*)((uint64_t)plt_head + got_magic_push_offset + 6);
  return *got_magic_addr;
}

void* get_dl_runtime_resolve_addr(void* plt_head) {
  uint32_t plt_head_jmp_offset = *((uint32_t*)((uint64_t)plt_head+8));
  uint64_t* plt_head_jmp_addr = (uint64_t*)((uint64_t)plt_head + 6 + plt_head_jmp_offset + 6);
  return (void*)*plt_head_jmp_addr;
}

uint32_t* get_dl_runtime_resolve_jmp_r11_addr(void* _dl_runtime_resolve_addr) {
  uint32_t* addr = (uint32_t*)((uint64_t)_dl_runtime_resolve_addr + (uint64_t)0x5e);
  //  0x41 0xff 0xe3 0x66 //  0x66e3ff41
  //  [jmp      r11]
  if ( ((*addr) & 0x00ffffff) != 0x00e3ff41) {
    return NULL;
  } else {
    return addr;
  }
}


void* link_noexec(void* plt_func) {
  uint64_t PAGE_SIZE = sysconf(_SC_PAGESIZE);
  uint64_t PAGE_BOUND_MASK = ~(PAGE_SIZE-1);

  if ( ((uint8_t*)plt_func)[0] != 0xff) { // verify that -fPIE + full relro don't mess w/ function pointers
                                          // (by having them point directly to the real code)
                                          // 0xff for jmp as it would appear in a plt entry
    return plt_func;
  }
  void* plt_shortcut = get_got_entry(plt_func);
  //uint32_t plt_push =  get_plt_push(plt_func);
  void* plt_head = get_plt_init_jmp_addr(plt_func);

  uint64_t got_magic = get_got_magic(plt_head);
  void* _dl_runtime_resolve_addr = get_dl_runtime_resolve_addr(plt_head);
  //printf("got_magic: %#016lx, _dl_runtime_resolve_addr: %p\n", got_magic, _dl_runtime_resolve_addr);
  if ((got_magic==0) && (_dl_runtime_resolve_addr==NULL)) {
    //puts("such relro. much now. wow.");
    return plt_shortcut;
  }
  uint32_t* _dl_runtime_resolve_jmp_r11_addr = get_dl_runtime_resolve_jmp_r11_addr(_dl_runtime_resolve_addr);
  if (_dl_runtime_resolve_jmp_r11_addr == NULL) {
    puts("Failed to acquire _dl_runtime_resolve properly.");
    exit(2);
  }

  uint32_t _dl_runtime_resolve_jmp_r11_val = *_dl_runtime_resolve_jmp_r11_addr;

  // rewrite
  void* _dl_runtime_resolve_page = (void*)((uint64_t)_dl_runtime_resolve_jmp_r11_addr & PAGE_BOUND_MASK);
  int mp = mprotect(_dl_runtime_resolve_page, PAGE_SIZE, PROT_WRITE);
  if (mp != 0) { perror("mprotect(PROT_WRITE)"); exit(1); }
  *_dl_runtime_resolve_jmp_r11_addr = (_dl_runtime_resolve_jmp_r11_val & 0xff9090c3) | 0x009090c3; //0x66909090;
  mp = mprotect(_dl_runtime_resolve_page, PAGE_SIZE, PROT_READ | PROT_EXEC);
  if (mp != 0) { perror("mprotect(PROT_READ|PROT_EXEC)"); exit(1); }
  // link
  ((void (*)(void*))plt_func)(NULL);

  // clean up
  mp = mprotect(_dl_runtime_resolve_page, PAGE_SIZE, PROT_WRITE);
  if (mp != 0) { perror("mprotect(PROT_WRITE)"); exit(1); }
  *_dl_runtime_resolve_jmp_r11_addr = _dl_runtime_resolve_jmp_r11_val;
  mp = mprotect(_dl_runtime_resolve_page, PAGE_SIZE, PROT_READ | PROT_EXEC);
  if (mp != 0) { perror("mprotect(PROT_READ|PROT_EXEC)"); exit(1); }

/*
  char buf[128];
  printf("buf: %p\n", buf);

  char* ret = NULL;
  __asm__ __volatile__ (
    "_0: mov %3, %%rdi;\n"
    "nop;\n"
    "call _1;\n"
    "jmp _4;\n"
    "_1: ;\n"
    "mov %0, %%rax;\n"
    "sub4: .byte 0x48, 0x83, 0xec, 0x08;\n"
    "mov %%rax, (%%rsp);\n"
    "pushq %1;\n"
    "movq  %2, %%rax;\n"
    ".intel_syntax noprefix;\n"
    "_3: .byte 0xff, 0xe0;\n" //call rax
    "_4: ;\n"
    "nop;\n"
    :
    : "m"(plt_push), "m"(got_magic), "ro"(_dl_runtime_resolve_addr), "r" (buf)
    :
  );
  __asm__ __volatile__ (
    "mov %%rax, %0;\n"
    : "=r" (ret)
    :
    : "rsi"
  );

  printf("%s\n",buf);
*/
  void* new_shortcut = get_got_entry(plt_func);
  return new_shortcut;
}


char* notgets(void* thing /* must be (c|m)alloc'd */, size_t size /* whole block size */, size_t off) {
  size_t pos = 0;//off;
  char* buf = NULL;
  if (thing == NULL || size == 0) {
    size = 128;
    thing = calloc(size, sizeof(char));
    if (thing == NULL) {
      return NULL;
    }
  }
  buf = (char*)thing + off;

  char* cur = buf;
  char* fg = fgets(cur, size-off, stdin);
  if (fg == NULL) {
    free(buf);
    return NULL;
  }
  char* nl = strchr(cur, '\n');
  while (nl == NULL) {
    pos += strlen(cur);
    if (pos != size-1-off) {
      return buf;
    }
    size *= 2;
    thing = realloc(thing, size);
    buf = (char*)thing + off;
//    buf = (char*)realloc(buf-off, size+off) + off;
    cur = &(buf[pos]);
    fg = fgets(cur, (size / 2) + 1, stdin);
    if (fg == NULL) {
      return buf;
    }
    nl = strchr(cur, '\n');
  }
  *nl = '\0';

  //proper fgets should itself stop at a newline so the below should never run
  for (char* end = &(buf[size-1]); end==nl; end--) {
    ungetc(*end, stdin);
    *end = '\0';
  }

  return buf;
}



char* safer_gets(char* s) {
  apr_hash_index_t* hidx = heapOrNot(s);
  if ( hidx != NULL ) {
    void** key = NULL;
    keyval_t* val = NULL;
    apr_hash_this(hidx, (const void**)&key, NULL, (void**) &val);
    if (s == val->ptr_key) {
      return notgets(val->ptr_key, val->size_val, 0);
    } else {
      size_t off = (size_t)s - (size_t)val->ptr_key;
      return notgets(val->ptr_key, val->size_val, off);
    }
  }
  strcpy(s, "NO! YOU'RE SUPPOSED TO USE THE RETURN VALUE OF gets(3) HERE!");
  return notgets(NULL, 0, 0);
}

__attribute__ ((__constructor__))
int notmain() {
/*  printf("gets: %p\n", (void*)gets);
  void* g = (void*)gets;
  printf("g: %p\n", (void*)g);
  printf("stuff: %#016lx\n", ((uint64_t*)g)[0]);
  printf("stuff: %#016lx\n", ((uint64_t*)gets)[0]);
  char b[128];
  gets(b);
*/
  hash_init();
  void* real_gets = link_noexec((void*)gets);
  (void)real_gets;

  size_t safer_gets_addr = (size_t)safer_gets;
  char asm[12] = { 0x48, 0xb8, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0xff, 0xe0 };
  memcpy(&(asm[2]), (char*)&safer_gets_addr, sizeof(void*));

  uint64_t PAGE_SIZE = sysconf(_SC_PAGESIZE);
  uint64_t PAGE_BOUND_MASK = ~(PAGE_SIZE-1);

  // rewrite
  void* gets_page = (void*)((uint64_t)real_gets & PAGE_BOUND_MASK);
  int mp = mprotect(gets_page, PAGE_SIZE, PROT_WRITE);
  if (mp != 0) { perror("mprotect(PROT_WRITE)"); exit(1); }
  memcpy((char*)real_gets, asm, sizeof(asm));
  mp = mprotect(gets_page, PAGE_SIZE, PROT_READ | PROT_EXEC);
  if (mp != 0) { perror("mprotect(PROT_READ|PROT_EXEC)"); exit(1); }

  return 0;
}

#endif

