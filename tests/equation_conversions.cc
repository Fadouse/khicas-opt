#include "giacPCH.h"
#include "usual.h"
#include "subst.h"
#include <cassert>
#include <iostream>

namespace giac {
gen _cart2param(const gen &,GIAC_CONTEXT);
gen _cart2polar(const gen &,GIAC_CONTEXT);
gen _param2cart(const gen &,GIAC_CONTEXT);
gen _param2polar(const gen &,GIAC_CONTEXT);
gen _polar2cart(const gen &,GIAC_CONTEXT);
gen _polar2param(const gen &,GIAC_CONTEXT);
}
using namespace giac;

static gen parse(const char *s,GIAC_CONTEXT) { return gen(s,contextptr).eval(1,contextptr); }
static gen expr(const gen &eq) {
  assert(eq.is_symb_of_sommet(at_equal));
  const vecteur &v=*eq._SYMBptr->feuille._VECTptr;
  return v[0]-v[1];
}
static void expect_equal(const gen &a,const gen &b,GIAC_CONTEXT) {
  gen delta=_simplify(a-b,contextptr);
  if (!is_zero(delta)) {
    std::cerr << "Mismatch: " << a << " != " << b << " (" << delta << ")\n";
    std::abort();
  }
}
static unsigned case_count=0;
static gen call(gen (*fn)(const gen &,const context *),const char *s,GIAC_CONTEXT) {
  gen result=fn(parse(s,contextptr),contextptr);
  std::cout << s << " -> " << result << '\n';
  assert(!is_undef(result)); ++case_count;
  return result;
}

int main(int argc,char **argv) {
  context ctx; const context *contextptr=&ctx;
  angle_radian(true,contextptr);
  gen xy=parse("[x,y]",contextptr),t=parse("t",contextptr);
  gen p=call(_cart2polar,"(x^2+y^2=4,[x,y],[r,theta])",contextptr);
  expect_equal(expr(p),parse("r-2",contextptr),contextptr);
  p=call(_cart2polar,"(y=x,[x,y],[r,theta])",contextptr);
  expect_equal(expr(p),parse("r*(sin(theta)-cos(theta))",contextptr),contextptr);
  expect_equal(subst(expr(p),parse("theta",contextptr),parse("pi",contextptr),false,contextptr),parse("r",contextptr),contextptr);
  p=call(_polar2cart,"(r=2,[r,theta],[x,y])",contextptr);
  expect_equal(expr(p),parse("sqrt(x^2+y^2)-2",contextptr),contextptr);
  p=call(_polar2cart,"(r^2=4,[r,theta],[x,y])",contextptr);
  expect_equal(expr(p),parse("x^2+y^2-4",contextptr),contextptr);
  p=call(_cart2param,"(y=x^2,[x,y],t)",contextptr);
  expect_equal(p,parse("[[t,t^2]]",contextptr),contextptr);
  p=call(_cart2param,"(x=3,[x,y],t)",contextptr);
  expect_equal(p,parse("[[3,t]]",contextptr),contextptr);
  p=call(_cart2param,"(x^2=4,[x,y],t)",contextptr);
  assert(p.type==_VECT && p._VECTptr->size()==2);
  p=call(_cart2param,"(x*y=0,[x,y],t)",contextptr);
  assert(p.type==_VECT && p._VECTptr->size()==2);
  assert(equalposcomp(*p._VECTptr,parse("[0,t]",contextptr)));
  assert(equalposcomp(*p._VECTptr,parse("[t,0]",contextptr)));
  p=call(_cart2param,"((x-y)^2=0,[x,y],t)",contextptr);
  expect_equal(p,parse("[[t,t]]",contextptr),contextptr);
  p=call(_cart2param,"(x^2+y^2=4,[x,y],t)",contextptr);
  assert(p.type==_VECT && p._VECTptr->size()==1);
  expect_equal(p,parse("[[2*cos(t),2*sin(t)]]",contextptr),contextptr);
  for (unsigned i=0;i<p._VECTptr->size();++i) {
    const vecteur &v=*(*p._VECTptr)[i]._VECTptr;
    expect_equal(v[0]*v[0]+v[1]*v[1],4,contextptr);
  }
  p=call(_polar2param,"(r=1+cos(theta),[r,theta],t)",contextptr);
  expect_equal(p,parse("[[(1+cos(t))*cos(t),(1+cos(t))*sin(t)]]",contextptr),contextptr);
  p=call(_polar2param,"(theta=pi/4,[r,theta],t)",contextptr);
  expect_equal(p,parse("[[t/sqrt(2),t/sqrt(2)]]",contextptr),contextptr);
  p=call(_param2polar,"([3,4],t)",contextptr);
  expect_equal(p._VECTptr->front(),5,contextptr);
  expect_equal(p._VECTptr->back(),parse("arg(3+4*i)",contextptr),contextptr);
  p=call(_param2polar,"([-3,4],t)",contextptr);
  expect_equal(p._VECTptr->back(),parse("arg(-3+4*i)",contextptr),contextptr);
  p=call(_param2polar,"([cos(t),sin(t)],t,[r,theta])",contextptr);
  expect_equal(expr(p),parse("r-1",contextptr),contextptr);
  p=call(_param2polar,"([t,t^2],t,[r,theta])",contextptr);
  expect_equal(expr(p),parse("r*sin(theta)-r^2*cos(theta)^2",contextptr),contextptr);
  p=call(_param2cart,"([t,t^2],t,[x,y])",contextptr);
  expect_equal(expr(p),parse("y-x^2",contextptr),contextptr);
  p=call(_param2cart,"([t^2,t],t,[x,y])",contextptr);
  expect_equal(expr(p),parse("x-y^2",contextptr),contextptr);
  const char *curves[]={"([cos(t),sin(t)],t,[x,y])",
    "([(1-t^2)/(1+t^2),2*t/(1+t^2)],t,[x,y])",
    "([2*cos(t),3*sin(t)],t,[x,y])", "([t^2,t^3],t,[x,y])",
    "([2*t+1,3*t-2],t,[x,y])"};
  const char *samples[]={"[cos(t),sin(t)]", "[(1-t^2)/(1+t^2),2*t/(1+t^2)]",
    "[2*cos(t),3*sin(t)]", "[t^2,t^3]", "[2*t+1,3*t-2]"};
  for (unsigned i=0;i<5;++i) {
    p=call(_param2cart,curves[i],contextptr);
    gen residual=expr(p);
    assert(!is_zero(residual));
    expect_equal(subst(residual,*xy._VECTptr,*parse(samples[i],contextptr)._VECTptr,false,contextptr),0,contextptr);
  }
  p=call(_param2cart,"([2,3],t,[x,y])",contextptr);
  assert(p.type==_VECT && p._VECTptr->size()==2);
  // Custom coordinate names and collisions.
  p=call(_cart2param,"(v=u^2,[u,v],s)",contextptr);
  expect_equal(p,parse("[[s,s^2]]",contextptr),contextptr);
  // In native builds Giac raises exceptions for errors; the device returns undef.
  typedef gen (*function)(const gen &,const context *);
  struct bad { function fn; const char *args; } errors[]={
    {_cart2polar,"(y=x,[x,x],[r,theta])"},
    {_cart2polar,"(y=x,[x,y],[x,theta])"},
    {_cart2polar,"(y=r*x,[x,y],[r,theta])"},
    {_cart2param,"(y=x+t,[x,y],t)"},
    {_cart2param,"(1,[x,y],t)"},
    {_param2cart,"([sin(t),exp(t)],t,[x,y])"},
    {_param2cart,"([t+x,t^2],t,[x,y])"},
    {_param2polar,"([t,t^2,t^3],t)"},
    {_param2polar,"([t,t^2],t,[t,theta])"},
    {_polar2param,"(r=1,[r,theta],r)"}
  };
  for (unsigned i=0;i<sizeof(errors)/sizeof(*errors);++i) {
    bool failed=false;
    try { failed=is_undef(errors[i].fn(parse(errors[i].args,contextptr),contextptr)); }
    catch (const std::runtime_error &) { failed=true; }
    assert(failed); ++case_count;
  }
  angle_radian(false,contextptr);
  bool rejected=false;
  try { rejected=is_undef(_cart2polar(parse("(y=x,[x,y],[r,theta])",contextptr),contextptr)); }
  catch (const std::runtime_error &) { rejected=true; }
  assert(rejected && !angle_radian(contextptr)); ++case_count;
  angle_radian(true,contextptr);
  for (int i=1;i<argc;++i) {
    gen result=_read(string2gen(argv[i],false),contextptr);
    assert(!is_undef(result));
    std::cout << "PASS: script " << argv[i] << '\n';
  }
  std::cout << "PASS: " << case_count << " conversion cases\n";
}
