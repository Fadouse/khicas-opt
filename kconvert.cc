// KhiCAS equation conversions. GPL-2.0-or-later, like the Giac engine.
#include "giacPCH.h"
#include "usual.h"
#include "subst.h"
#include "solve.h"
#include "sym2poly.h"
#include "series.h"
#include "rpn.h"
#include "derive.h"
#include "lin.h"
#include "equation_normalize.h"

namespace giac {

gen _param2cart(const gen &args,GIAC_CONTEXT);

static bool pair(const gen &g) {
  return g.type==_VECT && g._VECTptr->size()==2;
}

static bool variables(const gen &g) {
  return pair(g) && g._VECTptr->front().type==_IDNT &&
    g._VECTptr->back().type==_IDNT &&
    g._VECTptr->front()!=g._VECTptr->back();
}

static bool depends(const gen &g,const gen &v) {
  return !lvarx(g,v).empty();
}

static bool disjoint(const gen &a,const gen &b) {
  const vecteur &v=*a._VECTptr;
  if (b.type==_IDNT)
    return v[0]!=b && v[1]!=b;
  const vecteur &w=*b._VECTptr;
  return v[0]!=w[0] && v[0]!=w[1] && v[1]!=w[0] && v[1]!=w[1];
}

static gen residual(const gen &g) {
  if (g.is_symb_of_sommet(at_equal) && pair(g._SYMBptr->feuille)) {
    const vecteur &v=*g._SYMBptr->feuille._VECTptr;
    return v[0]-v[1];
  }
  return g; // A scalar expression denotes g=0.
}

static bool scalar(const gen &g) {
  return g.type!=_VECT && g.type!=_STRNG && !is_undef(g);
}

struct curve_term {
  unsigned px,py;gen coefficient;
  curve_term(unsigned x,unsigned y,const gen &c):px(x),py(y),coefficient(c){}
};
typedef std::vector<curve_term> curve_polynomial_terms;

static bool curve_rational(const gen &g) {
  return g.type==_INT_ || g.type==_ZINT ||
    (g.type==_FRAC && (g._FRACptr->num.type==_INT_ || g._FRACptr->num.type==_ZINT) &&
     (g._FRACptr->den.type==_INT_ || g._FRACptr->den.type==_ZINT) && !is_zero(g._FRACptr->den));
}
// Return -1/0/1 for a proved sign, 2 when undecided. The engine's
// positivity shortcut treats a symbolic square as positive even at its
// possible zeros; that cannot justify discarding a degenerate conic.
static int curve_sign(const gen &g,unsigned depth,GIAC_CONTEXT) {
  if(depth>12 || taille(g,65)>64 || is_undef(g) || is_inf(g))return 2;
  if(is_zero(g))return 0;
  vecteur names=lidnt(g);bool numeric=true;
  for(unsigned j=0;j<names.size();++j)if(names[j]!=cst_pi){numeric=false;break;}
  if(numeric || g.type==_IDNT)
    return is_strictly_greater(g,0,contextptr)?1:is_strictly_greater(-g,0,contextptr)?-1:2;
  if(g.is_symb_of_sommet(at_neg)){
    int s=curve_sign(g._SYMBptr->feuille,depth+1,contextptr);return s==2?2:-s;
  }
  if(g.is_symb_of_sommet(at_inv)){
    int s=curve_sign(g._SYMBptr->feuille,depth+1,contextptr);return s==0?2:s;
  }
  if(g.is_symb_of_sommet(at_pow) && pair(g._SYMBptr->feuille)){
    const vecteur &v=*g._SYMBptr->feuille._VECTptr;
    int sign=curve_sign(v[0],depth+1,contextptr);
    if(sign==2)return 2;
    if(v[1].type==_INT_ && v[1].val){if(!sign)return v[1].val>0?0:2;return (v[1].val%2)?sign:1;}
    if(sign==1 && curve_rational(v[1]))return 1;
    return 2;
  }
  if(g.is_symb_of_sommet(at_prod) && g._SYMBptr->feuille.type==_VECT){
    int sign=1;const vecteur &v=*g._SYMBptr->feuille._VECTptr;
    for(unsigned j=0;j<v.size();++j){int t=curve_sign(v[j],depth+1,contextptr);if(t==2)return 2;sign*=t;}
    return sign;
  }
  if(g.is_symb_of_sommet(at_plus) && g._SYMBptr->feuille.type==_VECT){
    // Factoring short, low-degree parameter polynomials exposes possible
    // zero factors such as (a-b)^2, instead of assuming them nonzero.
    vecteur powers=lop(g,at_pow);
    bool bounded=true;
    for(unsigned j=0;j<powers.size();++j){
      const gen &f=powers[j]._SYMBptr->feuille;
      if(!pair(f) || f._VECTptr->back().type!=_INT_ || f._VECTptr->back().val<0 || f._VECTptr->back().val>8){bounded=false;break;}
    }
    if(bounded){gen factored=_factor(g,contextptr);if(!is_undef(factored) && factored!=g)return curve_sign(factored,depth+1,contextptr);}
    const vecteur &v=*g._SYMBptr->feuille._VECTptr;
    bool nonnegative=true,nonpositive=true,positive=false,negative=false;
    for(unsigned j=0;j<v.size();++j){
      int t=curve_sign(v[j],depth+1,contextptr);
      positive=positive || t==1;negative=negative || t==-1;
      nonnegative=nonnegative && is_positive(v[j],contextptr);
      nonpositive=nonpositive && is_positive(-v[j],contextptr);
    }
    if(nonnegative && positive)return 1;if(nonpositive && negative)return -1;
  }
  return 2;
}

// Keep the old negative-form proof fallback when the bounded positive-form
// analysis is undecided, but do not repeat it for an already proved sign.
static int curve_proved_sign(const gen &g,GIAC_CONTEXT) {
  int s=curve_sign(g,0,contextptr);
  return s==2 && curve_sign(-g,0,contextptr)==1?-1:s;
}

static gen curve_coefficient(const curve_polynomial_terms &p,unsigned x,unsigned y) {
  for(unsigned i=0;i<p.size();++i)if(p[i].px==x && p[i].py==y)return p[i].coefficient;
  return 0;
}
static bool curve_add_term(curve_polynomial_terms &p,unsigned x,unsigned y,const gen &c,unsigned &work) {
  if(!work || x+y>8)return false;--work;
  if(is_zero(c))return true;
  for(unsigned i=0;i<p.size();++i)if(p[i].px==x && p[i].py==y){
    p[i].coefficient+=c;
    if(is_zero(p[i].coefficient))p.erase(p.begin()+i);
    return true;
  }
  if(p.size()>=64)return false;
  p.push_back(curve_term(x,y,c));return true;
}
static bool curve_multiply(const curve_polynomial_terms &a,const curve_polynomial_terms &b,
                           curve_polynomial_terms &p,unsigned &work) {
  p.clear();
  for(unsigned i=0;i<a.size();++i)for(unsigned j=0;j<b.size();++j)
    if(!curve_add_term(p,a[i].px+b[j].px,a[i].py+b[j].py,a[i].coefficient*b[j].coefficient,work))return false;
  return true;
}
static bool curve_collect_polynomial(const gen &g,const gen &x,const gen &y,
                                     curve_polynomial_terms &out,unsigned depth,unsigned &work,GIAC_CONTEXT) {
  if(!work || depth>16)return false;--work;out.clear();
  if(g==x){out.push_back(curve_term(1,0,1));return true;}
  if(g==y){out.push_back(curve_term(0,1,1));return true;}
  if(!depends(g,x) && !depends(g,y)){
    if(!is_zero(g))out.push_back(curve_term(0,0,g));return true;
  }
  if(g.type!=_SYMB)return false;
  const gen &f=g._SYMBptr->feuille;
  if(g.is_symb_of_sommet(at_neg)){
    if(!curve_collect_polynomial(f,x,y,out,depth+1,work,contextptr))return false;
    for(unsigned i=0;i<out.size();++i)out[i].coefficient=-out[i].coefficient;return true;
  }
  if(f.type!=_VECT)return false;
  const vecteur &v=*f._VECTptr;
  if(g.is_symb_of_sommet(at_division) && v.size()==2){
    if(depends(v[1],x)||depends(v[1],y))return false;
    gen denominator=ratnormal(v[1],contextptr);
    if(!curve_rational(denominator)||is_zero(denominator) ||
       !curve_collect_polynomial(v[0],x,y,out,depth+1,work,contextptr))return false;
    for(unsigned i=0;i<out.size();++i)out[i].coefficient=out[i].coefficient/denominator;
    return true;
  }
  if(g.is_symb_of_sommet(at_plus)){
    if(v.size()>64)return false;
    for(unsigned i=0;i<v.size();++i){curve_polynomial_terms a;
      if(!curve_collect_polynomial(v[i],x,y,a,depth+1,work,contextptr))return false;
      for(unsigned j=0;j<a.size();++j)if(!curve_add_term(out,a[j].px,a[j].py,a[j].coefficient,work))return false;
    }return true;
  }
  if(g.is_symb_of_sommet(at_prod)){
    if(v.size()>16)return false;out.push_back(curve_term(0,0,1));
    for(unsigned i=0;i<v.size();++i){curve_polynomial_terms a,p;
      if(!curve_collect_polynomial(v[i],x,y,a,depth+1,work,contextptr) || !curve_multiply(out,a,p,work))return false;
      out.swap(p);
    }return true;
  }
  if(g.is_symb_of_sommet(at_pow) && v.size()==2 && v[1].type==_INT_ && v[1].val>=0 && v[1].val<=8){
    curve_polynomial_terms a;
    if(!curve_collect_polynomial(v[0],x,y,a,depth+1,work,contextptr))return false;
    out.push_back(curve_term(0,0,1));
    for(int i=0;i<v[1].val;++i){curve_polynomial_terms p;if(!curve_multiply(out,a,p,work))return false;out.swap(p);}return true;
  }
  return false;
}
static bool curve_polynomial(const gen &g,const gen &x,const gen &y,curve_polynomial_terms &out,GIAC_CONTEXT) {
  if(taille(g,513)>512)return false;unsigned work=4096;
  return curve_collect_polynomial(g,x,y,out,0,work,contextptr);
}
static bool curve_folium_param(const curve_polynomial_terms &p,const gen &t,gen &out,GIAC_CONTEXT) {
  if(p.size()!=2 && p.size()!=3)return false;
  gen L=ratnormal(curve_coefficient(p,3,0),contextptr);
  if(!curve_rational(L)||is_zero(L) || !is_zero(ratnormal(curve_coefficient(p,0,3)-L,contextptr)))return false;
  gen K=-curve_coefficient(p,1,1);
  if(is_zero(K)){
    if(p.size()!=2)return false;
    out=vecteur(1,makevecteur(t,-t));return true;
  }
  if(p.size()!=3 || curve_proved_sign(K,contextptr)==2)return false;
  gen X=ratnormal((K/L)*t/(1+pow(t,3)),contextptr);
  out=vecteur(1,makevecteur(X,ratnormal(t*X,contextptr)));
  *logptr(contextptr)<<"Rational parametrization: t=-1 is excluded.\n";
  return true;
}

static bool curve_lemniscate_param(const curve_polynomial_terms &p,const gen &t,gen &out,GIAC_CONTEXT) {
  if(p.size()!=3 && p.size()!=5)return false;
  gen L=curve_coefficient(p,4,0);
  int sl=curve_proved_sign(L,contextptr);if(sl==0 || sl==2)return false;
  if(!is_zero(ratnormal(curve_coefficient(p,0,4)-L,contextptr)) ||
     !is_zero(ratnormal(curve_coefficient(p,2,2)-2*L,contextptr)) ||
     !is_zero(ratnormal(curve_coefficient(p,2,0)+curve_coefficient(p,0,2),contextptr)))return false;
  gen K=ratnormal(curve_coefficient(p,0,2)/L,contextptr);
  if(is_zero(K)){if(p.size()!=3)return false;out=vecteur(1,makevecteur(0,0));return true;}
  if(p.size()!=5 || !angle_radian(contextptr))return false;
  int sk=curve_proved_sign(K,contextptr);if(sk==2)return false;
  bool rotated=sk==-1;
  gen s=sin(t,contextptr),c=cos(t,contextptr),X=sqrt(rotated?-K:K,contextptr)*c/(1+s*s);
  out=vecteur(1,rotated?makevecteur(X*s,X):makevecteur(X,X*s));return true;
}

static bool curve_origin_pencil(const curve_polynomial_terms &p,const gen &t,gen &out,GIAC_CONTEXT) {
  if(p.empty())return false;unsigned low=9,high=0;
  for(unsigned i=0;i<p.size();++i){
    if(!curve_rational(p[i].coefficient))return false;
    unsigned n=p[i].px+p[i].py;if(n<low)low=n;if(n>high)high=n;
  }
  if(high<2 || high>8 || low+1!=high)return false;
  gen P=0,Q=0;
  for(unsigned i=0;i<p.size();++i){
    gen term=p[i].coefficient*pow(t,int(p[i].py));
    if(p[i].px+p[i].py==high)P+=term;else Q+=term;
  }
  // A common direction factor is a line component. Let factor/solve handle it
  // instead of cancelling it out of the parameter map.
  gen common=_gcd(makesequence(P,Q),contextptr);
  if(is_undef(common)||depends(common,t))return false;
  gen X=ratnormal(-Q/P,contextptr);
  vecteur branches(1,makevecteur(X,ratnormal(t*X,contextptr)));
  // Every nonvertical nonzero point has t=y/x. Add the missing vertical point
  // (or vertical component) and an isolated origin when the map misses it.
  gen verticalP=curve_coefficient(p,0,high),verticalQ=curve_coefficient(p,0,high-1);
  if(is_zero(verticalP) && is_zero(verticalQ))branches.push_back(makevecteur(0,t));
  else if(!is_zero(verticalP) && !is_zero(verticalQ))branches.push_back(makevecteur(0,-verticalQ/verticalP));
  bool origin_at_zero=!is_zero(curve_coefficient(p,high,0)) && is_zero(curve_coefficient(p,high-1,0));
  if(!origin_at_zero && !(is_zero(verticalP)&&is_zero(verticalQ)))branches.push_back(makevecteur(0,0));
  *logptr(contextptr)<<"Rational parametrization: retain nonzero denominator conditions; constant branches include missing points.\n";
  out=branches;return true;
}

// Classify degree-two curves before rational pencils or root isolation.
// Symbolic signs must also preserve possible zero factors and degeneracies.
// Complete real trigonometric/hyperbolic charts have no missing endpoints.
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("Os")))
#endif
static bool curve_conic_param(const curve_polynomial_terms &p,const gen &t,gen &out,GIAC_CONTEXT) {
  if(!angle_radian(contextptr) || p.empty() || p.size()>6)return false;
  for(unsigned j=0;j<p.size();++j)
    if(p[j].px+p[j].py>2 || taille(p[j].coefficient,33)>32 || !is_zero(im(p[j].coefficient,contextptr)))return false;
  gen A=curve_coefficient(p,2,0),B=curve_coefficient(p,1,1),C=curve_coefficient(p,0,2);
  gen D=curve_coefficient(p,1,0),E=curve_coefficient(p,0,1),F=curve_coefficient(p,0,0);
  gen det=ratnormal(4*A*C-B*B,contextptr);
  if(is_zero(A) && is_zero(B) && is_zero(C))return false;
  if(is_zero(det)){
    // Rank-one quadratic: u=x+B*y/(2*A) makes a parabola a polynomial
    // graph, or a pair of parallel lines. Swap coordinates if necessary.
    bool swapped=is_zero(A);
    if(swapped){swapgen(A,C);swapgen(D,E);}
    int sa=curve_proved_sign(A,contextptr);if(sa==0 || sa==2)return false;
    gen shear=B/(2*A),linear=ratnormal(E-D*shear,contextptr);
    vecteur branches;
    int sl=curve_proved_sign(linear,contextptr);
    if(sl==1 || sl==-1){
      gen U=t-D/(2*A),V=ratnormal((-A*t*t+D*D/(4*A)-F)/linear,contextptr);
      gen X=ratnormal(U-shear*V,contextptr);
      branches.push_back(swapped?makevecteur(V,X):makevecteur(X,V));
    }
    else if(is_zero(linear)){
      gen discriminant=ratnormal(D*D-4*A*F,contextptr);
      int sd=curve_proved_sign(discriminant,contextptr);
      if(sd==-1){out=vecteur(0);return true;}
      if(sd==2)return false;
      gen root=sqrt(discriminant,contextptr);
      for(int sign=1;sign>=-1;sign-=2){
        gen X=ratnormal((-D+sign*root)/(2*A)-shear*t,contextptr);
        branches.push_back(swapped?makevecteur(t,X):makevecteur(X,t));
        if(is_zero(root))break;
      }
    }
    else return false;
    out=branches;return true;
  }
  int sd=curve_proved_sign(det,contextptr);if(sd==2)return false;
  gen h=ratnormal((B*E-2*C*D)/det,contextptr),k=ratnormal((B*D-2*A*E)/det,contextptr);
  gen rho=ratnormal(-F+(C*D*D-B*D*E+A*E*E)/det,contextptr);
  if(is_zero(A) && is_zero(C)){
    // Centered rectangular hyperbola B*X*Y=rho: exponential charts are
    // shorter than rotated cosh/sinh sums and cover both signs of X.
    if(is_zero(rho)){out=makevecteur(makevecteur(h+t,k),makevecteur(h,k+t));return true;}
    gen product=ratnormal(rho/B,contextptr);
    int sp=curve_proved_sign(product,contextptr);if(sp==0 || sp==2)return false;
    bool positive=sp==1;
    gen radius=sqrt(positive?product:-product,contextptr),X=radius*exp(t,contextptr),Y=(positive?1:-1)*radius*exp(-t,contextptr);
    out=makevecteur(makevecteur(h+X,k+Y),makevecteur(h-X,k-Y));return true;
  }
  gen l1=A,l2=C,c=1,s=0;
  bool sheared=false,swapped=false;
  if(!is_zero(B)){
    // Rational eigenvalue gaps often give the textbook short rotated
    // chart. Keep those rather than introducing new shear radicals.
    gen gap;
    bool simple_rotation=false;
    if(curve_rational(A) && curve_rational(B) && curve_rational(C)){
      gap=sqrt((A-C)*(A-C)+B*B,contextptr);
      simple_rotation=curve_rational(gap);
    }
    // A proved nonzero pivot permits a unit-determinant shear, avoiding
    // nested eigenvector radicals. If neither pivot is proved nonzero,
    // retain the eigenvector path (the parameter may cross zero).
    int sa=curve_proved_sign(A,contextptr);
    if(!simple_rotation && (sa==0 || sa==2)){
      int sc=curve_proved_sign(C,contextptr);
      if(sc==1 || sc==-1){swapgen(A,C);swapgen(h,k);swapped=true;sa=sc;}
    }
    if(!simple_rotation && (sa==1 || sa==-1)){
      l1=A;l2=ratnormal(det/(4*A),contextptr);s=ratnormal(B/(2*A),contextptr);sheared=true;
    }
    else {
    // Exact orthonormal eigenvectors; no atan quadrant or angle ambiguity.
    int sb=curve_proved_sign(B,contextptr);if(sb==0 || sb==2)return false;
    if(!simple_rotation)gap=sqrt((A-C)*(A-C)+B*B,contextptr);
    l1=(A+C+gap)/2;l2=(A+C-gap)/2;
    c=sqrt((1+(A-C)/gap)/2,contextptr);
    s=sb*sqrt((1-(A-C)/gap)/2,contextptr);
    }
  }
  vecteur charts;
  if(is_zero(rho)){
    if(sd==1){out=vecteur(1,swapped?makevecteur(k,h):makevecteur(h,k));return true;}
    gen slope=sqrt(-l1/l2,contextptr);
    charts.push_back(makevecteur(t,slope*t));charts.push_back(makevecteur(t,-slope*t));
  }
  else {
    gen r1=ratnormal(rho/l1,contextptr),r2=ratnormal(rho/l2,contextptr);
    int sr1=curve_proved_sign(r1,contextptr),sr2=curve_proved_sign(r2,contextptr);
    bool pos1=sr1==1,pos2=sr2==1,neg1=sr1==-1,neg2=sr2==-1;
    if(pos1 && pos2)charts.push_back(makevecteur(sqrt(r1,contextptr)*cos(t,contextptr),sqrt(r2,contextptr)*sin(t,contextptr)));
    else if(neg1 && neg2){out=vecteur(0);return true;}
    else if((pos1 && neg2) || (neg1 && pos2)){
      gen U=sqrt(pos1?r1:-r1,contextptr)*(pos1?cosh(t,contextptr):sinh(t,contextptr));
      gen V=sqrt(pos2?r2:-r2,contextptr)*(pos2?cosh(t,contextptr):sinh(t,contextptr));
      charts.push_back(makevecteur(U,V));charts.push_back(pos1?makevecteur(-U,V):makevecteur(U,-V));
    }
    else return false;
  }
  vecteur branches;branches.reserve(charts.size());
  for(unsigned j=0;j<charts.size();++j){
    const vecteur &v=*charts[j]._VECTptr;
    gen X=ratnormal(h+c*v[0]-s*v[1],contextptr),Y=ratnormal(k+(sheared?v[1]:s*v[0]+c*v[1]),contextptr);
    branches.push_back(swapped?makevecteur(Y,X):makevecteur(X,Y));
  }
  out=branches;return true;
}

static bool curve_constant_linear_graph(const gen &f,const vecteur &xy,const gen &t,int first,gen &out,GIAC_CONTEXT) {
  for(int j=0;j<2;++j){int i=j?1-first:first;gen a,b;
    if(!is_linear_wrt(f,xy[i],a,b,contextptr) || !curve_rational(a) || is_zero(a))continue;
    gen value=subst(ratnormal(-b/a,contextptr),xy[1-i],t,false,contextptr);
    out=vecteur(1,i?makevecteur(t,value):makevecteur(value,t));return true;
  }return false;
}

// A real odd radius is sign(g)*abs(g)^(1/n), not the principal complex root.
// Only a nonzero rational leading coefficient is removed, so no exceptional
// angle where that coefficient vanishes is lost.
static bool curve_odd_polar_radius(const gen &f,const gen &r,const gen &theta,const gen &t,gen &out,GIAC_CONTEXT) {
  if(taille(f,129)>128)return false;
  vecteur terms;
  if(f.is_symb_of_sommet(at_plus)&&f._SYMBptr->feuille.type==_VECT)terms=*f._SYMBptr->feuille._VECTptr;
  else terms.push_back(f);
  if(terms.size()>8)return false;
  gen rhs=0,coefficient=0;int n=0;
  for(unsigned i=0;i<terms.size();++i){
    gen g=terms[i];
    if(!depends(g,r)){rhs-=g;continue;}
    gen c=1;
    if(g.is_symb_of_sommet(at_neg)){c=-1;g=gen(g._SYMBptr->feuille);}
    if(g.is_symb_of_sommet(at_prod)&&g._SYMBptr->feuille.type==_VECT){
      const vecteur &v=*g._SYMBptr->feuille._VECTptr;gen power=1;
      if(v.size()>8)return false;
      for(unsigned j=0;j<v.size();++j){if(!depends(v[j],r))c=c*v[j];else power=power*v[j];}
      g=power;
    }
    c=ratnormal(c,contextptr);
    if(!curve_rational(c))return false;
    if(!g.is_symb_of_sommet(at_pow)||g._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &v=*g._SYMBptr->feuille._VECTptr;
    if(v.size()!=2||v[0]!=r||v[1].type!=_INT_||v[1].val<3||v[1].val>9||!(v[1].val%2))return false;
    if(n&&n!=v[1].val)return false;n=v[1].val;coefficient+=c;
  }
  if(!n||!curve_rational(coefficient)||is_zero(coefficient))return false;
  gen value=subst(rhs/coefficient,theta,t,false,contextptr);
  // Reject explicitly nonreal radii; symbols without a real assumption are
  // left to the existing general conversion path and its domain convention.
  if(!is_zero(im(value,contextptr)))return false;
  // Keep the real odd root pointwise exact without sending a trigonometric
  // argument through abs()'s general normalization on the calculator stack.
  gen radius;
  if(curve_rational(value))
    radius=sign(value,contextptr)*pow(abs(value,contextptr),gen(1)/n,contextptr);
  else
    radius=gen(symbolic(at_sign,value))*gen(symbolic(at_pow,
      makesequence(gen(symbolic(at_abs,value)),gen(1)/n)));
  out=vecteur(1,makevecteur(radius*cos(t,contextptr),radius*sin(t,contextptr)));return true;
}

static bool curve_polar_nonzero(const gen &g,GIAC_CONTEXT) {
  int s=curve_proved_sign(g,contextptr);return s==1 || s==-1;
}

// Real polar curves: prefer an explicit radius without a general solve().
// A square root denotes its real domain; choosing the nonnegative radius
// loses no Cartesian points for a centered quadratic (pi-periodic H).
static gen curve_polar_sqrt(const gen &q,GIAC_CONTEXT) {
  *logptr(contextptr)<<"Real polar radius: radicand >= 0; denominator != 0.\n";
  gen base=q,c=equation_numeric_factor(base);
  if(equation_rational(c) && is_strictly_positive(c,contextptr))
    return sqrt(c,contextptr)*sqrt(base,contextptr);
  return sqrt(q,contextptr);
}

static bool curve_conic_polar(const curve_polynomial_terms &p,const gen &r,
                              const gen &theta,gen &result,GIAC_CONTEXT) {
  if(p.empty() || p.size()>6)return false;
  for(unsigned j=0;j<p.size();++j)
    if(p[j].px+p[j].py>2 || taille(p[j].coefficient,33)>32 || !is_zero(im(p[j].coefficient,contextptr)))return false;
  gen A=curve_coefficient(p,2,0),B=curve_coefficient(p,1,1),C=curve_coefficient(p,0,2);
  gen D=curve_coefficient(p,1,0),E=curve_coefficient(p,0,1),F=curve_coefficient(p,0,0);
  gen cs=cos(theta,contextptr),sn=sin(theta,contextptr);
  gen L=D*cs+E*sn;
  if(is_zero(A) && is_zero(B) && is_zero(C)){
    // If F can vanish, dividing by L could discard a whole radial line.
    if((is_zero(D) && is_zero(E)) || !curve_polar_nonzero(F,contextptr))return false;
    result=symb_equal(r,ratnormal(-F/L,contextptr));
    *logptr(contextptr)<<"Real polar radius: denominator != 0.\n";return true;
  }
  gen H=is_zero(B)?(A==C?A:A*cs*cs+C*sn*sn):A*cs*cs+B*cs*sn+C*sn*sn;
  // Definiteness proves H never vanishes for any real angle. This proof
  // uses the small Cartesian coefficients, not a trigonometric sign test.
  bool definite=curve_proved_sign(ratnormal(4*A*C-B*B,contextptr),contextptr)==1 && curve_polar_nonzero(A,contextptr);
  if(is_zero(D) && is_zero(E)){
    if(!definite && !curve_polar_nonzero(F,contextptr))return false;
    if(definite && curve_proved_sign(-F/A,contextptr)==-1){result=vecteur(0);return true;}
    result=symb_equal(r,curve_polar_sqrt(ratnormal(-F/H,contextptr),contextptr));return true;
  }
  if(!definite)return false;
  if(is_zero(F)){
    // The omitted r=0 factor is still represented at an angle L=0.
    // A real linear combination of sin/cos always has such an angle.
    result=symb_equal(r,ratnormal(-L/H,contextptr));return true;
  }
  gen root=curve_polar_sqrt(ratnormal(L*L-4*H*F,contextptr),contextptr);
  gen first=symb_equal(r,ratnormal((-L+root)/(2*H),contextptr));
  gen second=symb_equal(r,ratnormal((-L-root)/(2*H),contextptr));
  result=first==second?first:gen(makevecteur(first,second));return true;
}

static bool curve_compact_polar(const gen &g,const gen &x,const gen &y,
                                const gen &r,const gen &theta,gen &result,GIAC_CONTEXT) {
  curve_polynomial_terms p;
  if(!curve_polynomial(g,x,y,p,contextptr))return false;
  if(curve_conic_polar(p,r,theta,result,contextptr))return true;
  gen leading=curve_coefficient(p,4,0),xx=curve_coefficient(p,2,0);
  if(curve_polar_nonzero(leading,contextptr) && curve_coefficient(p,0,4)==leading &&
     curve_coefficient(p,2,2)==2*leading && curve_coefficient(p,0,2)==-xx) {
    bool shape=true;
    for(unsigned i=0;i<p.size();++i){
      const curve_term &t=p[i];
      if(!((t.px==4 && t.py==0) || (t.px==0 && t.py==4) || (t.px==2 && t.py==2) ||
           (t.px==2 && t.py==0) || (t.px==0 && t.py==2)))shape=false;
    }
    if(shape){
      gen k=-xx/leading;if(is_undef(k) || is_inf(k))return false;
      result=symb_equal(r,curve_polar_sqrt(k*cos(2*theta,contextptr),contextptr));
      *logptr(contextptr)<<"Same Cartesian curve; origin retained. Angle labels at the origin may differ.\n";
      return true;
    }
  }
  leading=curve_coefficient(p,3,0);
  if(!curve_polar_nonzero(leading,contextptr) || curve_coefficient(p,0,3)!=leading)return false;
  for(unsigned i=0;i<p.size();++i){
    const curve_term &t=p[i];
    if(!((t.px==3 && t.py==0) || (t.px==0 && t.py==3) || (t.px==1 && t.py==1)))return false;
  }
  gen k=-curve_coefficient(p,1,1)/leading;
  if(is_undef(k) || is_inf(k))return false;
  gen sine=sin(theta,contextptr),cosine=cos(theta,contextptr),denominator=pow(cosine,3)+pow(sine,3);
  if(curve_polar_nonzero(k,contextptr)){
    result=symb_equal(r,k*sine*cosine/denominator);
    *logptr(contextptr)<<"Same Cartesian curve; origin retained. Explicit radius excludes "<<denominator<<"=0.\n";
  }
  else {
    // At k=0 the folium degenerates to the entire line x+y=0. Dividing
    // by this denominator would incorrectly reduce that line to the origin.
    result=symb_equal(r*denominator,k*sine*cosine);
    *logptr(contextptr)<<"Same Cartesian curve; origin retained. Implicit radius preserves zero-parameter cases.\n";
  }
  return true;
}

static gen equation(const gen &g,GIAC_CONTEXT) {
  if (is_undef(g)) return g;
  gen result;
  if(equation_primitive(symb_equal(g,0),result,contextptr))return result;
  gen n=normal(g,contextptr);
  if(is_undef(n))return n;
  gen relation=symb_equal(n,0);
  return equation_primitive(relation,result,contextptr)?result:relation;
}

// Substitution is simultaneous, so one coordinate cannot capture the other.
gen _cart2polar(const gen &args,GIAC_CONTEXT) {
  if (args.type==_STRNG && args.subtype==-1) return args;
  if (args.type!=_VECT || args._VECTptr->size()!=3)
    return gensizeerr("cart2polar(eq,[x,y],[r,theta])");
  const vecteur &v=*args._VECTptr;
  if (!variables(v[1]) || !variables(v[2]) || !disjoint(v[1],v[2]) || !scalar(v[0]))
    return gensizeerr("Use distinct unassigned coordinate names and a scalar equation");
  if (!angle_radian(contextptr)) return gensizeerr("Equation conversion requires radians");
  const vecteur &w=*v[2]._VECTptr;
  if (depends(v[0],w[0]) || depends(v[0],w[1]))
    return gensizeerr("Output coordinates already occur in input");
  const vecteur &xy=*v[1]._VECTptr;
  gen input=residual(v[0]),compact;
  if(curve_compact_polar(input,xy[0],xy[1],w[0],w[1],compact,contextptr))return compact;
  gen converted=subst(input,xy,
    makevecteur(w[0]*cos(w[1],contextptr),w[0]*sin(w[1],contextptr)),false,contextptr);
  // Preserve the existing pointwise substitution for all other curves.
  return equation(_tlin(converted,contextptr),contextptr);
}

gen _polar2cart(const gen &args,GIAC_CONTEXT) {
  if (args.type==_STRNG && args.subtype==-1) return args;
  if (args.type!=_VECT || args._VECTptr->size()!=3)
    return gensizeerr("polar2cart(eq,[r,theta],[x,y])");
  const vecteur &v=*args._VECTptr;
  if (!variables(v[1]) || !variables(v[2]) || !disjoint(v[1],v[2]) || !scalar(v[0]))
    return gensizeerr("Use distinct unassigned coordinate names and a scalar equation");
  if (!angle_radian(contextptr)) return gensizeerr("Equation conversion requires radians");
  const vecteur &w=*v[2]._VECTptr;
  if (depends(v[0],w[0]) || depends(v[0],w[1]))
    return gensizeerr("Output coordinates already occur in input");
  // Canonical chart: r>=0, principal argument, origin checked separately.
  // Do not square this relation: that would introduce extra branches.
  return equation(subst(residual(v[0]),*v[1]._VECTptr,
    makevecteur(sqrt(w[0]*w[0]+w[1]*w[1],contextptr),
                arg(w[0]+cst_i*w[1],contextptr)),false,contextptr),contextptr);
}

// Return all branches as [[X(t),Y(t)],...], suitable for plotparam.
gen _cart2param(const gen &args,GIAC_CONTEXT) {
  if (args.type==_STRNG && args.subtype==-1) return args;
  if (args.type!=_VECT || args._VECTptr->size()!=3)
    return gensizeerr("cart2param(eq,[x,y],t)");
  const vecteur &v=*args._VECTptr;
  if (!variables(v[1]) || v[2].type!=_IDNT || !disjoint(v[1],v[2]) || !scalar(v[0]))
    return gensizeerr("Use distinct unassigned coordinate and parameter names");
  if (depends(v[0],v[2])) return gensizeerr("Parameter already occurs in input");
  const vecteur &xy=*v[1]._VECTptr;
  gen f=residual(v[0]);
  if (!depends(f,xy[0]) && !depends(f,xy[1]))
    return gensizeerr("Equation does not define a curve in these coordinates");
  // An explicit graph needs only substitution, no equation solver.
  if (v[0].is_symb_of_sommet(at_equal) && pair(v[0]._SYMBptr->feuille)) {
    const vecteur &e=*v[0]._SYMBptr->feuille._VECTptr;
    for (int side=0;side<2;++side)
      for (int i=0;i<2;++i)
        if (e[side]==xy[i] && !depends(e[1-side],xy[i])) {
          gen value=subst(e[1-side],xy[1-i],v[2],false,contextptr);
          return vecteur(1,i?makevecteur(v[2],value):makevecteur(value,v[2]));
        }
  }
  // Direct algebraic graphs and textbook lemniscates avoid general solving.
  gen direct;
  if(curve_constant_linear_graph(f,xy,v[2],1,direct,contextptr))return direct;
  curve_polynomial_terms polynomial;
  bool bounded_polynomial=curve_polynomial(f,xy[0],xy[1],polynomial,contextptr);
  if(bounded_polynomial && (curve_conic_param(polynomial,v[2],direct,contextptr) ||
      curve_lemniscate_param(polynomial,v[2],direct,contextptr) ||
      curve_folium_param(polynomial,v[2],direct,contextptr)))return direct;
  // Split reducible curves before isolation, so x*y=0 retains the vertical
  // component x=0 as well as y=0. Multiplicities do not create new branches.
  gen factored=_factor(f,contextptr);
  if (is_undef(factored)) return factored;
  vecteur factors;
  if (factored.is_symb_of_sommet(at_prod) && factored._SYMBptr->feuille.type==_VECT)
    factors=*factored._SYMBptr->feuille._VECTptr;
  else factors.push_back(factored);
  vecteur components;
  for (unsigned i=0;i<factors.size();++i) {
    gen part=factors[i];
    if (part.is_symb_of_sommet(at_pow) && pair(part._SYMBptr->feuille)) {
      const vecteur &power=*part._SYMBptr->feuille._VECTptr;
      if (power[1].type==_INT_) {
        if (power[1].val<=0) continue; // denominator exclusions stay with the input
        part=power[0];
      }
    }
    if (depends(part,xy[0]) || depends(part,xy[1])) components.push_back(part);
    else if (!curve_polar_nonzero(part,contextptr))
      return gensizeerr("Parameter factor may vanish; assume it is nonzero first");
  }
  if (components.size()>1) {
    vecteur out;
    for (unsigned i=0;i<components.size();++i) {
      gen branches=_cart2param(makesequence(symb_equal(components[i],0),v[1],v[2]),contextptr);
      if (is_undef(branches) || branches.type!=_VECT) return branches;
      const vecteur &b=*branches._VECTptr;
      out.reserve(out.size()+b.size());
      for (unsigned j=0;j<b.size();++j) out.push_back(b[j]);
    }
    return out;
  }
  if (components.size()==1) f=components[0];
  bounded_polynomial=curve_polynomial(f,xy[0],xy[1],polynomial,contextptr);
  int solved=depends(f,xy[1])?1:0;
  if(bounded_polynomial){
    unsigned dx=0,dy=0;
    for(unsigned i=0;i<polynomial.size();++i){if(polynomial[i].px>dx)dx=polynomial[i].px;if(polynomial[i].py>dy)dy=polynomial[i].py;}
    if(dx && (!dy || dx<dy))solved=0;
  }
  if(curve_constant_linear_graph(f,xy,v[2],solved,direct,contextptr))return direct;
  if(bounded_polynomial && curve_origin_pencil(polynomial,v[2],direct,contextptr))return direct;
  gen roots=_solve(makesequence(symb_equal(f,0),xy[solved]),contextptr);
  if (is_undef(roots)) return roots;
  if (roots.type!=_VECT || roots._VECTptr->empty())
    return gensizeerr("Cannot isolate this curve; try another coordinate or parametrization");
  vecteur out;
  out.reserve(roots._VECTptr->size());
  for (unsigned i=0;i<roots._VECTptr->size();++i) {
    gen value=(*roots._VECTptr)[i];
    if (!scalar(value) || depends(value,xy[solved]) || value.is_symb_of_sommet(at_solve))
      return gensizeerr("Unresolved solution branch");
    value=subst(value,xy[1-solved],v[2],false,contextptr);
    out.push_back(solved?makevecteur(v[2],value):makevecteur(value,v[2]));
  }
  return out;
}

gen _polar2param(const gen &args,GIAC_CONTEXT) {
  if (args.type==_STRNG && args.subtype==-1) return args;
  if (args.type!=_VECT || args._VECTptr->size()!=3)
    return gensizeerr("polar2param(eq,[r,theta],t)");
  const vecteur &v=*args._VECTptr;
  if (!variables(v[1]) || v[2].type!=_IDNT || !disjoint(v[1],v[2]) || !scalar(v[0]))
    return gensizeerr("Use distinct unassigned coordinate and parameter names");
  if (!angle_radian(contextptr)) return gensizeerr("Equation conversion requires radians");
  // Reuse the all-branches solver with axes [theta,r]. This also handles rays.
  const vecteur &rt=*v[1]._VECTptr;
  if(depends(v[0],v[2]))return gensizeerr("Parameter already occurs in input");
  gen direct;
  if(curve_odd_polar_radius(residual(v[0]),rt[0],rt[1],v[2],direct,contextptr))return direct;
  gen branches=_cart2param(makesequence(v[0],makevecteur(rt[1],rt[0]),v[2]),contextptr);
  if (is_undef(branches) || branches.type!=_VECT) return branches;
  vecteur out;
  out.reserve(branches._VECTptr->size());
  for (unsigned i=0;i<branches._VECTptr->size();++i) {
    const vecteur &b=*(*branches._VECTptr)[i]._VECTptr;
    out.push_back(makevecteur(b[1]*cos(b[0],contextptr),b[1]*sin(b[0],contextptr)));
  }
  return out;
}

gen _param2polar(const gen &args,GIAC_CONTEXT) {
  if (args.type==_STRNG && args.subtype==-1) return args;
  if (args.type!=_VECT || (args._VECTptr->size()!=2 && args._VECTptr->size()!=3))
    return gensizeerr("param2polar([X(t),Y(t)],t,[r,theta]); omit [r,theta] for a polar pair");
  const vecteur &v=*args._VECTptr;
  if (!pair(v[0]) || v[1].type!=_IDNT)
    return gensizeerr("Expected two coordinate expressions and an unassigned parameter");
  if (!angle_radian(contextptr)) return gensizeerr("Equation conversion requires radians");
  const vecteur &p=*v[0]._VECTptr;
  if (!scalar(p[0]) || !scalar(p[1]) || is_equal(p[0]) || is_equal(p[1]))
    return gensizeerr("Use [X(t),Y(t)] expressions, not equations");
  if (v.size()==3) {
    if (!variables(v[2]) || !disjoint(v[2],v[1]))
      return gensizeerr("Use distinct unassigned polar coordinate names");
    const vecteur &rt=*v[2]._VECTptr;
    if (depends(v[0],rt[0]) || depends(v[0],rt[1]))
      return gensizeerr("Output coordinates already occur in input");
    gen xy=makevecteur(gen(identificateur(" khicas_curve_x")),gen(identificateur(" khicas_curve_y")));
    gen cart=_param2cart(makesequence(v[0],v[1],xy),contextptr);
    if (is_undef(cart)) return cart;
    if (cart.type==_VECT) { // A constant parametrization is a point: conjunction.
      vecteur out;
      for (unsigned i=0;i<cart._VECTptr->size();++i)
        out.push_back(_cart2polar(makesequence((*cart._VECTptr)[i],xy,v[2]),contextptr));
      return out;
    }
    return _cart2polar(makesequence(cart,xy,v[2]),contextptr);
  }
  return makevecteur(_simplify(sqrt(p[0]*p[0]+p[1]*p[1],contextptr),contextptr),
                     arg(p[0]+cst_i*p[1],contextptr));
}

static bool rational_in(const gen &g,const gen &t) {
  vecteur vars=lvarx(g,t);
  return vars.empty() || (vars.size()==1 && vars[0]==t);
}

gen _param2cart(const gen &args,GIAC_CONTEXT) {
  if (args.type==_STRNG && args.subtype==-1) return args;
  if (args.type!=_VECT || args._VECTptr->size()!=3)
    return gensizeerr("param2cart([X(t),Y(t)],t,[x,y])");
  const vecteur &v=*args._VECTptr;
  if (!pair(v[0]) || v[1].type!=_IDNT || !variables(v[2]) || !disjoint(v[2],v[1]))
    return gensizeerr("Use two expressions and distinct unassigned variable names");
  const vecteur &p=*v[0]._VECTptr, &xy=*v[2]._VECTptr;
  if (!scalar(p[0]) || !scalar(p[1]) || is_equal(p[0]) || is_equal(p[1]) ||
      depends(v[0],xy[0]) || depends(v[0],xy[1]))
    return gensizeerr("Use [X(t),Y(t)]; output coordinates must not occur in input");
  for (int i=0;i<2;++i)
    if (p[i]==v[1])
      return symb_equal(xy[1-i],subst(p[1-i],v[1],xy[i],false,contextptr));
  if (!depends(p[0],v[1]) && !depends(p[1],v[1]))
    return makevecteur(symb_equal(xy[0],p[0]),symb_equal(xy[1],p[1]));
  gen a=p[0],b=p[1],t=v[1];
  if (!rational_in(a,t) || !rational_in(b,t)) {
    if (!angle_radian(contextptr)) return gensizeerr("Trigonometric conversion requires radians");
    // Rationalize sin(t),cos(t) with the tangent half-angle substitution.
    // Reject residual transcendental dependence instead of returning a bogus resultant.
    gen u(identificateur(" khicas_curve_u"));
    if (depends(v[0],u)) return gensizeerr("Internal parameter collision");
    gen half=tan(t/2,contextptr);
    a=subst(_halftan(a,contextptr),half,u,false,contextptr);
    b=subst(_halftan(b,contextptr),half,u,false,contextptr);
    if (depends(a,t) || depends(b,t) || !rational_in(a,u) || !rational_in(b,u))
      return gensizeerr("Cannot eliminate this parametrization; keep the parametric form");
    t=u;
  }
  gen f=_numer(normal(a-xy[0],contextptr),contextptr);
  gen g=_numer(normal(b-xy[1],contextptr),contextptr);
  gen result=_resultant(makesequence(f,g,t),contextptr);
  if (is_undef(result)) return result;
  if (depends(result,t) || is_zero(result))
    return gensizeerr("Elimination was inconclusive");
  *logptr(contextptr) << "Algebraic closure: retain parameter range and denominator exclusions.\n";
  return equation(result,contextptr);
}

#define CURVE_COMMAND(name) \
  static const char name##_s[]=#name; \
  static define_unary_function_eval(__##name,&_##name,name##_s); \
  define_unary_function_ptr5(at_##name,alias_at_##name,&__##name,0,true)
CURVE_COMMAND(cart2param);
CURVE_COMMAND(cart2polar);
CURVE_COMMAND(param2cart);
CURVE_COMMAND(param2polar);
CURVE_COMMAND(polar2cart);
CURVE_COMMAND(polar2param);
#undef CURVE_COMMAND
}
