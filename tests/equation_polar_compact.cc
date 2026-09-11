#include "giacPCH.h"
#include <cassert>
#include <iostream>
namespace giac {gen _cart2polar(const gen &,GIAC_CONTEXT);}
using namespace giac;
static gen parse(const char *s,GIAC_CONTEXT){return gen(s,contextptr).eval(1,contextptr);}
static gen residual(const gen &g){assert(g.is_symb_of_sommet(at_equal));return g._SYMBptr->feuille[0]-g._SYMBptr->feuille[1];}
static void expect_same(const gen &a,const gen &b,GIAC_CONTEXT){
 if(!is_zero(_simplify(a-b,contextptr))){std::cerr<<"NOT EQUAL "<<a<<" vs "<<b<<'\n';std::abort();}
}
static gen convert(const char *s,GIAC_CONTEXT){return _cart2polar(parse(s,contextptr),contextptr);}
int main(){
 context ctx;const context *contextptr=&ctx;angle_radian(true,contextptr);
 gen r=parse("r",contextptr),theta=parse("theta",contextptr),xy=parse("[x,y]",contextptr);
 gen sine=sin(theta,contextptr),cosine=cos(theta,contextptr),D=pow(cosine,3)+pow(sine,3);
 const char *lemniscates[]={"((x^2+y^2)^2=4*(x^2-y^2),[x,y],[r,theta])",
 "((x^2+y^2)^2=-4*(x^2-y^2),[x,y],[r,theta])",
 "((x^2+y^2)^2=0,[x,y],[r,theta])",
 "(4*(x^2-y^2)=(x^2+y^2)^2,[x,y],[r,theta])",
 "(3*x^4+6*x^2*y^2+3*y^4=12*x^2-12*y^2,[x,y],[r,theta])"};
 const int coefficients[]={4,-4,0,4,4};
 for(unsigned i=0;i<5;++i){
  gen eq=convert(lemniscates[i],contextptr);
  expect_same(eq._SYMBptr->feuille[0],r,contextptr);
  expect_same(pow(eq._SYMBptr->feuille[1],2),coefficients[i]*cos(2*theta,contextptr),contextptr);
  expect_same(subst(residual(eq),makevecteur(r,theta),makevecteur(0,cst_pi/4),false,contextptr),0,contextptr);
  if(coefficients[i])for(int half=0;half<=1;++half)
   expect_same(subst(residual(eq),makevecteur(r,theta),makevecteur(2,(coefficients[i]>0?gen(0):cst_pi/2)+half*cst_pi),false,contextptr),0,contextptr);
 }
 gen positive=convert(lemniscates[0],contextptr);
 // The documented contract is Cartesian curve equivalence: at r=0 the
 // compact form deliberately removes redundant angle labels, not the origin.
 assert(!is_zero(subst(residual(positive),makevecteur(r,theta),makevecteur(0,0),false,contextptr)));
 gen zero=convert(lemniscates[2],contextptr);
 expect_same(subst(residual(zero),r,0,false,contextptr),0,contextptr);
 gen symbolic=convert("((x^2+y^2)^2=b*(x^2-y^2),[x,y],[r,theta])",contextptr);
 expect_same(symbolic._SYMBptr->feuille[0],r,contextptr);
 expect_same(pow(symbolic._SYMBptr->feuille[1],2),parse("b*cos(2*theta)",contextptr),contextptr);
 gen custom=convert("((u^2+v^2)^2=4*(u^2-v^2),[u,v],[rho,phi])",contextptr);
 expect_same(residual(custom),parse("rho-2*sqrt(cos(2*phi))",contextptr),contextptr);
 const char *folia[]={"(x^3+y^3=6*x*y,[x,y],[r,theta])", "(x^3+y^3=-6*x*y,[x,y],[r,theta])",
 "(6*x*y=x^3+y^3,[x,y],[r,theta])"};
 for(unsigned i=0;i<3;++i){
  const int k=i==1?-6:6;gen eq=convert(folia[i],contextptr);
  expect_same(eq._SYMBptr->feuille[0],r,contextptr);
  gen radius=eq._SYMBptr->feuille[1];expect_same(radius,k*sine*cosine/D,contextptr);
  expect_same(subst(radius,theta,0,false,contextptr),0,contextptr); // origin is present
  const char *angles[]={"pi/6","pi/4","pi/2","pi","5*pi/4"};
  for(const char *angle:angles){
   gen t=parse(angle,contextptr),value=subst(radius,theta,t,false,contextptr).eval(1,contextptr);
   gen x=value*cos(t,contextptr),y=value*sin(t,contextptr);
   expect_same(pow(x,3)+pow(y,3)-k*x*y,0,contextptr);
  }
  gen pole=3*cst_pi/4;
  expect_same(subst(D,theta,pole,false,contextptr),0,contextptr);
  assert(!is_zero(subst(k*sine*cosine,theta,pole,false,contextptr).eval(1,contextptr)));
  // At a forbidden denominator angle the original curve has no nonzero radius.
  gen x=2*cos(pole,contextptr),y=2*sin(pole,contextptr);
  assert(!is_zero(_simplify(pow(x,3)+pow(y,3)-k*x*y,contextptr)));
  // The negative-radius representation maps to the same point as its
  // nonnegative-radius principal-angle representation.
  gen t=k>0?5*cst_pi/4:cst_pi/4;
  gen negative=subst(radius,theta,t,false,contextptr).eval(1,contextptr);
  assert(is_strictly_positive(-negative,contextptr));
  expect_same(subst(radius,theta,t-cst_pi,false,contextptr),-negative,contextptr);
 }
 gen degenerate=convert("(x^3+y^3=0,[x,y],[r,theta])",contextptr);
 expect_same(residual(degenerate),r*D,contextptr);
 expect_same(subst(residual(degenerate),theta,3*cst_pi/4,false,contextptr),0,contextptr);
 assert(!is_zero(subst(residual(degenerate),makevecteur(r,theta),makevecteur(2,0),false,contextptr)));
 symbolic=convert("(x^3+y^3=3*a*x*y,[x,y],[r,theta])",contextptr);
 expect_same(residual(symbolic),r*D-3*parse("a",contextptr)*sine*cosine,contextptr);
 expect_same(subst(residual(symbolic),parse("a",contextptr),0,false,contextptr),r*D,contextptr);
 // An unknown overall factor cannot be cancelled: at a=0 the input is the plane.
 gen factor=convert("(a*(x^2+y^2)^2=4*a*(x^2-y^2),[x,y],[r,theta])",contextptr);
 expect_same(subst(residual(factor),parse("a",contextptr),0,false,contextptr),0,contextptr);
 factor=convert("(a*x^3+a*y^3=3*a*x*y,[x,y],[r,theta])",contextptr);
 expect_same(subst(residual(factor),parse("a",contextptr),0,false,contextptr),0,contextptr);
 gen near=convert("((x^2+y^2)^2=4*(x^2-y^2)+1,[x,y],[r,theta])",contextptr);
 assert(!is_zero(subst(residual(near),r,0,false,contextptr)));
 gen line=convert("(y=x,[x,y],[r,theta])",contextptr);
 expect_same(subst(residual(line),r,0,false,contextptr),0,contextptr);
 expect_same(subst(residual(line),theta,cst_pi/4,false,contextptr),0,contextptr);
 std::cout<<"PASS: compact lemniscate/folium forms, signed radii, origins, pole exclusions and parameter degeneracies\n";
}
