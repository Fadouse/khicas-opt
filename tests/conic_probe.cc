#include "giacPCH.h"
#include "parametric_display.h"
#include <iostream>
#include <chrono>
#include <pthread.h>
#include <cstdlib>
#include <cassert>
namespace giac {gen _cart2param(const gen &,GIAC_CONTEXT);}
static int probe_inner(int argc,char **argv){using namespace giac;context ctx;const context *c=&ctx;angle_radian(true,c);
 if(argc>2)gen(argv[2],c).eval(1,c);
 auto start=std::chrono::steady_clock::now();
 gen args=gen(argv[1],c).eval(1,c),raw=_cart2param(args,c);
 if(std::getenv("KHICAS_OUTER_SIMPLIFY"))raw=_simplify(raw,c);
 gen call=symbolic(at_cart2param,args),view;
 bool labelled=parametric_display_view(call,raw,view);
 std::cout<<"RAW "<<raw<<'\n';if(labelled)std::cout<<"VIEW "<<view<<'\n';
 std::cout<<"NODES "<<taille(raw,4096)<<' '<<(labelled?taille(view,4096):0)<<'\n';
 std::cerr<<"SECONDS "<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<'\n';
 return is_undef(raw)?3:raw.type==_VECT?0:4;
}
static int probe(int argc,char **argv){try{return probe_inner(argc,argv);}catch(const std::runtime_error &error){std::cout<<"RAW undef\nNODES 0 0\n";std::cerr<<error.what()<<'\n';return 3;}}
struct args_t {int argc;char **argv;int result;};
static void *worker(void *p){args_t *a=static_cast<args_t *>(p);a->result=probe(a->argc,a->argv);return 0;}
int main(int argc,char **argv){assert(argc>=2);if(!std::getenv("KHICAS_TEST_STACK_KIB"))return probe(argc,argv);
 pthread_attr_t attr;assert(!pthread_attr_init(&attr));assert(!pthread_attr_setstacksize(&attr,65536));assert(!pthread_attr_setguardsize(&attr,4096));pthread_t t;args_t a={argc,argv,0};assert(!pthread_create(&t,&attr,worker,&a));pthread_attr_destroy(&attr);assert(!pthread_join(t,0));return a.result;}
