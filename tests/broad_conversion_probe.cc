// All six repository conversion entry points, with the common guarded stack.
#include "giacPCH.h"
#include "parametric_display.h"
#include "guarded_probe_stack.h"
#include <chrono>
#include <iostream>
#include <cstdlib>
#include <sys/resource.h>
namespace giac {
gen _cart2polar(const gen &,GIAC_CONTEXT);
gen _param2polar(const gen &,GIAC_CONTEXT);
gen _cart2param(const gen &,GIAC_CONTEXT);
gen _polar2cart(const gen &,GIAC_CONTEXT);
gen _polar2param(const gen &,GIAC_CONTEXT);
gen _param2cart(const gen &,GIAC_CONTEXT);
}
static int compute(int argc,char **argv){
  using namespace giac;
  if(argc<3)return 64;
  rlimit memory={768*1024*1024,768*1024*1024},core={0,0};
  setrlimit(RLIMIT_AS,&memory);setrlimit(RLIMIT_CORE,&core);
  context ctx;const context *c=&ctx;angle_radian(true,c);
  try {
    if(argc>3)gen(argv[3],c).eval(1,c);
    auto start=std::chrono::steady_clock::now();
    gen args=gen(argv[2],c).eval(1,c),out;std::string name=argv[1];
    if(name=="cart2polar")out=_cart2polar(args,c);
    else if(name=="param2polar")out=_param2polar(args,c);
    else if(name=="cart2param")out=_cart2param(args,c);
    else if(name=="polar2cart")out=_polar2cart(args,c);
    else if(name=="polar2param")out=_polar2param(args,c);
    else if(name=="param2cart")out=_param2cart(args,c);
    else return 64;
    if(std::getenv("KHICAS_OUTER_SIMPLIFY"))out=_simplify(out,c);
    std::cout<<"RAW "<<out<<"\nNODES "<<taille(out,65537)<<'\n';
    gen view;
    if(name=="cart2param" && parametric_display_view(symbolic(at_cart2param,args),out,view))
      std::cout<<"VIEW "<<view<<'\n';
    std::cerr<<"SECONDS "<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<'\n';
    return is_undef(out)?3:0;
  } catch(const std::runtime_error &error){std::cerr<<error.what()<<'\n';return 3;}
}
int main(int argc,char **argv){return guarded_probe::run(argc,argv,compute);}
