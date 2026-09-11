#include "giacPCH.h"
#include <iostream>
#include <chrono>
#include <pthread.h>
#include <cstdlib>
#include <cassert>
#include <sys/resource.h>
namespace giac {gen _cart2polar(const gen &,GIAC_CONTEXT);gen _param2polar(const gen &,GIAC_CONTEXT);}
static int compute(int argc,char **argv){using namespace giac;
  context ctx;const context *c=&ctx;angle_radian(true,c);
  try {
    if(argc>3)gen(argv[3],c).eval(1,c);
    auto start=std::chrono::steady_clock::now();
    gen args=gen(argv[2],c).eval(1,c);
    gen out=std::string(argv[1])=="param"?_param2polar(args,c):_cart2polar(args,c);
    if(std::getenv("KHICAS_OUTER_SIMPLIFY"))out=_simplify(out,c);
    std::cout<<"RAW "<<out<<"\nNODES "<<taille(out,4096)<<'\n';
    std::cerr<<"SECONDS "<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<'\n';
    return is_undef(out)?3:0;
  }catch(const std::runtime_error &e){std::cerr<<e.what()<<'\n';return 3;}
}
struct task {int argc;char **argv;int result;};
static void *worker(void *p){task *t=static_cast<task *>(p);t->result=compute(t->argc,t->argv);return 0;}
int main(int argc,char **argv){
  assert(argc>=3);rlimit core={0,0};setrlimit(RLIMIT_CORE,&core);rlimit memory={512*1024*1024,512*1024*1024};setrlimit(RLIMIT_AS,&memory);
  if(!std::getenv("KHICAS_TEST_STACK_KIB"))return compute(argc,argv);
  pthread_attr_t attr;assert(!pthread_attr_init(&attr));assert(!pthread_attr_setstacksize(&attr,65536));assert(!pthread_attr_setguardsize(&attr,4096));
  pthread_t thread;task t={argc,argv,0};assert(!pthread_create(&thread,&attr,worker,&t));pthread_attr_destroy(&attr);assert(!pthread_join(thread,0));return t.result;
}
