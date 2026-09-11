#ifndef KHICAS_GUARDED_PROBE_STACK_H
#define KHICAS_GUARDED_PROBE_STACK_H
// Host-only, single-invocation test harness. Keep library TLS on the main
// thread while testing the computation on a separately mapped bounded stack.
// A pthread exposed an old host NTL FFT-table teardown fault after successful
// evaluation; no CG50 code uses this harness or the host NTL shared library.
#include <cstdlib>
#include <sys/mman.h>
#include <unistd.h>
#include <ucontext.h>
namespace guarded_probe {
static int (*compute)(int,char **);
static int argument_count, result;
static char **arguments;
static void invoke(){ result=compute(argument_count,arguments); }
static int run(int argc,char **argv,int (*function)(int,char **)){
  const char *size=std::getenv("KHICAS_TEST_STACK_KIB");
  if(!size)return function(argc,argv);
  char *end=0;
  unsigned long kib=std::strtoul(size,&end,10);
  if(!end || *end || kib<16 || kib>8192)return 7;
  long page=sysconf(_SC_PAGESIZE);
  if(page<=0 || (kib*1024)%page)return 7;
  size_t bytes=kib*1024,total=bytes+2*page;
  void *mapping=mmap(0,total,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
  if(mapping==MAP_FAILED)return 7;
  char *stack=static_cast<char *>(mapping)+page;
  if(mprotect(stack,bytes,PROT_READ|PROT_WRITE)){
    munmap(mapping,total);return 7;
  }
  ucontext_t caller,callee;
  if(getcontext(&callee)){munmap(mapping,total);return 7;}
  callee.uc_stack.ss_sp=stack;callee.uc_stack.ss_size=bytes;
  callee.uc_stack.ss_flags=0;callee.uc_link=&caller;
  compute=function;argument_count=argc;arguments=argv;result=7;
  makecontext(&callee,invoke,0);
  int status=swapcontext(&caller,&callee);
  munmap(mapping,total);
  return status?7:result;
}
}
#endif
