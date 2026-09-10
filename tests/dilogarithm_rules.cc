#include "giacPCH.h"
#include <cassert>
#include <iostream>
#include <pthread.h>
namespace giac {
bool integrate_dilog_primitive(const gen &,const gen &,gen &,GIAC_CONTEXT);
bool integrate_dilog_definite(const gen &,const gen &,const gen &,const gen &,gen &,GIAC_CONTEXT);
gen integration_syntax(const gen &,GIAC_CONTEXT);
}
static void checks(){using namespace giac;context ctx;const context *c=&ctx;gen x=identificateur("x"),res;
 for(const char *f:{"ln(1-x^33)/x","ln(1-x^(1/17))/x","ln(1-x)/x^2","ln(1-x)^2/x","ln(1-x)/(x*(1-x)^2)","ln(1-x)/(x+1)","ln(1-exp(-x^2))","ln(1-x-y)/x","ln(1-x)*sin(x)/x"})
  assert(!integrate_dilog_primitive(integration_syntax(gen(f,c),c),x,res,c));
 const char *cases[][3]={
 {"ln(1-x)/x","0","2"},{"ln(1-x)/(x*(1-x))","0","1"},
 {"ln(1-1/x)/x","0","1"},{"ln(1+x)/x","-2","0"},
 {"ln(1+x)/x","0","+infinity"},{"ln(1+x)/x","0","a"},
 {"ln(1-x)/x^2","0","2"},{"ln(1-x^2)/x^3","0","2"}};
 for(const auto &r:cases)assert(!integrate_dilog_definite(integration_syntax(gen(r[0],c),c),x,gen(r[1],c).eval(1,c),gen(r[2],c).eval(1,c),res,c));
 std::cout<<"PASS: 9 unsupported primitives and 8 invalid/unsupported endpoint domains declined by the bounded Li2 rules\n";
}
static void *worker(void *){checks();return 0;}
int main(){checks();pthread_attr_t a;assert(!pthread_attr_init(&a));assert(!pthread_attr_setstacksize(&a,65536));assert(!pthread_attr_setguardsize(&a,4096));pthread_t t;assert(!pthread_create(&t,&a,worker,0));pthread_attr_destroy(&a);assert(!pthread_join(t,0));}
